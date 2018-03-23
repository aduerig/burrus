#ifndef ENGINE_H
#define ENGINE_H


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <bitset>
#include <unordered_map>

typedef unsigned long long U64;

enum Color
{   
    BLACK = 0,
    WHITE = 1
};

// encoding board as 2 element u64 array for ease of indexing later
struct position
{
    // U64 black;
    // U64 white;
    // U64 board[2]; // first black, then white
    U64 white_board;
    U64 black_board;
    U64 pass_counter = 0;
};

struct precomputed_masks
{
   U64 left_diag_mask_excluded;
   U64 right_diag_mask_excluded;
   U64 file_mask_excluded;
   U64 file_mask;
   U64 rank_mask;
};


class Engine
{
    public:
        Engine();
        Engine(U64 *board_data);


        //Vars
        position pos;

        U64 *row_mask;
        U64 *col_mask;

        U64 *inv_row_mask;
        U64 *inv_col_mask;

        U64 *diag_left_mask;
        U64 *diag_right_mask;

        // pushing and popping moves from stack

        void push_white_move(int move);
        void push_black_move(int move);
        void pop_move();
        void flip_white_stones(U64 stone, int square, U64 own_occ, U64 opp_occ);
        void flip_black_stones(U64 stone, int square, U64 own_occ, U64 opp_occ);

        //printing
        void print_bit_rep(U64 num);
        void print_char();


        void init_engine();
        void reset_engine();
        void clean_up();
        void init_position();
        void init_position(U64 *board_data);
        void init_lsb_lookup();

        void fill_square_masks();

        int encode_move(int stone_square, int color);
        
        // masks
        void init_masks();
        U64 make_col_mask(U64 mask);
        void fill_col_mask_arr();
        U64 make_row_mask(U64 mask);
        void fill_row_mask_arr();
        U64 make_diag_left_mask(U64 mask);
        void fill_diag_left_mask_arr();
        U64 make_diag_right_mask(U64 mask);
        void fill_diag_right_mask_arr();

        // maximum number of moves allowed in the game
        int get_max_move_length();
        U64 get_color(int color);
        std::string color_to_string(int color);
        U64 get_all();

        // move encoding and decoding
        void print_move_info(int move);

        // pushing and popping moves from stack
        void stack_push();
        void stack_pop();

        // board helper functions
        int bitboard_to_square(U64 piece);
        U64 square_to_bitboard(int square);

        //move gen helpers
        bool check_legal(int move, int color);
        int* generate_white_moves();
        int* generate_black_moves();

        // terminating conditions
        int score_board();
        int get_winner();
        int is_not_terminal(int* moves, int color);
        int is_terminal(int* moves, int color);

        //move_gen
        
        U64 one_rook_attacks(U64 rook, U64 occ, int square);
        U64 one_bishop_attacks_ANTI(U64 bishop, int square, U64 occ);
        U64 one_bishop_attacks(U64 bishop, U64 occ);

        // get rank/file/diag info
        int get_rank(U64 num);
        int get_file(U64 num);
        int get_diag(int rank, int file);

        // bitboard tricks
        int lsb_digit(U64 board);
        U64 lsb_board(U64 board);
        U64 msb_digit(U64 board);
        U64 msb_board(U64 board);

        // reversing and flipping
        U64 reverse_8_bits(U64 x);
        U64 reverse_64_bits(U64 x);
        U64 horizontal_flip(U64 x);
        U64 vertical_flip(U64 x);

        
        U64 cardinal_black_moves();
        U64 cardinal_white_moves();
        U64 north_moves(U64 mine, U64 prop, U64 empty);        
        U64 south_moves(U64 mine, U64 prop, U64 empty);
        U64 east_moves(U64 mine, U64 prop, U64 empty);
        U64 west_moves(U64 mine, U64 prop, U64 empty);

        U64 diag_white_moves();
        U64 diag_black_moves();
        U64 north_east_moves(U64 mine, U64 prop, U64 empty);        
        U64 south_east_moves(U64 mine, U64 prop, U64 empty);
        U64 south_west_moves(U64 mine, U64 prop, U64 empty);
        U64 north_west_moves(U64 mine, U64 prop, U64 empty);


        //floods

        U64 all_rook_attacks(U64 rook, U64 occ);
        U64 all_bishop_attacks(U64 rook, U64 occ);

        U64 vert_flood(U64 rooks, U64 prop);
        U64 hori_flood(U64 rooks, U64 prop);

        U64 north_flood(U64 rooks, U64 prop);
        U64 south_flood(U64 rooks, U64 prop);
        U64 east_flood(U64 rooks, U64 prop);
        U64 west_flood(U64 rooks, U64 prop);

        U64 left_diag_flood(U64 bishops, U64 prop);
        U64 right_diag_flood(U64 bishops, U64 prop);

        U64 north_east_flood(U64 bishops, U64 prop);
        U64 south_east_flood(U64 bishops, U64 prop);
        U64 south_west_flood(U64 bishops, U64 prop);
        U64 north_west_flood(U64 bishops, U64 prop);

    private:
        int max_move_length;
        int move_arr_size;

        int board_stack_index;
        U64* board_stack;

        int* move_list;

        precomputed_masks square_masks[64];

        std::unordered_map<U64, int> lsb_lookup;
};

#endif