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
  
#define BUFFER_GLOB_SUFIX "_GLOBAL"

//Struct Region
typedef struct Global_Var
{
    int buffer_message_size; //Use for the remap on the consumer and producer process
    int active_consumers;
    int active_productors;
    int historical_consumers;
    int historical_buffer_messages; //Historical count of messages inserted into the buffer
    int historical_productor;
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

typedef struct Global_Message
{
    pid_t pid;
    time_t date_time;
    short int magic_number;
    char message[20];
} Global_Message;

//End Struct Region

bool flag = true;
char *app_name = NULL;
double middleSeconds = -1;
char *buffer_message_name = NULL;
bool isAutoMode = false;
Global_Var *ptr_buff_glob_var = NULL;
Global_Message *ptr_buff_glob_mess = NULL;
int pid = 0;

//Begin Semaphore Region
sem_t *sem_finalize = NULL;
sem_t *sem_consumer = NULL;
sem_t *sem_producer = NULL;
sem_t *sem_last_read = NULL;
// End Semaphore Region


int InitilizeSemaphores()
{
    sem_producer = sem_open("SEM_BUFF_PRODUCER", 0);
    if ( sem_producer == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    sem_consumer = sem_open("SEM_BUFF_CONSUMER", 0);
    if ( sem_consumer == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    sem_last_read = sem_open("SEM_BUF_GLOB_READ_INDEX", 0);
    if ( sem_last_read == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    if (sem_open("SEM_BUF_GLOB_WRITE_INDEX", 0) == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    if (sem_open("SEM_BUF_GLOB_DISABLE_PROCESS", 0) == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    sem_finalize = sem_open("SEM_BUF_GLOB_FINALIZER", 0);
    if ( sem_finalize == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
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
 * Exit Sequence
 * **/
void ExitProcess()
{

}

/**
 * Read Message from buffer
*/
int ReadMessage()
{
    printf("Call ReadMessage Function\n");
    //Start if the buffer has message
    sem_wait(sem_consumer);
    //Block others consumers
    sem_wait(sem_last_read);
    printf("************************************************************ \n");
    printf("New Message to Read: \n");
    int last_read_position = ptr_buff_glob_var->last_read_position;
    int max_messages = ptr_buff_glob_var->buffer_message_size;
    //Validate if the last position was the end of the buffer, if true reset to start position
    if (last_read_position == (max_messages - 1))
    {
        last_read_position = -1;
    }
    int positon_to_read = last_read_position + 1;

    //Process the message readed
    int magic_number = ptr_buff_glob_mess[positon_to_read].magic_number;

    printf("\t %s \n",ptr_buff_glob_mess[positon_to_read].date_time);
    printf("\t Magic Number: %i \n",magic_number);
    //Set the new last read position index
    ptr_buff_glob_var->last_read_position = positon_to_read;
    //Release for other consumer process
    sem_post(sem_last_read);
    //Producers process can write one more message
    sem_post(sem_producer);
    return magic_number;
}

int SyncBuffer()
{  
    char *buffer_var_name = malloc(strlen(buffer_message_name) + strlen(BUFFER_GLOB_SUFIX) + 1);
    if(buffer_var_name == NULL)
    {
        return EXIT_FAILURE;
    }
    strcpy(buffer_var_name, buffer_message_name);
    strcat(buffer_var_name, BUFFER_GLOB_SUFIX);

    //Global Variables Buffer   
    // O_EXCL If the shared memory object already exist 
    printf("************************************************************ \n");
    printf("%s : %i - Opening Global Var Buffer \n", app_name, pid);
    int shm_fd = shm_open(buffer_var_name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error creating the Shared Memory Object");
        return EXIT_FAILURE;
    } 
    ptr_buff_glob_var = (Global_Var *)mmap(NULL, sizeof(Global_Var), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_var == MAP_FAILED){
        perror("Error during mapping process");
        return EXIT_FAILURE;
    }
    //Read the message buffer size (max of message)
    int message_count = ptr_buff_glob_var->buffer_message_size;
    printf("%i \n", message_count);

    //Global Message Buffer
    printf("************************************************************ \n");
    printf("%s : %i - Opening the Global Message Buffer \n", app_name, pid);
    shm_fd = shm_open(buffer_message_name, O_RDONLY, 0666);
    if (shm_fd == -1)
    {
        perror("Error creating the Shared Memory Object");
        return EXIT_FAILURE;
    } 
    ptr_buff_glob_mess = (Global_Message *)mmap(NULL, (message_count * sizeof(Global_Message)), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_mess == MAP_FAILED){
        perror("Error during mapping process");
        return EXIT_FAILURE;
    }
    printf("************************************************************ \n");
    return EXIT_SUCCESS;
}


void AutomatedConsumerProcess()
{

}

void ManualConsumerProcess()
{
    int magicNumber = 0;
    while(flag)
    {
        if(ptr_buff_glob_var->finalize == 1)
        {
            //Finalize process by Finalizaer Global Var
            printf("%s : %i - Start Finalize Process | Reason: Global Var Finalize Process \n", app_name, pid);
            break;
        }
        if(getchar() == 10)
        {  
            magicNumber = ReadMessage();
        }
        if(magicNumber == pid % 6)
        {
            //Finalize process by Special Message (magic number)
            printf("%s : %i - Start Finalize Process | Reason: Magic Number \n", app_name, pid);
            break;
        }
    }
    flag = false;
    ExitProcess();
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
    pid = getpid();
    int value, option_index = 0;	
    char *mode = NULL;
    /* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "b:s:m:h", long_options, &option_index)) != -1) {
		switch (value) {
			case 'b':
				buffer_message_name = strdup(optarg);
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
    if (buffer_message_name == NULL || strcmp("", buffer_message_name) == 0 || middleSeconds <= 0 || strcmp("", mode) == 0 || ( strcmp("A", mode) != 0 && strcmp("M", mode) != 0) )
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
    
    //Initialize Semaphores
    if(InitilizeSemaphores() == 1)
    {
        return EXIT_FAILURE;
    }
    //Open Buffer
    if(SyncBuffer()  == 1)
    {
        return EXIT_FAILURE;
    }
    //Review if the finalize global var is rised
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