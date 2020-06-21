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


int main(){ 
    /* name of the shared memory object */
    char buffer_name [10]= ""; 
    printf("Enter buffer name: ");
    scanf("%s",buffer_name);

    int shm_fd = shm_open(buffer_name, O_RDONLY, 0666); 

    void* ptr = mmap(0, 1, PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (ptr == MAP_FAILED){
        perror("MMAP FAILED, Error mmapping the file, Buffer hasn't been created!\n");
        return EXIT_FAILURE;
    }

    shm_unlink(buffer_name); 
    printf("Closing the shm...\n");
    return 0; 
} 