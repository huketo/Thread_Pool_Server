#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>
#include <map>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <resolv.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "myqueue.h"

using namespace std;

string str; // ID 값들을 저장해 둘 변수
int K;      // 통신 횟수
int T;      // 생성할 쓰레드 클라이언트 수
int port;   // 포트 번호를 저장할 변수
char *ip;   // ip주소를 저장할 변수

#define BUFSIZE 1024 // 통신에 사용할 버퍼 사이즈

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef map<string, int> StrIntMap; // words count시 사용할 구조체

void add_String(char *);                        // ID값을 저장하는 함수
void *thread_function(void *arg);               // 쓰레드가 사용할 함수
void countWords(istream &in, StrIntMap &words); // id.txt 에서 ID값의 횟수를 카운팅하는 함수

int main(int argc, char *argv[])
{
    int rc, status;
    char check;
    ifstream f1;

    if (argc != 3) // 실행시 IP주소와 포트번호를 함께 입력
    {
        printf("Usage : %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    ip = argv[1];
    port = atoi(argv[2]);

    printf("**************        Threaded Clients      **************\n");
    printf("server port	: %d\n", port);
    printf("server ip	: %s\n", ip);
    printf("**************               Log            **************\n");

    cout << "number of clients: ";
    cin >> T;

    cout << "number of communications(K): ";
    cin >> K;

    pthread_t thread_pool[T];
    int done[T];

    for (int i = 1; i <= T; i++)
    {
        done[i] = 0;
        pthread_create(&thread_pool[i], NULL, &thread_function, (void *)(long)i);
    }

    // 쓰레드가 모두 종료될 때 까지 기다림.
    for (int i = T - 1; i >= 0; i--)
    {
        done[i] = 1;
        rc = pthread_join(thread_pool[i], (void **)&status);
        if (rc != 0)
        {
            return -1;
        }
    }

    sleep(10);

    printf("**********************************************************\n");
    cout << "Check if all numbers (of K times) have been retrieved(Y/n) ";
    cin >> check;

    while (1)
    {
        if (check == 'Y' || check == 'y')
        {
            break;
        }

        else if (check == 'n')
        {
            exit(1);
        }

        else
        {
            cout << "Check if all numbers (of K times) have been retrieved(Y/n) ";
            cin >> check;
        }
    }

    cout << endl;

    // id값을 저장할 파일 생성
    string filePath = "id.txt";

    // id값을 id.txt에 저장
    ofstream writeFile(filePath.data());
    if (writeFile.is_open())
    {
        writeFile << str;
        writeFile.close();
    }

    // id.txt에서 id값이 K번 보내졌는 지 확인한다.
    f1.open("id.txt", ios::in);
    StrIntMap w;
    countWords(f1, w);

    for (StrIntMap::iterator p = w.begin();
         p != w.end(); ++p)
    {
        cout << p->first << " occurred "
             << p->second << " times.\n";
    }

    return 0;
}

void add_String(char *a)
{
    pthread_mutex_lock(&mutex);
    str = str + a + " ";
    pthread_mutex_unlock(&mutex);
}

void countWords(istream &in, StrIntMap &words)
{
    string s;

    while (in >> s)
    {
        ++words[s];
    }
}

void *thread_function(void *arg)
{
    int i = 0;
    while (i < K)
    {
        // 필요한 변수 선언
        int sockFD;
        int err;
        int *p_int;

        char buffer[BUFSIZE];
        int bytecount;
        int buffer_len = 0;

        struct sockaddr_in my_addr;

        // 클라이언트에 사용할 소켓 기술어
        sockFD = socket(AF_INET, SOCK_STREAM, 0);

        if (sockFD == -1)
        {
            printf("Error initializing socket %d\n", errno);
            exit(1);
        }

        p_int = (int *)malloc(sizeof(int));
        *p_int = 1;

        if ((setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, (char *)p_int, sizeof(int)) == -1) ||
            (setsockopt(sockFD, SOL_SOCKET, SO_KEEPALIVE, (char *)p_int, sizeof(int)) == -1))
        {
            printf("Error setting options %d\n", errno);
            free(p_int);
            exit(1);
        }
        free(p_int);

        // 접속할 서버의 IP 등 정보 세팅
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(port);

        memset(&(my_addr.sin_zero), 0, 8);
        my_addr.sin_addr.s_addr = inet_addr(ip);

        if (connect(sockFD, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
        {
            if ((err = errno) != EINPROGRESS)
            {
                fprintf(stderr, "Error connecting socket %d\n", errno);
                exit(1);
            }
        }

        buffer_len = BUFSIZE;

        memset(buffer, '\0', buffer_len);

        sprintf(buffer, "%d", arg);

        if ((bytecount = send(sockFD, buffer, strlen(buffer), 0)) == -1)
        {
            fprintf(stderr, "Error sending data %d\n", errno);
            exit(1);
        }

        if ((bytecount = recv(sockFD, buffer, buffer_len, 0)) == -1)
        {
            fprintf(stderr, "Error receiving data %d\n", errno);
            exit(1);
        }
        printf("Received string \"%s\"\n", buffer);
        add_String(buffer);

        close(sockFD);
        i++;
    }
    pthread_exit((void *)0);
}