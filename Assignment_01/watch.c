/**
 * Author: Fezile Manana
 * Course: Operating Systems - CO20-320202
 **/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>

/**
 * watcher(int argc, char **argv, int *n_flag, int *b_flag, int *e_flag, int n_arg):
 * 
 * Parametres: _flag's are used to signal what options have been parsed from
 *             the standard input.
 *             n_arg carries information on how long watcher should sleep if '-n'
 *             was parsed.
 * Return: on failure watcher() returns 1. On success watcher returns 0.
 **/
int watcher(int argc, char *argv[], int *nflag, int *bflag, int *eflag, int n_arg)
{
    while (1)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("Fork failed");
            exit(-1);
        }
        else if (pid == 0) //Child process
        {
            char *args[argc - optind + 1];
            for(int i = optind; i < argc; i++)
                args[i - optind] = argv[i];
                
            args[argc - optind] = NULL;
            /*  args[0] contains the last parametre parsed from the standard input.
            *   args must terminate with a NULL pointer to pass the array to execvp.
            */
           
            if(execvp(*args, args) == -1)
            {
                if(*bflag)
                    printf("a\n");
                    // Print the special value a to the standard output

                perror("Execution failed");
                _exit(EXIT_FAILURE);
            }
            _exit(EXIT_SUCCESS);
        }
        else //Parent process
        {
            int wstatus;
            (void) waitpid(-1, &wstatus, WUNTRACED); //Return if a child has ended
            
            if(*eflag && wstatus)
                exit(EXIT_FAILURE);
                // Immediately terminate the program if child returned with non-zero
                // exit code and if eflag is raised.

            if(*nflag)
                (void) sleep(n_arg);
            else
            {
                // Default value of sleep is 2 seconds if nflag is not raised
                (void) sleep(2);
            }
        }
    }
    return EXIT_FAILURE;           
}

int main(int argc, char *argv[])
{
    int option;
    int nflag = 0;
    int bflag = 0;
    int eflag = 0;
    int n_arg;
    while ((option = getopt(argc, argv, "+n:be")) != -1)
    {
        switch (option)
        {
            case 'n':
            {
                nflag = 1;
                n_arg = atoi(optarg);
                if(n_arg < 1)
                {
                    perror("Invalid time parameter");
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'b':
            {
                bflag = 1;
                break;
            }
            case 'e':
            {
                eflag = 1;
                break;
            }
            case '?':
            {
                perror("Invalid option");
                return EXIT_FAILURE;
            }
        }
    }

    if(watcher(argc, argv, &nflag, &bflag, &eflag, n_arg))
        return EXIT_FAILURE;
        

    return EXIT_SUCCESS;
}