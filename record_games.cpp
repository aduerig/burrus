/*
 *  To compile on bridges, make sure you have done
 *  module load gcc mpi/gcc_openmpi
 *
 */

/* to run on bridges
 * mpirun -n num_proccessors ./param
 */

// run serial to play 10 games of montecarlo vs montecarlo
// ./param_serial -rank 0 -ngames 10

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include "engine.hpp"
#include "player.hpp"
#include "communicator.hpp"


/*  
 * OUTPUT: the path to the directory with the newest Neural Network. will be "../data/model_%d" 
 *         where the %d is whatever number model we're on.
 *
 * This function assumes that it's being run from some directory in "othello" and so it has to go up one directory
 *     to get to the data/ directory.
 * From there, it simply tries to open a file called "checkpoint" in data/model_%d/ starting %d at 0.
 * If it can open the file successfully, then it adds 1 to i and tries the next value
 * Once it CAN'T open that checkpoint file, then the previous value of i is the most current model.
 * So, it returns the directory for that model.
 *  
 */


std::string get_newest_model_name()
{
    FILE *fp;
    char *fnm = (char *) calloc(50, sizeof(char));
    int i = 0;
    int model_name_int;
    do
    {
        sprintf(fnm,"data/model_%d/checkpoint", i);
        fp = fopen(fnm, "r");
        if (!fp)
        {
            if(i - 1 < 0)
            {
                printf("Exiting with return code 5 because no model folders exists in data/model\n");
                exit(5);
            }
            // sprintf(fnm, "data/model_%d", i-1); # this returns full model path
            model_name_int = i-1;
            free(fnm);
            return "model_" + std::to_string(model_name_int);
        }
        fclose(fp);
        ++i;
    } while (1);
}

/* INPUT:
 *         model_path: contain directory to the model that was used to play these games
 *         local_rank: the rank of the specific MPI process
 *         game_number: which game on this process the results are from
 *         e: the engine that played the game. Need for all the board states.
 *         num_moves: the number of moves that were played that game
 *         result: 0 for black win, 1 for white win, 2 for draw
 *         MC_chances: the MonteCarlo chances for every square at every move
 *         saved_values: the player->get_saved_value() from each move
 *
 * This function makes a file at ${model_path}/games/${local_rank}_${game_number}.game 
 * and for each move in the game prints:
 *         U64 giving location of current player's pieces
 *         U64 giving location of opponent's pieces
 *         64 x MonteCarlo values for all 64 moves
 *         1 x saved_value
 *         integer result (1 if current player wins, 0 if current player loses, 2 if draw)
 *
 * NOTE: Because I get the game boards by poppping moves from the stack, I actually save the games in REVERSE order.
 *       If you use the function I wrote below (read_game_info(...)), it should read everything correctly.
 */


void load_ull_into_int_arr(Engine* e, int* int_board_loader, U64 from_board)
{
    int index;
    // printf("%llu\n", from_board);
    // e->print_char();
    memset(int_board_loader, 0, 64 * sizeof(int));
    while(from_board)
    {
        index = e->lsb_digit(from_board);
        int_board_loader[index] = 1;
        // printf("index: %i\n", index);
        from_board -= (1ULL << index);
    }
}

// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string gen_random(const int len) 
{
    std::string new_str;
    for(int i = 0; i < len; i++)
    {
        new_str.append(" ");
    }

    static const char alphanum[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) 
    {
        new_str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return new_str;
}


void save_game_info(std::string model_path, int local_rank, int game_number, Engine* e, 
                                        int num_moves, int result, float **MC_chances, 
                                        int* recorded_moves)
{
    // Filename: ${model_path}/games/${local_rank}_${game_number}.game
    FILE *fp;
    char* fnm = (char*) calloc(50, sizeof(char));
    int* int_board_loader = (int*) calloc(64, sizeof(int));
    
    sprintf(fnm, "%s/games/%s.game", model_path.c_str(), gen_random(10).c_str());
    fp = fopen(fnm, "w");

    int result_parsed;
    int multiplier;
    if(result == 0)
    {
        result_parsed = -1;
    }
    else if(result == 1)
    {
        result_parsed = 1;
    }
    else if(result == 2)
    {
        result_parsed = 0;
    }

    // save num_moves
    fprintf(fp, "%d\n", num_moves); //doesnt record moves it didn't make a decision in
    for (int i = num_moves-1; i >= 0; --i)
    {
        e->stack_pop();

        // Save Pieces of player whose turn it is (U64). then other player
        U64 first_pieces;
        U64 second_pieces;
        if (i % 2 == 0) // Black plays when num_moves is even. Save black first, then white.
        { 
            first_pieces = e->pos.black_board;
            second_pieces = e->pos.white_board;
        }
        else // White plays when num_moves is odd. Save white first, then black.
        {
            first_pieces = e->pos.white_board;
            second_pieces = e->pos.black_board;
        }

        load_ull_into_int_arr(e, int_board_loader, e->pos.black_board);
        for(int j = 0; j < 64; j++)
        {
            fprintf(fp, "%i,", int_board_loader[j]);
        }
        fprintf(fp, "\n");

        load_ull_into_int_arr(e, int_board_loader, e->pos.white_board);
        for(int j = 0; j < 64; j++)
        {
            fprintf(fp, "%i,", int_board_loader[j]);
        }
        fprintf(fp, "\n");

        fprintf(fp, "%i\n", recorded_moves[i]);

        // Save monte carlo search results (64 x 32 bit floats)
        for (int j = 0; j < 64; ++j)
        {
            fprintf(fp, "%f,", MC_chances[i][j]); // MC chances for move i square j
        }
        fprintf(fp, "\n");

        multiplier = (2 * (i % 2)) - 1;        
        fprintf(fp, "%d\n", result_parsed * multiplier);
    }

    fclose(fp);
}

/* INPUT:  
 *         std::string game_path. Should contain the path to the directory where the games are
 *         n_moves should be the address of an integer variable
 *         player_boards should be an uninitialized pointer to U64 type
 *         opponent boards should be an uninitialized pointer to U64 type
 *         MCvals should be an uninitialized float **
 *         result should be an uninitialized int *
 *         saved_values should be an uninitialized float *
 * OUTPUT:
 *         n_moves[0]: the number of moves that happened this game (and length of the next 3 arrays)
 *         player_boards[i]: the location of all tokens for the "current player" at move i
 *         -- Note, this means player_boards[0] contains black, player_boards[1] contains white, etc.
 *         opponent_boards[i]: the location of all tokens for the "opponent player" at move i
 *         -- Note, this means opponent_boards[0] contains white, opponent_boards[1] contains black, etc.
 *         MCvals[i]: the values returned by the neural network for the current player for all 64 squares at move i
 *         saved_values[i]: the value from player->get_saved_value() for each move
 *         result[i]: 1 if current player at move i won, 0 if they lost, and 2 if draw.
 *         -- For a draw, it will be a bunch of 2s.
 *         -- For a black win, it will be 10101010101010... (because black moves first)
 *         -- For a white win, it will be 01010101010101... (because black moves first)
 *
 * This function reads a file that was written by save_game_info(...) and saves the info in the pointers passed in
 * NOTE: the game files are written backwards (last move first), so I populate the arrays end to start, 
 *       so they are in the correct order
 */

// void read_game_info(std::string game_path, int *n_moves, U64 *player_boards, U64 *opponent_boards, float **MCvals, int *result, float *saved_values)
// {

//     // dummy holder
//     char* dummy_return;
//     // Open file
//     FILE *fp = fopen(game_path.c_str(),"r");
//     // Read number of moves
//     char *buf = (char *) calloc(30, sizeof(char));
//     dummy_return = fgets(buf, 30, fp);
//     *n_moves = atoi(buf);
//     // Allocate memory
//     player_boards = (U64 *) calloc(n_moves[0], sizeof(U64)); 
//     opponent_boards = (U64 *) calloc(n_moves[0], sizeof(U64)); 
//     MCvals = (float **) calloc(n_moves[0], sizeof(float *));
//     result = (int *) calloc(n_moves[0], sizeof(int));
//     saved_values = (float *) calloc(n_moves[0], sizeof(float));
//     // Loop over number of moves
//     for (int i = n_moves[0]-1;i >= 0; --i)
//     {
//         // Read player board
//         dummy_return = fgets(buf,30,fp);
//         // Turn a string rep of U64 into a U64 rep and store in player_boards[i]
//         sscanf(buf, "%llu", &(player_boards[i]));
//         // Read opponent board
//         dummy_return = fgets(buf, 30, fp);
//         // Turn a string rep of U64 into a U64 rep and store in opponent_boards[i]
//         sscanf(buf, "%llu", &(opponent_boards[i]));
//         // Read 64 previous MC search results
//         MCvals[i] = (float *) calloc(64, sizeof(float));
//         for (int j = 0; j < 64; ++j)
//         {
//             dummy_return = fgets(buf, 30, fp);
//             MCvals[i][j] = atof(buf);
//         }    
//         // Read saved value
//         dummy_return = fgets(buf, 30, fp);
//         saved_values[i] = atof(buf);
//         // Read winner 
//         dummy_return = fgets(buf, 30, fp);
//         result[i] = atoi(buf);
//     }
//     // Close file
//     fclose(fp);
// }

/*  INPUT:
 *          model_path and local_rank are same as save_game_info
 *          gt, w1t, pt, and w2t are the four time results from the main function.
 *          save the 4 values incase something takes a long time we can figure out what it is.
 */

void save_timer_info(std::string model_path, int local_rank, 
                                                 std::chrono::duration<double, std::nano> gt, 
                                                 std::chrono::duration<double, std::nano> w1t) 
{
    FILE *fp;
    char *fnm = (char *) calloc(50, sizeof(char));
    sprintf(fnm,"%s/games/%d.timers",model_path.c_str(), local_rank);
    fp = fopen(fnm, "w");
    fprintf(fp,"%g  %g", gt.count(), w1t.count());
    fclose(fp);
}


int play_game(Engine* e, std::vector<MonteCarlo*> players, int* num_moves, float **MC_chances, 
                    int* total_MC_chances, int* recorded_moves, int print_level)
{
    int move;
    int* move_list;
    float* MCc;

    int color = BLACK;
    int player_tracker = players[BLACK]->get_color();

    if (print_level > 0) e->print_char();

    move_list = e->generate_moves(color);
    while(e->is_not_terminal(move_list, color))
    {
        move = players[player_tracker]->move(move_list);
        e->push_move(move, color);

        if (print_level > 0) printf("move number %i\n", num_moves[0]); // && num_moves[0] == 51
        if (print_level > 0) e->print_char();
        
        std::memcpy(MC_chances[num_moves[0]], players[player_tracker]->get_saved_action_probs(), 
                                                                            64 * sizeof(float));

        recorded_moves[num_moves[0]] = move;
        num_moves[0]++;

        if (print_level > 1) std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        color = 1 - color;
        player_tracker = 1 - player_tracker;

        move_list = e->generate_moves(color);
    }
    return e->get_winner();
}


void record_games(int local_rank, int games_per_proc, int iterations_per_move, int print_level)
{
    if (local_rank == -1)
    {
        fprintf(stderr,"ERROR: local_rank = -1. Please specify local rank with -rank\n");
        exit(0);
    }
    if (games_per_proc == -1)
    {
        fprintf(stderr,"ERROR: games_per_proc = -1. Please provide games per proc with -ngames\n");
        exit(0);
    }


    Engine* e = new Engine();

    std::chrono::time_point<std::chrono::system_clock> game_start_timer, 
                            game_end_timer, wait1_start_timer, wait1_end_timer;
    std::chrono::duration<double, std::nano> game_time_result, wait1_time_result;
    

    // Read in the new neural network model name
    if (print_level > 1) std::cout << "local_rank: " << local_rank 
                         << " about to get model path " << std::endl; 
    
    std::string model_name = get_newest_model_name();
    std::string model_path = "data/" + model_name;

    if (print_level > 1) std::cout << "local_rank: " << local_rank 
                         << " got the newest model dir" << std::endl;
    if (print_level > 1) std::cout << "local_rank: " << local_rank 
                         << " model_name: " << model_name << std::endl;
 
    std::vector<MonteCarlo*> players;

    PyCommunicator* comm = new PyCommunicator(model_name);
    comm->setup_python_communication();

    players.push_back(new MonteCarlo(BLACK, e, model_name, iterations_per_move, true, comm));
    players.push_back(new MonteCarlo(WHITE, e, model_name, iterations_per_move, true, comm));
     
    // Variable for the number of moves in the game
    int* num_moves = (int*) calloc(1, sizeof(int));
    int* total_MC_chances = (int*) calloc(1, sizeof(int));

    int* recorded_moves = (int*) calloc(e->get_max_move_length(), sizeof(int));

    int max_MC_chances_per_game = e->get_max_move_length();
    float** MC_chances = (float**) calloc(max_MC_chances_per_game, sizeof(float*));
    for(int j = 0; j < max_MC_chances_per_game; j++)
    {
        MC_chances[j] = (float*) calloc(64, sizeof(float));
    }


    /*~*~*~*~*~ Possibly decrement a game timer here ~*~*~*~*~*/
    game_start_timer = std::chrono::system_clock::now();


    // Loop over the number of games we're playing per processor per model
    for (int i = 0; i < games_per_proc; ++i)
    {
        total_MC_chances[0] = 0;
        num_moves[0] = 0;

        if(print_level > 1) std::cout << "Playing game " << i + games_per_proc * local_rank 
                                      << " on processor: " << local_rank << std::endl;

        // play_game returns 0 for black win, 1 for white win, and 2 for draw
        int result = play_game(e, players, num_moves, MC_chances, total_MC_chances, 
                                recorded_moves, print_level);

        if(print_level > 1) std::cout << "Processor: " << local_rank 
                            << " finished playing game, saving data now to path: " 
                            << model_path << std::endl;

        // save the game's info.
        save_game_info(model_path, local_rank, i, e, num_moves[0], result, MC_chances, recorded_moves);

        if(print_level > 1) std::cout << "Processor: " << local_rank 
                            << " finished saving data, resetting engine now." << std::endl;

        e->reset_engine();
    }

    if (print_level > 1) std::cout << "Processor: " << local_rank 
                         << " finished playing and saving " << games_per_proc 
                         << " games." << std::endl;
        
    /*~*~*~*~*~ Possibly increment game timer here ~*~*~*~*~*/
    game_end_timer = std::chrono::system_clock::now();
    game_time_result = cast_nano2(game_end_timer-game_start_timer);
        
    /*~*~*~*~*~ Possibly decrement wait1 timer here ~*~*~*~*~*/
    wait1_start_timer = std::chrono::system_clock::now();    

    /*~*~*~*~*~ Possibly increment wait1 timer here ~*~*~*~*~*/
    wait1_end_timer = std::chrono::system_clock::now();
    wait1_time_result = cast_nano2(wait1_end_timer - wait1_start_timer);


    if (print_level > 1) printf("Finished timing, now saving timer info\n");

    /*~*~*~ Possibly save all of the timer information ~*~*~*/
    save_timer_info(model_path, local_rank, game_time_result, wait1_time_result);

    if (print_level > 1) printf("Cleaning up engine and players\n");

    /////////////// clean up ///////////////
    
    free(recorded_moves);
    for(int j = 0; j < max_MC_chances_per_game; j++)
    {
        free(MC_chances[j]);
    }
    free(MC_chances);

    players[BLACK]->cleanup();
    players[WHITE]->cleanup();
    delete(players[0]);
    delete(players[1]);
    players.clear();
    players.shrink_to_fit();

    e->clean_up();
    delete(e);

    free(num_moves);
    comm->clean_up();
    delete(comm);

    ////////////////////////////////////////

    if (print_level > 1) printf("record_games has finished\n");
}


// ./record_games -ngames 1 -rank 0
int main(int argc, char * argv[])
{
    int local_rank = -1; 
    int games_per_proc = -1;
    int iterations_per_move = 100;
    int print_level = 0;

    for (int i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i],"-rank") == 0)
        {
            local_rank = atoi(argv[i+1]);
        }
        if (strcmp(argv[i],"-ngames") == 0)
        {
            games_per_proc = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-print") == 0)
        {
            print_level = atoi(argv[i+1]);
        }
    }

    srand(time(NULL)); // seed rand
    
    record_games(local_rank, games_per_proc, iterations_per_move, print_level);
    return 0;
}