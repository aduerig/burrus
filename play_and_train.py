import os
import sys
import subprocess
from optparse import OptionParser 

import cnn_resnet as cnn


# main script which:
# 1) spawns players with hi_mpi
# 2) on finish, concats data
# 3) tells cnn_resnet to train

MODELS_DIRECTORY = 'data'

def main():
    # parser.add_option("-n", "--num_processors", dest="np", 
    #                 help="Number of games", default = 10) 
    print('calling hi_mpi')
    #subprocess.call('mpirun -n 2 ./hi_mpi -ngames 200', shell=True)

    concat_files() # path looks like 'data/model_0/games'
    cnn.train()



def concat_files():
    out_filename = 'all_games.game'

    model_count = len(next(os.walk(MODELS_DIRECTORY))[1])-1
    latest_model_path = 'model_' + str(model_count)
    path = os.path.join(MODELS_DIRECTORY, latest_model_path, 'games')

    if os.path.exists(os.path.join(path, out_filename)):
        return
    with open(os.path.join(path, out_filename), 'wb+') as outfile:
        for file in os.listdir(path):
            if file == out_filename or file[-5:] != '.game':
                continue
            with open(os.path.join(path, file), 'rb') as readfile:
                outfile.write(readfile.read())


if __name__ == "__main__":
    main()