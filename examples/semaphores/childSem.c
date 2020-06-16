#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>          /* O_CREAT, O_EXEC          */

const char *semName = "asdfsd";

void child(void)
{
    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 0);

    if (sem_id == SEM_FAILED){
        perror("Child   : [sem_open] Failed\n"); return;        
    }

    printf("Child   : I am done! Release Semaphore\n");
    if (sem_post(sem_id) < 0)
        printf("Child   : [sem_post] Failed \n");
}

int main(int argc, char *argv[])
{
    pid_t pid;
    

    
    child();
    printf("Child   : Done with sem_open \n");


    return 0;
}