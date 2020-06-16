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

    sem_t *sem_start = sem_open(sem_startName, O_CREAT, 0600, 0);


    printf("Enter the buffer's name: ");
    char buffername [5]= "";
    scanf("%s", buffername);
    // ftok to generate unique key 
    key_t key = ftok(buffername,65); 
  
    // shmget returns an identifier in shmid 
    int shmid = shmget(key,1024,0666); 
  
    // shmat to attach to shared memory 
    char *str = (char*) shmat(shmid,(void*)0,0); 
  
    char buffer[] = "xyz";
    strcpy (str,buffer);
    printf("Data written in memory: %s\n",str);
      


    // Do loop
    int continue_loop;
    sem_getvalue(sem_start, &continue_loop);
    while(continue_loop != 0){
        printf("continue producing stuff...\n");
        sleep(1);
        sem_getvalue(sem_start, &continue_loop);
    }
    printf("Stopping producer...\n");        


    //detach from shared memory  
    shmdt(str); 
  
    return 0; 
} 