#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include<sys/shm.h>
#include<semaphore.h>
#include<sys/mman.h>
#include<time.h>
#include<sys/time.h>


struct Historyinfo
{
    char command[20];
    pid_t ppid;
    struct timeval start_time;
    struct timeval end_time;
    long dur;
}Historyinfo;
struct shmmem{
    int i;
    struct Historyinfo prehis[100];
    sem_t *mutex;
}shmmem;
int shm_fd;
struct shmmem* setup(){
    char * shm_name="my shared his";
    shm_fd=shm_open(shm_name,O_CREAT|O_RDWR,0666);
    if(shm_fd == -1)
    {
        printf("Shared memory failed");
        exit(0);
    }
    ftruncate(shm_fd,sizeof(shmmem));
    struct shmmem * sh_ptr = mmap(NULL,sizeof(shmmem),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    if(sh_ptr==MAP_FAILED)
    {
        printf("Mapping Failed");
        exit(0);
    }
    return sh_ptr;

}
struct shmmem * temp;

void cleanup()
{
    sem_destroy(temp->mutex);
    munmap(temp,sizeof(struct shmmem));
    shm_unlink("my shared his");
    close(shm_fd);
}

char* timevalToStandardTime(struct timeval time) {
    time_t rtime = time.tv_sec;
    struct tm* timeinfo;
    char* timestr;

    timeinfo = localtime(&rtime);

    timestr = asctime(timeinfo);
    size_t len = strlen(timestr);
    if (len > 0 && timestr[len - 1] == '\n') {
        timestr[len - 1] = '\0';
    }

    return timestr;
}


int  check_dot_slash_clear(char ** inputc){
    char arr[2] = "./" ;
    char arr2[5] = "clear" ;

    if(strncmp(*inputc, arr ,2)==0)
    {
        if(strcmp(*inputc , "./fib") == 0)
        {
        char cmd = *inputc[1] ;
        printf("cmd is %c" , cmd) ;
        if(execl("fib" , "fib" , cmd , NULL) == -1)
        {
            printf("fib was not executed ") ;
        }
        }
        else{
        printf("no fib") ;
    
        }
        if(strcmp(*inputc , "./helloworld") == 0){
            if(execl("helloworld" , "helloworld" , NULL) == -1)
            {
                printf("helloworld was not executed ") ;
            }

        }
        return 1 ;
    }
    else if (strncmp(*inputc , arr , 5 ) == 0 )
    {
        execl("/usr/bin/clear" , "clear" , NULL) ;
        printf("clear was not executed ") ;
    }
    return 0 ; 

    
}
void writehis(char *com,int ppid){//hh
    if(ppid==0){
        return;
    }
    FILE *fd=fopen("output.txt","a");
    if(fd==NULL){
        printf("File not opened");
        exit(0);
    }
    char temp1[40];
    char temp2[20];
    strcpy(temp1,com);
    snprintf(temp2,sizeof(temp2),"%d",ppid);
    strcat(temp1," ");
    strcat(temp1,temp2);
    fprintf(fd,"%s\n",temp1);
    fclose(fd);
    return;

}
void executecommand(char * command)
{
    char * inputc[100];
    int cnt=0;
    char *tok=strtok(command," ");
    if(tok==NULL){
        printf("No commands found");
        return;
    }
    while(tok!=NULL){
        inputc[cnt]=tok;
        cnt++;
        tok=strtok(NULL," ");
    }
    inputc[cnt]=NULL;
    if(strcmp(inputc[0],"history")==0){
        char * tem[]={"cat","output.txt",NULL};
        execvp(tem[0],tem);
        printf("The command %s was not executed" , inputc[0]) ; // if execvp fails
    }
    if(execvp(inputc[0],inputc)==-1){
        printf("The command %s was not executed" , inputc[0]) ; // if execvp fails
        exit(0);
    }
    

}
void recurcomexec(int fd[2],char* commands,int max,int ind,int prev)
{
    char * token;
    if(ind==max){
        return;
    }
    if(ind==0){
        token=strtok(commands,"|");
        if(token==NULL){
            printf("No commands found");
            return;
        }
    }else{
        token=strtok(NULL,"|");
        if(token==NULL){
            printf("No commands found");
            return;
        }
    }
    // error in opening pipe 
    if(pipe(fd) == -1){
        printf("Error in opening pipe") ; 
    }
    if(ind==0 && max!=1){
        if(fork()==0){
            sem_wait(temp->mutex);
            strcpy((temp->prehis)[temp->i].command,token);
            gettimeofday(&((temp->prehis)[temp->i].start_time),NULL);
            (temp->prehis)[temp->i].ppid=getpid();
            writehis(token,getpid());
            close(fd[0]);
            dup2(fd[1],STDOUT_FILENO);
            close(fd[1]);
            executecommand(token);

        }
    }
    else if(ind==max-1 && max!=1){
        if(fork()==0){
            sem_wait(temp->mutex);
            strcpy((temp->prehis)[temp->i].command,token);
            gettimeofday(&((temp->prehis)[temp->i].start_time),NULL);
            (temp->prehis)[temp->i].ppid=getpid();
            writehis(token,getpid());
            close(fd[1]);
            close(fd[0]);
            dup2(prev,STDIN_FILENO);
            close(prev);
            executecommand(token);
            printf("The cmds %s was not executed" , token) ; // if execvp fails
            exit(0);
        }
    }
    if(ind!=0 && ind!=max-1 && ind!=max && max!=1)
    {
        if(fork()==0){
            sem_wait(temp->mutex);
            strcpy((temp->prehis)[temp->i].command,token);
            gettimeofday(&((temp->prehis)[temp->i].start_time),NULL);
            (temp->prehis)[temp->i].ppid=getpid();
            // store the starting time of the child process 
            writehis(token,getpid());
            close(fd[0]);
            dup2(prev,STDIN_FILENO);
            close(prev);
            dup2(fd[1],STDOUT_FILENO);
            close(fd[1]);
            executecommand(token);
            printf("The cmds %s was not executed" , token) ;
            exit(0);
        }
    }
    close(fd[1]);
    wait(NULL);
    gettimeofday(&((temp->prehis)[temp->i].end_time),NULL);
    (temp->prehis)[temp->i].dur=((temp->prehis)[temp->i].end_time.tv_sec-(temp->prehis)[temp->i].start_time.tv_sec)*1000+((temp->prehis)[temp->i].end_time.tv_usec-(temp->prehis)[temp->i].start_time.tv_usec)/1000;
    temp->i++;
    sem_post(temp->mutex);
    recurcomexec(fd,commands,max,ind+1,fd[0]);

}
void executecommnopipe(char * input){
    if(fork()==0){
        sem_wait(temp->mutex);
        strcpy((temp->prehis)[temp->i].command,input);
        gettimeofday(&((temp->prehis)[temp->i].start_time),NULL);
        (temp->prehis)[temp->i].ppid=getpid();
        //sleep(3);(To test if duration is working)
        writehis(input,getpid());
        executecommand(input);
    }
    gettimeofday(&((temp->prehis)[temp->i].end_time),NULL);
    //store the end time .
    (temp->prehis)[temp->i].dur=((temp->prehis)[temp->i].end_time.tv_sec-(temp->prehis)[temp->i].start_time.tv_sec)*1000+
    ((temp->prehis)[temp->i].end_time.tv_usec-(temp->prehis)[temp->i].start_time.tv_usec)/1000;
    // set duration by calculating time difference . 
    temp->i++;
    sem_post(temp->mutex);
}
char *input;
void takeinput() {
    input = (char *)malloc(100);
    if(input==NULL)
    {
        printf("Malloc failed");
        exit(0);
    }
    char ch;
    int c = 0;
    int commandnumpipe = 0;
    while ((ch = getchar()) != EOF) {
        if (ch == '\n') {
            input[c] = '\0';
            if (c > 0) {
                if(commandnumpipe==0){
                    executecommnopipe(input);
                    c=0;
                }
                int fd[2];
                recurcomexec(fd, input, commandnumpipe + 1, 0, 0);
                close(fd[0]);
                close(fd[1]);
            }
            c = 0;
            commandnumpipe = 0;
        } else {
            input[c] = ch;
            if (ch == '|') {
                commandnumpipe++;
            }
            c++;
        }
    }

    free(input);
}
void display_history(int SIGNUM){

    for(int i=0;i<temp->i;i++){
        if(temp->prehis[i].ppid==0){
            continue;
        }
        
        printf("\ncommand : %s \t process id : %d \t duration : %ld \t start time : %s\n",temp->prehis[i].command,temp->prehis[i].ppid,temp->prehis[i].dur,timevalToStandardTime(temp->prehis[i].start_time));
    }
    cleanup();
    exit(0);
}
void setuptemp_()
{
    signal(SIGINT,display_history);
    temp=setup();
    temp->i=0;
    temp->mutex=(sem_t*)malloc(sizeof(sem_t));
    if(temp->mutex==NULL){
        printf("Malloc failed");
        exit(0);
    }
    sem_init(temp->mutex,1,1);
}

int main()
{
    setuptemp_() ;
    takeinput(); // Infinite while loop 

    
}

