#include <stdio.h>      // standard input output
#include <stdlib.h>     // EXIT_FAILURE
#include <string.h>     // memcpy 
#include <sys/shm.h>    // shm
#include <sys/mman.h>   // mmap
#include <fcntl.h>      // shared memory permission

int main(){ 
    /* name of the shared memory object */
    char buffer_name [10]= ""; 
    printf("Enter buffer name: ");
    scanf("%s",buffer_name);

    /**
    int shm_fd = shm_open(buffer_name, O_RDONLY, 0666); 

    void* ptr = mmap(0, 1, PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (ptr == MAP_FAILED){
        perror("MMAP FAILED, Error mmapping the file, Buffer hasn't been created!\n");
        return EXIT_FAILURE;
    }
    **/

    shm_unlink(buffer_name); 
    printf("Closing the shm...\n");
    return 0; 
} 