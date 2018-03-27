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


std::chrono::duration<double, std::nano> cast_nano(std::chrono::duration<double> x)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(x);
}

int play_game(Engine* e, std::vector<Player*> players, int* num_moves)
{
    int move;
    int* move_list;

    // printf("inital board state\n");
    e->print_char();

    move_list = e->generate_black_moves();
    while(e->is_not_terminal(move_list, BLACK))
    {
        // BLACKS MOVE

        // std::cout << e->color_to_string(BLACK) << " to move." << std::endl;
        // std::cout <<  "moves avaliable: " << move_list[0] << std::endl;
        move = players[BLACK]->move(move_list);
        // std::cout <<  "making move: " << move << std::endl;
        e->push_black_move(move);
        num_moves[0]++;
        e->print_char();
        // printf("score of board above %i\n", e->score_board());
        // std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

        move_list = e->generate_white_moves();
        if(e->is_terminal(move_list, WHITE))
        {
            break;
        }
        // WHITES MOVE

        // std::cout << e->color_to_string(WHITE) << " to move." << std::endl;
        // std::cout <<  "moves avaliable: " << move_list[0] << std::endl;
        move = players[WHITE]->move(move_list);
        // std::cout <<  "making move: " << move << std::endl;
        e->push_white_move(move);
        num_moves[0]++;
        e->print_char();
        // printf("score of board above %i\n", e->score_board());
        // std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

        move_list = e->generate_black_moves();
    }
    int winner = e->get_winner();
    // std::cout << "game over, winner is: " << e->color_to_string(winner) << " in " << num_moves[0]  << " moves" << std::endl;
    // e->print_char();
    return winner;
}


int main()
{
    srand(time(NULL));
    Engine* e = new Engine();
    
    std::vector<Player*> players;
    // warinign players must be instaniated in the right order, 0 then 1
    // players.push_back(new Rand(0, e)); // black
    players.push_back(new Human(0, e)); // white
    players.push_back(new Minimax(1, e, 10)); // white
    // players.push_back(new Rand(1, e)); // black

    std::chrono::time_point<std::chrono::system_clock> t1, t2;
    std::chrono::duration<double, std::nano> time_cast_result;

    int* num_moves = (int*) malloc(sizeof(int));

    num_moves[0] = 0;
    t1 = std::chrono::system_clock::now();

    int num_games = 1;
    num_moves[0] = 0;
    int result_store[3] = {0, 0, 0};
    
    for(int i = 0; i < num_games; i++)
    {
        result_store[play_game(e, players, num_moves)]++;
        e->reset_engine();
        // num_moves[0] = 0;
    }

    // timing fun

    t2 = std::chrono::system_clock::now();
    time_cast_result = cast_nano(t2 - t1);
    double temp = (double) time_cast_result.count() / num_moves[0];
    double temp2 = (double) time_cast_result.count() / num_games;

    std::cout << "total moves made: " << num_moves[0] << " with " << temp << " nanoseconds per move" << std::endl;
    std::cout << "total games played: " << num_games << " with " << temp2 << " nanoseconds per game" << std::endl;
    std::cout << "resulting in nodes per second of: " << 1.0 / (temp * .000000001) << std::endl;
    std::cout << "resulting in games per second of: " << 1.0 / (temp2 * .000000001) << std::endl;

    printf("out of %i games\nwhite won: %i\nblack won %i\ndraws %i\nwhite win percentage: %f\n", 
                    num_games, result_store[1], result_store[0], result_store[2], (float)result_store[1] / num_games);

    // clean up
    delete(players[0]);
    delete(players[1]);
    players.clear();
    players.shrink_to_fit();
    e->clean_up();
    delete(e);
    free(num_moves);
    return 0;
}