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
char * buffer_var_name = NULL;
int pid = 0;
Global_Var *ptr_buff_glob_var = NULL;
Global_Message *ptr_buff_glob_mess = NULL;
sem_t *sem_finalize = NULL;
sem_t *sem_consumer = NULL;
sem_t *sem_producer = NULL;
//End Region Global Varibales

/**
 * \brief Print help for this application
 */
void print_help(void)
{
	printf("\n Usage: %s [OPTIONS]\n\n", app_name);
	printf("  Options:\n");
	printf("   -h --help                 Print this help\n");
	printf("   -b --buffer_name          Buffer name for attach the process, Need to be diferent to empty\n");
    printf("   --f --forcefree            Force Release Global Memory (Buffers and Semaphores) \n");
	printf("\n");
}

/**
 * Initialize the semaphores needed by the system
*/
int InitilizeSemaphores()
{
    printf("************************************************************ \n");
    printf("%s : %i - Start Semaphores Sync \n", app_name, pid);
    sem_producer = sem_open("SEM_BUFF_PRODUCER", 0);
    sem_consumer = sem_open("SEM_BUFF_CONSUMER", 0);
    sem_finalize = sem_open("SEM_BUF_GLOB_FINALIZER", 0);
    if ( sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED || sem_finalize == SEM_FAILED)
    {
        perror("Error");
        return EXIT_FAILURE;
    }
    printf("%s : %i - End Semaphores Sync \n", app_name, pid);
    printf("************************************************************ \n");
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

int DestroySemaphores()
{
    printf("************************************************************ \n");
    printf("%s : %i - Closing all Semaphores \n", app_name, pid);
    if(sem_unlink("SEM_BUFF_PRODUCER") == -1 || sem_unlink("SEM_BUFF_CONSUMER") == -1 ||
        sem_unlink("SEM_BUF_GLOB_READ_INDEX") == -1 || sem_unlink("SEM_BUF_GLOB_WRITE_INDEX") == -1 || 
        sem_unlink("SEM_BUF_GLOB_DISABLE_PROCESS") == -1 || sem_unlink("SEM_BUF_GLOB_FINALIZER") == -1)
    {
        perror("Error");
        return EXIT_FAILURE;
    }
    printf("************************************************************ \n");
    return EXIT_SUCCESS;
}

int Free()
{
    DestroySemaphores();
    printf("Closing the shm...\n");
    shm_unlink(buffer_message_name); 
    shm_unlink(buffer_var_name); 
    printf("Shm Closed \n");
    //Free Memory 
    munmap(ptr_buff_glob_var, sizeof(Global_Var));
    free(buffer_var_name);
    sem_close(sem_consumer);
    sem_close(sem_producer);
    sem_close(sem_finalize);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) { 
    static struct option long_options[] = {
		{"buffer_name", required_argument, 0, 'b'},
        {"forcefree", no_argument, 0, 'f' },
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}
	};
    app_name = argv[0];
    app_name = app_name+2; //Delete ./ From the app Name
    int force_free = 0;
    pid = getpid();
    printf("%s : %i - Starting Finalizer Process... \n", app_name, pid);
    int value, option_index = 0;	
    /* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "b:f:h", long_options, &option_index)) != -1) {
		switch (value) {
			case 'b':
                //Read buffer message name
				buffer_message_name = strdup(optarg);
				break;
            case 'f':
				force_free = 1;
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

    //Special Rutine only for release global mem (semaphores and buffers)
    if(force_free == 1)
    {
        if(Free() == 1)
        {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    else
    {
        //Set Finalize Flag
        ptr_buff_glob_var->finalize = 1;
        //Wake up consumer/producers wainting if the buffer is empty of full;
        sem_post(sem_consumer);
        printf("%s : %i - Trying to Wake Up Consumers \n", app_name, pid);
        sem_post(sem_producer);
        printf("%s : %i - Trying to Wake Up Producers \n", app_name, pid);
        printf("%s : %i - Wait untill all process (consumers/producres) end ... \n", app_name, pid);
        //Wait all consumer and producers ends
        sem_wait(sem_finalize);
    }
    return EXIT_SUCCESS; 
} 