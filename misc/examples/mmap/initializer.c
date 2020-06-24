#include <stdio.h>      // standard input output
#include <stdlib.h>     // EXIT_FAILURE
#include <string.h>     // memcpy 
#include <fcntl.h>      // shared memory permission
#include <sys/shm.h>    // shm
#include <sys/mman.h>   // mmap
#include <unistd.h>     // ftruncate
#include <stdint.h>     // for int variants
#include <inttypes.h>   // For scanf of uint8_t

#define MSGSIZE 16
#define GLOB_SIZE 64

int main() { 
    /* name of the shared memory object */
    char buffer_name [10]= ""; 
    printf("Enter buffer name: ");
    scanf("%s",buffer_name);
    
    /* the size (in bytes) of shared memory object */
    printf("Enter the amount of messages the buffer will hold: " );
    uint8_t totmsgs;
    scanf("%" SCNu8, &totmsgs); 

    uint32_t SIZE = GLOB_SIZE + MSGSIZE*totmsgs; 
  
    /* shared memory file descriptor */
    int shm_fd; 
  
    /* pointer to shared memory obect */
    void* ptr; 
  
    /* create the shared memory object */
    shm_fd = shm_open(buffer_name, O_CREAT | O_RDWR, 0666); 
  
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE); 
  
    /* memory map the shared memory object */
    ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0); 
    if (ptr == MAP_FAILED){
        perror("MMAP FAILED, Error mmapping the file, Buffer wasn't created!\n");
        return EXIT_FAILURE;
    }

    // Writes the totmsgs in the beginning of the buffer
    // dest , src , size
    memcpy(ptr,&totmsgs,1);

    // For testing!
    uint8_t value = 11;
    memcpy(ptr+50,&value,1);
} 