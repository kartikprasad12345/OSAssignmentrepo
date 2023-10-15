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

static int ncpu  ;
static int tslice ;
static int i = 0 ;


typedef struct task{
    double waiting_time ;
    double execution_time ;                                                     //can be calculated by child1 after termination
    struct timeval start_time ;
    struct timeval termination_time ;
    char * file_name ;
    pid_t pid ;   
    
    int flag ;                                                                  //  unused -1    ready - 0    running-1    terminated-2 
    /*sem_t sem ;                                                              - shell , scheduler , child1 write at different times
                                                                                so a sem_t should not be required */
}task ;

typedef struct queue {                                                          // queue is for ready  , running  
    int front, rear, size; 
    task * array[MAX_PID] ;                                                     // ready and running never exceed MAX_PID
    sem_t sem ;
    task *(*dequeue)(struct queue *);
    task *(*enqueue)(struct queue * , task *);  
    void (*display)(struct queue * );          
                                                                                // enqueue can return void too though
                                                                                // shm_ptr -> ready_queue.display(shm_ptr->ready_queue) ;

    bool (*is_empty)(struct queue *);

}queue ;

task * dequeue(queue * q)
{                                                      // dequeue(&(shptr->ready_queue))
    if(q->is_empty(q)){
        printf("Queue is empty\n") ;
        return NULL ;
    }
    else{
        task * temp = q->array[q->front] ; 
        q->front = (q->front + 1) % MAX_PID ;
        q->size-- ;
        return temp ; 
    }
}

task * enqueue(queue * q , task * t)
{                                       // enqueue(&(shptr->ready_queue) , t))
    if(q->size == MAX_PID){
        printf("Queue is full") ;
        return NULL ;
    }
    else{
        q->rear = (q->rear + 1)% MAX_PID ;
        q->array[q->rear] = t ;
        q->size = q->size + 1 ;
        return t ;
    }
}

bool is_empty(queue * q)
{
    return (q->size == 0) ;
}

void display(queue * q)
{                                                          // display(sh_ptr->ready_queue)
    if(q->is_empty(q)){
        printf("Queue is empty\n") ;
    }
    else{
        int i = q->front ;
        while(i != q->rear){
            printf("%d " , q->array[i]->pid) ;
            i = (i+1) % MAX_PID ;
        }
        printf("\npid of process  ( %s ) is %d\n" , q->array[i]->file_name ,  q->array[i]->pid) ;
    }
    printf("size %d\n\n" , q->size) ;
}


typedef struct shm_t
{
    int ncpu ; 
    int tslice ; 
    int count ;                                                                  // processes currently in process_table  with flag = 0 or 1
    int top  ;                                                                   // last process idx in process_table
    int termination_top ;                                                        // last process idx in termination_queue(array)
    task process_table[MAX_PID] ;                                                 // not a queue  , simpleshell will check for NULL or status ==2 
    queue ready_queue ;   
    queue running_queue ;                                                        // ready_queue->array[MAX_PID]  , ready_queue->size , ready_queue->front , ready_queue->rear
    task termination_queue[100] ;
    pid_t scheduler_pid ;
    sem_t sem ;    
    sem_t null_sem ;                                                               // for process_table (running & ready will have their own semaphores)
}shm_t ;


shm_t * sh_ptr ;                                                                 // enqueuing and dequeuing from shared memory will require it

shm_t * setup()
{
    char * shm_name="Process_Table";
    int shm_fd=shm_open(shm_name,O_CREAT|O_RDWR,0666);
    ftruncate(shm_fd,sizeof(shm_t));
    sh_ptr = mmap(NULL,sizeof(shm_t),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    return sh_ptr;

}

void submit_task(char * file_name)
{
    pid_t pid1 = fork() ;
    if(pid1 == 0){                                                                // shared memory is available in all child processes .
        task * t = (task*) malloc(sizeof(task)) ;
        if(t == NULL){
            printf("In submit_task , malloc failed for task %s\n" , file_name) ;
        }
        if (t->file_name == NULL)
        {
            printf("malloc failed for filename %s\n", file_name);
        } 
        t->waiting_time = 0.0 ;
        t->flag = 0  ;  //ready
        t->file_name = (char *)malloc(sizeof(char) * strlen(file_name)) ;
        strcpy(t->file_name , file_name) ;  
        printf("\n123 Process %s\n" , t->file_name) ;
        pid_t pid_grandchild = fork() ; 
        t->pid = pid_grandchild ;
        if(pid_grandchild > 0)
        {                                                                                        
            kill(pid_grandchild , SIGSTOP) ;        
            printf("Process %s grandchild_pid %d start_time %ld flag : %d\n" , t->file_name , t->pid , t->start_time.tv_sec, t->flag) ;                                
            printf("Still in submit_task\n") ;
        }
        if(t->pid == -1){
            printf("Error in fork") ; 
            exit(0) ; 
        }
        printf("Hey\n\n");
        if (pid_grandchild == 0) 
        {
            printf("\nExecuting %s\n", t->file_name);
            fflush(stdout) ;                                   // to be changed later
            char *args[] = { file_name , NULL};
            execvp(args[0], args);
            printf("Error in executing file %s\n", t->file_name);
        }

        else{        
            printf("THEEE\n") ;                                                            //child1.
            sem_wait(&(sh_ptr->sem)) ;
                printf("Entered semaphore in submit_task\n") ;
                int n = sh_ptr->top;
                int j =  0 ;                                                             // finding place in process table for the new task
                while(!sh_ptr->process_table[i].flag == -1  )
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
                gettimeofday(&t->start_time , NULL) ;
                sh_ptr->process_table[n] = *t ;
                printf("In submit_task , process table :\n") ;
                int  p = 0 ;
                while(p < n)
                {
                    printf("Process %s pid %d start_time %ld flag : %d\n" , sh_ptr->process_table[p].file_name , sh_ptr->process_table[p].pid , sh_ptr->process_table[p].start_time.tv_sec, sh_ptr->process_table[p].flag) ;
                    p++ ;
                }
            printf("Leaving semaphore in submit_task") ;                                                // ready_queue is a queue , so we need to maintain front , rear , size
            sem_post(&(sh_ptr->sem)) ;


            sem_wait(&(sh_ptr->ready_queue.sem)) ;
                printf("Enter ready_queue semaphore in submit_task\n") ; 
                    if(sh_ptr->ready_queue.size  == 0)
                    {
                        printf("posting null_sem in submit task\n") ;
                        sem_post(&(sh_ptr->null_sem)) ;                                         // null_sem is for ready_queue
                    }

                    else{
                        printf("Not posting null_sem\n") ;
                    }
                    sh_ptr->ready_queue.enqueue(&(sh_ptr->ready_queue), &sh_ptr->process_table[n]) ;// &sh_ptr->process_table[0] is the address of the process we should not loose info until it is sent to termination queue
                printf("leaving ready_queue semaphore in submit_task\n");
            sem_post(&(sh_ptr->ready_queue.sem));


            sem_wait(&(sh_ptr->sem)) ;
                printf("Enter semaphore in submit task\n");
                sh_ptr->count++ ;                                                                       // process table is not a queue , so we need to maintain count
                sh_ptr->top = n ;                                                                                      // top is the last idx used in process table
                printf("Leaving semaphore in submit task\n");



            sem_post(&(sh_ptr->sem)) ;
                int wstatus ;
                printf("\nWaiting for %d \n\n" , t->pid);                                               //*** waiting for the child process *** 
                waitpid(t->pid, &wstatus , 0) ;
                printf("\n\neverything is over\n");
  

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
                sh_ptr->termination_queue[sh_ptr->termination_top] =  sh_ptr->process_table[n];          // copy but does it matter , no . no semaphore required maybe   
                strcpy(sh_ptr->termination_queue[sh_ptr->termination_top].file_name , sh_ptr->process_table[n].file_name) ;      
                sh_ptr->termination_top ++ ;
                kill(sh_ptr->process_table[n].pid , SIGTERM) ;
                printf("The flag is set to -1\n") ;
                printf("\nTermination top %d \n\n" , sh_ptr->termination_top) ;
                printf("leaving sem  in scheduler for termination\n") ;
                printf("leaving sem  in scheduler for termination print\n");
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
    {   printf("Waiting for post \n\n") ;
        sem_wait(&(sh_ptr->null_sem)) ;                                                                            // wait for shell to submit a task
        printf("In scheduler\n\n") ;                                                                            // wait for shell to submit a tas                                                                                         // wait for shell to submit a task    

        for(int j = 0 ;  j < sh_ptr->ncpu ; j ++)
        {
            printf("Atleast I am here\n") ;
            sem_wait(&(sh_ptr->ready_queue.sem)) ;
            printf("1 . Entering run sem in simpleshed\n") ; 
                task * t = sh_ptr->ready_queue.dequeue(&(sh_ptr->ready_queue)) ;  
            printf("1. Leaving run sem in simplesched\n");
            sem_post(&(sh_ptr->ready_queue.sem)) ;                                        // by default , dequeue returns the front element
            if(t == NULL){
                printf("In simple_scheduler , ready  dequeue returned NULL for %d time \n" , j ) ;                                                                                         
                break ;
            }
            sem_wait(&(sh_ptr->sem)) ;
            printf("Entering semaphore in simple_scheduler\n") ;
            t->flag = 1 ;     
            printf("%d\n" , t->pid ) ;
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
        sem_wait(&(sh_ptr->ready_queue.sem)) ; 
                int r = sh_ptr->ready_queue.front ;                                                                        // add tslice in each task present in the ready queue with flag = 0                                    
                int s = sh_ptr->ready_queue.rear ; 
                printf("rear %d\n" , s) ;
                printf("front %d\n" , r) ;
                printf("Display ready queue\n") ;
                sh_ptr->ready_queue.display(&sh_ptr->ready_queue) ;
        sem_post(&(sh_ptr->ready_queue.sem)) ; 
        printf("before sleeping for tslice\n You can enter a task here though \n") ;
        
        sleep(sh_ptr->tslice)  ; // tslice * 1000

        printf("And after the sleep\n"); 
        while(s != r  )
        {
                printf("In here\n") ;
                sem_wait(&(sh_ptr->ready_queue.sem)) ;
                task * t = sh_ptr->ready_queue.array[i] ;
                sem_post(&(sh_ptr->ready_queue.sem)) ;
                sem_wait(&(sh_ptr->sem)) ;
                printf("flag in simple scheduler : %d  %d\n" , t->flag , t->pid) ;
                printf("setting waiting time \n");
                if(t->flag == 0)
                {
                    t->waiting_time += (double)sh_ptr->tslice ;        // add tslice in each task present in the ready queue with flag = 0
                }
                sem_post(&(sh_ptr->sem)) ;
                r = (r - 1) % MAX_PID ;
            } 

        printf("Leaving run sem in simplesched\n");
            while(sh_ptr->running_queue.size != 0){    
                sem_wait(&(sh_ptr->running_queue.sem)) ;     
                    printf("Entering run sem in simpleshed\n") ;                                                         // check for termination of processes in running queue
                    printf("In while\n") ;
                    sh_ptr->running_queue.display(&sh_ptr->running_queue) ;
                    task * t = sh_ptr->running_queue.dequeue(&(sh_ptr->running_queue)) ;
                    kill(t->pid , SIGSTOP) ; 
                    printf("stopped %d\n in scheduler after running dequeue\n" , t->pid) ;
                    printf("Leaving run sem in simplesched\n");
                sem_post(&(sh_ptr->running_queue.sem)) ;
                printf("%d\n" , t->pid) ;
                printf("%f\n" , t->waiting_time) ;
                printf("( %s )\n" , t->file_name) ;
                printf("FLAG : %d\n" , t->flag) ;
                //printf("Start time : ( %ld  ) " , t->start_time.tv_sec) ;
                if(t == NULL){
                    printf("Problem in implementation\n") ;
                }
                //printf("\n\n%s\n" , t->file_name)  ;
                printf("%d" , t->flag) ;
                printf("HEy\n") ;
                if(t->flag == 1){                                                   
                    printf("Heya\n\n") ;                                                                            // cannot be tested 
                    t->flag = 0 ;   
                    printf("\n%s\n" , t->file_name)  ;   
                    sem_wait(&sh_ptr->ready_queue.sem)  ;  
                        printf("entering ready sem\n")        ;                                                     // process  will be stopped until it is continued by the scheduler .
                        if(sh_ptr->ready_queue.size == 0){
                            printf("posting null_sem in flag 1 scheduler\n") ;
                            sem_post(&(sh_ptr->null_sem)) ;                                         // null_sem is for ready_queue
                        }
                        else{
                            printf("Not posting null_sem\n") ;
                        }
                        sh_ptr->ready_queue.enqueue(&(sh_ptr->ready_queue) , t) ;
                        printf("READY QUEUE\n"); 

                        sh_ptr->ready_queue.display(&sh_ptr->ready_queue) ;
                        printf("\n");
                        printf("leaving ready sem\n") ;
                    sem_post(&sh_ptr->ready_queue.sem) ;

                }
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
                submit_task(args[1]) ;
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
                int scheduler_pid  = fork() ;                                                                // deamon process
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

                    int scheduler_shm_fd = shm_open("Process_Table", O_RDWR, 0);
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
    sh_ptr->ncpu = atoi(argv[1]) ;
    sh_ptr->tslice = atoi(argv[2]) ;
    sh_ptr->ncpu = 2 ;
    sh_ptr->tslice = 4 ;
    sh_ptr->ready_queue.front = 0 ;
    sh_ptr->ready_queue.rear = MAX_PID - 1  ;
    sh_ptr->ready_queue.size = 0 ;
    sh_ptr->running_queue.front = 0 ;
    sh_ptr->running_queue.rear = MAX_PID - 1 ;
    sh_ptr->running_queue.size = 0 ;
    sem_init(&(sh_ptr->sem) , 1 , 1) ;
    sem_init(&(sh_ptr->ready_queue.sem) , 1 , 1) ;              
    sem_init(&(sh_ptr->running_queue.sem) , 1 , 1) ;
    sem_init(&(sh_ptr->null_sem) , 1 , 0) ;
    sh_ptr->ready_queue.dequeue = dequeue ;
    sh_ptr->ready_queue.enqueue = enqueue ;
    sh_ptr->ready_queue.is_empty = is_empty ;
    sh_ptr->ready_queue.display = display ;
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
