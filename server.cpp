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

#define BUFSIZE 1024           // 통신에 사용할 버퍼 크기
#define SOCKETERROR (-1)       // 소켓에러시 반환할 값
#define SERVER_BACKLOG 100000  // 최대 접속 허용 수
#define THREAD_POOL_SIZE 10000 // 쓰레드 풀 사이즈

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void *handle_connection(void *p_client_socket); // 통신에 사용할 함수
int check(int exp, const char *msg);            // 에러 체크를 위한 함수
void *thread_function(void *arg);               // 쓰레드가 사용할 함수

int main(int argc, char *argv[])
{
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    if (argc != 2) // 포트 번호와 함께 입력
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 추후 있을 연결을 위해 쓰레드들을 미리 생성한다.
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create socket");

    // 주소 스트럭쳐를 초기화
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
        // 연결을 기다리다가 accept 한다.
        addr_size = sizeof(SA_IN);
        check(client_socket = accept(server_socket, (SA *)&client_addr, (socklen_t *)&addr_size), "accept failed");
        printf("Connected!\n");

        // 연결된 소켓을 메모리에 저장한다.
        int *pclient;
        pclient = (int *)malloc(sizeof(*pclient));
        *pclient = client_socket;

        // 뮤텍스
        pthread_mutex_lock(&mutex);
        enqueue(pclient);
        pthread_cond_signal(&condition_var); // 기다리고 있는 쓰레드를 깨운다.
        pthread_mutex_unlock(&mutex);
    }

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
            // 다시 시도
            pclient = dequeue();
        }
        pthread_mutex_unlock(&mutex);
        if (pclient != NULL)
        {
            // 연결 시도
            handle_connection(pclient);
        }
    }
}

void *handle_connection(void *p_client_socket)
{
    int client_socket = *((int *)p_client_socket);
    free(p_client_socket);
    char buffer[BUFSIZE];

    read(client_socket, buffer, sizeof(buffer));
    printf("REQUEST: %s\n", buffer);

    write(client_socket, buffer, sizeof(buffer));
    close(client_socket);

    return NULL;
}
