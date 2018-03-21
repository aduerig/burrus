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
    e->print_bit_rep(e->get_color(1)); // print whites pieces
    e->print_char();

    std::cout << "cardinal_moves for white" << std::endl;
    e->print_bit_rep(e->cardinal_moves(1));

    std::cout << "finished driver" << std::endl;

    return 0;
}


// for(int i = 0; i < 64; i++)
// {
// 	;
// }