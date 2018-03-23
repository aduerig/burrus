// to compile:  g++ ../engine.hpp ../engine.cpp tests.cpp -std=c++14 -o test


#include "../engine.hpp"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

vector<string> read_board(ifstream &fpi)
{
  string line;
  vector<string> str_board;
  for (int i = 0; i < 8; ++i)
  {
    getline(fpi,line);
    str_board.push_back(line);
  }
  return str_board;
}


U64 get_board(vector<string> lines, int color)
{
  U64 board = 0;
  for (int y = 0; y < 8; ++y)
  {
    for (int x = 0; x < 8; ++x)
    {
      if ((lines[y][x] == 'B') && (color == 0)) 
      {
        board |= (1ULL << (8*(7-y) + x));
      }
      else if ((lines[y][x] == 'W') && (color == 1)) { board |= (1ULL << (8*(7-y) + x)); } 
    }
  }

  return board;
}

vector<int> read_moves(ifstream &fpi)
{
  vector<int> moves;
  string line;
  getline(fpi, line);
  string color;
  int n_moves;
  stringstream(line) >> color >> n_moves;
  int a_move;
  char *p;
  p = &(line[0]);
  int n = 0;
  for (int i = 0; i < (int) line.length(); ++i)
  {
    if (n < 2)
    {
      if (*(p+i) == ' ') { ++n; }
    }
    else
    {
      if (*(p+i-1) == ' ') { a_move = atof(p+i); moves.push_back(a_move); }
    }  
  }
  return moves;
}

int main(int argc, char * argv[])
{
  Engine* e = new Engine();

  int n_tests = 6;

  for (int i = 1; i <= n_tests; ++i)
  {

    char *fnm = (char *) calloc(30,sizeof(char));

    sprintf(fnm,"test%d.dat",i);
    cout << endl << "Test " << i << endl;
    ifstream fpi(fnm);
    string line;
    e->reset_engine();
    vector<string> str_board;
    str_board = read_board(fpi); 

    e->pos.black_board = 0;
    e->pos.white_board = 0;

    e->pos.black_board = get_board(str_board,0);
    e->pos.white_board = get_board(str_board,1);



    cout << "Board: " << endl;
    e->print_char();

    vector<int> black_moves, white_moves;
    
    black_moves = read_moves(fpi);
    white_moves = read_moves(fpi);

    int *eng_black_moves, *eng_white_moves, *eng_moves;
    eng_moves = (int *) calloc(70,sizeof(int));
    eng_black_moves = (int *) calloc(70,sizeof(int));
    eng_white_moves = (int *) calloc(70,sizeof(int));

    eng_moves = e->generate_black_moves();
    eng_black_moves[0] = eng_moves[0];
    for (int idx = 1; idx <= eng_moves[0]; ++idx) { eng_black_moves[idx] = eng_moves[idx]; }
    eng_moves = e->generate_white_moves();
    eng_white_moves[0] = eng_moves[0];
    for (int idx = 1; idx <= eng_moves[0]; ++idx) { eng_white_moves[idx] = eng_moves[idx]; }


    cout << " my black moves: " << (int) black_moves.size();
    for (int i = 0; i < (int) black_moves.size(); ++i) { cout << " " << black_moves[i]; }
    cout << endl;
    
    cout << "eng black moves: " << eng_black_moves[0];
    for (int i = 1; i <= eng_black_moves[0]; ++i) { cout << " " << eng_black_moves[i]; }
    cout << endl;

    cout << " my white moves: " << (int) white_moves.size();
    for (int i = 0; i < (int) white_moves.size(); ++i) { cout << " " << white_moves[i]; }
    cout << endl;

    cout << "eng white moves: " << eng_white_moves[0];
    for (int i = 1; i <= eng_white_moves[0]; ++i) { cout << " " << eng_white_moves[i]; }
    cout << endl; 

    // check move counts
    if ((int) black_moves.size() == eng_black_moves[0]) { cout << "We agree on black move count!" << endl; }
    else { cout << "Test file says there are " << (int) black_moves.size() << " black moves, but the engine found " << eng_black_moves[0] << " black moves" << endl; }
    if ((int) white_moves.size() == eng_white_moves[0]) { cout << "We agree on white move count!" << endl; }
    else { cout << "Test file says there are " << (int) white_moves.size() << " white moves, but the engine found " << eng_white_moves[0] << " white moves" << endl; }

    // check move numbers
    int *eng_black_moves_used = (int *) calloc(eng_black_moves[0] + 1, sizeof(int));
    int *eng_white_moves_used = (int *) calloc(eng_white_moves[0] + 1, sizeof(int));
    for (int my_idx = 0; my_idx < (int) black_moves.size(); ++my_idx)
    {
      int flag = 0;
      for (int eng_idx = 1; eng_idx <= eng_black_moves[0]; ++eng_idx)
      {
        if (black_moves[my_idx] == eng_black_moves[eng_idx]) { flag = 1; eng_black_moves_used[eng_idx] = 1;}
      }
      if (flag == 1) { cout << "We both found black move: " << black_moves[my_idx] << endl; }
      else { cout << "Test file lists black move: " << black_moves[my_idx] << " but engine didn't find it" << endl; }
    }
    for (int eng_idx = 1; eng_idx <= eng_black_moves[0]; ++eng_idx)
    {
      if (eng_black_moves_used[eng_idx] != 1) { cout << "Engine found black move: " << eng_black_moves[eng_idx] << " that was not listed in test file" << endl; }
    }

    for (int my_idx = 0; my_idx < (int) white_moves.size(); ++my_idx)
    {
      int flag = 0;
      for (int eng_idx = 1; eng_idx <= eng_white_moves[0]; ++eng_idx)
      {
        if (white_moves[my_idx] == eng_white_moves[eng_idx]) { flag = 1; eng_white_moves_used[eng_idx] = 1;}
      }
      if (flag == 1) { cout << "We both found white move: " << white_moves[my_idx] << endl; }
      else { cout << "Test file lists white move: " << white_moves[my_idx] << " but engine didn't find it" << endl; }
    }
    for (int eng_idx = 1; eng_idx <= eng_white_moves[0]; ++eng_idx)
    {
      if (eng_white_moves_used[eng_idx] != 1) { cout << "Engine found white move: " << eng_white_moves[eng_idx] << " that was not listed in test file" << endl; }
    }


/*
This checked the bitboard to see if the engine found the same moves.
Instead, I use the above section. Currently, they both agree.
Keeping this incase something breaks and they dont.

cout << "Checking black moves:" << endl;
    U64 bm = e->cardinal_black_moves() | e->diag_black_moves();
    for (int j = 0; j < (int) black_moves.size(); ++j)
    {
      if ((bm & (1ULL << black_moves[j])) != 0) { cout << "found move " << black_moves[j] << endl; }
      else { cout << "didn't find move " << black_moves[j] << endl; }
    }
cout << "Checking white moves:" << endl;
    U64 wm = e->cardinal_white_moves() | e->diag_white_moves();
    for (int j = 0; j < (int) white_moves.size(); ++j)
    {
      if ((wm & (1ULL << white_moves[j])) != 0) { cout << "found move " << white_moves[j] << endl; }
      else { cout << "didn't find move " << white_moves[j] << endl; }
    }
*/
// Print a bunch of extra info
    if (1 == 0)
    {
      cout << "Found black moves: " << endl;
      U64 bmc = e->cardinal_black_moves();
      U64 bmd = e->diag_black_moves();
      cout << "Cardinals: " << endl;
      e->print_bit_rep(bmc);
      cout << "Diagonals: " << endl;
      e->print_bit_rep(bmd);
      cout << "Found white moves: " << endl;
      U64 wmc = e->cardinal_white_moves();
      U64 wmd = e->diag_white_moves();
      cout << "Cardinals: " << endl;
      e->print_bit_rep(wmc);
      cout << "Diagonals: " << endl;
      e->print_bit_rep(wmd);
    }

// Make each black move and check that the resulting board is correct
// Note, it only makes the black moves that are given in the test file
// because there's nothing to test against for moves that aren't
    for (int j = 0; j < (int) black_moves.size(); ++j)
    {
      Engine* me = new Engine();
      me->pos.black_board = 0;
      me->pos.white_board = 0;
      string a_line;
      getline(fpi,a_line);
      int move_spot;
      stringstream(a_line) >> move_spot;
      vector<string> str_moved_board; 
      str_moved_board = read_board(fpi);
      me->pos.black_board = get_board(str_moved_board,0);
      me->pos.white_board = get_board(str_moved_board,1);
      Engine* eng = new Engine();
      eng->pos.black_board = e->pos.black_board;
      eng->pos.white_board = e->pos.white_board;
      eng->push_black_move(black_moves[j]);
      cout << "Making black move: " << black_moves[j] << endl;
      if ((me->pos.black_board == eng->pos.black_board) && (me->pos.white_board == eng->pos.white_board))
      {
        cout << "We agree on board position for this move!" << endl;
      }
      else
      {
        cout << "We do not agree on board position for this move!" << endl;
        cout << "Test board: " << endl;
        me->print_char();
        cout << "Engine board: " << endl;
        eng->print_char();
      }
    }

// Make each white move and check that the resulting board is correct
    for (int j = 0; j < (int) white_moves.size(); ++j)
    {
      Engine* me = new Engine();
      me->pos.black_board = 0;
      me->pos.white_board = 0;
      string a_line;
      getline(fpi,a_line);
      int move_spot;
      stringstream(a_line) >> move_spot;
      vector<string> str_moved_board;
      str_moved_board = read_board(fpi);
      me->pos.black_board = get_board(str_moved_board,0);
      me->pos.white_board = get_board(str_moved_board,1);
      Engine* eng = new Engine();
      eng->pos.black_board = e->pos.black_board;
      eng->pos.white_board = e->pos.white_board;
      eng->push_white_move(white_moves[j]);
      cout << "Making white move: " << white_moves[j] << endl;
//      eng->print_char();
      if ((me->pos.black_board == eng->pos.black_board) && (me->pos.white_board == eng->pos.white_board))
      {
        cout << "We agree on board position for this move!" << endl;
      }
      else
      {
        cout << "We do not agree on board position for this move!" << endl;
        cout << "Test board: " << endl;
        me->print_char();
        cout << "Engine board: " << endl;
        eng->print_char();
      }
    }


//    e->print_bit_rep(e->cardinal_moves(1));

    string dummy;
    cout << "Type something and press enter for next test." << endl;
    cin >> dummy;
  }
  cout << "finished driver" << endl;

  return 0;
}
