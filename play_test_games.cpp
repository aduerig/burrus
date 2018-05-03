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


int play_game(Engine* e, std::vector<Player*> players, int* num_moves, int print_level)
{
    int move;
    int* move_list;
    
    int color = BLACK;
    int player_tracker = players[BLACK]->get_color();
    num_moves[0] = 0;

    if (print_level > 0) e->print_char();

    move_list = e->generate_moves(color);
    while(e->is_not_terminal(move_list, color))
    {
        move = players[player_tracker]->move(move_list);
        e->push_move(move, color);

        if (print_level > 0) printf("player: %i, color: %i\nmove number %i\n", 
                                    player_tracker, color, num_moves[0]);
        if (print_level > 0) e->print_char();        

        num_moves[0]++;
        if (print_level > 1) std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
        
        color = 1 - color;
        player_tracker = 1 - player_tracker;

        move_list = e->generate_moves(color);
    }
    return e->get_winner();
}


void run_driver(int games_to_play, int iterations_per_move, std::string model_name, 
                int print_level, std::string player1, std::string player2, int depth)
{
    Engine* e = new Engine();

    // std::chrono::time_point<std::chrono::system_clock> game_start_timer, game_end_timer, wait1_start_timer, wait1_end_timer;
    // std::chrono::duration<double, std::nano> game_time_result, wait1_time_result;

    if (model_name == "recent") // Read in the new neural network model name
    {
        model_name = get_newest_model_name();
    }

    if (print_level > 0) std::cout << "playing with model: " << model_name << std::endl;

    PyCommunicator* comm;
    if (player1 == "monte" || player2 == "monte")
    {
        comm = new PyCommunicator(model_name);
        comm->setup_python_communication();
    }

    std::vector<Player*> players;
    

    if (player1 == "monte")
    {
        players.push_back(new MonteCarlo(BLACK, e, model_name, iterations_per_move, false, comm));
    }
    else if (player1 == "rand")
    {
        players.push_back(new Rand(BLACK, e));
    }
    else if (player1 == "human")
    {
        players.push_back(new Human(BLACK, e));
    }
    else if (player1 == "minimax")
    {
        players.push_back(new Minimax(BLACK, e, depth));
    }

    if (player2 == "monte")
    {
        players.push_back(new MonteCarlo(WHITE, e, model_name, iterations_per_move, false, comm));
    }
    else if (player2 == "rand")
    {
        players.push_back(new Rand(WHITE, e));
    }
    else if (player2 == "human")
    {
        players.push_back(new Human(WHITE, e));
    }
    else if (player2 == "minimax")
    {
        players.push_back(new Minimax(WHITE, e, depth));
    }


    int* num_moves = (int*) calloc(1, sizeof(int));    // Variable for the number of moves in the game
    int result_store[3] = {0, 0, 0};
        
    // Loop over the number of games we are playing
    for (int i = 0; i < games_to_play; i++)
    {
        if(print_level > 0) std::cout << "Playing game " << i << std::endl;
        result_store[play_game(e, players, num_moves, print_level)]++;
        if(print_level > 0) std::cout << "finished playing game" << std::endl;

        e->reset_engine(); // free and reset the engine for a new game
    }

    if (print_level > 0) std::cout << "Finished playing " << games_to_play << " games." << std::endl;
    if (print_level > 0) printf("Cleaning up engine and players\n");

    printf("out of %i games\nwhite won: %i\nblack won %i\ndraws %i\n"
           "white win percentage (excluding draws): %f\n", 
            games_to_play, result_store[1], result_store[0], result_store[2], 
            (float) result_store[1] / games_to_play);

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

    if (player1 == "monte" || player2 == "monte")
    {
        comm->clean_up();
        delete(comm);
    }

    ///////////////////////////////////////

    printf("driver has finished\n");
}

// ./driver -iter 20 -ngames 10 -name model_0
int main(int argc, char * argv[])
{
    int iterations_per_move = 50; 
    int games_to_play = 1;
    std::string model_name = "recent";
    std::string player1 = "monte"; // monte, rand, human, minimax
    std::string player2 = "rand";
    int print_level = 0;
    int depth = 0;

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
        if (strcmp(argv[i], "-player1") == 0)
        {
            player1 = argv[i+1];
        }
        if (strcmp(argv[i], "-player2") == 0)
        {
            player2 = argv[i+1];
        }
        if (strcmp(argv[i], "-depth") == 0)
        {
            depth = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-print") == 0)
        {
            print_level = atoi(argv[i+1]);
        }
    }

    printf("iter: %i, games: %i, model_name: %s, player1: %s, player2: %s, depth: %i\n", 
            iterations_per_move, games_to_play, model_name.c_str(), player1.c_str(), player2.c_str(), depth);


    bool print_level_bool = false;
    if(print_level)
    {
        print_level_bool = true;
    }

    srand(time(NULL)); // seed rand    
    run_driver(games_to_play, iterations_per_move, model_name, print_level, player1, player2, depth);
    
    return 0;
}