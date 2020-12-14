#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include "myqueue.h"

#define BUFSIZE 1024
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 100000
#define THREAD_POOL_SIZE 10000

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void *handle_connection(void *p_client_socket);
int check(int exp, const char *msg);
void *thread_function(void *arg);

int main(int argc, char *argv[])
{
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    if (argc != 2)
    { //prot번호 받기
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    //first off create a bunch of thread to handle future connections.
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create socket");

    //initialize the address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    check(bind(server_socket, (SA *)&server_addr, sizeof(server_addr)), "Bind Failed!");
    check(listen(server_socket, SERVER_BACKLOG), "Listen Failed!");

    printf("****  A Very Simple ID Exchanger ****\n");
    printf("server port	: %s\n", argv[1]);
    printf("max connection	: %d\n", SERVER_BACKLOG);
    printf("****            Log              ****\n");

    while (true)
    {
        printf("Waiting for connections...\n");
        //wait for, and eventually accept an incoming connection
        addr_size = sizeof(SA_IN);
        check(client_socket = accept(server_socket, (SA *)&client_addr, (socklen_t *)&addr_size), "accept failed");
        printf("Connected!\n");

        //do whatever we do with connections.

        //put the connection somewhere so that an available thread
        //can find it

        int *pclient;
        pclient = (int *)malloc(sizeof(*pclient));
        *pclient = client_socket;

        //make sure only one thread messes with queue at a time
        pthread_mutex_lock(&mutex);
        enqueue(pclient);
        pthread_cond_signal(&condition_var);
        pthread_mutex_unlock(&mutex);
        //pthread_create(&t, NULL, handle_connection, pclient);
        //handle_connection(pclient);
    } //while

    return 0;
}

int check(int exp, const char *msg)
{
    if (exp == SOCKETERROR)
    {
        perror(msg);
        exit(1);
    }

    return exp;
}

void *thread_function(void *arg)
{
    while (true)
    {
        int *pclient;
        pthread_mutex_lock(&mutex);
        if ((pclient = dequeue()) == NULL)
        {
            pthread_cond_wait(&condition_var, &mutex);
            //try again
            pclient = dequeue();
        }
        pthread_mutex_unlock(&mutex);
        if (pclient != NULL)
        {
            //we have a connection
            handle_connection(pclient);
        }
    }
}

void *handle_connection(void *p_client_socket)
{
    int client_socket = *((int *)p_client_socket);
    free(p_client_socket); //we really don't need this anymore.
    char buffer[BUFSIZE];

    read(client_socket, buffer, sizeof(buffer));
    printf("REQUEST: %s\n", buffer);

    write(client_socket, buffer, sizeof(buffer));
    close(client_socket);

    //printf("closing connection\n");
    return NULL;
}
