#include "player.hpp"
#include "engine.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>

int play_game(Engine* e, std::vector<Player*> players)
{
    int move;
    int* move_list;

    printf("inital board state\n");
    e->print_char();

    move_list = e->generate_black_moves();
    while(e->is_not_terminal(move_list, BLACK))
    {
        // BLACKS MOVE

        std::cout << e->color_to_string(BLACK) << " to move." << std::endl;
        std::cout <<  "moves avaliable: " << move_list[0] << std::endl;
        move = players[BLACK]->move(move_list);
        std::cout <<  "making move: " << move << std::endl;
        e->push_black_move(move);
        e->print_char();
        printf("score of board above %i\n", e->score_board());
        std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

        move_list = e->generate_white_moves();
        if(e->is_terminal(move_list, WHITE))
        {
            break;
        }
        // WHITES MOVE

        std::cout << e->color_to_string(WHITE) << " to move." << std::endl;
        std::cout <<  "moves avaliable: " << move_list[0] << std::endl;
        move = players[WHITE]->move(move_list);
        std::cout <<  "making move: " << move << std::endl;
        e->push_white_move(move);
        e->print_char();
        printf("score of board above %i\n", e->score_board());
        std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

        move_list = e->generate_black_moves();
    }
    int winner = e->get_winner();
    std::cout << "game over, winner is: " << e->color_to_string(winner) << std::endl;
    e->print_char();
    return winner;
}


int main()
{
    srand(time(NULL));
    Engine* e = new Engine();
    
    std::vector<Player*> players;
    players.push_back(new Human(0, e)); // white
    players.push_back(new MonteCarlo(1, e, "bad_path", true)); // white
    
    play_game(e, players);
    
    // clean up
    delete(players[0]);
    delete(players[1]);
    players.clear();
    players.shrink_to_fit();
    e->clean_up();
    delete(e);
    return 0;
}