#include "engine.hpp"
#include "player.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>

// g++ bitboard.hpp bitboard.cpp player.hpp player.cpp play.cpp -std=c++14 -o run

//optimized
// g++ bitboard.hpp bitboard.cpp player.hpp player.cpp play.cpp -std=c++14 -O3 -funroll-loops -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized  -o run


// to profile: valgrind --tool=callgrind ./play
// more profiling info: http://zariko.taba.free.fr/c++/callgrind_profile_only_a_part.html

//memleaks and stuff: valgrind --tool=memcheck --leak-check=yes --log-file=out.log ./play

// cache misses: valgrind --tool=cachegrind ./play
// cachegrind read: cg_annotate cachegrind.out.601



int play_game(Engine* e, std::vector<Player*> players, int* num_moves)
{
    int move;
    int* move_list;

    printf("inital board state\n");
    e->print_char();

    move_list = e->generate_black_moves();
    while(e->is_not_terminal(move_list, BLACK))
    {
        // BLACKS MOVE

        std::cout << e->color_to_string(BLACK) << " to move." << std::endl;
        std::cout <<  "moves avaliable: " << move_list[0] << std::endl;
        move = players[BLACK]->move(move_list);
        std::cout <<  "making move: " << move << std::endl;
        e->push_black_move(move);
        num_moves[0]++;
        e->print_char();
        printf("score of board above %i\n", e->score_board());
        // std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

        move_list = e->generate_white_moves();
        if(e->is_terminal(move_list, WHITE))
        {
            break;
        }
        // WHITES MOVE

        std::cout << e->color_to_string(WHITE) << " to move." << std::endl;
        std::cout <<  "moves avaliable: " << move_list[0] << std::endl;
        move = players[WHITE]->move(move_list);
        std::cout <<  "making move: " << move << std::endl;
        e->push_white_move(move);
        num_moves[0]++;
        e->print_char();
        printf("score of board above %i\n", e->score_board());
        // std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

        move_list = e->generate_black_moves();
    }
    int winner = e->get_winner();
    std::cout << "game over, winner is: " << e->color_to_string(winner) << " in " << num_moves[0]  << " moves" << std::endl;
    e->print_char();
    return winner;
}


int main()
{
    srand(time(NULL));
    Engine* e = new Engine();
    
    std::vector<Player*> players;
    // warning players must be instaniated in the right order, 0 then 1
    int is_training = 0;
    // players.push_back(new Rand(BLACK, e)); // black
    players.push_back(new MonteCarlo(BLACK, e, "model_0", 5, is_training)); // black
    // players.push_back(new MonteCarlo(WHITE, e, "model_0", 500, is_training)); // white
    players.push_back(new Rand(WHITE, e)); // white

    int* num_moves = (int*) malloc(sizeof(int));
    num_moves[0] = 0;

    int num_games = 1;
    int result_store[3] = {0, 0, 0};
    
    for(int i = 0; i < num_games; i++)
    {
        result_store[play_game(e, players, num_moves)]++;
        e->reset_engine();
    }

    std::cout << "total moves made: " << num_moves[0] << std::endl;
    printf("out of %i games\nwhite won: %i\nblack won %i\ndraws %i\nwhite win percentage: %f\n", 
                    num_games, result_store[1], result_store[0], result_store[2], (float)result_store[1] / num_games);

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
    return 0;
}