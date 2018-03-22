#include "engine.hpp"
#include "player.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

// g++ bitboard.hpp bitboard.cpp player.hpp player.cpp play.cpp -std=c++14 -o run

//optimized
// g++ bitboard.hpp bitboard.cpp player.hpp player.cpp play.cpp -std=c++14 -O3 -funroll-loops -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized  -o run


// to profile: valgrind --tool=callgrind ./play
// more profiling info: http://zariko.taba.free.fr/c++/callgrind_profile_only_a_part.html

//memleaks and stuff: valgrind --tool=memcheck --leak-check=yes --log-file=out.log ./play

// cache misses: valgrind --tool=cachegrind ./play
// cachegrind read: cg_annotate cachegrind.out.601


std::string color_to_string(int color)
{
    if(color == 1)
    {
        return "White";
    }
    else if(color == 0)
    {
        return "Black";
    }
    else
    {
        return "Draw";
    }
}

std::chrono::duration<double, std::nano> cast_nano(std::chrono::duration<double> x)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(x);
}

int play_game(Engine* e, std::vector<Player*> players, int* num_moves)
{
    int moves_made = 0;
    int color = 1;
    int term;
    int move;
    int* move_list;

    while(1)
    {
        move_list = e->generate_legal_moves(color);

        term = e->is_terminal(color, move_list);
        if(term)
        {
            int winner = get_winner();
            std::cout << "game over, winner is: " << color_to_string(winner) << " in " << moves_made  << " moves" << std::endl;
            e->print_char();
            return(winner);
        }

        std::cout << color_to_string(color) << " to move." << std::endl;
        std::cout <<  "moves avaliable: " << move_list[0] << std::endl;
        move = players[color]->move(move_list);
        std::cout <<  "making move: " << move << std::endl;
        e->print_move_info(move);
        e->push_move(move);
        num_moves[0]++;
        e->print_char();
        std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

        moves_made++;
        color = 1-color;
    }
}


int main()
{
    srand(time(NULL));
    Engine* e = new Engine();
    
    std::vector<Player*> players;
    players.push_back(new Rand(0, e)); // black
    players.push_back(new Rand(1, e)); // white

    std::chrono::time_point<std::chrono::system_clock> t1, t2;
    std::chrono::duration<double, std::nano> time_cast_result;

    num_moves[0] = 0;
    t1 = std::chrono::system_clock::now();

    int result;
    num_moves[0] = 0;
    
    for(int i = 0; i < 10; i++)
    {
        result = play_game(e, players, num_moves);
        e->reset_engine();  
    }

    t2 = std::chrono::system_clock::now();
    time_cast_result = cast_nano(t2 - t1);
    double temp = (double) time_cast_result.count() / num_moves[0];

    std::cout << "total moves made: " << num_moves[0] << " with " << temp << " nanoseconds per move" << std::endl;
    std::cout << "resulting in NPS of: " << 1.0 / (temp * .000000001) << std::endl;

    players.clear();
    delete(e);
    free(num_moves);
    return 0;
}