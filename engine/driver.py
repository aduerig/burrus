from optparse import OptionParser
import os
import pandas as pd

from engine import Engine
from random_model import RandomModel
from montecarlo_model import MonteCarloModel
from net_model import NetModel
from cnnevaluator import CnnEvaluator


def main():
    parser = OptionParser()
    parser.add_option("-n", "--num_games", dest="num_games",
                      help="Number of games", default = 10)
    parser.add_option("--p1", dest="player_1",
                      help="random for now", default="random")
    parser.add_option("--p2", dest="player_2",
                      help="random, monte", default="random")
    parser.add_option("-p", "--print", dest="printing", action="store_true",
                      help="print every move", default=False)
    parser.add_option("-s", "--store", dest="store", action="store_true",
                      help="store data if a NetModel is running", default=True)

    options, args = parser.parse_args()
    models = {"random": RandomModel,
              "monte": MonteCarloModel,
              "net": NetModel}


    players =  {'B': models[options.player_1]('B'),
                'W': models[options.player_2]('W')}
    e = Engine()
    cnnEvaluator = CnnEvaluator()
    # generate n games
    data = {'W': {'boards':[], 'search_vals':[], 'results':[]},
            'B': {'boards':[], 'search_vals':[], 'results':[]}}
    for _i in range(int(options.num_games)):
        over = False
        e.set_board_to_beginning()
        curr = 'B'

        # game loop
        while True:
            if len(e.get_moves(curr)) == 0:
                if len(e.get_moves(opposite(curr))) == 0:
                    break
                curr = opposite(curr)
            to_move = players[curr].choose_move(e, data[curr], cnnEvaluator)
            e.play_move(to_move, curr)
            curr=opposite(curr)
            if(options.printing == True):
                print(e)

        store_data(data, e.winning_player())

        print('Winning player:'+ e.winning_player())
    write_to_file(data, cnnEvaluator.model_number)


#helpers
def store_data(data, winner):
    if winner == 'B':
        p1 = 1
        p2 = -1
    elif winner == 'W':
        p1 = -1
        p2 = 1
    else:
        p1, p2 = 0,0
    if len(data['B']['boards']) > 0: # black is storing data
        diff = len(data['B']['boards']) - len(data['B']['results'])
        data['B']['results'] += [p1 for i in range(diff)]
    if len(data['W']['boards']) > 0: # white is storing data
        diff = len(data['W']['boards']) - len(data['W']['results'])
        data['W']['results'] += [p2 for i in range(diff)]

def write_to_file(data, num):
    dir = "games_"+str(num)
    try:
        os.makedirs(dir)
    except OSError as e:
        pass
    i=0
    #this should be replaced with a "rank" input from mpi
    while os.path.exists(os.path.join(dir,"games%s.csv") % i):
        i+=1
    df = pd.DataFrame.from_dict(data['B']).append(pd.DataFrame.from_dict(data['W']))
    df.to_csv(os.path.join(dir,"games%s.csv") % i)




def opposite(piece):
    if piece == 'W':
        return 'B'
    if piece == 'B':
        return 'W'
    return None

if __name__ == "__main__":
    main()