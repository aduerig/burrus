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
#include "engine.hpp"
#include "player.hpp"
#include "param_serial.hpp"


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


Params::Params()
{
    // blank
}


std::string Params::get_newest_model_name()
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

void Params::save_game_info(std::string model_path, int local_rank, int game_number, Engine* e, 
                                        int num_moves, int result, float **MC_chances, float *saved_values)
{
    // Filename: ${model_path}/games/${local_rank}_${game_number}.game
    FILE *fp;
    char *fnm = (char *) calloc(50, sizeof(char));
    sprintf(fnm, "%s/games/%d_%d.game", model_path.c_str(), local_rank, game_number);
    fp = fopen(fnm, "w");

    // save num_moves
    fprintf(fp,"%d\n",num_moves);  
    for (int i = num_moves-1; i >= 0; --i)
    {
        e->stack_pop();
        // Save Pieces of player whose turn it is (U64)
        // Save Pieces of other player (U64)
        if (i % 2 == 0) // Black plays when num_moves is even. Save black first, then white.
        { 
            fprintf(fp,"%llu\n%llu\n", (unsigned long long)e->pos.black_board, (unsigned long long)e->pos.white_board);
        }
        else // White plays when num_moves is odd. Save white first, then black.
        {
            fprintf(fp,"%llu\n%llu\n", e->pos.white_board, e->pos.black_board);
        }

        // Save monte carlo search results (64 x 32 bit floats)
        for (int j = 0; j < 64; ++j)
        {
            fprintf(fp, "%g\n", MC_chances[i][j]); // MC chances for move i square j
        }
       
        // save the saved value 
        fprintf(fp, "%f\n", saved_values[i]);

        // Save if current player won (1 x Int)
        // i % 2 == 0 for black. result is 0 for black win.
        // i % 2 == 1 for white. result is 1 for white win.
        // fprintf(fp, "%d\n", (result == 2 ? 2 : ((i % 2) == result)));
        int result_parsed;
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
        fprintf(fp, "%d\n", result_parsed);
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
void Params::read_game_info(std::string game_path, int *n_moves, U64 *player_boards, U64 *opponent_boards, float **MCvals, int *result, float *saved_values)
{

    // dummy holder
    char* dummy_return;
    // Open file
    FILE *fp = fopen(game_path.c_str(),"r");
    // Read number of moves
    char *buf = (char *) calloc(30, sizeof(char));
    dummy_return = fgets(buf, 30, fp);
    *n_moves = atoi(buf);
    // Allocate memory
    player_boards = (U64 *) calloc(n_moves[0], sizeof(U64)); 
    opponent_boards = (U64 *) calloc(n_moves[0], sizeof(U64)); 
    MCvals = (float **) calloc(n_moves[0], sizeof(float *));
    result = (int *) calloc(n_moves[0], sizeof(int));
    saved_values = (float *) calloc(n_moves[0],sizeof(float));
    // Loop over number of moves
    for (int i = n_moves[0]-1;i >= 0; --i)
    {
        // Read player board
        dummy_return = fgets(buf,30,fp);
        // Turn a string rep of U64 into a U64 rep and store in player_boards[i]
        sscanf(buf, "%llu", &(player_boards[i]));
        // Read opponent board
        dummy_return = fgets(buf, 30, fp);
        // Turn a string rep of U64 into a U64 rep and store in opponent_boards[i]
        sscanf(buf, "%llu", &(opponent_boards[i]));
        // Read 64 previous MC search results
        MCvals[i] = (float *) calloc(64, sizeof(float));
        for (int j = 0; j < 64; ++j)
        {
            dummy_return = fgets(buf, 30, fp);
            MCvals[i][j] = atof(buf);
        }    
        // Read saved value
        dummy_return = fgets(buf, 30, fp);
        saved_values[i] = atof(buf);
        // Read winner 
        dummy_return = fgets(buf, 30, fp);
        result[i] = atoi(buf);
    }
    // Close file
    fclose(fp);
}

/*  INPUT:
 *          model_path and local_rank are same as save_game_info
 *          gt, w1t, pt, and w2t are the four time results from the main function.
 *          save the 4 values incase something takes a long time we can figure out what it is.
 */

void Params::save_timer_info(std::string model_path, int local_rank, 
                                                 std::chrono::duration<double, std::nano> gt, 
                                                 std::chrono::duration<double, std::nano> w1t) 
{
    FILE *fp;
    char *fnm = (char *) calloc(50,sizeof(char));
    sprintf(fnm,"%s/games/%d.timers",model_path.c_str(),local_rank);
    fp = fopen(fnm,"w");
    fprintf(fp,"%g  %g",gt.count(), w1t.count());
    fclose(fp);
}


/* MRD I copied this from play.cpp but added the MC_chances parameter and the few lines that use it*/
int Params::play_game(Engine* e, std::vector<MonteCarlo*> players, int* num_moves, float **MC_chances, int* total_MC_chances, float *saved_values, bool print_on)
{
    int move;
    int* move_list;
    float *MCc;
    num_moves[0] = 0;
    move_list = e->generate_black_moves();

    if (print_on && num_moves[0] == 51) e->print_char();

    while(e->is_not_terminal(move_list, BLACK))
    {
        move = players[BLACK]->move(move_list);
        e->push_black_move(move);
        if (print_on && num_moves[0] == 51) printf("move number %i\n", num_moves[0]);
        if (print_on && num_moves[0] == 51) e->print_char();
        
        MC_chances[num_moves[0]] = (float *) calloc(64, sizeof(float));
        total_MC_chances[0]++;
        MCc = players[BLACK]->get_saved_q(); // When players was std::vector<Players *>, this errors
        for (int i = 0; i < 64; ++i) 
        { 
            MC_chances[num_moves[0]][i] = MCc[i]; 
        }
        saved_values[num_moves[0]] = players[BLACK]->get_saved_value();
        num_moves[0]++;
        move_list = e->generate_white_moves();
        if(e->is_terminal(move_list, WHITE))
        {
            break;
        }
        move = players[WHITE]->move(move_list);
        e->push_white_move(move);
        if (print_on && num_moves[0] == 51) printf("move number %i\n", num_moves[0]);
        if (print_on && num_moves[0] == 51) e->print_char();
        MC_chances[num_moves[0]] = (float *) calloc(64, sizeof(float));
        MCc = players[WHITE]->get_saved_q();
        for (int i = 0; i < 64; ++i) 
        { 
            MC_chances[num_moves[0]][i] = MCc[i]; 
        }
        saved_values[num_moves[0]] = players[WHITE]->get_saved_value();
        num_moves[0]++;
        move_list = e->generate_black_moves();
    }
    return e->get_winner();
}


void Params::call_python_script_helper(new_params params, std::string model_name)
{
    std::string command = "python python_model_communicator.py " + params.semaphore_name + " " + 
                            params.shared_memory_name + " " + 
                            model_name;

    printf("EXECUTING: %s\n", command.c_str());
    pid_t pid = fork();
    if(pid != 0)
    {
        int ret_val = system(command.c_str());
        exit(0);
    }
}


// model and python communication
// 0 is success
// 1 is semaphore error
// 2 is shared memory error
int Params::setup_python_communication()
{
    // printf("SEEDING RANDOM\n");
    // srand (time(NULL));

    pSemaphore = NULL;
    pSharedMemory_code = NULL;
    pSharedMemory_rest = NULL;

    // sender flag
    send_code = -1; // -1 is uninitilized, 0 is c sent, 1 is python sent, 2 is c sent python kill

    
    printf("COMMUNICATION CHANNEL INITIALIZING IN C++!\n");

    // Create the shared memory
    fd = shm_open(params.shared_memory_name.c_str(), O_RDWR | O_CREAT | O_EXCL, params.permissions);    

    if (fd == -1) 
    {
        fd = 0;
        printf("Creating the shared memory failed\n");
        return 2;
    }

    else 
    {
        // The memory is created as a file that's 0 bytes long. Resize it.
        rc = ftruncate(fd, params.size);
        if (rc) 
        {
            printf("Resizing the shared memory failed\n");
            return 2;
        }
        else 
        {
            // MMap the shared memory
            //void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
            pSharedMemory_code = mmap((void *)0, (size_t)params.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (pSharedMemory_code == MAP_FAILED) 
            {
                pSharedMemory_code = NULL;
                printf("MMapping the shared memory failed\n");
                return 2;
            }
            else 
            {
                pSharedMemory_rest = (void*) (((int32_t*) pSharedMemory_code) + 1);
                printf("pSharedMemory_code = %p\n", pSharedMemory_code);
                printf("pSharedMemory_rest = %p\n", pSharedMemory_rest);
            }
        }
    }
    
    if (pSharedMemory_code) 
    {
        // Create the semaphore
        pSemaphore = sem_open(params.semaphore_name.c_str(), O_CREAT, params.permissions, 0);
    
        if (pSemaphore == SEM_FAILED) 
        {
            printf("Creating the semaphore failed\n");
            return 1;
        }
        else 
        {
            printf("pSemaphore =  %p\n", (void *)pSemaphore);
        }
    }

    printf("communication channel established\n");
    return 0;
}

void Params::send_end_code_python()
{
    send_code = 2;
    memcpy(pSharedMemory_code, &send_code, sizeof(int32_t));
}

// 0 is success
// 1 is semaphore error
// 2 is shared memory error
int Params::destroy_communication()
{
    send_end_code_python();

    // Announce for one last time that the semaphore is free again so that python can quit
    printf("Final release of the semaphore and send_code followed by a 5 second pause\n"); 
    rc = release_semaphore(pSemaphore);
    sleep(5); // race condition, where the python takes 5 seconds to quit

    printf("Final wait to acquire the semaphore\n"); 
    rc = acquire_semaphore(pSemaphore);
    if (!rc) 
    {
        printf("Destroying the shared memory.\n");

        rc = munmap(pSharedMemory_code, (size_t)params.size); // Un mmap the memory
        if (rc) 
        {
            printf("Unmapping the memory failed\n");
            return 2;
        }
        
        if (-1 == close(fd)) // close file descriptor 
        {
            printf("Closing the memory's file descriptor failed\n");
            return 2;
        }
    
        rc = shm_unlink(params.shared_memory_name.c_str()); // destroy the shared memory.
        if (rc) 
        {
            printf("Unlinking the memory failed\n");
            return 2;
        }
    }

    printf("Destroying the semaphore.\n");
    // Clean up the semaphore
    rc = sem_close(pSemaphore);
    if (rc) 
    {
        printf("Closing the semaphore failed\n");
        return 1;
        
    }
    rc = sem_unlink(params.semaphore_name.c_str());
    if (rc) 
    {
        printf("Unlinking the semaphore failed\n");
        return 1;
        
    }

    return 0;
}

void Params::cleanup()
{
    printf("begining communication cleanup\n");
    int ret_val = destroy_communication();
    if(ret_val == 1)
    {
        printf("Setup python communication returned with error 1 (problems with semaphore) exiting.");
        exit(0);
    }
    else if(ret_val == 2)
    {
        printf("Setup python communication returned with error 2 (problems with shared memory) exiting.");
        exit(0);
    }
    printf("finished communication cleanup\n");
}


// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string Params::gen_random(const int len) 
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

int Params::release_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    rc = sem_post(pSemaphore);
    if(rc) 
    {
        printf("Releasing the semaphore failed\n");
    }
    return rc;
}

int Params::acquire_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    rc = sem_wait(pSemaphore);
    if(rc) 
    {
        printf("Acquiring the semaphore failed\n");
    }
    return rc;
}


void Params::run_params(int local_rank, int games_per_proc)
{
    srand(time(NULL)); // seed rand

    bool print_on = true;
    int iterations_per_move = 3;


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

    std::chrono::time_point<std::chrono::system_clock> game_start_timer, game_end_timer, wait1_start_timer, wait1_end_timer;
    std::chrono::duration<double, std::nano> game_time_result, wait1_time_result;
    

    // Read in the new neural network model name
    if (print_on) std::cout << "local_rank: " << local_rank << " about to get model path " << std::endl; 
    
    std::string model_name = get_newest_model_name();
    std::string model_path = "data/" + model_name;

    if (print_on) std::cout << "local_rank: " << local_rank << " got the newest model dir" << std::endl;
    if (print_on) std::cout << "local_rank: " << local_rank << " model_name: " << model_name << std::endl;
 


    // Make the two players for the next game
    // I had to change this to MonteCarlo* instead of Player* 
    // because if it was Player*, then when I want to call
    // get_saved_q() in the play_game function,
    // the compiler complains

    /*
    param.cpp: In function ‘int play_game(Engine*, std::vector<Player*>, int*, float**)’:
    param.cpp:216:31: error: ‘class Player’ has no member named ‘get_saved_q’
                     MCc = players[BLACK]->get_saved_q();
                                                                 ^
    param.cpp:227:31: error: ‘class Player’ has no member named ‘get_saved_q’
                     MCc = players[WHITE]->get_saved_q();
                                                                 ^
    make: *** [param] Error 1
    */

    // But changing it to MonteCarlo* works...
    // Although if we want to use a minimax in part of the parameterization,
    // it won't work as it is now.
    // std::vector<Player*> players;
    std::vector<MonteCarlo*> players;




    //////////////////////// CODE FOR MONTECARLO PYTHON COMMUNICATOR //////////////////////



    // asigning values to struct
    params.semaphore_name = gen_random(10);
    params.shared_memory_name = gen_random(10);
    params.size = 4096;
    // params.semaphore_name = "other_stuff";
    // params.shared_memory_name = "shared_memory_1";
    params.permissions = 0600;

    printf("params - size: %d\n", params.size);
    printf("params - semaphore_name: %s\n", params.semaphore_name.c_str());
    printf("params - shared_memory_name: %s\n", params.shared_memory_name.c_str());
    printf("params - permissions: %d\n", params.permissions);

    call_python_script_helper(params, model_name);



    // inits





    // spawn pythoner
    int ret_val = setup_python_communication();
    if(ret_val == 1)
    {
        printf("Setup python communication returned with error 1 (problems with semaphore) exiting.");
        exit(0);
    }
    else if(ret_val == 2)
    {
        printf("Setup python communication returned with error 2 (problems with shared memory) exiting.");
        exit(0);
    }


    ///////////////////////////////////////////////////////////////////////////////////////







    /* currently, there's an error when a MonteCarlo player tries to play */
    // black must be pushed onto the vector first
    players.push_back(new MonteCarlo(BLACK, e, model_name, iterations_per_move, true, 
                        pSemaphore, pSharedMemory_code, pSharedMemory_rest)); // black
    players.push_back(new MonteCarlo(WHITE, e, model_name, iterations_per_move, true,
                        pSemaphore, pSharedMemory_code, pSharedMemory_rest)); // white
     
    // Variable for the number of moves in the game
    int* num_moves = (int*) calloc(1, sizeof(int));
    int* total_MC_chances = (int*) calloc(1, sizeof(int));
        
    /*~*~*~*~*~ Possibly decrement a game timer here ~*~*~*~*~*/
    game_start_timer = std::chrono::system_clock::now();
    // Loop over the number of games we're playing per processor per model
    for (int i = 0; i < games_per_proc; ++i)
    {
        total_MC_chances[0] = 0;
        if(print_on) std::cout << "Playing game " << i + games_per_proc * local_rank << " on processor: " << local_rank << std::endl;
        float **MC_chances = (float **) calloc(e->get_max_move_length(), sizeof(float *));
        float *saved_values = (float *) calloc(e->get_max_move_length(), sizeof(float));
        // result holds 0 for black win, 1 for white win, and 2 for draw
        int result = play_game(e, players, num_moves, MC_chances, total_MC_chances, saved_values, print_on);
        if(print_on) std::cout << "Processor: " << local_rank << " finished playing game, saving data now to path: " << model_path << std::endl;
        // save the game's info.
        save_game_info(model_path, local_rank, i, e, num_moves[0], result, MC_chances, saved_values);
        if(print_on) std::cout << "Processor: " << local_rank << " finished saving data, resetting engine now." << std::endl;

        // free and reset the engine for a new game
        for(int j = 0; j < total_MC_chances[0]; j++)
        {
            free(MC_chances[j]);
        }
        free(MC_chances);
        free(saved_values);
        e->reset_engine();
    }
    players.clear();

    if (print_on) std::cout << "Processor: " << local_rank << " finished playing and saving " << games_per_proc << " games." << std::endl;
        
    /*~*~*~*~*~ Possibly increment game timer here ~*~*~*~*~*/
    game_end_timer = std::chrono::system_clock::now();
    game_time_result = cast_nano2(game_end_timer-game_start_timer);
        
    /*~*~*~*~*~ Possibly decrement wait1 timer here ~*~*~*~*~*/
    wait1_start_timer = std::chrono::system_clock::now();    

    /*~*~*~*~*~ Possibly increment wait1 timer here ~*~*~*~*~*/
    wait1_end_timer = std::chrono::system_clock::now();
    wait1_time_result = cast_nano2(wait1_end_timer - wait1_start_timer);


    if (print_on) printf("Finished timing, now saving timer info\n");

    /*~*~*~ Possibly save all of the timer information ~*~*~*/
    save_timer_info(model_path, local_rank, game_time_result, wait1_time_result);

    if (print_on) printf("Cleaning up engine and players\n");

    // clean up
    players[BLACK]->cleanup();
    players[WHITE]->cleanup();

    delete(players[0]);
    delete(players[1]);
    players.clear();
    players.shrink_to_fit();
    e->clean_up();
    delete(e);
    free(num_moves);

    cleanup();

    if (print_on) printf("param_serial has finished\n");
}


int main(int argc, char * argv[])
{
    int local_rank = -1; 
    int games_per_proc = -1;

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
    }


    Params* p = new Params();
    p->run_params(local_rank, games_per_proc);
    return 0;
}