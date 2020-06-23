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
#include <getopt.h>
#include <sys/types.h>
#include <stdbool.h>

#include "../include/randomGenerators.h"

#define MSGSIZE 16
#define GLOB_SIZE 64

char *app_name = NULL;
double middleSeconds = -1;
char *buffer_name = NULL;
bool isAutoMode = false;

/**
 * \brief Print help for this application
 */
void print_help(void)
{
	printf("\n Usage: %s [OPTIONS]\n\n", app_name);
	printf("  Options:\n");
	printf("   -h --help                 Print this help\n");
	printf("   -b --buffer_name          Buffer name for attach the process, Need to be diferent to empty\n");
    printf("   -s --seconds              Time in seconds for random algorithm waiting time generator, Need to be greater than zero \n");
    printf("   -m --mode                 Execution Mode: M = manual | A = automatic \n");
	printf("\n");
}


int main(int argc, char *argv[]){ 

    static struct option long_options[] = {
		{"buffer_name", required_argument, 0, 'b'},
		{"seconds", required_argument, 0, 's'},
		{"help", no_argument, 0, 'h'},
        {"mode", required_argument, 0, 'm'},
		{NULL, 0, 0, 0}
	};
    app_name = argv[0];
    app_name = app_name+2; //Delete ./ From the app Name
    int pid = getpid();
    int value, option_index = 0;	
    char *mode = NULL;
    /* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "b:s:m:h", long_options, &option_index)) != -1) {
		switch (value) {
			case 'b':
				buffer_name = strdup(optarg);
				break;
			case 's':
                sscanf(optarg, "%lf", &middleSeconds);
				break;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
            case 'm':
                mode = strdup(optarg);
                break;
			default:
				break;
		}
	}
    if (buffer_name == NULL || strcmp("", buffer_name) == 0 || middleSeconds <= 0 || strcmp("", mode) == 0 || ( strcmp("A", mode) != 0 && strcmp("M", mode) != 0) )
    {
        printf("%s : %i - Please use -h to see right parameters format \n", app_name, pid);
        return EXIT_FAILURE;
    }

    if( strcmp("A", mode) == 0)
    {
        isAutoMode = true;
    }
    else
    {
        isAutoMode = false;
    }
    

    printf("%s \n", buffer_name);
    printf("%lf \n", middleSeconds);
    printf("%d \n", isAutoMode);

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


    // Sleep Section



    //For testing!
    uint8_t value1;
    memcpy(&value1,ptr+50,1); 
    printf("Value read: %d\n", value1);

    memcpy(&value1,ptr+51,1); 
    printf("Value read: %d\n", value1);

    return 0; 
} 