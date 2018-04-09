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
    Node* children_nodes;
    int num_children;
    bool expanded;
    bool terminal;
    int color;
    int move;
    int visits;
    float policy; // the inital policy value found from the net running on the above node
    float value; // the inital set of the value net run on this board position
    float tree_value; // all sub nodes will add their value functions to it
    float q_val; // updated in the backprop stats
};

class MonteCarlo: public Player
{
    public:
        MonteCarlo(int col, Engine* engine, std::string m_path, bool training);
        Node* init_default_node();
        int* generate_moves_wrapper(int p_color);
        void push_move_wrapper(int move, int p_color);
        Node* expand_node(Node* node, int* move_list, int p_color);
        Node* traverse_tree(Node* node);
        int move(int* move_list);
        void backup_stats(Node* node);
        float compute_puct(Node* node);
        Node* max_child(Node* node);

    private:
        std::string model_path;
        int max_sims;

        // think about collisions, backprop will not go to correct parent
        std::unordered_map<U64, Node*> node_storage;
        
        Node* curr_root;
        bool is_training;
        float explore_constant = 1;
        float saved_value;
        float* saved_q;
};


#endif