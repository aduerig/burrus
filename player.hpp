#ifndef PLAYER_H
#define PLAYER_H

#include "engine.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// typedef int (*foo_ptr_t)( int );
typedef void (*fp)(int);

class Player
{
    public:
        int color;
        Engine *e;

        Player(int col, Engine *engine);
        int get_color();
        virtual int move(int* move_list)=0;

};

class Rand: public Player
{
    public:
        Rand(int col, Engine* engine);
        int move(int* move_list);
};

class Human: public Player
{
    public:
        Human(int col, Engine* engine);
        int move(int* move_list);
        int parse_coords(std::string seq);
};

class Minimax: public Player
{
    public:
        Minimax(int col, Engine* engine, int search_limit);
        // int minimax_white(int depth);
        // int minimax_black(int depth);
        // int minimax(int depth, int color);
        // int move(int* move_list);
        int* copy_move_list(int* move_list);
        // double simple_board_eval_helper(unsigned long long pieces, double val);
        // double simple_board_eval(int color, int* move_list);
        int decode_terminal_score(int term);

    private:
        int depth_search_limit;
        int node_count;
};


#endif