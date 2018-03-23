// to compile:  g++ engine.hpp engine.cpp driver.cpp -std=c++14 -o run


#include "engine.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>




int main()
{
    Engine* e = new Engine();
    e->reset_engine();

    // e->pos.board[1] = 0b0000000000000000000000000000000000010000000000001110111110000000; // places white pieces
    // e->print_bit_rep(e->get_color(1)); // print whites pieces
    printf("the board\n");
    e->print_char();

    // std::cout << "cardinal_moves for white" << std::endl;
    // e->print_bit_rep(e->cardinal_moves(1));

    printf("all possible white move squares\n");
    e->print_bit_rep(e->cardinal_moves(1) | e->diag_moves(1));

    printf("moves for white\n");
    int* move_list = e->generate_moves(1);
    int num_moves = move_list[0];
    move_list++;
    for(int i = 0; i < num_moves; i++)
    {
        printf("move %i, looks like\n", move_list[i]);
        e->push_move(move_list[i]);
        e->print_char();
        e->pop_move();
    }

    printf("finished driver\n");

    return 0;
}