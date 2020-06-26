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

#include "../include/common.h"

//Begin Region Global Variables 
char *app_name = NULL;
char *buffer_message_name = NULL;
int pid = 0;
//End Region Global Varibales

/**
 * Initialize the semaphores needed by the system
*/
int InitilizeSemaphores(int messageCount)
{
    printf("************************************************************ \n");
    printf("%s : %i - Start Semaphores Initializing \n", app_name, pid);
    if (sem_open("SEM_BUFF_PRODUCER", O_CREAT | O_EXCL, 0644, messageCount) == SEM_FAILED ||
        sem_open("SEM_BUFF_CONSUMER", O_CREAT | O_EXCL, 0644, 0) == SEM_FAILED ||
        sem_open("SEM_BUF_GLOB_READ_INDEX", O_CREAT | O_EXCL, 0644, 1) == SEM_FAILED ||
        sem_open("SEM_BUF_GLOB_WRITE_INDEX", O_CREAT | O_EXCL, 0644, 1) == SEM_FAILED ||
        sem_open("SEM_BUF_GLOB_DISABLE_PROCESS", O_CREAT | O_EXCL, 0644, 1) == SEM_FAILED ||
        sem_open("SEM_BUF_GLOB_FINALIZER", O_CREAT | O_EXCL, 0644, 0) == SEM_FAILED)
    {
        perror("Error");
        printf("%s : %i - Please Run Finalizer with -r flag \n", app_name, pid);
        return EXIT_FAILURE;
    }
    printf("%s : %i - End Semaphores Initializing \n", app_name, pid);
    printf("************************************************************ \n");
    return EXIT_SUCCESS;
}

int InitializeBuffers(int messageCount)
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
    printf("%s : %i - Creating the Global Var Buffer \n", app_name, pid);
    int shm_fd = shm_open(buffer_var_name, O_RDWR|O_CREAT|O_EXCL, 0666);
    if (shm_fd == -1)
    {
        perror("Error creating the Shared Memory Object");
        return EXIT_FAILURE;
    }
    printf("%s : %i - Created the Shared Memory Object \n", app_name, pid);
    if(ftruncate(shm_fd, sizeof(Global_Var)) == -1)
    {
        perror("Error during truncate process");
        return EXIT_FAILURE;
    }
    printf("%s : %i - Truncate the Shared Memory Object \n", app_name, pid);
    Global_Var *ptr_buff_glob_var = (Global_Var *)mmap(NULL, sizeof(Global_Var), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_var == MAP_FAILED){
        perror("Error during mapping process");
        return EXIT_FAILURE;
    } 
    printf("%s : %i - Shared Memory Object Mapped \n", app_name, pid);
    ptr_buff_glob_var->buffer_message_size = messageCount;
    ptr_buff_glob_var->active_consumers = 0;
    ptr_buff_glob_var->active_productors = 0;
    ptr_buff_glob_var->consumers_delete_by_key = 0;
    ptr_buff_glob_var->finalize = 0;
    ptr_buff_glob_var->historical_buffer_messages =0;
    ptr_buff_glob_var->historical_consumers = 0;
    ptr_buff_glob_var->historical_productor = 0;
    ptr_buff_glob_var->last_read_position = -1;
    ptr_buff_glob_var->last_write_position = -1;
    ptr_buff_glob_var->total_block_time = 0;
    ptr_buff_glob_var->total_cpu_time = 0;
    ptr_buff_glob_var->total_kernel_time = 0;
    ptr_buff_glob_var->total_wait_time = 0;
    ptr_buff_glob_var->total_user_time = 0;
    munmap(ptr_buff_glob_var, sizeof(Global_Var));
    free(buffer_var_name);

    //Global Message Buffer
    printf("************************************************************ \n");
    printf("%s : %i - Creating the Global Message Buffer \n", app_name, pid);
    //shm_unlink(buffer_message_name); 
    shm_fd = shm_open(buffer_message_name, O_RDWR|O_CREAT|O_EXCL, 0666);
    if (shm_fd == -1)
    {
        perror("Error creating the Shared Memory Object");
        return EXIT_FAILURE;
    }
    printf("%s : %i - Created the Shared Memory Object \n", app_name, pid);
    if(ftruncate(shm_fd, messageCount * sizeof(Global_Message)) == -1)
    {
        perror("Error during truncate process");
        return EXIT_FAILURE;
    }
    printf("%s : %i - Truncate the Shared Memory Object \n", app_name, pid);
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
    printf("   -s --size                 Maximum amount of messages the buffer will hold \n");    
	printf("\n");
}

int main(int argc, char *argv[]) { 
    static struct option long_options[] = {
		{"buffer_name", required_argument, 0, 'b'},
		{"size", required_argument, 0, 's'},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}
	};
    app_name = argv[0];
    app_name = app_name+2; //Delete ./ From the app Name
    pid = getpid();
    printf("%s : %i - Starting Initializer Process... \n", app_name, pid);
    int value, option_index = 0;	
    int message_count = 0;
    /* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "b:s:h", long_options, &option_index)) != -1) {
		switch (value) {
			case 'b':
                //Read buffer message name
				buffer_message_name = strdup(optarg);
				break;
			case 's':
                //Read the number of message that user wants
                sscanf(optarg, "%i", &message_count);
				break;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			default:
				break;
		}
	}
    if (buffer_message_name == NULL || strcmp("", buffer_message_name) == 0 || message_count <= 0 )
    {
        printf("%s : %i - Please use -h to see right parameters format \n", app_name, pid);
        return EXIT_FAILURE;
    }
    //InitilizeSemaphores(message_count);
    
    if(InitilizeSemaphores(message_count) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }
    if(InitializeBuffers(message_count) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
} 