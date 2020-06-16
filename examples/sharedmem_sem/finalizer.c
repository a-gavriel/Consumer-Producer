#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>          /* O_CREAT, O_EXEC          */
#include <string.h>
#include <unistd.h> // sleep
  
const char *sem_exitName = "sem_exit";
const char *sem_startName = "sem_start";

int main() {     
    sem_t *sem_exit = sem_open(sem_exitName, O_CREAT, 0600, 0);
    sem_t *sem_start = sem_open(sem_startName, O_CREAT, 0600, 0);


    int startvalue;
    sem_getvalue(sem_start, &startvalue);

    printf("Enter the buffer's name: ");
    char buffername [5] = "";
    scanf("%s", buffername);
    // ftok to generate unique key 
    key_t key = ftok(buffername,65); 
  
    // shmget returns an identifier in shmid 
    int shmid = shmget(key,1024,0666|IPC_CREAT); 
    

  
    printf("Press ENTER to finalize\n");
    char inp [5] = "";
    fflush(stdout);getchar(); getchar(); 

    printf("Finalizer  : about to wait for initializer! \n");
    if (sem_wait(sem_start) < 0)
        printf("Finalizer  : [sem_wait] Failed\n");

    // Exit semaphore
    if (sem_post(sem_exit) < 0)
        printf("Finalizer  : [sem_post] Failed\n");
    printf("Finalizer  : Ready to finish all processes! \n");

    printf("Finalizer  : Finalizing all processes! \n");

    if (sem_close(sem_exit) != 0){
        perror("Finalizer  : [sem_close] Failed\n"); return 1;
    }

    if (sem_unlink(sem_exitName) < 0){
        printf("Finalizer  : [sem_unlink1] Failed\n"); return 1;
    }
    if (sem_unlink(sem_startName) < 0){
        printf("Finalizer  : [sem_unlink2] Failed\n"); return 1;
    }

    
        
    // destroy the shared memory 
    shmctl(shmid,IPC_RMID,NULL); 
     
    return 0; 
} 