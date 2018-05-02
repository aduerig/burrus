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
#include "communicator.hpp"

// g++ bitboard.hpp bitboard.cpp player.hpp player.cpp play.cpp -std=c++14 -o run

//optimized
// g++ bitboard.hpp bitboard.cpp player.hpp player.cpp play.cpp -std=c++14 -O3 -funroll-loops -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized  -o run


// to profile: valgrind --tool=callgrind ./play
// more profiling info: http://zariko.taba.free.fr/c++/callgrind_profile_only_a_part.html

//memleaks and stuff: valgrind --tool=memcheck --leak-check=yes --log-file=out.log ./play

// cache misses: valgrind --tool=cachegrind ./play
// cachegrind read: cg_annotate cachegrind.out.601




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

int play_game(Engine* e, std::vector<Player*> players, int* num_moves, bool print_level)
{
    int move;
    int* move_list;
    num_moves[0] = 0;
    move_list = e->generate_black_moves();

    if (print_level) e->print_char();
    while(e->is_not_terminal(move_list, BLACK))
    {
        move = players[BLACK]->move(move_list);
        e->push_black_move(move);
        if (print_level) printf("move number %i\n", num_moves[0]); //
        if (print_level) e->print_char();        
        num_moves[0]++;

        //TEMP
        // std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
        //TEMP

        move_list = e->generate_white_moves();
        if(e->is_terminal(move_list, WHITE))
        {
            break;
        }
        move = players[WHITE]->move(move_list);
        e->push_white_move(move);
        if (print_level) printf("move number %i\n", num_moves[0]);
        if (print_level) e->print_char();
        num_moves[0]++;

        // TEMP
        // std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
        // TEMP

        move_list = e->generate_black_moves();
    }
    return e->get_winner();
}


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


void run_driver(int games_to_play, int iterations_per_move, std::string model_name, bool print_level, int depth)
{
    Engine* e = new Engine();

    // std::chrono::time_point<std::chrono::system_clock> game_start_timer, game_end_timer, wait1_start_timer, wait1_end_timer;
    // std::chrono::duration<double, std::nano> game_time_result, wait1_time_result;
    

    if(model_name == "recent") // Read in the new neural network model name
    {
        model_name = get_newest_model_name();
    }


    if (print_level) std::cout << "playing with model: " << model_name << std::endl;


    PyCommunicator* comm = new PyCommunicator(model_name);
    comm->setup_python_communication();


    std::vector<Player*> players;

    players.push_back(new MonteCarlo(BLACK, e, model_name, iterations_per_move, false, comm)); // white
    players.push_back(new Rand(WHITE, e)); // black
    // players.push_back(new Minimax(BLACK, e, depth)); // white
    // players.push_back(new MonteCarlo(WHITE, e, model_name, iterations_per_move, false, 
                        // pSemaphore, pSharedMemory_code, pSharedMemory_rest)); // white

    int* num_moves = (int*) calloc(1, sizeof(int));    // Variable for the number of moves in the game
    int result_store[3] = {0, 0, 0};
        
    // Loop over the number of games we are playing
    for (int i = 0; i < games_to_play; i++)
    {
        if(print_level) std::cout << "Playing game " << i << std::endl;
        result_store[play_game(e, players, num_moves, print_level)]++;
        if(print_level) std::cout << "finished playing game" << std::endl;

        e->reset_engine(); // free and reset the engine for a new game
    }

    if (print_level) std::cout << "Finished playing " << games_to_play << " games." << std::endl;
    if (print_level) printf("Cleaning up engine and players\n");

    printf("out of %i games\nwhite won: %i\nblack won %i\ndraws %i\nwhite win percentage: %f\n", 
                games_to_play, result_store[1], result_store[0], result_store[2], (float)result_store[1] / games_to_play);

    /////////////// clean up //////////////
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
    ///////////////////////////////////////

    printf("driver has finished\n");
}


// example call
// ./driver -iter 20 -ngames 10 -name model_0
int main(int argc, char * argv[])
{
    
    // int iterations_per_move = 500;
    // int games_to_play = 10;
    // std::string model_name = "model_5";

    int iterations_per_move = -1; 
    int games_to_play = -1;
    std::string model_name = "hurglblrg";
    int depth = 0;
    int print_level = 0;

    for (int i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], "-iter") == 0)
        {
            iterations_per_move = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-ngames") == 0)
        {
            games_to_play = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-name") == 0)
        {
            model_name = argv[i+1];
        }
        if (strcmp(argv[i], "-print") == 0)
        {
            print_level = atoi(argv[i+1]);
        }
    }

    printf("iter: %i, games: %i, model_name: %s\n", iterations_per_move, games_to_play, model_name.c_str());

    if(model_name == "hurglblrg" || games_to_play == -1 || iterations_per_move == -1)
    {
        printf("improper argument string, you need -name, -ngames, and -iter\n -print is optional arg (default off)\n");
        exit(0);
    }

    bool print_level_bool = false;
    if(print_level)
    {
        print_level_bool = true;
    }

    srand(time(NULL)); // seed rand    
    run_driver(games_to_play, iterations_per_move, model_name, print_level_bool, depth);
    
    return 0;
}