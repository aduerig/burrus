#include "player.hpp"
#include "engine.hpp"
#include <stdlib.h>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <ctype.h>
#include <map>

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

std::chrono::duration<double, std::nano> cast_nano2(std::chrono::duration<double> x)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(x);
}

// PLAYER


Player::Player(int col, Engine* engine)
{
    color = col;
    e = engine;
}

int Player::get_color()
{
    return this->color;
}

//
// RAND
//

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

    return move_list[move_choice];
}

//
// HUMAN
//

Human::Human(int col, Engine* engine) : Player(col, engine)
{
    //
}

int Human::parse_coords(std::string seq)
{
    std::map<char, int> char_mapper;
    char_mapper['A'] = 0;
    char_mapper['B'] = 1;
    char_mapper['C'] = 2;
    char_mapper['D'] = 3;
    char_mapper['E'] = 4;
    char_mapper['F'] = 5;
    char_mapper['G'] = 6;
    char_mapper['H'] = 7;

    // im dumb
    std::map<char, int> int_mapper;
    int_mapper['0'] = 0;
    int_mapper['1'] = 1;
    int_mapper['2'] = 2;
    int_mapper['3'] = 3;
    int_mapper['4'] = 4;
    int_mapper['5'] = 5;
    int_mapper['6'] = 6;
    int_mapper['7'] = 7;
    int_mapper['8'] = 8;

    char first = toupper(seq[0]);
    char second = seq[1];

    int first_int = char_mapper[first];
    int second_int = int_mapper[second];

    return first_int + second_int * 8;
}

int Human::move(int* move_list)
{
    if(move_list[0] == 0)
    {
        std::cout << e->color_to_string(get_color()) << " passed because they have no moves (human)" << std::endl;
        return -1;
    }

    int chosen_move = -1;
    int attempt_move;
    std::string input;
    while(chosen_move == -1)
    {
        std::cout << e->color_to_string(get_color()) << " enter your move" << std::endl;
        std::cin >> input;
        try
        {
            attempt_move = parse_coords(input);
        }
        catch(std::string input)
        {
            std::cout << input << ": was not a valid string to enter" << std::endl;
        }

        for(int i = 0; i < move_list[0]; i++)
        {
            if(attempt_move == move_list[i+1])
            {
                chosen_move = attempt_move;
            }
        }
        if(chosen_move == -1)
        {
            printf("%i was not a valid move\n", attempt_move);
        }
    }

    return chosen_move;
}

//
// Minimax
//

Minimax::Minimax(int col, Engine* engine, int search_limit) : Player(col, engine)
{
    depth_search_limit = search_limit;
}

int Minimax::minimax_white(int depth, double alpha, double beta)
{
    node_count++;
    int* move_list = e->generate_white_moves();

    if(depth < 1 || e->is_terminal(move_list, color))
    {
        return e->score_board();
    }
    
    int num_moves = move_list[0];
    int* copied_move_list = copy_move_list(move_list);

    int value;
    int bestVal = -100000;
    for(int i = 0; i < num_moves; i++)
    {
        e->push_white_move(copied_move_list[i]);
        value = minimax_black(depth-1, alpha, beta);
        e->pop_move();

        bestVal = my_max(bestVal, value);
        alpha = my_max(alpha, bestVal);
        if(beta <= alpha)
        {
            free(copied_move_list);
            return bestVal;
        }
    }
    free(copied_move_list);

    if(num_moves == 0)
    {
        e->push_white_move(-1);
        bestVal = minimax_black(depth-1, alpha, beta);
        e->pop_move();
    }

    return bestVal;
}

int Minimax::minimax_black(int depth, double alpha, double beta)
{
    node_count++;
    int* move_list = e->generate_black_moves();
    
    if(depth < 1 || e->is_terminal(move_list, color))
    {
        return e->score_board();
    }
    
    int num_moves = move_list[0];
    int* copied_move_list = copy_move_list(move_list);

    int value;
    int bestVal = 100000;
    
    for(int i = 0; i < num_moves; i++)
    {
        e->push_black_move(copied_move_list[i]);
        value = minimax_white(depth-1, alpha, beta);
        e->pop_move();

        bestVal = my_min(bestVal, value);
        beta = my_min(beta, bestVal);
        if(beta <= alpha)
        {
            free(copied_move_list);
            return bestVal;
        }
    }
    free(copied_move_list);

    if(num_moves == 0)
    {
        e->push_black_move(-1);
        bestVal = minimax_white(depth-1, alpha, beta);
        e->pop_move();
    }

    return bestVal;
}


int Minimax::move(int* move_list)
{
    if(move_list[0] == 0)
    {
        // std::cout << e->color_to_string(get_color()) << " passed because they have no moves (minimax)" << std::endl;
        return -1;
    }


    //timing
    // std::chrono::time_point<std::chrono::system_clock> t1, t2;
    // std::chrono::duration<double, std::nano> time_cast_result;
    // t1 = std::chrono::system_clock::now();
    //end timing

    node_count = 0;

    int num_moves = move_list[0];
    int* copied_move_list = copy_move_list(move_list);


    int best_score;
    int best_move = copied_move_list[0];
    int temp;

    if(color)
    {
        e->push_white_move(copied_move_list[0]);
        best_score = minimax_black(depth_search_limit, -100000, 100000);
        e->pop_move();

        // printf("MINIMAXING FOR WHITE num_moves %i\n", num_moves);
        for(int i = 1; i < num_moves; i++)
        {
            e->push_white_move(copied_move_list[i]);
            temp = minimax_black(depth_search_limit, -100000, 100000);
            e->pop_move();
            if(temp > best_score)
            {
                best_score = temp;
                best_move = copied_move_list[i];
            }
        }
    }
    else
    {
        e->push_black_move(copied_move_list[0]);
        best_score = minimax_white(depth_search_limit, -100000, 100000);
        e->pop_move();

        // printf("MINIMAXING FOR BLACK\n");
        for(int i = 1; i < num_moves; i++)
        {
            e->push_black_move(copied_move_list[i]);
            temp = minimax_white(depth_search_limit, -100000, 100000);
            e->pop_move();
            if(temp < best_score)
            {
                best_score = temp;
                best_move = copied_move_list[i];
            }
        }
    }
    free(copied_move_list);

    //timing
    // t2 = std::chrono::system_clock::now();
    // time_cast_result = cast_nano2(t2 - t1);
    // double timing_temp = (double) time_cast_result.count() / node_count;

    // std::cout << "total nodes examined: " << node_count << " with " << timing_temp << " nanoseconds per move" << std::endl;
    // std::cout << "resulting in NPS of: " << 1.0 / (timing_temp * .000000001) << std::endl;
    //end timing

    return(best_move);
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
        return 0;
    }
    else if(term == 1)
    {
        return 1000;
    }
    else
    {
        return -1000;
    }
}