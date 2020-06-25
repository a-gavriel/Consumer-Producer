#include <stdio.h>      // standard input output
#include <stdlib.h>     // EXIT_FAILURE
#include <string.h>     // memcpy 
#include <fcntl.h>      // shared memory permission
#include <sys/shm.h>    // shm
#include <sys/mman.h>   // mmap
#include <unistd.h>     // ftruncate
#include <stdint.h>     // for int variants
#include <inttypes.h>   // For scanf of uint8_t
#include <stdbool.h>
#include <semaphore.h>
#include <getopt.h>
#include <sys/types.h>

#define BUFFER_GLOB_SUFIX "_GLOBAL"


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

typedef struct Global_Message
{
    pid_t pid;
    time_t date_time;
    short int magic_number;
    char message[20];
} Global_Message;

//Begin Region Global Variables 
char *app_name = NULL;
char *buffer_message_name = NULL;
char * buffer_var_name = NULL;
int pid = 0;
Global_Var *ptr_buff_glob_var = NULL;
Global_Message *ptr_buff_glob_mess = NULL;
sem_t *sem_finalize = NULL;
//End Region Global Varibales

/**
 * Initialize the semaphores needed by the system
*/
int InitilizeSemaphores()
{
    //Semaphore for wake up the finalizer process after all process get disabled
    sem_finalize = sem_open("SEM_BUF_GLOB_FINALIZER", 0);
    if ( sem_finalize == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int InitializeBuffers()
{ 
    buffer_var_name = malloc(strlen(buffer_message_name) + strlen(BUFFER_GLOB_SUFIX) + 1);
    if(buffer_var_name == NULL)
    {
        return EXIT_FAILURE;
    }
    strcpy(buffer_var_name, buffer_message_name);
    strcat(buffer_var_name, BUFFER_GLOB_SUFIX);

    //Global Variables Buffer   
    // O_EXCL If the shared memory object already exist  
    printf("************************************************************ \n");
    printf("%s : %i - Opening the Global Var Buffer \n", app_name, pid);
    int shm_fd = shm_open(buffer_var_name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error creating the Shared Memory Object");
        return EXIT_FAILURE;
    }
    printf("%s : %i - Created the Shared Memory Object \n", app_name, pid);

    ptr_buff_glob_var = (Global_Var *)mmap(NULL, sizeof(Global_Var), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_var == MAP_FAILED){
        perror("Error during mapping process");
        return EXIT_FAILURE;
    } 
    printf("%s : %i - Shared Memory Object Mapped \n", app_name, pid);
    printf("************************************************************ \n");
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

int main(int argc, char *argv[]) { 
    static struct option long_options[] = {
		{"buffer_name", required_argument, 0, 'b'},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}
	};
    app_name = argv[0];
    app_name = app_name+2; //Delete ./ From the app Name
    pid = getpid();
    int value, option_index = 0;	
    /* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "b:h", long_options, &option_index)) != -1) {
		switch (value) {
			case 'b':
                //Read buffer message name
				buffer_message_name = strdup(optarg);
				break;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			default:
				break;
		}
	}
    if (buffer_message_name == NULL || strcmp("", buffer_message_name) == 0 )
    {
        printf("%s : %i - Please use -h to see right parameters format \n", app_name, pid);
        return EXIT_FAILURE;
    }
    if(InitializeBuffers() == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }
    if(InitilizeSemaphores() == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    //Set Finalize Flag
    ptr_buff_glob_var->finalize = 1;

    //Wait all consumer and producers ends
    //sem_wait(sem_finalize);
    printf("Closing the shm...\n");
    shm_unlink(buffer_message_name); 
    shm_unlink(buffer_var_name); 
    printf("Shm Closed \n");
    munmap(ptr_buff_glob_var, sizeof(Global_Var));
    free(buffer_var_name);
    return 0; 
} 