/**
 * Author: Fezile Manana
 * Course: CO20-320202
 * 
 * Disclaimer: The some parts of the following program were obtained from:
 * https://github.com/libevent/libevent/blob/master/sample/hello-world.c
 * 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <arpa/inet.h>

#define FORTUNE_SIZE 160 // short apothegms are defined as 160 chars or less in fortune
#define BUFFER_SIZE 33 // The size of the read buffer

typedef struct game_state
{
    char challenge[BUFFER_SIZE], missing_word[BUFFER_SIZE];
    char *guess;
    int wins, games_played;
    int inactive;
    struct event_base *base;
} game_state;

static void read_cb(struct bufferevent *, void *);
static void write_cb(struct bufferevent *, void *);
static void event_cb(struct bufferevent *, short, void *);
static void accept_conn_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);
static void accept_error_cb(struct evconnlistener *, void *);
static void get_fortune(struct game_state *);
static void replace_word(struct game_state *);
static void signal_cb(evutil_socket_t, short, void *);


static void get_fortune(struct game_state *current_game_status)
{
    pid_t pid;
    int pipe_fort[2];
    char fortune[FORTUNE_SIZE];
    char* buff = (char *) malloc(sizeof(char));
    
    // Initialise fortune buffer
    memset(fortune, 0, sizeof(fortune));

    // Create pipe for furtune
    int ret = pipe(pipe_fort);
    if(ret == -1)
    {
        perror("Fortune: Could not create pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if(pid == -1)
    {
        perror("Fortune: Error forking");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0) //Child process
    {
        int save_fd;
        // backup stdout fd to later restore
        save_fd = dup(STDOUT_FILENO);
        if(save_fd == -1)
        {
            perror("Error: dup call");
            exit(EXIT_FAILURE);
        }

        // Close stdout fd
        if(close(STDOUT_FILENO))
        {
            perror("Error: closing STDOUT fd");
            exit(EXIT_FAILURE);
        }

        // Replace stdout fd with fortune pipe
        if(dup(pipe_fort[1]) == -1)
        {
            perror("Error: dup call");
            exit(EXIT_FAILURE);
        }

        char *args[] = { "fortune", "-n", "32", "-s" };
            // Execute fortune
        if(execvp(*args, args) == -1)
        {
            perror("Failed to execute fortune");
            exit(EXIT_FAILURE);
        }

        // Restoring stdout fd
        if(dup2(pipe_fort[1], save_fd))
        {
            perror("Error restoring stdout fd");
            exit(EXIT_FAILURE);
        }
        // Close remaining fds...
        exit(EXIT_SUCCESS);
    }
    else //Parent process
    {
        int wstatus;
        if(waitpid(0, &wstatus, WUNTRACED) == -1)
        {
            perror("Fortune: Child process return error");
            exit(EXIT_FAILURE);
        }
        read(pipe_fort[0], fortune, FORTUNE_SIZE);
        buff = strcat(buff, fortune);
        
        strcpy(current_game_status->challenge, buff);
        free(buff);
    }
}

static void replace_word(struct game_state *current_game_status)
{
    int count, word_no;
    char *word, *position;
    char fortune[33];
    char delimit[] = " \t\r\n\v\f.,;!~`_-"; // POSIX whitespace characters 
                                            // and other punctuation characters
    // count number of words
    position = current_game_status->challenge;
    while ((position = strpbrk(position, delimit)) != NULL) {
        position++; count++;    
    }

hide_word:
    // initialize randomizer and choose word
    srand(time(NULL));
    word_no = rand() % count;
    count = 0;

    // go through phrase word by word
    strcpy(fortune, current_game_status->challenge);
    word = strtok(fortune, delimit);
    while (word) {
        if (count == word_no) {
            // this word gets removed
            position = strstr(current_game_status->challenge, word);
            if (!position) {
                // something is wrong ... try again
                goto hide_word;
            }
            memset(position, '_', strlen(word));
            strcpy(current_game_status->missing_word, word);
            break;
        }
        count++;
        word = strtok(NULL, delimit);
    }
    if (word == NULL) {
        // this means that no word was chosen
        goto hide_word;
    }
}

static void read_cb(struct bufferevent *bev, void *data)
{
    char msg[100];
    char *word;
    char *delim = " R:\n\t\r\v\f";
    struct game_state *current_game_status = (game_state *) data;
    if(current_game_status == NULL)
    {
        perror("Could not allocate game state struct");
        exit(EXIT_FAILURE);
    }
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    char *reply;
    reply = malloc(len);
    evbuffer_remove(input, reply, len);
    current_game_status->guess = reply;
    
    if(current_game_status->inactive)
    {
        bufferevent_free(bev);
        free(current_game_status);
        free(reply);
    }

    if(strstr(current_game_status->guess, "Q:") == current_game_status->guess)
    {
        sprintf(msg, "Q: You mastered %d/%d challenges. Good bye!\n", current_game_status->wins, current_game_status->games_played);
        bufferevent_write(bev, msg, strlen(msg));
        printf("Client disconnecting\n");
        current_game_status->inactive = 1;
    }
    else
    {
        if((strstr(current_game_status->guess, "R:") == current_game_status->guess))
        {
            current_game_status->games_played++;
            memmove(current_game_status->guess, current_game_status->guess + 2, strlen(current_game_status->guess) - 2);
            word = strtok(current_game_status->guess, delim);
            if(strcmp(word, current_game_status->missing_word) != 0)
            {
                sprintf(msg, "F: Wrong guess - expected: %s\n", current_game_status->missing_word);
                bufferevent_write(bev, msg, strlen(msg));
                current_game_status->guess = NULL;
            }
            else
            {
                current_game_status->wins++;
                bufferevent_write(bev, "O: Congradulations - challenge passed!\n", 40);
            }
            free(reply);
            // Update current game challenge
            get_fortune(current_game_status);
            replace_word(current_game_status);
            bufferevent_write(bev, "C: ", 4);
            bufferevent_write(bev, current_game_status->challenge, strlen(current_game_status->challenge));
        }
        else
        {
            bufferevent_write(bev, "M: Invalid input. Try again!\n", 30);
            free(reply);
            get_fortune(current_game_status);
            replace_word(current_game_status);
            bufferevent_write(bev, "C: ", 4);
            bufferevent_write(bev, current_game_status->challenge, strlen(current_game_status->challenge));
        }
    }

}

static void write_cb(struct bufferevent *bev, void *data)
{

}

static void event_cb(struct bufferevent *bev, short events, void *data)
{
    if (events & BEV_EVENT_ERROR)
        perror("Error from bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
        printf("Client disconnecting\n");
        bufferevent_free(bev);
    }
}

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *data)
{
    int ret;
    char welcome_msg[100];
    /* We got a new connection! Set up a bufferevent for it. */
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    struct game_state *current_game_status;
    
    current_game_status = (struct game_state *) malloc(sizeof(game_state));

    if(current_game_status == NULL)
    {
        perror("Could not allocate game state");
        exit(EXIT_FAILURE);
    }
    current_game_status->base = base;
    strcpy(welcome_msg, "M: Guess the missing ____!\n");
    strcat(welcome_msg, "M: Send your guess in the for R: 'word\\r\\n'\n");
    ret = bufferevent_write(bev, welcome_msg, strlen(welcome_msg));
    if(ret < 0)
        perror("Could not print welcome message");

    bufferevent_setcb(bev, read_cb, write_cb, event_cb, current_game_status);
    bufferevent_enable(bev, EV_READ|EV_PERSIST);
    get_fortune(current_game_status);
    replace_word(current_game_status);
    bufferevent_write(bev ,"C: ", 4);
    bufferevent_write(bev, current_game_status->challenge, strlen(current_game_status->challenge));
}

static void accept_error_cb(struct evconnlistener *listener, void *data)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. " "Shutting down.\n", err, evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}

static void signal_cb(evutil_socket_t sig, short events, void *data)
{
    struct event_base *base = data;
	struct timeval delay = { 1, 0 };

	puts("Caught an interrupt signal.\nExiting in a seconds...\n");

    event_base_loopexit(base, &delay);
}

int main(int argc, char **argv)
{
    struct event_base *base;
    struct evconnlistener *listener;
    struct event *signal_event;
    struct sockaddr_in sin;

    if(argc < 2)
    {
        fprintf(stderr, "Please supply a port number\n");
        exit(EXIT_FAILURE);
    }
    else if(argc > 2)
    {
        fprintf(stderr, "Too many arguments\n");
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        puts("Invalid port");
        return EXIT_FAILURE;
    }

    base = event_base_new();
    if (!base)
    {
        puts("Couldn't open event base");
        return EXIT_FAILURE;
    }

    /* Clear the sockaddr before using it, in case there are extra
         * platform-specific fields that can mess us up. */
    memset(&sin, 0, sizeof(sin));
    /* This is an INET address */
    sin.sin_family = AF_INET;
    /* Listen on the given port. */
    sin.sin_port = htons(port);

    listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                       (struct sockaddr *)&sin, sizeof(sin));
    if(!listener)
    {
        perror("Couldn't create listener");
        return EXIT_FAILURE;
    }

    signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
    if(!signal_event || event_add(signal_event, NULL) < 0)
    {
		perror("Could not create signal event!");
		return EXIT_FAILURE;
    }

    evconnlistener_set_error_cb(listener, accept_error_cb);
    event_base_dispatch(base);

    evconnlistener_free(listener);
	event_free(signal_event);
    event_base_free(base);

    return EXIT_SUCCESS;
}