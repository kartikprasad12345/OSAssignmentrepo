#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
 
// A structure to represent a queue
struct Queue {
    int front, rear, size;
    unsigned capacity;
    int* array;
    int front1, rear1, size1;
    unsigned capacity1;
    int* array1;
    int front2, rear2, size2;
    unsigned capacity2;
    int* array2;
    int front3, rear3, size3;
    unsigned capacity3;
    int* array3;

};
 
// function to create a queue
// of given capacity.
// It initializes size of queue as 0
struct Queue* createQueue(unsigned capacity,unsigned capacity1,unsigned capacity2,unsigned capacity3)
{
    struct Queue* queue = (struct Queue*)malloc(
        sizeof(struct Queue));
    queue->capacity = capacity;
    queue->capacity1=capacity1;
    queue->capacity2=capacity2;
    queue->capacity3=capacity3;
    queue->front = queue->size = 0;
    queue->front1 = queue->size1 = 0;
    queue->front2 = queue->size2 = 0;
    queue->front3 = queue->size3 = 0;
 
    // This is important, see the enqueue
    queue->rear = capacity - 1;
    queue->rear1 = capacity1 - 1;
    queue->rear2 = capacity2 - 1;
    queue->rear3 = capacity3 - 1;
    queue->array = (int*)malloc(
        queue->capacity * sizeof(int));
    queue->array1 = (int*)malloc(queue->capacity1 * sizeof(int));
    queue->array2 = (int*)malloc(queue->capacity2 * sizeof(int));
    queue->array3 = (int*)malloc(queue->capacity3 * sizeof(int));
    return queue;
}
 
// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue* queue,int priori)
{
    if(priori==1){
        return (queue->size == queue->capacity);
    }
    if(priori==2){
        return (queue->size1== queue->capacity1);
    }
    if(priori==3){
        return (queue->size2== queue->capacity2);
    }
    if(priori==4){
        return (queue->size3== queue->capacity3);
    }
}
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue,int priori)
{
    if(priori==1){
        return (queue->size == 0);
    }
    if(priori==2){
        return (queue->size1 == 0);
    }
    if(priori==3){
        return (queue->size2 == 0);
    }
    if(priori==4){
        return (queue->size3 == 0);
    }
}
 
// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, int item,int priori)
{
    if(priori==1){
        if (isFull(queue,priori))
            return;
        queue->rear = (queue->rear + 1)
                    % queue->capacity;
        queue->array[queue->rear] = item;
        queue->size = queue->size + 1;
        // printf("%d enqueued to queue 1\n", item);
    }else if(priori==2){
        if (isFull(queue,priori))
            return;
        queue->rear1 = (queue->rear1 + 1)
                    % queue->capacity1;
        queue->array1[queue->rear1] = item;
        queue->size1 = queue->size1 + 1;
        // printf("%d enqueued to queue 2\n", item);
    }else if(priori==3){
        if (isFull(queue,priori))
            return;
        queue->rear2 = (queue->rear2 + 1)
                    % queue->capacity2;
        queue->array2[queue->rear2] = item;
        queue->size2 = queue->size2 + 1;
        // printf("%d enqueued to queue 2\n", item);
    }else{
        if (isFull(queue,priori))
            return;
        queue->rear3 = (queue->rear3 + 1)
                    % queue->capacity3;
        queue->array3[queue->rear3] = item;
        queue->size3 = queue->size3 + 1;
        // printf("%d enqueued to queue 2\n", item);
    }
    
}
 
// Function to remove an item from queue.
// It changes front and size
int dequeue(struct Queue* queue,int priori)
{
    if(priori==1){
        if (isEmpty(queue,priori))
            return -1;
        int item = queue->array[queue->front];
        queue->front = (queue->front + 1)
                    % queue->capacity;
        queue->size = queue->size - 1;
        return item;
    }else if(priori==2){
        if (isEmpty(queue,priori))
            return -1;
        int item = queue->array1[queue->front1];
        queue->front1 = (queue->front1+ 1)
                    % queue->capacity1;
        queue->size1 = queue->size1 - 1;
        return item;
    }else if(priori==3){
        if (isEmpty(queue,priori))
            return -1;
        int item = queue->array2[queue->front2];
        queue->front2 = (queue->front2+ 1)
                    % queue->capacity2;
        queue->size2 = queue->size2 - 1;
        return item;
    }else{
        if (isEmpty(queue,priori))
            return -1;
        int item = queue->array3[queue->front3];
        queue->front3 = (queue->front3+ 1)
                    % queue->capacity3;
        queue->size3 = queue->size3 - 1;
        return item;
    }
}
 
// Function to get front of queue
// int front(struct Queue* queue)
// {
//     if (isEmpty(queue))
//         return INT_MIN;
//     return queue->array[queue->front];
// }
 
// // Function to get rear of queue
// int rear(struct Queue* queue)
// {
//     if (isEmpty(queue))
//         return INT_MIN;
//     return queue->array[queue->rear];
// }

// Driver program to test above functions./
int dequeuepriori(struct Queue* queue){
    for(int i=4;i>=1;i--){
        int n=dequeue(queue,i);
        if(n!=-1){
            return n;
        }
    }
}
int isemptypriori(struct Queue* queue){
    for(int i=4;i>=1;i--){
        if(isEmpty(queue,i)==0){
            return 0;
        }
    }
    return 1;
}
int isfullpriori(struct Queue* queue){
    for(int i=4;i>=1;i--){
        if(isFull(queue,i)==0){
            return 0;
        }
    }
    return 1;
}
int main()
{
    struct Queue* queue = createQueue(1000,1000,1000,1000);
 
    enqueue(queue, 10,1);
    enqueue(queue, 20,2);
    enqueue(queue, 30,1);
    enqueue(queue, 40,1);
    enqueue(queue, 50, 1);
    enqueue(queue, 60,2);
    enqueue(queue, 70,3);
    enqueue(queue, 80,2);
    enqueue(queue, 90,1);
    enqueue(queue, 100,4);
    enqueue(queue, 110,4);
    enqueue(queue, 120,3);
    enqueue(queue, 130,2);
    while(isemptypriori(queue)==0){
        printf("%d\n",dequeuepriori(queue));
    }

    return 0;
}