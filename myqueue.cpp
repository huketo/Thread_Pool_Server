#include "myqueue.h"
#include <stdlib.h>

node_t *head = NULL;
node_t *tail = NULL;

void enqueue(int *client_socket)
{
    node_t *newnode;
    newnode = (node_t *)malloc(sizeof(*newnode));
    newnode->client_socket = client_socket;
    newnode->next = NULL;
    if (tail == NULL)
    {
        head = newnode;
    }
    else
    {
        tail->next = newnode;
    }
    tail = newnode;
}

// 큐가 비어 있으면 NULL을 반환
// 가져올 포인터가 있으면 client_socket으로 반환
int *dequeue()
{
    if (head == NULL)
    {
        return NULL;
    }
    else
    {
        int *result = head->client_socket;
        node_t *temp = head;
        head = head->next;
        if (head == NULL)
        {
            tail = NULL;
        }
        free(temp);
        return result;
    }
}
