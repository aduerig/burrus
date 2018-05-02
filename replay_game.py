import numpy as np
import os
import utils


def load_games(games_dir):
    curr_path = os.path.join(games_dir, 'all_games.game')
    boards, evals, results = utils.read_in_games(curr_path)
    return [boards, evals, results]


def main():    
    data_dir = utils.get_games_dir('recent')
    print('Using model at: ' + data_dir)
    games = load_games(data_dir)

    for i in reversed(range(len(games[0]))):
        utils.print_board(games[0][i])
        input("Press enter to see next board state")

if __name__ == "__main__":
    main()