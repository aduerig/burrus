#ifndef ENGINE_H
#define ENGINE_H


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <bitset>
#include <unordered_map>

typedef unsigned long long U64;

enum MoveType
{
    REGULAR = 0,
    CASTLE = 1,
    ENPASSANT = 2,
    PROMOTION = 3
};

// encoding board as 2 element u64 array for ease of indexing later
struct position
{
    // U64 black;
    // U64 white;
    U64 board[2]; // first black, then white
};

struct precomputed_masks
{
   U64 left_diag_mask_excluded;
   U64 right_diag_mask_excluded;
   U64 file_mask_excluded;
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
        U64 *diag_left_mask;
        U64 *diag_right_mask;

        // pushing and popping moves from stack

        void push_move(int move);
        void pop_move();
        void flip_stones(U64 stone, U64 own_occ, U64 opp_occ, int color);

        //printing
        void print_bit_rep(U64 num);
        void print_char();


        void init_engine();
        void reset_engine();
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
        U64 get_all();

        // move encoding and decoding
        void print_move_info(int move);

        // pushing and popping moves from stack
        void stack_push();
        void stack_pop();

        int encode_move(U64 stone_square, int color);
        int decode_color(int move);
        int decode_loc(int move);

        // board helper functions
        int bitboard_to_square(U64 piece);
        U64 square_to_bitboard(int square);

        //move gen helpers
        bool check_legal(int move, int color);
        int* generate_moves(int color);

        // terminating conditions
        int score_board();
        int is_terminal(int* moves);

        //move_gen
        U64 cardinal_moves(int color);
        U64 north_moves(U64 mine, U64 prop, U64 empty);        
        U64 south_moves(U64 mine, U64 prop, U64 empty);
        U64 east_moves(U64 mine, U64 prop, U64 empty);
        U64 west_moves(U64 mine, U64 prop, U64 empty);

        U64 diag_moves(int color);
        U64 north_east_moves(U64 mine, U64 prop, U64 empty);        
        U64 south_east_moves(U64 mine, U64 prop, U64 empty);
        U64 south_west_moves(U64 mine, U64 prop, U64 empty);
        U64 north_west_moves(U64 mine, U64 prop, U64 empty);
        
        U64 one_rook_attacks(U64 rook, U64 occ);

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

    private:
        int max_move_length;
        int move_arr_size;

        int board_stack_index;
        U64* board_stack;

        precomputed_masks square_masks[64];

        std::unordered_map<U64, int> lsb_lookup;
};

#endif