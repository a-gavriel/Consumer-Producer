#define _GNU_SOURCE
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/shm.h> 
#include <sys/stat.h> 
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/types.h>
#include <stdint.h>

#define MSGSIZE 16
#define GLOB_SIZE 64

int main(){ 
    /* name of the shared memory object */
    char buffer_name [10]= ""; 
    printf("Enter buffer name: ");
    scanf("%s",buffer_name);

    /* the size (in bytes) of shared memory object */
    uint32_t SIZE = 1; 
  
    /* shared memory file descriptor */ /* create the shared memory object */
    int shm_fd = shm_open(buffer_name, O_RDONLY, 0666); 
  
    /* pointer to shared memory obect */
    void* ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (ptr == MAP_FAILED){
        perror("MMAP FAILED, Error mmapping the file, Buffer hasn't been created!\n");
        return EXIT_FAILURE;
    }

    uint8_t totmsgs;
    memcpy(&totmsgs,ptr,1); 
    printf("Number of messages: %d\n", totmsgs);
    uint32_t newSIZE = GLOB_SIZE + MSGSIZE*totmsgs; 

    /* remap the shared memory object */
    void* temp = mremap(ptr, SIZE, newSIZE, MREMAP_MAYMOVE); 
    if(temp == MAP_FAILED){
        perror("REMAP FAILED, Error on mremap()");
        return EXIT_FAILURE;
    }
    ptr = temp;


    //For testing!
    uint8_t value;
    memcpy(&value,ptr+50,1); 
    printf("Value read: %d\n", value);

    memcpy(&value,ptr+51,1); 
    printf("Value read: %d\n", value);

    return 0; 
} 