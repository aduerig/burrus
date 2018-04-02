#ifndef PLAYER_H
#define PLAYER_H

#include "engine.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>

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
        int minimax_white(int depth, double alpha, double beta);
        int minimax_black(int depth, double alpha, double beta);
        int move(int* move_list);
        int* copy_move_list(int* move_list);
        int decode_terminal_score(int term);

    private:
        int depth_search_limit;
        int node_count;
};

//class impl of node
// class Node
// {
//     public:
//         Node(U64 b_hash);

//     private:
//         U64 board_hash;
// }

// struct impl of node
struct Node
{
    U64 board_hash;
    Node* parent_node;
    int visits;
    int score;
    float policy;
    float value;
};

class MonteCarlo: public Player
{
    public:
        MonteCarlo(int col, Engine* engine, std::string m_path);
        int move(int* move_list);
        Node* init_default_node();
        Node* expand_node(Node* node);

    private:
        std::string model_path;
        int max_sims;
        std::unordered_map<U64, Node*> node_storage;
        Node* curr_root;
};


#endif