#ifndef PLAYER_H
#define PLAYER_H

#include "communicator.hpp"
#include "engine.hpp"
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>

#include <errno.h> 
#include <unistd.h> 
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdarg.h>
#include <cstdio>
#include <string>
#include <inttypes.h>
#include <unistd.h>
#include <vector>
#include <random>
#include <math.h>

// typedef int (*foo_ptr_t)( int );
typedef void (*fp)(int);

/* MRD I put some timers in param.cpp */
std::chrono::duration<double, std::nano> cast_nano2(std::chrono::duration<double> x);

class Player
{
    public:
        int color;
        Engine *e;

        Player(int col, Engine *engine);
        int get_color();
        virtual int move(int* move_list)=0;
        virtual void cleanup()=0;

};

class Rand: public Player
{
    public:
        Rand(int col, Engine* engine);
        int move(int* move_list);
        void cleanup();
};

class Human: public Player
{
    public:
        Human(int col, Engine* engine);
        int move(int* move_list);
        int parse_coords(std::string seq);
        void cleanup();
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
        void cleanup();

    private:
        int depth_search_limit;
        int node_count;
};

struct Node
{
    U64 board_hash;
    Node* parent_node;
    Node* children_nodes;
    int num_children;
    bool expanded;
    bool is_terminal;
    bool is_pass;
    int color;
    int move;
    int visits;
    float policy; // the inital policy value found from the net running on the above node
    float value; // the inital set of the value net run on this board position
    float calced_q; // all sub nodes will add their value functions to it
    float total_action_value; // updated in the backprop stats
};

class MonteCarlo: public Player
{
    public:
        MonteCarlo(int col, Engine* engine, std::string m_path, int sims, bool training, 
            PyCommunicator* arg_comm);
        
        int move(int* move_list);
        Node* traverse_tree(Node* node, int p_color);
        void expand_node(Node* node);
        void expand_node(Node* node, int* move_list);
        void backup_stats(Node* node);
        Node* max_child_puct(Node* node);
        Node* max_child_visits(Node* node);
        float compute_puct(Node* node);
        int node_argmax(Node* node, int num_nodes);
        Node* choose_node_random(Node* node);
        void calc_action_probs(Node* node);
        void cleanup();

        // model and communication
        void load_board_state_to_int_arr_sender(int p_color);
        float* send_and_recieve_model_data(int p_color);
        void fill_random_ints(int* ints_to_fill, int num_ints);
        void add_dirichlet_noise(float epsilon, float alpha);

        // helper funcs
        int color_multiplier(int p_color);
        int get_true_result();
        void init_default_node(Node* node);
        Node* create_default_node();
        int* generate_moves_wrapper(int p_color);
        void push_move_wrapper(int move, int p_color);
        void print_node_info(Node* node);
        void print_best_graph(Node* node);
        void print_all_subnodes(Node* node);
        void print_all_subnodes_helper(Node* node, int depth);

        // saver funcs
        float* get_saved_action_probs();
        float get_saved_value();

        int no_decision;


    private:
        std::string model_path;
        int max_sims;

        // think about collisions, backprop will not go to correct parent
        std::unordered_map<U64, Node*> node_storage;
        U64 node_storage_counter;
        
        Node* curr_root;
        bool is_training;
        float explore_constant;
        float saved_value;
        float* saved_action_probs;
        bool print_on;

        // communication variables
        PyCommunicator* comm;

        // data holders
        int num_ints_send;
        int num_floats_recieve;

        int32_t* int_arr_sender;
        float* scaled_probabilities;

        int temperature;
    };


#endif
