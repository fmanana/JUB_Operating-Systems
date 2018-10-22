/**
 * Author: Fezile Manana
 * Course: CO20-320202
 **/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#define SIZE 20 // The size of the array of coins

// Specify printing flags for printing before and after flipping coins
enum print_flags { Before, After };

static int P = 100; // Default number of threads
static int N = 10000; // Default number of flips
static char coins[SIZE];
pthread_mutex_t lock;

// coin_init(): Initialises coins[] array
void coins_init()
{
    for (int i = 0; i < floor(SIZE/2); i++)
        coins[i] = (char) 'O';

    for (int i = floor(SIZE/2); i < SIZE; i++)
        coins[i] = (char) 'X';
}

// First method with global lock
void *method_1()
{
    // Begin criticial section
    pthread_mutex_lock(&lock);
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            if (coins[j] == (char) 'O')
                coins[j] = (char) 'X';
            else
                coins[j] = (char) 'O';
        }
    }
    pthread_mutex_unlock(&lock);
    // End critical section

    return NULL;
}

// Second method with table lock
void *method_2()
{
    for (int i = 0; i < N; i++)
    {
        // Begin critical section
        pthread_mutex_lock(&lock);
        for (int j = 0; j < SIZE; j++)
        {
            if (coins[j] == (char) 'O')
                coins[j] = (char) 'X';
            else
                coins[j] = (char) 'O';
        }
        pthread_mutex_unlock(&lock);
        // End critical section
    }

    return NULL;
}

// Third method with lock on each coin
void *method_3()
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            // Begin critical section
            pthread_mutex_lock(&lock);
            if (coins[j] == (char) 'O')
                coins[j] = (char) 'X';
            else
                coins[j] = (char) 'O';
            pthread_mutex_unlock(&lock);
            // End critical section
        }
    }

    return NULL;
}

void run_threads(int p, void *(proc)(void *))
{
    pthread_t *thread_id;
    // Allocate memory for the threads
    thread_id = calloc(p, sizeof(pthread_t));

    if(thread_id == NULL && p > 0)
    {
        perror("Failed to allocate threads");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < p; i++)
    {
        if(pthread_create(&thread_id[i], NULL, *proc, NULL))
        {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < p; i++)
    {
        if(pthread_join(thread_id[i], NULL))
        {
            perror("Error joining threads");
            exit(EXIT_FAILURE);
        }
    }
    
    free(thread_id);
}

void print_coins(int flag, const char *coins, const char *msg)
{
    printf("coins: ");
    for(int i = 0; i < SIZE; i++)
    {
        printf("%c", coins[i]);
    }
    if(flag)
        printf(" (end - %s lock)\n", msg);
    else
        printf(" (start - %s lock)\n", msg);
}

/** timeit(int, void *(void*), const char *): Modified to enable inbuilt printing of coins
 * Parametres: p - the number of threads to be created
 *             *proc - Function pointer to the function to be executed
 *             *msg - print message passed from main
 */
static double timeit(int p, void *(*proc)(void *), const char *msg)
{
    print_coins(Before, coins, msg);

    clock_t t1, t2;
    t1 = clock();
    run_threads(p, proc);
    t2 = clock();
    double time = ((double)t2 - (double)t1) / CLOCKS_PER_SEC * 1000;
    
    print_coins(After, coins, msg);
    printf("%d threads x %d flips: %f ms\n", P, N, time);
    
    return time;
}


int main(int argc, char *argv[])
{
    int option;
    while ((option = getopt(argc, argv, "n:p:")) != -1)
    {
        switch(option)
        {
            case 'n':
            {
                N = atoi(optarg);
                if (N < 0)
                {
                    perror("Invalid -n argument");
                    exit(EXIT_FAILURE);
                }
                // Update the number of flips if specified
                break;
            }
            case 'p':
            {
                P = atoi(optarg);
                if(P < 0)
                {
                    perror("Invalid -p argument");
                    exit(EXIT_FAILURE);
                }
                // Update the number of threads if specified
                break;
            }
            default:
            {
                perror("Invalid option");
                exit(EXIT_FAILURE);
                break;
            }
        }
    }

    // Initialise coins
    coins_init();
    timeit(P, (void *) method_1, "global");
    printf("\n");
    timeit(P, (void *) method_2, "table");
    printf("\n");
    timeit(P, (void *) method_3, "coin");

    pthread_mutex_destroy(&lock);

    return EXIT_SUCCESS;
}