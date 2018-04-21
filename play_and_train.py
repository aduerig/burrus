import os
import sys
import subprocess
from optparse import OptionParser 

# main script which:
# 1) spawns players with hi_mpi
# 2) on finish, concats data
# 3) tells cnn_resnet to train

MODELS_DIRECTORY = 'data'

def main():
    # parser.add_option("-n", "--num_processors", dest="np", 
    #                 help="Number of games", default = 10)

    #get the first job id
    while 1:
        concat_files()
        print('calling cnn.train()')
        cnn_train = subprocess.call('srun cnn_train.sh', shell=True)


        print('calling hi_mpi')
        hi_mpi = subprocess.call('srun hi_mpi_script.sh', shell=True)



def concat_files():
    out_filename = 'all_games.game'

    model_count = len(next(os.walk(MODELS_DIRECTORY))[1])-1
    latest_model_path = 'model_' + str(model_count)
    path = os.path.join(MODELS_DIRECTORY, latest_model_path, 'games')
    if not os.path.isdir(path):
        return
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