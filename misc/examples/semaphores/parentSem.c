#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>          /* O_CREAT, O_EXEC          */


const char *semName = "asdfsd";

void parent(void){
    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 0);

    if (sem_id == SEM_FAILED){
        perror("Parent  : [sem_open] Failed\n"); return;
    }

    printf("Parent  : Wait for Child to Print\n");
    if (sem_wait(sem_id) < 0)
        printf("Parent  : [sem_wait] Failed\n");
    printf("Parent  : Child Printed! \n");
    
    if (sem_close(sem_id) != 0){
        perror("Parent  : [sem_close] Failed\n"); return;
    }

    if (sem_unlink(semName) < 0){
        printf("Parent  : [sem_unlink] Failed\n"); return;
    }
}


int main(int argc, char *argv[])
{
    pid_t pid;
    
  

    parent();

    printf("Parent  : Done with sem_open \n");


    return 0;
}