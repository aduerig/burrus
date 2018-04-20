/*
 *  To compile on bridges, make sure you have done
 *  module load gcc mpi/gcc_openmpi
 *
 * to run on bridges
 * mpirun -n num_proccessors ./hi_mpi -ngames 25000
 * 
 * That will start num_processors all playing 25000/num_processors games from calling param_serial 
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <string>
#include "mpi.h"

int main(int argc, char * argv[])
{
    int np;
    int local_rank;
    int n_games_total = -1;
    int n_games_per_proc = 0;
    bool print_on = true;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &local_rank);
    MPI_Comm_size (MPI_COMM_WORLD, &np);

    for (int i = 0; i < argc-1; ++i)
    {
        if (strcmp(argv[i],"-ngames") == 0) 
        { 
            n_games_total = atoi(argv[i+1]); 
            n_games_per_proc = n_games_total / np;
        }
    }

    if (n_games_total == -1 || n_games_per_proc == 0)
    {
        fprintf(stderr,"ERROR: please specify the total number of games with the -ngames flag\n");
        MPI_Finalize();
        return 1;
    }
    
    /* Call serial param executable */

    char *command = (char *) calloc(80,sizeof(char));
    sprintf(command,"./param_serial -rank %d -ngames %d",local_rank,n_games_per_proc);
    
    if (print_on) std::cout << "local rank: " << local_rank << " about to do command: " << command << std::endl;
    
    int status = system(command);
    
    if (print_on) std::cout << "local rank: " << local_rank << " back from command" << std::endl;
    
    MPI_Finalize();
    
    return 0;
}
