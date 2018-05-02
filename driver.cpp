#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include "engine.hpp"
#include "player.hpp"
#include "driver.hpp"

// g++ bitboard.hpp bitboard.cpp player.hpp player.cpp play.cpp -std=c++14 -o run

//optimized
// g++ bitboard.hpp bitboard.cpp player.hpp player.cpp play.cpp -std=c++14 -O3 -funroll-loops -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized  -o run


// to profile: valgrind --tool=callgrind ./play
// more profiling info: http://zariko.taba.free.fr/c++/callgrind_profile_only_a_part.html

//memleaks and stuff: valgrind --tool=memcheck --leak-check=yes --log-file=out.log ./play

// cache misses: valgrind --tool=cachegrind ./play
// cachegrind read: cg_annotate cachegrind.out.601




/*
 *  To compile on bridges, make sure you have done
 *  module load gcc mpi/gcc_openmpi
 *
 */

/* to run on bridges
 * mpirun -n num_proccessors ./param
 */

// run serial to play 10 games of montecarlo vs montecarlo
// ./param_serial -rank 0 -ngames 10



/*  
 * OUTPUT: the path to the directory with the newest Neural Network. will be "../data/model_%d" 
 *         where the %d is whatever number model we're on.
 *
 * This function assumes that it's being run from some directory in "othello" and so it has to go up one directory
 *     to get to the data/ directory.
 * From there, it simply tries to open a file called "checkpoint" in data/model_%d/ starting %d at 0.
 * If it can open the file successfully, then it adds 1 to i and tries the next value
 * Once it CAN'T open that checkpoint file, then the previous value of i is the most current model.
 * So, it returns the directory for that model.
 *  
 */


Driver::Driver()
{
    // blank
}


/* MRD I copied this from play.cpp but added the MC_chances parameter and the few lines that use it*/
int Driver::play_game(Engine* e, std::vector<Player*> players, int* num_moves, bool print_on)
{
    int move;
    int* move_list;
    num_moves[0] = 0;
    move_list = e->generate_black_moves();

    if (print_on) e->print_char();
    while(e->is_not_terminal(move_list, BLACK))
    {
        move = players[BLACK]->move(move_list);
        e->push_black_move(move);
        if (print_on) printf("move number %i\n", num_moves[0]); //
        if (print_on) e->print_char();        
        num_moves[0]++;

        //TEMP
        // std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
        //TEMP

        move_list = e->generate_white_moves();
        if(e->is_terminal(move_list, WHITE))
        {
            break;
        }
        move = players[WHITE]->move(move_list);
        e->push_white_move(move);
        if (print_on) printf("move number %i\n", num_moves[0]);
        if (print_on) e->print_char();
        num_moves[0]++;

        // TEMP
        // std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
        // TEMP

        move_list = e->generate_black_moves();
    }
    return e->get_winner();
}


void Driver::call_python_script_helper(new_params params, std::string model_name)
{
    std::string command = "python python_model_communicator.py " + params.semaphore_name + " " + 
                            params.shared_memory_name + " " + 
                            model_name;

    printf("EXECUTING: %s\n", command.c_str());
    pid_t pid = fork();
    if(pid != 0)
    {
        int ret_val = system(command.c_str());
        exit(0);
    }
}


// model and python communication
// 0 is success
// 1 is semaphore error
// 2 is shared memory error
int Driver::setup_python_communication()
{
    // printf("SEEDING RANDOM\n");
    // srand (time(NULL));

    pSemaphore = NULL;
    pSharedMemory_code = NULL;
    pSharedMemory_rest = NULL;

    // sender flag
    send_code = -1; // -1 is uninitilized, 0 is c sent, 1 is python sent, 2 is c sent python kill

    
    printf("COMMUNICATION CHANNEL INITIALIZING IN C++!\n");

    // Create the shared memory
    fd = shm_open(params.shared_memory_name.c_str(), O_RDWR | O_CREAT | O_EXCL, params.permissions);    

    if (fd == -1) 
    {
        fd = 0;
        printf("Creating the shared memory failed\n");
        return 2;
    }

    else 
    {
        // The memory is created as a file that's 0 bytes long. Resize it.
        rc = ftruncate(fd, params.size);
        if (rc) 
        {
            printf("Resizing the shared memory failed\n");
            return 2;
        }
        else 
        {
            // MMap the shared memory
            //void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
            pSharedMemory_code = mmap((void *)0, (size_t)params.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (pSharedMemory_code == MAP_FAILED) 
            {
                pSharedMemory_code = NULL;
                printf("MMapping the shared memory failed\n");
                return 2;
            }
            else 
            {
                pSharedMemory_rest = (void*) (((int32_t*) pSharedMemory_code) + 1);
                printf("pSharedMemory_code = %p\n", pSharedMemory_code);
                printf("pSharedMemory_rest = %p\n", pSharedMemory_rest);
            }
        }
    }
    
    if (pSharedMemory_code) 
    {
        // Create the semaphore
        pSemaphore = sem_open(params.semaphore_name.c_str(), O_CREAT, params.permissions, 0);
    
        if (pSemaphore == SEM_FAILED) 
        {
            printf("Creating the semaphore failed\n");
            return 1;
        }
        else 
        {
            printf("pSemaphore =  %p\n", (void *)pSemaphore);
        }
    }

    printf("communication channel established\n");
    return 0;
}

void Driver::send_end_code_python()
{
    send_code = 2;
    memcpy(pSharedMemory_code, &send_code, sizeof(int32_t));
}

// 0 is success
// 1 is semaphore error
// 2 is shared memory error
int Driver::destroy_communication()
{
    send_end_code_python();

    // Announce for one last time that the semaphore is free again so that python can quit
    printf("Final release of the semaphore and send_code followed by a 5 second pause\n"); 
    rc = release_semaphore(pSemaphore);
    sleep(1); // race condition, where the python takes 5 seconds to quit

    printf("Final wait to acquire the semaphore\n"); 
    rc = acquire_semaphore(pSemaphore);
    if (!rc) 
    {
        printf("Destroying the shared memory.\n");

        rc = munmap(pSharedMemory_code, (size_t)params.size); // Un mmap the memory
        if (rc) 
        {
            printf("Unmapping the memory failed\n");
            return 2;
        }
        
        if (-1 == close(fd)) // close file descriptor 
        {
            printf("Closing the memory's file descriptor failed\n");
            return 2;
        }
    
        rc = shm_unlink(params.shared_memory_name.c_str()); // destroy the shared memory.
        if (rc) 
        {
            printf("Unlinking the memory failed\n");
            return 2;
        }
    }

    printf("Destroying the semaphore.\n");
    // Clean up the semaphore
    rc = sem_close(pSemaphore);
    if (rc) 
    {
        printf("Closing the semaphore failed\n");
        return 1;
        
    }
    rc = sem_unlink(params.semaphore_name.c_str());
    if (rc) 
    {
        printf("Unlinking the semaphore failed\n");
        return 1;
        
    }

    return 0;
}

void Driver::cleanup()
{
    printf("begining communication cleanup\n");
    int ret_val = destroy_communication();
    if(ret_val == 1)
    {
        printf("Setup python communication returned with error 1 (problems with semaphore) exiting.");
        exit(0);
    }
    else if(ret_val == 2)
    {
        printf("Setup python communication returned with error 2 (problems with shared memory) exiting.");
        exit(0);
    }
    printf("finished communication cleanup\n");
}


// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string Driver::gen_random(const int len) 
{
    std::string new_str;
    for(int i = 0; i < len; i++)
    {
        new_str.append(" ");
    }

    static const char alphanum[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) 
    {
        new_str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return new_str;
}

int Driver::release_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    rc = sem_post(pSemaphore);
    if(rc) 
    {
        printf("Releasing the semaphore failed\n");
    }
    return rc;
}

int Driver::acquire_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    rc = sem_wait(pSemaphore);
    if(rc) 
    {
        printf("Acquiring the semaphore failed\n");
    }
    return rc;
}


std::string Driver::get_newest_model_name()
{
    FILE *fp;
    char *fnm = (char *) calloc(50, sizeof(char));
    int i = 0;
    int model_name_int;
    do
    {
        sprintf(fnm,"data/model_%d/checkpoint", i);
        fp = fopen(fnm, "r");
        if (!fp)
        {
            if(i - 1 < 0)
            {
                printf("Exiting with return code 5 because no model folders exists in data/model\n");
                exit(5);
            }
            // sprintf(fnm, "data/model_%d", i-1); # this returns full model path
            model_name_int = i-1;
            free(fnm);
            return "model_" + std::to_string(model_name_int);
        }
        fclose(fp);
        ++i;
    } while (1);
}


void Driver::run_driver(int games_to_play, int iterations_per_move, std::string model_name, bool print_on, int depth)
{
    Engine* e = new Engine();

    std::chrono::time_point<std::chrono::system_clock> game_start_timer, game_end_timer, wait1_start_timer, wait1_end_timer;
    std::chrono::duration<double, std::nano> game_time_result, wait1_time_result;
    

    // Read in the new neural network model name

    // std::string model_name = "model_0";

    if(model_name == "recent")
    {
        model_name = get_newest_model_name();
    }


    std::string model_path = "data/" + model_name;

    if (print_on) std::cout << "playing with model: " << model_name << std::endl;



    //////////////////////// CODE FOR MONTECARLO PYTHON COMMUNICATOR //////////////////////



    // asigning values to struct
    params.semaphore_name = gen_random(10);
    params.shared_memory_name = gen_random(10);
    params.size = 4096;
    // params.semaphore_name = "other_stuff";
    // params.shared_memory_name = "shared_memory_1";
    params.permissions = 0600;

    printf("params - size: %d\n", params.size);
    printf("params - semaphore_name: %s\n", params.semaphore_name.c_str());
    printf("params - shared_memory_name: %s\n", params.shared_memory_name.c_str());
    printf("params - permissions: %d\n", params.permissions);

    call_python_script_helper(params, model_name);


    // spawn pythoner
    int ret_val = setup_python_communication();
    if(ret_val == 1)
    {
        printf("Setup python communication returned with error 1 (problems with semaphore) exiting.");
        exit(0);
    }
    else if(ret_val == 2)
    {
        printf("Setup python communication returned with error 2 (problems with shared memory) exiting.");
        exit(0);
    }


    ///////////////////////////////////////////////////////////////////////////////////////




    std::vector<Player*> players;

    players.push_back(new MonteCarlo(BLACK, e, model_name, iterations_per_move, false,
                        pSemaphore, pSharedMemory_code, pSharedMemory_rest)); // white
    players.push_back(new Rand(WHITE, e)); // black
    // players.push_back(new Minimax(BLACK, e, depth)); // white
    // players.push_back(new MonteCarlo(WHITE, e, model_name, iterations_per_move, false, 
                        // pSemaphore, pSharedMemory_code, pSharedMemory_rest)); // white

    // Variable for the number of moves in the game
    int* num_moves = (int*) calloc(1, sizeof(int));

    int result_store[3] = {0, 0, 0};
        
    // Loop over the number of games we're playing per processor per model
    for (int i = 0; i < games_to_play; ++i)
    {
        if(print_on) std::cout << "Playing game " << i << std::endl;
        result_store[play_game(e, players, num_moves, print_on)]++;
        if(print_on) std::cout << "finished playing game" << std::endl;

        // free and reset the engine for a new game
        e->reset_engine();
    }
    players.clear();

    if (print_on) std::cout << "Finished playing " << games_to_play << " games." << std::endl;
    if (print_on) printf("Cleaning up engine and players\n");

    printf("out of %i games\nwhite won: %i\nblack won %i\ndraws %i\nwhite win percentage: %f\n", 
                games_to_play, result_store[1], result_store[0], result_store[2], (float)result_store[1] / games_to_play);

    // clean up
    players[BLACK]->cleanup();
    players[WHITE]->cleanup();

    delete(players[0]);
    delete(players[1]);
    players.clear();
    players.shrink_to_fit();
    e->clean_up();
    delete(e);
    free(num_moves);
    cleanup();

    if (print_on) printf("driver has finished\n");
}


// example call
// ./driver -iter 20 -ngames 10 -name model_0
int main(int argc, char * argv[])
{
    
    // int iterations_per_move = 500;
    // int games_to_play = 10;
    // std::string model_name = "model_5";

    int iterations_per_move = -1; 
    int games_to_play = -1;
    std::string model_name = "hurglblrg";
    int depth = 0;
    int print_on = 0;

    for (int i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], "-iter") == 0)
        {
            iterations_per_move = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-ngames") == 0)
        {
            games_to_play = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-name") == 0)
        {
            model_name = argv[i+1];
        }
        if (strcmp(argv[i], "-print") == 0)
        {
            print_on = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-print") == 0)
        {
            print_on = atoi(argv[i+1]);
        }
    }

    printf("iter: %i, games: %i, model_name: %s\n", iterations_per_move, games_to_play, model_name.c_str());

    if(model_name == "hurglblrg" || games_to_play == -1 || iterations_per_move == -1)
    {
        printf("improper argument string, you need -name, -ngames, and -iter\n -print is optional arg (default off)\n");
        exit(0);
    }

    bool print_on_bool = false;
    if(print_on)
    {
        print_on_bool = true;
    }

    srand(time(NULL)); // seed rand
    Driver* p = new Driver();
    
    p->run_driver(games_to_play, iterations_per_move, model_name, print_on_bool, depth);
    
    return 0;
}


    // srand(time(NULL));
    // Engine* e = new Engine();
    
    // std::vector<Player*> players;
    // // warning players must be instaniated in the right order, 0 then 1
    // int is_training = 0;
    // // players.push_back(new Rand(BLACK, e)); // black
    // players.push_back(new MonteCarlo(BLACK, e, "model_0", 5, is_training)); // black
    // // players.push_back(new MonteCarlo(WHITE, e, "model_0", 500, is_training)); // white
    // players.push_back(new Rand(WHITE, e)); // white

    // int* num_moves = (int*) malloc(sizeof(int));
    // num_moves[0] = 0;

    // int num_games = 1;
    // int result_store[3] = {0, 0, 0};
    
    // for(int i = 0; i < num_games; i++)
    // {
    //     result_store[play_game(e, players, num_moves)]++;
    //     e->reset_engine();
    // }

    // std::cout << "total moves made: " << num_moves[0] << std::endl;
    // printf("out of %i games\nwhite won: %i\nblack won %i\ndraws %i\nwhite win percentage: %f\n", 
    //                 num_games, result_store[1], result_store[0], result_store[2], (float)result_store[1] / num_games);

    // // clean up
    // players[BLACK]->cleanup();
    // players[WHITE]->cleanup();

    // delete(players[0]);
    // delete(players[1]);
    // players.clear();
    // players.shrink_to_fit();
    // e->clean_up();
    // delete(e);
    // free(num_moves);