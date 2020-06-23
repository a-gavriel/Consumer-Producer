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
#include <semaphore.h>
#include <fcntl.h>          /* O_CREAT, O_EXEC          */

#include "../include/randomGenerators.h"

#define MSGSIZE 16
#define GLOB_SIZE 64

bool flag = true;
char *app_name = NULL;
double middleSeconds = -1;
char *buffer_name = NULL;
bool isAutoMode = false;
void *bufferPtr = NULL;
uint32_t SIZE = 1; 

//Begin Semaphore Region
char *semBufferName = "sem_buffer";
sem_t *semBuffer = NULL;
// End Semaphore Region

void InitializeSemaphores()
{
   semBuffer = sem_open(semBufferName, 0); /* Open a preexisting semaphore. */
}

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

/**
 * Read Finalizar Global (Buffer) Variable
*/
bool ValidateFinalizerSignal()
{
    //If finalizar global variable is 1 return true
}

/**
 * Exit Sequence (Finalizer Process start)
 * **/
void ExitFromFinalizerProcess()
{

}

/**
 * Exit Sequence (Read special message from buffer)
 * **/
void ExitFromSpecialMessage()
{

}

/**
 * Read Message from buffer
*/
char *ReadMessage()
{

}

int SyncBuffer()
{  
    /* shared memory file descriptor */ /* create the shared memory object */
    int shm_fd = shm_open(buffer_name, O_RDONLY, 0666); 
  
    /* pointer to shared memory obect */
    bufferPtr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (bufferPtr == MAP_FAILED){
        perror("MMAP FAILED, Error mmapping the file, Buffer hasn't been created!\n");
        return EXIT_FAILURE;
    }
    if(RemapBuffer() == 0)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

int RemapBuffer()
{
    uint8_t totmsgs;
    memcpy(&totmsgs,bufferPtr,1); 
    printf("Number of messages: %d\n", totmsgs);
    uint32_t newSIZE = GLOB_SIZE + MSGSIZE*totmsgs; 

    /* remap the shared memory object */
    void* temp = mremap(bufferPtr, SIZE, newSIZE, MREMAP_MAYMOVE); 
    if(temp == MAP_FAILED){
        perror("REMAP FAILED, Error on mremap()");
        return EXIT_FAILURE;
    }
    bufferPtr = temp;
    return EXIT_SUCCESS;
}


void AutomatedConsumerProcess()
{

}

void ManualConsumerProcess()
{
    short int exitMode = 0;
    char *message = NULL;
    while(flag)
    {
        if(ValidateFinalizerSignal())
        {
            exitMode = 0;
            break;
        }
        if(getchar() == 10)
        {
            //Start Critical Region
            sem_wait(semBuffer);           
            message = ReadMessage();
            sem_post(semBuffer);
            //End Critical Region (Release Sem)
        }
    }
    if(exitMode == 0)
    {
        ExitFromFinalizerProcess();
    }
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

    //Open Buffer
    /*
    if(SyncBuffer()  == 1)
    {
        return EXIT_FAILURE;
    }
    */
    //Select mode
    if(isAutoMode)
    {
        AutomatedConsumerProcess();
    }
    else
    {
        ManualConsumerProcess();
    }
    

    

    // Sleep Section




    //For testing!
    //uint8_t value1;
    //memcpy(&value1,bufferPtr+50,1); 
    //printf("Value read: %d\n", value1);

    //memcpy(&value1,bufferPtr+51,1); 
    //printf("Value read: %d\n", value1);

    return 0; 
} 