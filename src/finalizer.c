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
    printf(KNRM"************************************************************ \n");
    printf(KGRN"%s : %i - Start Semaphores Sync \n", app_name, pid);
    sem_producer = sem_open("SEM_BUFF_PRODUCER", 0);
    sem_consumer = sem_open("SEM_BUFF_CONSUMER", 0);
    sem_finalize = sem_open("SEM_BUF_GLOB_FINALIZER", 0);
    if ( sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED || sem_finalize == SEM_FAILED)
    {
        perror(KRED"Error in semaphores sync");
        return EXIT_FAILURE;
    }
    printf("%s : %i - End Semaphores Sync \n", app_name, pid);
    printf(KNRM"************************************************************ \n");
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
    printf(KNRM"************************************************************ \n");
    printf(KGRN"%s : %i - Opening the Global Var Buffer \n", app_name, pid);
    int shm_fd = shm_open(buffer_var_name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror(KRED"Error creating the Shared Memory Object");
        return EXIT_FAILURE;
    }
    printf("%s : %i - Created the Shared Memory Object \n", app_name, pid);

    ptr_buff_glob_var = (Global_Var *)mmap(NULL, sizeof(Global_Var), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_var == MAP_FAILED)
    {
        perror(KRED"Error during mapping process");
        return EXIT_FAILURE;
    } 
    printf("%s : %i - Shared Memory Object Mapped \n", app_name, pid);
    printf(KNRM"************************************************************ \n");
    return EXIT_SUCCESS;
}

int DestroySemaphores()
{
    printf(KNRM"************************************************************ \n");
    printf(KGRN"%s : %i - Closing all Semaphores \n", app_name, pid);
    int unlinkProducer = sem_unlink("SEM_BUFF_PRODUCER");
    int unlinkConsumer = sem_unlink("SEM_BUFF_CONSUMER");
    int unlinkGlobR = sem_unlink("SEM_BUF_GLOB_READ_INDEX") ;
    int unlinkGlobW = sem_unlink("SEM_BUF_GLOB_WRITE_INDEX");
    int unlinkDisable = sem_unlink("SEM_BUF_GLOB_DISABLE_PROCESS");
    int unlinkFinalizer = sem_unlink("SEM_BUF_GLOB_FINALIZER");

    if( unlinkProducer == -1 || unlinkConsumer == -1 ||unlinkGlobR == -1 || 
        unlinkGlobW == -1 || unlinkDisable == -1 || unlinkFinalizer == -1)
    {
        perror(KRED"Error detected closing semaphores");
        return EXIT_FAILURE;
    }
    printf(KNRM"************************************************************ \n");
    return EXIT_SUCCESS;
}

int Free()
{
    DestroySemaphores();
    printf(KNRM"************************************************************ \n");
    printf(KGRN"Closing the shm...\n");
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

int ShowStatistics(int messages)
{
    printf(KNRM"************************************************************ \n");
    printf(KCYN"%s : %i - Statistics: \n", app_name, pid);
    printf("%s : %i - Total Produced Messages: %i \n", app_name, pid, ptr_buff_glob_var->historical_buffer_messages);
    printf("%s : %i - Total Messages left in buffer: %i \n", app_name, pid, messages);
    printf("%s : %i - Total Producers Created: %i \n",  app_name, pid, ptr_buff_glob_var->historical_productor);
    printf("%s : %i - Total Consumers Created: %i \n", app_name, pid, ptr_buff_glob_var->historical_consumers);
    printf("%s : %i - Total Consumers Deleted by Key: %i \n", app_name, pid, ptr_buff_glob_var->consumers_delete_by_key);

    double elapsed_time = ptr_buff_glob_var->total_cpu_time;
    double sleep_time = ptr_buff_glob_var->total_wait_time;
    double blocked_time = ptr_buff_glob_var->total_block_time;
    double sys_time = ptr_buff_glob_var->total_kernel_time;
    double usr_time = ptr_buff_glob_var->total_user_time;
    double process_time = sys_time + usr_time;
    double total_suspended_time = elapsed_time - process_time;

    printf(KMAG"%s : %i - Accumulated Total time %f \n", app_name, pid, elapsed_time);
    printf("%s : %i - Accumulated Suspended time %f \n", app_name, pid, total_suspended_time);
    printf("%s : %i \t- Wait time %f \n", app_name, pid, sleep_time);  
    printf("%s : %i \t- Blocked time %f \n", app_name, pid, blocked_time);    
    printf("%s : %i - Processing time %f \n", app_name, pid, process_time);
    printf("%s : %i \t- System time %f \n", app_name, pid, sys_time);
    printf("%s : %i \t- User time %f \n", app_name, pid, usr_time);
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
    printf(KNRM"%s : %i - Starting Finalizer Process... \n", app_name, pid);
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
    }
    else
    {
        //Set Finalize Flag
        int actual_messages = 0;
        sem_getvalue(sem_consumer, &actual_messages); //Get the actuall messages inside the buffer
        printf("Read sem_consumer %i \n", actual_messages);
        ptr_buff_glob_var->finalize = 1;
        //Wake up consumer/producers wainting if the buffer is empty of full;
        sem_post(sem_consumer);
        printf(KNRM"%s : %i - Trying to Wake Up Consumers \n", app_name, pid);
        sem_post(sem_producer);
        printf("%s : %i - Trying to Wake Up Producers \n", app_name, pid);
        printf("%s : %i - Wait untill all process (consumers/producres) end ... \n", app_name, pid);
        //Wait all consumer and producers ends
        sem_wait(sem_finalize);
        printf("%s : %i - All process (consumers/producres) end ... \n", app_name, pid);
        ShowStatistics(actual_messages);
        Free();
    }
    return EXIT_SUCCESS; 
} 