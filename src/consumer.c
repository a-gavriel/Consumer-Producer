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
#include <stdbool.h>
#include <semaphore.h>
#include <fcntl.h>          /* O_CREAT, O_EXEC          */
#include <time.h>

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

typedef struct Global_Var
{
    int buffer_message_size; //Use for the remap on the consumer and producer process
    int active_consumers;
    int active_productors;
    int historical_consumers;
    int historical_buffer_messages; //Historical count of messages inserted into the buffer
    int historical_productor;
    int buffer_count_message; //Carry the count of the message in the buffer in the instant t
    int last_read_position;
    int last_write_position;
    int consumers_delete_by_key;
    double total_cpu_time;  //Sumatory of all process CPU time (Producer and Consumers)
    double total_wait_time; //Total Time wait by the process (producers and consumers) (poison and exponential generators )
    double total_block_time; //Total Time blocked by semaphores (producers and consumers)
    double total_kernel_time; //Total Time in Kernel mode (producers and consumers)
    double total_user_time; //Total Time in user mode (producers and consumers)
    short int finalize;
} Global_Var;

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
    return true;
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
char ReadMessage()
{
    int position = 0; 
    char *tempBuffer = bufferPtr + GLOB_SIZE + (position * 16);
    char producerId[4] = "";
    *producerId = *tempBuffer;
    *tempBuffer += 4;
    char dateTime[8] = "";
    *dateTime = *tempBuffer;
    *tempBuffer += 8;
    char magicNumber = *tempBuffer;
    printf("Consumer PID: %s \n", producerId);
    printf("DateTime: %s \n", dateTime);
    printf("Magic Number: %c \n", magicNumber);
    return magicNumber;
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


void AutomatedConsumerProcess()
{

}

void ManualConsumerProcess()
{
    short int exitMode = 0;
    char magicNumber = 'a';
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
            magicNumber = ReadMessage();
            printf("%c", magicNumber);
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
    clock_t begin = clock();

    //Read command Line args

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
      
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    //Save statistics

    //Release resources (mem, etc)

    return 0; 
} 