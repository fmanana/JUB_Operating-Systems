/**
 * Author: Fezile Manana
 * Course: Operating Systems - CO20-320202
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    char input[1];
    int option;
    
    // Adjust the value of optind to only parse the last option
    optind = argc - 1;
    
    while ((option = getopt(argc, argv, "lsp")) >= -1)
    {
        switch (option)
        {
            case 'l':
            default:
            {
                input[0] = getc(stdin);
                if (input[0] == EOF) //EOF reached
                    return EXIT_SUCCESS;
                
                int ret_val;
                while (input[0] != EOF)
                {
                    ret_val = putc(input[0], stdout);
                    if (ret_val == EOF) //Error handling for writing
                    {
                        const char err2[] = "Error copying string\n";
                        (void) write(STDERR_FILENO, err2, sizeof(err2));
                        return EXIT_FAILURE;
                    }
                    input[0] = getc(stdin);
                }   
                break;
            }
            case 's':
            {
                ssize_t n = read(STDIN_FILENO, &input, sizeof(input));
                while (n == sizeof(input))
                {
                    if (n == -1) //Error handling for reding from stdin
                    {
                        const char msg[] = "Error reading string\n";
                        (void) write(STDERR_FILENO, msg, sizeof(msg));
                        return EXIT_FAILURE;
                    }
                    n = write(STDOUT_FILENO, &input, sizeof(input));
                    if (n == -1 || n != sizeof(input)) //Error handling for writing to stdout
                    {
                        const char msg2[] = "Error writing string\n";
                        (void) write(STDERR_FILENO, msg2, sizeof(msg2));
                        return EXIT_FAILURE;
                    }
                    n = read(STDIN_FILENO, &input, sizeof(input));
                }
                break;
            }
            case 'p':
            {
                off_t offset = 0;
                while(sendfile(STDOUT_FILENO, STDIN_FILENO, &offset, sizeof(char)));
                break;
            }
            case '?':
                perror("Invalid option");
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}