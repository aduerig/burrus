#include "engine.hpp"
#include <stdlib.h>
#include <unordered_map>
#include <cmath>
#include <strings.h>




Engine::Engine()
{
    init_position();
    init_engine();
}

Engine::Engine(U64* board_data)
{
    init_position(board_data);
    init_engine();
}

void Engine::init_engine()
{
    max_move_length = 200; // game depth
    move_arr_size = 64; // This assumes there are only 64 possible legal moves at any one time (affects move array intilization)

    board_stack = (U64*) malloc(3 * (max_move_length + 1) * sizeof(U64));
    board_stack_index = 0;

    move_list = (int*) malloc(move_arr_size * sizeof(int));

    init_masks();
    init_lsb_lookup();
    fill_square_masks();
}

void Engine::reset_engine()
{
    board_stack_index = 0;
    pos.pass_counter = 0;
    init_position();
}

void Engine::clean_up()
{
    free(move_list);
    free(board_stack);
    free(row_mask);
    free(col_mask);
    free(inv_row_mask);
    free(inv_col_mask);
    free(diag_left_mask);
    free(diag_right_mask);
}

void Engine::init_position()
{
    pos.black_board = 0b0000000000000000000000000001000000001000000000000000000000000000ULL;
    pos.white_board = 0b0000000000000000000000000000100000010000000000000000000000000000ULL;
}

void Engine::init_position(U64 *board_data)
{
    pos.black_board = board_data[0];
    pos.white_board = board_data[1];
}

void Engine::init_lsb_lookup()
{
    for(int i=0; i<64; i++)
    {
        lsb_lookup.insert({(U64) std::pow(2,i),i});
    }
}

void Engine::fill_square_masks()
{
    U64 temp;

    for(int i = 0; i < 64; i++)
    {
        temp = 1ULL << i;

        int diag = get_diag(get_rank(temp), get_file(temp));
        int left_diag = diag >> 5;
        int right_diag = diag & 0x000000000000000F;

        square_masks[i].left_diag_mask_excluded = ~temp & diag_left_mask[left_diag];
        square_masks[i].right_diag_mask_excluded = ~temp & diag_right_mask[right_diag];
        square_masks[i].file_mask_excluded = ~temp & col_mask[get_file(temp)];

        square_masks[i].file_mask = col_mask[get_file(temp)];
        square_masks[i].rank_mask = row_mask[get_rank(temp)];
        // printf("\n");
        // print_bit_rep(square_masks[i].left_diag_mask_excluded);
    }
}

void Engine::init_masks()
{
    row_mask = (U64*) malloc(8 * sizeof(U64));  
    fill_row_mask_arr();

    col_mask = (U64*) malloc(8 * sizeof(U64));
    fill_col_mask_arr();

    inv_row_mask = (U64*) malloc(8 * sizeof(U64));  
    inv_col_mask = (U64*) malloc(8 * sizeof(U64));

    for(int i = 0; i < 8; i++)
    {
        inv_row_mask[i] = ~row_mask[i];
    }

    for(int i = 0; i < 8; i++)
    {
        inv_col_mask[i] = ~col_mask[i];
    }

    // Diag left masks start on left side and moves from left to right, top to bottom
    // [0] corresponds to bottom left corner
    // [0]-[7] moves up y axis along x=0
    // [7] is top left corner
    // [7]-[14] moves across x-axis along y=7
    // [14] is top right corner
    diag_left_mask = (U64*) malloc(15 * sizeof(U64));
    fill_diag_left_mask_arr();

    // Diag right masks start on bottom side and moves from left to right, bottom to top
    // [0] corresponds to bottom right corner
    // [0]-[7] moves down the x axis along y=0
    // [7] is bottom left corner
    // [7]-[14] moves up the y-axis along x=0
    // [14] is top left corner
    diag_right_mask = (U64*) malloc(15 * sizeof(U64));
    fill_diag_right_mask_arr();
}

U64 Engine::make_col_mask(U64 mask)
{
    for(int i = 0; i < 7; i++)
    {
        mask = mask | mask << 8;
    }
    return(mask);
}

void Engine::fill_col_mask_arr()
{
    for(int i = 0; i < 8; i++)
    {
        col_mask[i] = make_col_mask(1ULL << i);
    }
}

U64 Engine::make_row_mask(U64 mask)
{
    for(int i = 0; i < 7; i++)
    {
        mask = mask | mask << 1;
    }
    return(mask);
}

void Engine::fill_row_mask_arr()
{
    for(int i = 0; i < 8; i++)
    {
        row_mask[i] = make_row_mask(1ULL << 8*i);
    }
}

U64 Engine::make_diag_left_mask(U64 mask)
{
    U64 BR_mask = ~(row_mask[0] | col_mask[7]);
    for(int i = 0; i < 8; i++)
    {
        mask = mask | ((mask & BR_mask) >> 7);
    }
    return(mask);
}


void Engine::fill_diag_left_mask_arr()
{
    U64 start = 1;

    for(int i = 0; i < 8; i++)
    {
        diag_left_mask[i] = make_diag_left_mask(start);
        if(i != 7)
        {
            start = start << 8;
        }
    }
    start = start << 1;

    for(int j = 8; j < 15; j++)
    {
        diag_left_mask[j] = make_diag_left_mask(start);
        start = start << 1;
    }
}


U64 Engine::make_diag_right_mask(U64 mask)
{
    U64 TR_mask = ~((row_mask[7]) | (col_mask[7]));
    for(int i = 0; i < 8; i++)
    {
        mask = mask | ((mask & TR_mask) << 9);
    }
    return(mask);
}


void Engine::fill_diag_right_mask_arr()
{
    U64 start = 1ULL << 7;
    for(int i = 0; i < 8; i++)
    {
        diag_right_mask[i] = make_diag_right_mask(start);
        if(i != 7)
        {
            start = start >> 1;
        } 
    }
    start = start << 8;

    for(int j = 8; j < 15; j++)
    {
        diag_right_mask[j] = make_diag_right_mask(start);
        start = start << 8;
    }          
}

int Engine::get_max_move_length()
{
    return max_move_length;
}

std::string Engine::color_to_string(int color)
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

U64 Engine::get_all()
{
    return pos.white_board | pos.black_board;
}


void Engine::print_move_info(int move)
{
    // std::cout << "moving " << piece_name << " from " << decode_from(move) << " to " << decode_to(move);
}

// Takes in a move to be added to the move stack
// Returns nothing
// Alters the move stack and stack_index value
void Engine::stack_push()
{
    board_stack[board_stack_index] = pos.black_board;
    board_stack[board_stack_index+1] = pos.white_board;
    board_stack[board_stack_index+2] = pos.pass_counter;
    board_stack_index += 3;
}

// Takes in nothing
// Returns the last move in the move stack
// Alters the stack_index value
void Engine::stack_pop()
{
    board_stack_index -= 3;
    pos.black_board = board_stack[board_stack_index];
    pos.white_board = board_stack[board_stack_index+1];
    pos.pass_counter = board_stack[board_stack_index+2];
}

// Takes in a move, alters the BitboardEngine's representation to the NEXT state based on the CURRENT move action
void Engine::push_white_move(int move)
{
    if(move == -1)
    {
        pos.pass_counter++;
        stack_push();
        return;
    }
    pos.pass_counter = 0ULL;
    stack_push();

    U64 stone = square_to_bitboard(move);
    pos.white_board = stone | pos.white_board; // put move on board
    flip_white_stones(stone, move, pos.white_board, pos.black_board);
}

void Engine::push_black_move(int move)
{
    if(move == -1)
    {
        pos.pass_counter++;
        stack_push();
        return;
    }
    pos.pass_counter = 0ULL;
    stack_push();

    U64 stone = square_to_bitboard(move);
    pos.black_board = stone | pos.black_board; // put move on board
    flip_black_stones(stone, move, pos.black_board, pos.white_board);
}

// Takes in a move, alters the BitboardEngine's representation to the PREVIOUS state based on the LAST move action
void Engine::pop_move()
{
    stack_pop();
}

// void Engine::flip_stones(U64 stone, U64 own_occ, U64 opp_occ, int color)
// {
//     U64 flippers = 0;
//     U64 init_rook_attacks = one_rook_attacks(stone, own_occ);
//     U64 init_bishop_attacks = one_bishop_attacks(stone, own_occ);

//     // printf("init attacks\n");
//     // print_bit_rep(init_attacks);
    
//     U64 card_los = init_rook_attacks & own_occ;
//     U64 diag_los = init_bishop_attacks & own_occ;
    
//     U64 other_attacks;
//     U64 popped_board;

//     while(card_los)
//     {
//         popped_board = lsb_board(card_los);
//         other_attacks = one_rook_attacks(popped_board, own_occ);
//         card_los = card_los - popped_board;

//         flippers = flippers | (other_attacks & init_rook_attacks);
//     }

//     while(diag_los)
//     {
//         popped_board = lsb_board(diag_los);
//         other_attacks = one_bishop_attacks(popped_board, own_occ);
//         diag_los = diag_los - popped_board;

//         flippers = flippers | (other_attacks & init_bishop_attacks);
//     }

//     // printf("\nflippers\n");
//     // print_bit_rep(flippers);

//     pos.board[color] = pos.board[color] | (flippers & opp_occ);
//     pos.board[1-color] = pos.board[1-color] & ~flippers;
// }

// void Engine::flip_stones(U64 stone, U64 own_occ, U64 opp_occ, int color)
// {
//     U64 flippers = 0;

//     U64 inv_opp_occ = ~opp_occ;
//     U64 init_rook_attacks = one_rook_attacks(stone, inv_opp_occ);
//     U64 init_bishop_attacks = one_bishop_attacks(stone, inv_opp_occ);

//     U64 card_los = init_rook_attacks & own_occ;
//     U64 diag_los = init_bishop_attacks & own_occ;

//     // other_attacks = all_rook_attacks(card_los, ~own_occ);
//     flippers |= all_rook_attacks(card_los, opp_occ) & init_rook_attacks;


//     // other_attacks = all_bishop_attacks(diag_los, ~own_occ);
//     flippers |= all_bishop_attacks(diag_los, opp_occ) & init_bishop_attacks;


//     pos.board[color] |= flippers & opp_occ;
//     pos.board[1-color] &= ~flippers;
// }


void Engine::flip_white_stones(U64 stone, int square, U64 own_occ, U64 opp_occ)
{
    U64 flippers = 0;

    U64 inv_opp_occ = ~opp_occ;
    // U64 init_rook_attacks = one_rook_attacks(stone, inv_opp_occ, square);
    U64 init_rook_attacks = all_rook_attacks(stone, opp_occ);
    U64 init_bishop_attacks = one_bishop_attacks(stone, inv_opp_occ);

    U64 card_los = init_rook_attacks & own_occ;
    U64 diag_los = init_bishop_attacks & own_occ;

    // other_attacks = all_rook_attacks(card_los, ~own_occ);
    flippers |= all_rook_attacks(card_los, opp_occ) & init_rook_attacks;


    // other_attacks = all_bishop_attacks(diag_los, ~own_occ);
    flippers |= all_bishop_attacks(diag_los, opp_occ) & init_bishop_attacks;


    pos.white_board |= flippers & opp_occ;
    pos.black_board &= ~flippers;
}

void Engine::flip_black_stones(U64 stone, int square, U64 own_occ, U64 opp_occ)
{
    U64 flippers = 0;

    U64 inv_opp_occ = ~opp_occ;
    // U64 init_rook_attacks = one_rook_attacks(stone, inv_opp_occ, square);
    U64 init_rook_attacks = all_rook_attacks(stone, opp_occ);
    U64 init_bishop_attacks = one_bishop_attacks(stone, inv_opp_occ);

    U64 card_los = init_rook_attacks & own_occ;
    U64 diag_los = init_bishop_attacks & own_occ;

    // other_attacks = all_rook_attacks(card_los, ~own_occ);
    flippers |= all_rook_attacks(card_los, opp_occ) & init_rook_attacks;


    // other_attacks = all_bishop_attacks(diag_los, ~own_occ);
    flippers |= all_bishop_attacks(diag_los, opp_occ) & init_bishop_attacks;


    pos.black_board |= flippers & opp_occ;
    pos.white_board &= ~flippers;
}

void Engine::print_bit_rep(U64 board)
{
    int shifter;
    U64 to_print = horizontal_flip(board);
    U64 row;
    for(int i = 7; i > -1; i--)
    {
        shifter = i * 8;
        row = (to_print & row_mask[i]) >> shifter;
        std::bitset<8> one_row(row);
        std::cout << one_row << std::endl;
    }
}

void Engine::print_char()
{
    char* b = (char*) malloc(8*8*sizeof(char));
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            b[j+i*8] = '-';
        }
    }

    U64 p;
    U64 one_p;

    p = horizontal_flip(pos.white_board); // white
    while(p)
    {
        one_p = lsb_board(p);
        p = p & ~one_p;
        b[(7-get_file(one_p))+8*(7-get_rank(one_p))] = 'W';
    }

    p = horizontal_flip(pos.black_board); // black
    while(p)
    {
        one_p = lsb_board(p);
        p = p & ~one_p;
        b[(7-get_file(one_p))+8*(7-get_rank(one_p))] = 'B';
    }

    for(int i = 0; i < 8; i++)
    {
        char* tmp = (char *) calloc(30, sizeof(char));
        sprintf(tmp, "%d", 7-i);
        std::cout << tmp;
        free(tmp);

        for(int j = 0; j < 8; j++)
        {
            std::cout << b[j+i*8];
        }
        std::cout << std::endl;
    }
    std::cout << " ABCDEFGH" << std::endl;
}

// East << 1
// Southeast >> 7
// South >> 8
// Southwest >> 9
// West >> 1
// Northwest << 7
// North << 8
// Northeast << 9

int Engine::bitboard_to_square(U64 piece)
{
    return(lsb_digit(piece));
}

U64 Engine::square_to_bitboard(int square)
{
    return(1ULL << square);
}

// Generates and returns a list of legal moves for a color
int* Engine::generate_white_moves()
{
    move_list[0] = 0; // encode index in array

    U64 possible_moves = cardinal_white_moves() | diag_white_moves();
    U64 temp;

    while(possible_moves)
    {
        temp = lsb_board(possible_moves);
        move_list[move_list[0]+1] = bitboard_to_square(temp);
        move_list[0]++;
        possible_moves = possible_moves - temp;
    }
    return move_list;
}

int* Engine::generate_black_moves()
{
    move_list[0] = 0; // encode index in array

    U64 possible_moves = cardinal_black_moves() | diag_black_moves();
    U64 temp;

    while(possible_moves)
    {
        temp = lsb_board(possible_moves);
        move_list[move_list[0]+1] = bitboard_to_square(temp);
        move_list[0]++;
        possible_moves = possible_moves - temp;
    }
    return move_list;
}

int Engine::score_board()
{
    int total = 0;
    U64 temp;
    U64 white = pos.white_board;
    U64 black = pos.black_board;

    while(white)
    {
        temp = lsb_board(white);
        total++;
        white = white - temp;
    }

    while(black)
    {
        temp = lsb_board(black);
        total++;
        black = black - temp;
    }
    return total;
}

int Engine::get_winner()
{
    int score = score_board();
    if(score == 0)
    {
        return 2;
    }
    else if(score > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//alternative method to scoring (probably faster)
// http://chessprogramming.wikispaces.com/Population+Count#SWAR-Popcount-The%20PopCount%20routine
// int popCount (U64 x) {
//     x =  x       - ((x >> 1)  & k1); /* put count of each 2 bits into those 2 bits */
//     x = (x & k2) + ((x >> 2)  & k2); /* put count of each 4 bits into those 4 bits */
//     x = (x       +  (x >> 4)) & k4 ; /* put count of each 8 bits into those 8 bits */
//     x = (x * kf) >> 56; /* returns 8 most significant bits of x + (x<<8) + (x<<16) + (x<<24) + ...  */
//     return (int) x;
// }

// 0 for not over
// other numerals centered around 100
int Engine::is_not_terminal(int* move_list, int color)
{
    if(move_list[0] == 0)
    {
        if(pos.pass_counter == 2ULL)
        {
            return 0;
        }
    }
    return 1;
}

int Engine::is_terminal(int* move_list, int color)
{
    if(move_list[0] == 0)
    {
        if(pos.pass_counter == 2ULL)
        {
            return 1;
        }
    }
    return 0;
}


U64 Engine::one_rook_attacks(U64 rook, U64 o, int square)
{
    U64 o_rev = reverse_64_bits(o);
    U64 s_rev = reverse_64_bits(rook);

    U64 hori = (o - 2*rook) ^ reverse_64_bits(o_rev - 2*s_rev);
    hori = hori & square_masks[square].rank_mask;

    U64 o_mask = o & square_masks[square].file_mask;
    U64 o_rev_mask = reverse_64_bits(o_mask);
    U64 vert = (o_mask - 2*rook) ^ reverse_64_bits(o_rev_mask - 2*s_rev);
    vert = vert & square_masks[square].file_mask;

    return(hori | vert);
}


U64 Engine::one_bishop_attacks_ANTI(U64 bishop, int square, U64 occ)
{
    U64 line_mask = square_masks[square].right_diag_mask_excluded; // excludes square of slider

    U64 forward = occ & line_mask; // also performs the first subtraction by clearing the s in o
    U64 reverse = vertical_flip(forward); // o'-s'

    forward = forward - bishop; // o -2s
    reverse = reverse - vertical_flip(bishop); // o'-2s'
    forward = forward ^ vertical_flip(reverse);
    return forward & line_mask;      // mask the line again
}


U64 Engine::one_bishop_attacks(U64 bishop, U64 occ)
{
    int square = bitboard_to_square(bishop);

    // printf("you are being called\n");

    U64 line_mask = square_masks[square].left_diag_mask_excluded; // excludes square of slider

    U64 forward = occ & line_mask; // also performs the first subtraction by clearing the s in o
    U64 reverse = vertical_flip(forward); // o'-s'

    forward = forward - bishop; // o -2s
    reverse = reverse - vertical_flip(bishop); // o'-2s'
    forward = forward ^ vertical_flip(reverse);

    U64 attks = one_bishop_attacks_ANTI(bishop, square, occ) | (forward & line_mask);

    return attks;      // mask the line again
}


// U64 Engine::one_rook_attacks(U64 rook, U64 occ)
// {
//     int square = bitboard_to_square(rook);

//     U64 forward, reverse;
//     forward  = occ & square_masks[square].file_mask_excluded;

//     // print_bit_rep(occ);
//     // printf("mask\n");
//     // print_bit_rep(square_masks[square].file_mask_excluded);

//     printf("occ\n");
//     print_bit_rep(occ);
//     printf("\n");

//     reverse  = vertical_flip(forward);
//     forward -= rook;
//     reverse -= vertical_flip(rook);
//     forward ^= vertical_flip(reverse);
//     forward &= square_masks[square].file_mask_excluded;
//     print_bit_rep(forward);
//     exit(0);
//     return forward;
// }

// Takes in a 64 bit number with single bit
// Returns the rank piece is on 0-7, bottom to top
int Engine::get_rank(U64 num)
{
    U64 max0 = 128ULL; // 2^7
    if(num <= max0)
    {
        return 0;
    }

    U64 max1 = 32768ULL; // 2^15
    if(num <= max1)
    {
        return 1;
    }

    U64 max2 = 8388608ULL; // 2^23
    if(num <= max2)
    {
        return 2;
    }

    U64 max3 = 2147483648ULL; // 2^31
    if(num <= max3)
    {
        return 3;
    }

    U64 max4 = 549755813888ULL; // 2^39
    if(num <= max4)
    {
        return 4;
    }

    U64 max5 = 140737488355328ULL; // 2^47
    if(num <= max5)
    {
        return 5;
    }

    U64 max6 = 36028797018963968ULL; // 2^55
    if(num <= max6)
    {
        return 6;
    }

    U64 max7 = 9223372036854775808ULL; // 2^63
    if(num <= max7)
    {
        return 7;
    }
    return -1;
}


// Takes in a 64 bit number with single bit
// Returns the file piece is on 0-7, left to right
int Engine::get_file(U64 num)
{
    switch(num)
    {
        //2^[0, 8, 16, 24, 32, 40, 48, 56]
        case 1ULL:
        case 256ULL:
        case 65536ULL:
        case 16777216ULL:
        case 4294967296ULL:
        case 1099511627776ULL:
        case 281474976710656ULL:
        case 72057594037927936ULL:
            return 0;

        //2^[1,9,17,25,33,41,49,57]
        case 2ULL:
        case 512ULL:
        case 131072ULL:
        case 33554432ULL:
        case 8589934592ULL:
        case 2199023255552ULL:
        case 562949953421312ULL:
        case 144115188075855872ULL:
            return 1;

        // 2^[2,10,18,26,34,42,50,58]
        case 4ULL:
        case 1024ULL:
        case 262144ULL:
        case 67108864ULL:
        case 17179869184ULL:
        case 4398046511104ULL:
        case 1125899906842624ULL:
        case 288230376151711744ULL:
            return 2;

        // 2^[3,11,19,27,35,43,51,59]    
        case 8ULL:
        case 2048ULL:
        case 524288ULL:
        case 134217728ULL:
        case 34359738368ULL:
        case 8796093022208ULL:
        case 2251799813685248ULL:
        case 576460752303423488ULL:
            return 3;

        // 2^[4,12,20,28,36,44,52,60]
        case 16ULL:
        case 4096ULL:
        case 1048576ULL:
        case 268435456ULL:
        case 68719476736ULL:
        case 17592186044416ULL:
        case 4503599627370496ULL:
        case 1152921504606846976ULL:
            return 4;

        // 2^[5,13,21,29,37,45,53,61]
        case 32ULL:
        case 8192ULL:
        case 2097152ULL:
        case 536870912ULL:
        case 137438953472ULL:
        case 35184372088832ULL:
        case 9007199254740992ULL:
        case 2305843009213693952ULL:
            return 5;

        // 2^[6, 14, 22, 30, 38, 46, 54, 62]
        case 64ULL:
        case 16384ULL:
        case 4194304ULL:
        case 1073741824ULL:
        case 274877906944ULL:
        case 70368744177664ULL:
        case 18014398509481984ULL:
        case 4611686018427387904ULL:
            return 6;

        // 2^[7,15,23,31,39,47,55,63]
        case 128ULL:
        case 32768ULL:
        case 8388608ULL:
        case 2147483648ULL:
        case 549755813888ULL:
        case 140737488355328ULL:
        case 36028797018963968ULL:
        case 9223372036854775808ULL:
            return 7;

        default:
            return -1;
    }
}


int Engine::get_diag(int rank, int file)
{
    int total_val = rank+file;
    int right;

    //Total val also equals left diag index
    if(rank > file) //Above the middle diagonal line r = 7
    {
        right = 7+(total_val-2*file);
    }
    else //Below middle line
    {
        right = 7-(total_val-2*rank);
    }
    return (total_val << 5) | right;
}


// Takes in a bitboard and will return the bitboard representing only the least significant bit.
// therefore simply return ((lots of zeros)00000000000010)
// YOU MAY ASSUME A 1 EXISTS, (0000000000000000000) will not be given
#ifdef __linux__ // much faster than WIN32 
    int Engine::lsb_digit(unsigned long long board)
    {
        // return(ffsll(board)); // i think only works on linux
        return __builtin_ffsll (board) - 1;
    }
#else
    int Engine::lsb_digit(unsigned long long board)
    {
        return(lsb_lookup[lsb_board(board)]);
    }
#endif

// Takes in a bitboard
// Returns a bitboard with soley the least significant bit = 1
// All other bits = 0
U64 Engine::lsb_board(U64 board)
{
    return(board & (-board));
}

// See above, except return the move_list significant bit bitboard
U64 Engine::msb_board(U64 board)
{
    U64 sol = pow(2, msb_digit(board));
    return(sol);
}

// See above, except return the move_list significant bit bitboard
// Returns the index (right = 0 left = 63) corresponding to the most significant 1
// Probably a speed up to be had by checking if its above 32 bit
U64 Engine::msb_digit(U64 board)
{
    uint32_t first = board >> 32;
    uint32_t second = board & 0xFFFFFFFFLL;
    if(first == 0)
        return(63-(32 + __builtin_clz(second)));
    else
        return(63-__builtin_clz(first));
}


// https://stackoverflow.com/questions/25802605/what-does-performing-a-byteswap-mean
U64 Engine::reverse_8_bits(U64 x)
{
    return (x * 0x0202020202 & 0x010884422010) % 1023;
}


U64 Engine::reverse_64_bits(U64 x)
{
    return vertical_flip(horizontal_flip(x));
}

U64 Engine::horizontal_flip(U64 x)
{
    x = ((x >> 1) & 0x5555555555555555) + 2 * (x & 0x5555555555555555);
    x = ((x >> 2) & 0x3333333333333333) + 4 * (x & 0x3333333333333333);
    x = ((x >> 4) & 0x0f0f0f0f0f0f0f0f) + 16 * (x & 0x0f0f0f0f0f0f0f0f);
    return x;
}

U64 Engine::vertical_flip(U64 x)
{
    return __builtin_bswap64(x);
}


// move gen
// U64 Engine::cardinal_moves(int color)
// {
//     return north_moves(get_color(color), get_color(1-color), ~get_all()) | south_moves(get_color(color), get_color(1-color), ~get_all()) |
//            east_moves(get_color(color), get_color(1-color), ~get_all()) | west_moves(get_color(color), get_color(1-color), ~get_all());
// }

U64 Engine::cardinal_white_moves()
{
    U64 inv_get_all = ~get_all();
    return north_moves(pos.white_board, pos.black_board, inv_get_all) | south_moves(pos.white_board, pos.black_board, inv_get_all) |
           east_moves(pos.white_board, pos.black_board, inv_get_all) | west_moves(pos.white_board, pos.black_board, inv_get_all);
}

U64 Engine::cardinal_black_moves()
{
    U64 inv_get_all = ~get_all();
    return north_moves(pos.black_board, pos.white_board, inv_get_all) | south_moves(pos.black_board, pos.white_board, inv_get_all) |
           east_moves(pos.black_board, pos.white_board, inv_get_all) | west_moves(pos.black_board, pos.white_board, inv_get_all);
}

U64 Engine::north_moves(U64 mine, U64 prop, U64 empty)
{
    U64 moves = prop & (mine << 8);
    moves |= prop & (moves << 8);
    moves |= prop & (moves << 8);
    moves |= prop & (moves << 8);
    moves |= prop & (moves << 8);
    moves |= prop & (moves << 8);
    return empty & (moves << 8);
}

U64 Engine::south_moves(U64 mine, U64 prop, U64 empty)
{
    U64 moves = prop & (mine >> 8);
    moves |= prop & (moves >> 8);
    moves |= prop & (moves >> 8);
    moves |= prop & (moves >> 8);
    moves |= prop & (moves >> 8);
    moves |= prop & (moves >> 8);
    return empty & (moves >> 8);
}

U64 Engine::east_moves(U64 mine, U64 prop, U64 empty)
{
    prop = prop & inv_col_mask[7];
    mine = mine & inv_col_mask[7];
    U64 moves = prop & (mine << 1);
    moves |= prop & (moves << 1);
    moves |= prop & (moves << 1);
    moves |= prop & (moves << 1);
    moves |= prop & (moves << 1);
    moves |= prop & (moves << 1);
    return empty & (moves << 1);
}

U64 Engine::west_moves(U64 mine, U64 prop, U64 empty)
{
    prop = prop & inv_col_mask[0];
    mine = mine & inv_col_mask[0];
    U64 moves = prop & (mine >> 1);
    moves |= prop & (moves >> 1);
    moves |= prop & (moves >> 1);
    moves |= prop & (moves >> 1);
    moves |= prop & (moves >> 1);
    moves |= prop & (moves >> 1);
    return empty & (moves >> 1);
}

// U64 Engine::diag_moves(int color)
// {
//     return north_east_moves(get_color(color), get_color(1-color), ~get_all()) | south_east_moves(get_color(color), get_color(1-color), ~get_all()) |
//            south_west_moves(get_color(color), get_color(1-color), ~get_all()) | north_west_moves(get_color(color), get_color(1-color), ~get_all());
// }

U64 Engine::diag_white_moves()
{
    U64 inv_get_all = ~get_all();
    return north_east_moves(pos.white_board, pos.black_board, inv_get_all) | south_east_moves(pos.white_board, pos.black_board, inv_get_all) |
           south_west_moves(pos.white_board, pos.black_board, inv_get_all) | north_west_moves(pos.white_board, pos.black_board, inv_get_all);
}

U64 Engine::diag_black_moves()
{
    U64 inv_get_all = ~get_all();
    return north_east_moves(pos.black_board, pos.white_board, inv_get_all) | south_east_moves(pos.black_board, pos.white_board, inv_get_all) |
           south_west_moves(pos.black_board, pos.white_board, inv_get_all) | north_west_moves(pos.black_board, pos.white_board, inv_get_all);
}


U64 Engine::north_east_moves(U64 mine, U64 prop, U64 empty)
{
    prop = prop & inv_col_mask[7];
    mine = mine & inv_col_mask[7];
    U64 moves = prop & (mine << 9);
    moves |= prop & (moves << 9);
    moves |= prop & (moves << 9);
    moves |= prop & (moves << 9);
    moves |= prop & (moves << 9);
    moves |= prop & (moves << 9);
    return empty & (moves << 9);
}

U64 Engine::south_east_moves(U64 mine, U64 prop, U64 empty)
{
    prop = prop & inv_col_mask[7];
    mine = mine & inv_col_mask[7];
    U64 moves = prop & (mine >> 7);
    moves |= prop & (moves >> 7);
    moves |= prop & (moves >> 7);
    moves |= prop & (moves >> 7);
    moves |= prop & (moves >> 7);
    moves |= prop & (moves >> 7);
    return empty & (moves >> 7);
}

U64 Engine::south_west_moves(U64 mine, U64 prop, U64 empty)
{
    prop = prop & inv_col_mask[0];
    mine = mine & inv_col_mask[0];
    U64 moves = prop & (mine >> 9);
    moves |= prop & (moves >> 9);
    moves |= prop & (moves >> 9);
    moves |= prop & (moves >> 9);
    moves |= prop & (moves >> 9);
    moves |= prop & (moves >> 9);
    return empty & (moves >> 9);
}

U64 Engine::north_west_moves(U64 mine, U64 prop, U64 empty)
{
    prop = prop & inv_col_mask[0];
    mine = mine & inv_col_mask[0];
    U64 moves = prop & (mine << 7);
    moves |= prop & (moves << 7);
    moves |= prop & (moves << 7);
    moves |= prop & (moves << 7);
    moves |= prop & (moves << 7);
    moves |= prop & (moves << 7);
    return empty & (moves << 7);
}


//
// FLOODS
//

U64 Engine::all_rook_attacks(U64 rooks, U64 prop)
{
    return vert_flood(rooks, prop) | hori_flood(rooks, prop);
}

U64 Engine::all_bishop_attacks(U64 bishops, U64 prop)
{
    return left_diag_flood(bishops, prop) | right_diag_flood(bishops, prop);
}


U64 Engine::vert_flood(U64 rooks, U64 prop)
{
    return(north_flood(rooks, prop) | south_flood(rooks, prop));
}

U64 Engine::hori_flood(U64 rooks, U64 prop)
{
    return(east_flood(rooks, prop & inv_col_mask[7]) | west_flood(rooks, prop & inv_col_mask[0]));
}


U64 Engine::north_flood(U64 rooks, U64 prop)
{
    U64 north_flood = 0;

    while(rooks) 
    {
        north_flood = north_flood | rooks;
        rooks = (rooks << 8) & prop;
    }
    return(north_flood << 8);
}

U64 Engine::south_flood(U64 rooks, U64 prop)
{
    U64 south_flood = 0;

    while(rooks) 
    {
        south_flood = south_flood | rooks;
        rooks = (rooks >> 8) & prop;
    }
    return(south_flood >> 8);
}


U64 Engine::east_flood(U64 rooks, U64 prop)
{
    U64 east_flood = 0;
    rooks = rooks & inv_col_mask[7];

    while(rooks) 
    {
        east_flood = east_flood | rooks;
        rooks = (rooks << 1) & prop;
    }
    return((east_flood & inv_col_mask[7]) << 1);
}

U64 Engine::west_flood(U64 rooks, U64 prop)
{
    U64 west_flood = 0;
    rooks = rooks & inv_col_mask[0];

    while(rooks) 
    {
        west_flood = west_flood | rooks;
        rooks = (rooks >> 1) & prop;
    }
    return((west_flood & inv_col_mask[0]) >> 1);
}


U64 Engine::left_diag_flood(U64 bishops, U64 prop)
{
    return(north_west_flood(bishops, prop & inv_col_mask[0]) | south_east_flood(bishops, prop & inv_col_mask[7]));
}

U64 Engine::right_diag_flood(U64 bishops, U64 prop)
{
    return(north_east_flood(bishops, prop & inv_col_mask[7]) | south_west_flood(bishops, prop & inv_col_mask[0]));
}


U64 Engine::north_east_flood(U64 bishops, U64 prop)
{
    U64 east_flood = 0;
    bishops = bishops & inv_col_mask[7];

    while(bishops) 
    {
        east_flood = east_flood | bishops;
        bishops = (bishops << 9) & prop;
    }
    return((east_flood & inv_col_mask[7]) << 9);
}

U64 Engine::south_east_flood(U64 bishops, U64 prop)
{
    U64 east_flood = 0;
    bishops = bishops & inv_col_mask[7];

    while(bishops) 
    {
        east_flood = east_flood | bishops;
        bishops = (bishops >> 7) & prop;
    }
    return((east_flood & inv_col_mask[7]) >> 7);
}

U64 Engine::south_west_flood(U64 bishops, U64 prop)
{
    U64 west_flood = 0;
    bishops = bishops & inv_col_mask[0];

    while(bishops) 
    {
        west_flood = west_flood | bishops;
        bishops = (bishops >> 9) & prop;
    }
    return((west_flood & inv_col_mask[0]) >> 9);
}

U64 Engine::north_west_flood(U64 bishops, U64 prop)
{
    U64 west_flood = 0;
    bishops = bishops & inv_col_mask[0];

    while(bishops) 
    {
        west_flood = west_flood | bishops;
        bishops = (bishops << 7) & prop;
    }
    return((west_flood & inv_col_mask[0]) << 7);
}