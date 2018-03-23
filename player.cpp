#include "player.hpp"
#include "engine.hpp"
#include <stdlib.h>
#include <chrono>
#include <cstring>

int my_max(int a, int b) 
{
    int diff = a - b;
    int dsgn = diff >> 31;
    return a - (diff & dsgn);
}
int my_min(int a, int b) 
{
    int diff = a - b;
    int dsgn = diff >> 31;
    return b + (diff & dsgn);
}

// PLAYER


Player::Player(int col, Engine* engine)
{
    color = col;
    e = engine;
}

int Player::get_color()
{
    return(this->color);
}


// RAND


Rand::Rand(int col, Engine* engine) : Player(col, engine)
{
    //
}

int Rand::move(int* move_list)
{
    // int num_moves = move_list[0];
    if(move_list[0] == 0)
    {
        return -1;
    }
    int move_choice = rand() % move_list[0];
    move_list++;

    return(move_list[move_choice]);
}


Minimax::Minimax(int col, Engine* engine) : Player(col, engine)
{
    depth_search_limit = 5;
}

int* Minimax::copy_move_list(int* move_list)
{
    int* new_move_list = (int*) malloc(move_list[0] * sizeof(int));
    std::memcpy(new_move_list, move_list + 1, move_list[0] * sizeof(int));
    return(new_move_list);
}

int Minimax::decode_terminal_score(int term)
{
    if(term == 2)
    {
        return(0);
    }
    else if(term == 1)
    {
        return(999999);
    }
    else
    {
        return(-999999);
    }
}