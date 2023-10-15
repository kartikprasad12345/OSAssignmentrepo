#include <signal.h>
#include <stdio.h>
// #include "dummy_main.h" 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdbool.h>
#include <semaphore.h>

#define MAX_PID 20

static int ncpu = 1  ;
static int tslice = 4  ;
static int i = 0 ;


typedef struct task{
    double waiting_time ;
    double execution_time ;                                                     //can be calculated by child1 after termination
    struct timeval start_time ;
    struct timeval termination_time ;
    char * file_name ;
    pid_t pid ;   
    int priori ; 
    
    int flag ;                                                                  //  unused -1    ready - 0    running-1    terminated-2 
    /*sem_t sem ;                                                              - shell , scheduler , child1 write at different times
                                                                                so a sem_t should not be required */
}task ;

typedef struct queue {                                                          // queue is for ready  , running  
    int front, rear; 
    task * array[MAX_PID] ;                                                     // ready and running never exceed MAX_PID
    sem_t sem ;
    task *(*dequeue)(struct queue *);
    void (*enqueue)(struct queue * , task *);  
    void (*display)(struct queue * );          
                                                                                // enqueue can return void too though
                                                                                // shm_ptr -> ready_queue.display(shm_ptr->ready_queue) ;

    bool (*is_empty)(struct queue *);

}queue ;

typedef struct shm_t
{
    int ncpu ; 
    unsigned int  tslice ; 
    int count ;                                                                  // processes currently in process_table  with flag = 0 or 1
    int top  ;                                                                   // last process idx in process_table
    int termination_top ;                                                        // last process idx in termination_queue(array)
    task process_table[MAX_PID] ;                                                 // not a queue  , simpleshell will check for NULL or status ==2 
    queue ready_queue ;  
    queue ready_queue1;
    queue ready_queue2;
    queue ready_queue3;
    queue running_queue ;                                                        // ready_queue->array[MAX_PID]  , ready_queue->size , ready_queue->front , ready_queue->rear
    task termination_queue[100] ;
    pid_t scheduler_pid ;
    sem_t sem ;    
    sem_t null_sem ;                                                               // for process_table (running & ready will have their own semaphores)
}shm_t ;


shm_t * sh_ptr ;                                                                 // enqueuing and dequeuing from shared memory will require it


task * dequeue(queue * q)
{                                                      // dequeue(&(shptr->ready_queue))
    if(q->is_empty(q)){
        printf("Queue is empty\n") ;
        return NULL ;
    }
    else{
        q->front = (q->front + 1) % MAX_PID ;
        return q->array[q->front] ; 
    }
}

void enqueue(queue * q , task * t)
{                                       // enqueue(&(shptr->ready_queue) , t))
    if((q->rear + 1)% MAX_PID == q->front){
        printf("Queue is full") ;
        return ;
    }
    else{
        q->rear = (q->rear + 1) %  MAX_PID ;
        q->array[q->rear] = t ;
        printf("Enqueued %s\n" , q->array[q->rear]->file_name) ;
        return ;
    }
}

bool is_empty(queue * q)
{
    return (q->rear == q->front);
}

void display(queue * q)
{                                                          // display(sh_ptr->ready_queue)
    if(q->is_empty(q)){
        printf("Queue is empty\n") ;
    }
    else
    {
        int i = q->front + 1  ;
        while(i <= q->rear){
            printf("%d " , q->array[i]->pid) ;
            printf("\npid of process  ( %s ) is %d\n" , q->array[i]->file_name ,  q->array[i]->pid) ;
            i = (i+1) % MAX_PID ;
        }
    }
    printf("rear %d\n" , q->rear) ;
    printf("front %d\n" , q->front) ;
}


shm_t * setup()
{
    char * shm_name="Process_Table";
    int shm_fd=shm_open(shm_name,O_CREAT|O_RDWR,0666);
    ftruncate(shm_fd,sizeof(shm_t));
    sh_ptr = mmap(NULL,sizeof(shm_t),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    return sh_ptr;

}

void submit_task(char * file_name , int priority)
{
    pid_t pid1 = fork() ;
    if(pid1 == -1){
        printf("Error in fork") ; 
        exit(0) ; 
    }
    if(pid1 == 0){                                                                // shared memory is available in all child processes .
        pid_t pid_grandchild = fork() ; 
        if(pid_grandchild > 0){
            kill(pid_grandchild , SIGSTOP) ;
        }
        if(pid_grandchild == -1){
            printf("Error in fork") ; 
            exit(0) ; 
        }  
        if (pid_grandchild == 0) 
        {
            printf("\nExecuting %s\n %d \n", file_name , getpid());
            fflush(stdout) ;                                   // to be changed later
            char *args[] = { file_name , NULL};
            execvp(args[0], args);
            printf("Error in executing file %s\n", file_name);
        }
        else{
            task * t = (task*) malloc(sizeof(task)) ;
            if(t == NULL){
                printf("In submit_task , malloc failed for task %s\n" , file_name) ;
            }
            t->file_name = (char *)malloc(sizeof(char) * strlen(file_name)) ;
            if (t->file_name == NULL)
            {
                printf("malloc failed for filename %s\n", file_name);
            } 


            t->waiting_time = 0.0 ;
            t->flag = 0  ;  //ready
            strcpy(t->file_name , file_name) ;  
            printf("\n123 Process %s\n" , t->file_name) ;
            t->pid = pid_grandchild ;
            t->priori = priority ;
            printf("priority %d\n" , t->priori) ;
            (t->file_name , file_name) ;                                                                                             
            printf("THEEE\n") ;                                                            //child1.
            gettimeofday(&t->start_time , NULL) ;
            sem_wait(&(sh_ptr->sem)) ;
                printf("Entered semaphore in submit_task\n") ;
                int n = sh_ptr->top;
                int j =  0 ;                                                             // finding place in process table for the new task
                while(!sh_ptr->process_table[n].flag == -1  )
                {                                                                            // -1 -> unused
                    n = (n+1) % MAX_PID ;
                    j ++ ;
                    if(j == MAX_PID)
                    {
                        printf("MAX_PID reached . Try again later\n") ;
                        exit(0) ;
                    }
                }
                printf("Setting start time for %s\n" , t->file_name) ;
                sh_ptr->process_table[n] = *t ;
                task * address = &sh_ptr->process_table[n] ;
                sh_ptr->count++ ;                                                                       // process table is not a queue , so we need to maintain count
                sh_ptr->top = n + 1;  
                printf("In submit_task , process table :\n") ;
                int  p = 0 ;
                while(p <= n)
                {
                    printf("Process %s pid %d start_time %ld flag : %d\n" , sh_ptr->process_table[p].file_name , sh_ptr->process_table[p].pid , sh_ptr->process_table[p].start_time.tv_sec, sh_ptr->process_table[p].flag) ;
                    p++ ;
                }
                printf("Leaving semaphore in submit_task\n") ;                                                // ready_queue is a queue , so we need to maintain front , rear , size
                int priority = t->priori ;
                printf("priority %d\n" , priority) ;
            sem_post(&(sh_ptr->sem)) ;

            sem_wait(&(sh_ptr->ready_queue.sem)) ;
                printf("Enter semaphore in submit_task\n") ;                                   // null_sem is for ready_queu
                if(priority == 1 ){
                        printf("Entering ready_queue semaphore in submit_task\n");
                        sh_ptr->ready_queue.enqueue(&(sh_ptr->ready_queue) , address) ;
                    printf("leaving ready_queue semaphore in submit_task\n");

                }// &sh_ptr->process_table[0] is the address of the process we should not loose info until it is sent to termination queue
                if(priority == 2 )
                {
                    printf("Entering ready_queue semaphore in submit_task\n");
                    sh_ptr->ready_queue1.enqueue(&(sh_ptr->ready_queue1) , address) ;
                    printf("leaving ready_queue semaphore in submit_task\n");
                }
                if(priority == 3 )
                {
                    printf("Entering ready_queue semaphore in submit_task\n");
                    sh_ptr->ready_queue2.enqueue(&(sh_ptr->ready_queue2) , address) ;
                    printf("leaving ready_queue semaphore in submit_task\n");
                }
                if(priority == 4 )
                {
                    printf("Entering ready_queue4 semaphore in submit_task\n");
                    sh_ptr->ready_queue3.enqueue(&(sh_ptr->ready_queue3) , address) ;
                    printf("leaving ready_queue4 semaphore in submit_task\n");
                }
                sem_post(&(sh_ptr->ready_queue.sem)) ;                                         // null_sem is for ready_queue
                printf("here\n") ;
                printf("posting null_sem in submit task\n") ;  
                sem_post(&(sh_ptr->null_sem)) ;     
                printf("leaving ready_queue semaphore in submit_task\n");

            
                
            int wstatus ;
            printf("\nWaiting for %d \n\n" , t->pid);                                               //*** waiting for the child process *** 
            waitpid(t->pid, &wstatus , 0) ;
            printf("\n\neverything is over %d\n" , t->pid) ;
  

            sem_wait(&(sh_ptr->sem)) ;
                printf("Enter semaphore in submit task \n");
                sh_ptr->process_table[n].flag = 2 ;                                                   // terminated or unused ultimately might mean the same thing
                                                                                                    // killing the job .
                gettimeofday(&sh_ptr->process_table[n].termination_time , NULL) ;
                sh_ptr->process_table[n].pid = t->pid ;
                sh_ptr->process_table[n].start_time = t->start_time ;
                double execution = (sh_ptr->process_table[n].termination_time.tv_sec - sh_ptr->process_table[n].start_time.tv_sec)*1000.0
                        + (sh_ptr->process_table[n].termination_time.tv_usec - sh_ptr->process_table[n].start_time.tv_usec) / 1000.0;
                printf("Execution time %f\n" , execution) ;
                sh_ptr->process_table[n].execution_time = execution ;
                sh_ptr->process_table[n].flag = -1 ;
                sh_ptr->count -- ;
                printf("Top %d\n" , sh_ptr->termination_top) ;
                printf("Process %s\n" , t->file_name) ;
                sh_ptr->termination_queue[sh_ptr->termination_top] =  sh_ptr->process_table[n];          // copy but does it matter , no . no semaphore required maybe   
                sh_ptr->termination_queue[sh_ptr->termination_top].file_name = t->file_name ;     
                sh_ptr->termination_top ++ ;
                kill(sh_ptr->process_table[n].pid , SIGTERM) ;
                printf("killed %d\n" , sh_ptr->process_table[n].pid) ;
                printf("The flag is set to -1\n") ;
                printf("\nTermination top %d \n\n" , sh_ptr->termination_top) ;
                printf("leaving sem  in submit_task for termination\n") ;
                printf("Table top %s \n" , sh_ptr->process_table[n].file_name ); 
                printf("Hey , Leaving semaphore in submit_task\n");
                sem_post(&(sh_ptr->sem)) ;
                free(t->file_name) ;
                free(t) ;
                printf("Here too\n");
                _exit(0) ;
        }

    } 

}



/* 
    To optimize time delay , we should not set the fields in the shell loop itself
    call a fork to set the fields , call a fork inside that to execute the process
*/

void scheduler_handler(int sig)
{                                                                      // signal handler for scheduler process
            kill(getpid(), SIGKILL) ;
}



void simple_scheduler(){                                                                                               // open shared memory in simple_scheduler process .
    while(1)
    {   
        printf("Waiting for post \n\n") ;
        sem_wait(&(sh_ptr->null_sem)) ;                                                                            // wait for shell to submit a task
        printf("In scheduler\n\n") ;                                                                            // wait for shell to submit a tas                                                                                         // wait for shell to submit a task    

        for(int j = 0 ;  j < sh_ptr->ncpu ; j ++)
        {
            printf("Atleast I am here\n") ;
            printf("1 . Entering ready sem in simpleshed\n") ; 
            task * t   ;
            sem_wait(&(sh_ptr->ready_queue.sem)) ;
            if(sh_ptr->ready_queue3.front != sh_ptr->ready_queue3.rear)
            {
                t = sh_ptr->ready_queue3.dequeue(&(sh_ptr->ready_queue3)) ;
            }
            else if(sh_ptr->ready_queue2.front != sh_ptr->ready_queue2.rear)
            {
                t = sh_ptr->ready_queue2.dequeue(&(sh_ptr->ready_queue2)) ;
            }
            else if(sh_ptr->ready_queue1.front != sh_ptr->ready_queue1.rear)
            {
                t = sh_ptr->ready_queue1.dequeue(&(sh_ptr->ready_queue1)) ;
            }
            else
            {
                t = sh_ptr->ready_queue.dequeue(&(sh_ptr->ready_queue)) ;

            }
            sem_post(&(sh_ptr->ready_queue.sem)) ;
            printf("1. Leaving ready sem in simplesched\n");
                                       // by default , dequeue returns the front element
            if(t == NULL)
            {
                printf("In simple_scheduler , ready  dequeue returned NULL for %d time \n" , j ) ;                                                                                         
                break ;
            }
            sh_ptr->running_queue.enqueue(&(sh_ptr->running_queue) , t) ;                // enqueue in running queue
            sem_wait(&(sh_ptr->sem)) ;
                printf("Entering semaphore in simple_scheduler\n") ;
                t->flag = 1 ;     
                printf("%d\n %d" , t->pid  , t->flag) ;
                kill(t->pid , SIGCONT) ;     
                waitpid(t->pid, NULL, WNOHANG) ;                                              // all of this will be removed after debugging
                if(WIFCONTINUED(t->pid) == 0){
                    printf("Continued\n") ;
                }
                else{
                    printf("Not continued\n") ;
                }
                printf("Leaving semaphore in simple_scheduler\n") ;
            sem_post(&(sh_ptr->sem)) ;
                //printf
                printf("And after continued \n");                                              
        } 
        printf("before sleeping for tslice\n You can enter a task here though \n") ;
        
        //usleep(time * 1000000)  ; // tslice * 1000
        usleep(300 * 1000) ;


        sem_wait(&(sh_ptr->sem)) ;
        unsigned int  time = sh_ptr->tslice ;
        int cpu = sh_ptr->ncpu ;
        sem_post(&(sh_ptr->sem)) ;

        printf("\nAnd after the sleep\n"); 
        printf("After the sleep\n");
        sem_wait(&(sh_ptr->ready_queue.sem)) ; 
                int r = sh_ptr->ready_queue.front ;                                                                        // add tslice in each task present in the ready queue with flag = 0                                    
                int s = sh_ptr->ready_queue.rear ; 
                int r1=sh_ptr->ready_queue1.front ; 
                int s1=sh_ptr->ready_queue1.rear ; 
                int r2=sh_ptr->ready_queue2.front ;
                int s2= sh_ptr->ready_queue2.rear;
                int r3=sh_ptr->ready_queue3.front;
                int s3=sh_ptr->ready_queue3.rear;
                printf("Display ready queue\n") ;
                sh_ptr->ready_queue.display(&sh_ptr->ready_queue) ;
        sem_post(&(sh_ptr->ready_queue.sem)) ; 


        while(s3 != r3  )
        {
                printf("In here\n") ;
                sem_wait(&(sh_ptr->ready_queue.sem)) ;
                    printf("entering ready sem in simpleshed\n") ;
                        task * t = sh_ptr->ready_queue3.array[r] ;
                        if(t == NULL){
                            printf("In simple_scheduler , ready  dequeue returned NULL for %d time \n" , r) ;      
                            printf("leaving ready sem in simpleshed\n");
                            sem_post(&(sh_ptr->ready_queue.sem)) ;                                                                                   
                            break ;
                        }
                    printf("leaving ready sem in simpleshed\n");
                sem_post(&(sh_ptr->ready_queue.sem)) ;
                sem_wait(&(sh_ptr->sem)) ;
                    printf("flag in simple scheduler : %d  %d\n" , t->flag , t->pid) ;
                    printf("setting waiting time \n");
                    t->waiting_time = t->waiting_time + (double)time ;        // add tslice in each task present in the ready queue with flag = 0
                    printf("Set\n") ;
                sem_post(&(sh_ptr->sem)) ;
                r3 = (r3 - 1) % MAX_PID ;
        } 
        while(s2 != r2  )
        {
                printf("In here\n") ;
                sem_wait(&(sh_ptr->ready_queue.sem)) ;
                    printf("entering ready sem in simpleshed\n") ;
                        task * t = sh_ptr->ready_queue2.array[r] ;
                        if(t == NULL){
                            printf("In simple_scheduler , ready  dequeue returned NULL for %d time \n" , r) ;      
                            printf("leaving ready sem in simpleshed\n");
                            sem_post(&(sh_ptr->ready_queue.sem)) ;                                                                                   
                            break ;
                        }
                    printf("leaving ready sem in simpleshed\n");
                sem_post(&(sh_ptr->ready_queue.sem)) ;
                sem_wait(&(sh_ptr->sem)) ;
                    printf("flag in simple scheduler : %d  %d\n" , t->flag , t->pid) ;
                    printf("setting waiting time \n");
                    t->waiting_time = t->waiting_time + (double)time ;        // add tslice in each task present in the ready queue with flag = 0
                    printf("Set\n") ;
                sem_post(&(sh_ptr->sem)) ;
                r2 = (r2 - 1) % MAX_PID ;
        } 
        while(s1 != r1  )
        {
                printf("In here\n") ;
                sem_wait(&(sh_ptr->ready_queue.sem)) ;
                    printf("entering ready sem in simpleshed\n") ;
                        task * t = sh_ptr->ready_queue1.array[r] ;
                        if(t == NULL){
                            printf("In simple_scheduler , ready  dequeue returned NULL for %d time \n" , r) ;      
                            printf("leaving ready sem in simpleshed\n");
                            sem_post(&(sh_ptr->ready_queue.sem)) ;                                                                                   
                            break ;
                        }
                    printf("leaving ready sem in simpleshed\n");
                sem_post(&(sh_ptr->ready_queue.sem)) ;
                sem_wait(&(sh_ptr->sem)) ;
                    printf("flag in simple scheduler : %d  %d\n" , t->flag , t->pid) ;
                    printf("setting waiting time \n");
                    t->waiting_time = t->waiting_time + (double)time ;        // add tslice in each task present in the ready queue with flag = 0
                    printf("Set\n") ;
                sem_post(&(sh_ptr->sem)) ;
                r1 = (r1 - 1) % MAX_PID ;
        } 
        while(s != r  )
        {
                printf("In here\n") ;
                sem_wait(&(sh_ptr->ready_queue.sem)) ;
                    printf("entering ready sem in simpleshed\n") ;
                        task * t = sh_ptr->ready_queue.array[r] ;
                        if(t == NULL){
                            printf("In simple_scheduler , ready  dequeue returned NULL for %d time \n" , r) ;      
                            printf("leaving ready sem in simpleshed\n");
                            sem_post(&(sh_ptr->ready_queue.sem)) ;                                                                                   
                            break ;
                        }
                    printf("leaving ready sem in simpleshed\n");
                sem_post(&(sh_ptr->ready_queue.sem)) ;
                sem_wait(&(sh_ptr->sem)) ;
                    printf("flag in simple scheduler : %d  %d\n" , t->flag , t->pid) ;
                    printf("setting waiting time \n");
                    t->waiting_time = t->waiting_time + (double)time ;        // add tslice in each task present in the ready queue with flag = 0
                    printf("Set\n") ;
                sem_post(&(sh_ptr->sem)) ;
                r = (r - 1) % MAX_PID ;
        } 

        int j = 0 ;
        while(j++ < cpu){       
                printf("Heretoo\n") ;
                printf("Entering run sem in simpleshed\n") ;                                                         // check for termination of processes in running queue
                printf("In while\n") ;
                task * t = sh_ptr->running_queue.dequeue(&(sh_ptr->running_queue)) ;
                if(t == NULL)
                {
                    printf("In simple_scheduler , running  dequeue returned NULL for %d time \n" , j ) ;                                                                                         
                        break ;
                }
                sem_wait(&(sh_ptr->sem)) ;
                    if(t->flag != -1 )
                    {
                    kill(t->pid , SIGSTOP) ; 
                    }
                    printf("stopped %d\n in scheduler after running dequeue\n" , t->pid) ;
                    printf("Leaving run sem in simplesched\n");
                    int flag = t->flag ;
                    printf("22flag in simple scheduler : %d  %d\n" , t->flag , t->pid) ;
                sem_post(&(sh_ptr->sem)) ;
                //printf("\n\n%s\n" , t->file_name)  ;
                if(flag == 1 ){                                                
                    printf("Heya\n\n") ;                                                                            // cannot be tested 
                    sem_wait(&(sh_ptr->sem)) ;
                    printf("entering sem in scheduler for ready\n") ;
                    t->flag = 0 ;   
                    printf("\n%s\n" , t->file_name)  ;   
                    printf("leaving sem in scheduler for ready\n") ;
                    int p = t->priori ;
                    printf("priority %d\n" , p) ;
                    sem_post(&(sh_ptr->sem)) ;
                sem_wait(&sh_ptr->ready_queue.sem)  ;  
                    if(p == 4 ){
                        printf("HI\n") ;
                        printf("entering ready sem\n")        ;                                                     // process  will be stopped until it is continued by the scheduler .
            
                        sh_ptr->ready_queue3.enqueue(&(sh_ptr->ready_queue3) , t) ;
                        int a = sh_ptr->ready_queue3.rear - sh_ptr->ready_queue3.front ;
                        if(a > 0)
                        {
                            printf("posting null_sem in flag 1 scheduler\n") ;
                            sem_post(&(sh_ptr->null_sem)) ;                                         // null_sem is for ready_queue
                        }
                        else{
                            printf("not posting null_sem in flag 1 scheduler\n") ;
                        }
                        printf("READY QUEUE in sched \n"); 
                    }
                    if(p  == 3){
                        printf("entering ready sem\n")        ;                                                     // process  will be stopped until it is continued by the scheduler .
            
                        sh_ptr->ready_queue2.enqueue(&(sh_ptr->ready_queue2) , t) ;
                        int a = sh_ptr->ready_queue2.rear - sh_ptr->ready_queue2.front ;
                        if(a > 0)
                        {
                            printf("posting null_sem in flag 1 scheduler\n") ;
                            sem_post(&(sh_ptr->null_sem)) ;                                         // null_sem is for ready_queue
                        }
                        else{
                            printf("not posting null_sem in flag 1 scheduler\n") ;
                        }
                        printf("READY QUEUE in sched \n"); 
                }
                if(p == 2){
                        printf("entering ready sem\n")        ;                                                     // process  will be stopped until it is continued by the scheduler .
            
                        sh_ptr->ready_queue1.enqueue(&(sh_ptr->ready_queue1) , t) ;
                        int a = sh_ptr->ready_queue1.rear - sh_ptr->ready_queue1.front ;
                        if(a > 0)
                        {
                            printf("posting null_sem in flag 1 scheduler\n") ;
                            sem_post(&(sh_ptr->null_sem)) ;                                         // null_sem is for ready_queue
                        }
                        else{
                            printf("not posting null_sem in flag 1 scheduler\n") ;
                        }
                        printf("READY QUEUE in sched \n"); 

                }
                if(p == 1){
                        printf("entering ready sem\n")        ;                                                     // process  will be stopped until it is continued by the scheduler .
            
                        sh_ptr->ready_queue.enqueue(&(sh_ptr->ready_queue) , t) ;
                        int a = sh_ptr->ready_queue.rear - sh_ptr->ready_queue.front ;
                        if(a > 0)
                        {
                            printf("posting null_sem in flag 1 scheduler\n") ;
                            sem_post(&(sh_ptr->null_sem)) ;                                         // null_sem is for ready_queue
                        }
                        else{
                            printf("not posting null_sem in flag 1 scheduler\n") ;
                        }
                        printf("READY QUEUE in sched \n"); 
                }
                sem_post(&sh_ptr->ready_queue.sem) ;
                /*
                else if(t->flag == 2 ){
                    printf("I am here\n") ;
                    sem_wait(&sh_ptr->sem) ;
                    printf("entering sem in scheduler for termination\n") ;
                    kill(t->pid , SIGTERM) ;                                                                            // killing the job .
                    sh_ptr->count-- ;
                    printf("Top %d\n" , sh_ptr->termination_top) ;
                    //sh_ptr->termination_queue[sh_ptr->termination_top] = *t ;          // copy but does it matter , no . no semaphore required maybe               
                    //sh_ptr->termination_top ++ ;
                    t->flag = -1 ;
                    printf("The flag is set to -1\n") ;
                    printf("\nTermination top %d \n\n" , sh_ptr->termination_top) ;
                    printf("leaving sem  in scheduler for termination\n") ;
                    sem_post(&sh_ptr->sem) ;
                    printf("In submit_task child 1 TERMINATION QUEUE\n");
                    int k = 0 ;
                    while(k < sh_ptr->termination_top)
                    {
                        printf("PROCESS %d  TERMINATION QUEUE in scheduler \n" , k + 1);
                        //printf("Pid of the process %d" , sh_ptr->termination_queue[k].pid) ;
                        sem_wait(&(sh_ptr->sem)) ;
                        printf("entering sem in scheduler for termination print\n") ;
                        printf("Process %s\n" , sh_ptr->termination_queue[k].file_name) ;
                        printf("Start tim %ld \n " , sh_ptr->termination_queue[k].start_time.tv_sec) ;
                        printf("Process id %d\n" , sh_ptr->termination_queue[k].pid) ; 
                        printf("Waiting Time %f\n" , sh_ptr->termination_queue[k].waiting_time) ;
                        printf("Execution Time %f\n" , sh_ptr->termination_queue[k].execution_time) ;
                        sem_post(&(sh_ptr->sem)) ;
                        //  in milliseconds . check for error when calculating execution time.
                        printf("\n\n") ;
                        printf("%d\n" , k ) ;
                        k++ ;
                    
                    }
                    
                    }
                    */
}
}
}
}

char * shell_read_line()
{
            char * line =  NULL ;
            ssize_t bufsize = 0 ;
            ssize_t read;
            read = getdelim(&line, &bufsize, '\n', stdin) ;
            if (read != -1) {
                    if (line[read - 1] == '\n') {
                            line[read - 1] = '\0';
                    }
            } 
            //printf("(%s) shell_read_line " , line) ;
            return line ; 
}



char ** shell_split_line(char *line){
        int bufsize = 128 ;
        int position = 0 ;
        char **tokens = malloc(bufsize * sizeof(char*)) ;
        char *token = strtok(line, " ") ; ;
        if(!tokens){
            printf("shell: allocation error\n") ;
            exit(EXIT_FAILURE) ;
        }
        while(token != NULL){
            tokens[position] = token ;
            position++ ;
            if(position >= bufsize){
                bufsize *= 2  ;
                tokens = realloc(tokens, bufsize * sizeof(char*)) ;
                if(!tokens){
                    printf("shell: allocation error\n") ;
                    exit(EXIT_FAILURE) ;
                }
            }
            token = strtok(NULL," ") ;
        }
        tokens[position] = NULL ; 
        return tokens ;
}




int shell_execute(char ** args){
        pid_t wait_pid ;
        int status ; 

        if(strcmp(args[0] , "submit") == 0){                                                          // submit .
                if(args[2] == NULL){
                    submit_task(args[1] , 4);
                }
                else{
                submit_task(args[1] , atoi(args[2])) ;
                }
        }
        else{
            pid_t pid1 = fork() ;           
            if(pid1 == -1){
                printf("Error in fork") ; 
                exit(0) ; 
            }
            if(pid1 == 0)
            {
                char *args[] = {args[0], NULL};
                execvp(args[0], args);
                printf("error in executing file %s" , args[0]) ;
            }
            else{
                wait(NULL) ; 
            }
        }
        return 1 ;
}



void shell_handler(int sig)
{                                                                      // signal handler for shell process
            if(sig == SIGINT){
                wait(NULL) ;

                kill(sh_ptr->scheduler_pid, SIGTERM);                                             // since it is a deamon process , we will not get its output ??
                // print the contents maybe here , by the main shell as daemon maybe cannot print ??
                int m = 0 ;
                // printf is an asynchronous operation but it works fine here .
                while(m < sh_ptr->termination_top)
                {
                    sem_wait(&(sh_ptr->sem)) ;
                    printf("Process %s\n" , sh_ptr->termination_queue[m].file_name) ;
                    printf("Process id %d\n" , sh_ptr->termination_queue[m].pid) ;
                    printf("Start time %ld \n " , sh_ptr->termination_queue[m].start_time.tv_sec) ;
                    printf("Waiting Time %f\n" , sh_ptr->termination_queue[m].waiting_time) ;
                    printf("Termination Time %ld \n " , sh_ptr->termination_queue[m].termination_time.tv_sec) ;
                    printf("Exectution TIme %f\n" , sh_ptr->termination_queue[m].execution_time) ;
                //  in milliseconds . check for error when calculating execution time .
                    sem_post(&(sh_ptr->sem)) ;
                    printf("\n\n") ;
                    m++ ;
            

                }
                kill(getpid(), SIGTERM) ;
            }
}

void shell_loop(){
        int status = fork() ;
        char *  cmd_line ;
        char ** args ;
        shm_t * sh_ptr = setup() ;
        if(status < 0 )
        {
            printf("Something bad happened\n") ;
            exit(0) ;
        }

        else if (status == 0)
        {
                int scheduler_pid  = fork() ;                                                             
                if(scheduler_pid < 0 )
                {
                    printf("bad scheduler process\n") ;
                    exit(0) ;
                }
                else if(scheduler_pid == 0)
                {
                    struct sigaction sig ; 
                    memset(&sig, 0, sizeof(sig)) ;
                    sig.sa_handler = scheduler_handler ;
                    sigaction(SIGINT, &sig, NULL) ;

                    int scheduler_shm_fd = shm_open("Process_Table", O_RDWR, 0666);
                    shm_t* sh_ptr = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, scheduler_shm_fd, 0);                                                                 // open shared memory in simple_scheduler process  .
                    simple_scheduler() ;                                                                                    //scheduler function
                }
                else
                {
                    sh_ptr->scheduler_pid = scheduler_pid ;                               //setting the scheduler pid in shared memory , no semaphore required
                    _exit(0) ; 
                }
                /* calculate time of daemon and its ex-parent to rectify if not working properly */
        }
        else
        {
            struct sigaction sig ; 
            memset(&sig, 0, sizeof(sig)) ;
            sig.sa_handler = shell_handler ;
            sigaction(SIGINT, &sig, NULL) ;
            
            do
            {
                printf("CShell> \n") ;   
                // printf("Scheduler pid %d\n" , sh_ptr->scheduler_pid) ;             
                cmd_line =  shell_read_line() ;
                args = shell_split_line(cmd_line) ;
                status = shell_execute(args) ;
                //printf("\n\nEnd of queue\n\n") ;
                //printf("\nPROCESS %d\t %d \n\n" , i , getpid());
                free(cmd_line) ;
                free(args) ;
            }while(status) ;

        }

}



int main(int argc , char * argv[] )
{   // int dummy_main(int argc , char * argv[] ) later ..
    sh_ptr = setup() ;
    //printf("%p" , sh_ptr) ;
    //sh_ptr->ncpu = atoi(argv[1]) ;
    //sh_ptr->tslice = (unsigned int )atoi(argv[2]) ;
    sh_ptr->ncpu = 2 ;
    sh_ptr->tslice = 4 ;
    sh_ptr->ready_queue.front = 0 ;
    sh_ptr->ready_queue.rear = 0  ;
    sh_ptr->running_queue.front = 0 ;
    sh_ptr->running_queue.rear = 0 ;
    sem_init(&(sh_ptr->ready_queue1.sem) , 1 , 1) ;
    sem_init(&(sh_ptr->ready_queue2.sem) , 1 , 1) ;
    sem_init(&(sh_ptr->ready_queue3.sem) , 1 , 1) ;
    sem_init(&(sh_ptr->sem) , 1 , 1) ;
    sem_init(&(sh_ptr->ready_queue.sem) , 1 , 1) ;              
    sem_init(&(sh_ptr->running_queue.sem) , 1 , 1) ;
    sem_init(&(sh_ptr->null_sem) , 1 , 0) ;
    sh_ptr->ready_queue.dequeue = dequeue ;
    sh_ptr->ready_queue.enqueue = enqueue ;
    sh_ptr->ready_queue.is_empty = is_empty ;
    sh_ptr->ready_queue.display = display ;
    sh_ptr->ready_queue1.dequeue = dequeue ;
    sh_ptr->ready_queue1.enqueue = enqueue ;
    sh_ptr->ready_queue1.is_empty = is_empty ;
    sh_ptr->ready_queue1.display = display ;
    sh_ptr->ready_queue2.dequeue = dequeue ;
    sh_ptr->ready_queue2.enqueue = enqueue ;
    sh_ptr->ready_queue2.is_empty = is_empty ;
    sh_ptr->ready_queue2.display = display ;
    sh_ptr->ready_queue3.dequeue = dequeue ;
    sh_ptr->ready_queue3.enqueue = enqueue ;
    sh_ptr->ready_queue3.is_empty = is_empty ;
    sh_ptr->ready_queue3.display = display ;
    sh_ptr->running_queue.dequeue = dequeue ;
    sh_ptr->running_queue.enqueue = enqueue ;
    sh_ptr->running_queue.is_empty = is_empty ;
    sh_ptr->running_queue.display = display ;
    sh_ptr->count = 0 ;
    sh_ptr->top = 0 ;
    sh_ptr->termination_top  = 0 ;
    int j = 0 ;
    while(j < MAX_PID){
        sh_ptr->process_table[j].flag = -1  ;                                                       // indicates that the element is unused
        sh_ptr->process_table[j].pid = 0 ;
        sh_ptr->process_table[j].waiting_time = 0 ;
        sh_ptr->process_table[j].execution_time = 0 ;
        sh_ptr->process_table[j].start_time = (struct timeval){0} ;
        sh_ptr->process_table[j].termination_time = (struct timeval){0} ;

        sh_ptr->termination_queue[j].flag = -1  ;
        sh_ptr->termination_queue[j].pid = 0 ;
        sh_ptr->termination_queue[j].waiting_time = 0 ;
        sh_ptr->termination_queue[j].execution_time = 0 ;
        sh_ptr->termination_queue[j].start_time = (struct timeval){0} ;
        sh_ptr->termination_queue[j].termination_time = (struct timeval){0} ;

        sh_ptr->ready_queue.array[j] = NULL ;
        sh_ptr->running_queue.array[j] = NULL ;
        j++ ;
    }
    shell_loop() ;
    printf("exiting main\n") ;
}
