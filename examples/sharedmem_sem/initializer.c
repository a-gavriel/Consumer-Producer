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

    

    printf("Enter the buffer's name: ");
    char buffername [5]= "";
    scanf("%s", buffername);
    // ftok to generate unique key 
    key_t key = ftok(buffername,65); 
  
    // shmget returns an identifier in shmid 
    int shmid = shmget(key,1024,0666|IPC_CREAT); 
  
  
  
    printf("Initializer   : I am done! Release Semaphore\n");
    sem_post(sem_start);

    int finish = 0;
    //finish = sem_wait(sem_exit)  // TODO: Aun no sé si es necesario
    if ( finish < 0)
        printf("Initializer   : [sem_wait] Failed \n");

    
    return 0; 
} 