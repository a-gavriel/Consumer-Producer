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
#include <sys/times.h>

#include "../include/randomGenerators.h"
#include "../include/common.h"

bool flag = true;
char *app_name = NULL;
double mean_seconds = 0;
char *buffer_message_name = NULL;
bool isAutoMode = false;
Global_Var *ptr_buff_glob_var = NULL;
Global_Message *ptr_buff_glob_mess = NULL;
int pid = 0;
int message_count = 0;
int exit_by_key = 0;
double blocked_timer = 0.0;
double sleep_timer = 0.0;
int total_message_readed = 0;
int max_message = 0;

//Begin Semaphore Region
sem_t *sem_consumer = NULL;
sem_t *sem_finalize = NULL;
sem_t *sem_producer = NULL;
sem_t *sem_last_read = NULL;
sem_t *sem_disable_process = NULL;
// End Semaphore Region

//
/**
 * \brief Print help for this application
 */
void print_help(void)
{
	printf(KYEL"\n Usage: %s [OPTIONS]\n\n", app_name);
	printf("  Options:\n");
	printf("   -h --help                 Print this help\n");
	printf("   -b --buffer_name          Buffer name for attach the process, Need to be diferent to empty\n");
    printf("   -s --seconds              Time in seconds for random algorithm waiting time generator, Need to be greater than zero \n");
    printf("   -m --mode                 Execution Mode: M = manual | A = automatic \n");
	printf(KNRM"\n");
}

void PrintDateTime(time_t time)
{
    time_t t = time;
    struct tm tm = *localtime(&t);
    printf("%d-%02d-%02d %02d:%02d:%02d\n",
         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int InitilizeSemaphores()
{
    printf(KNRM"************************************************************ \n");
    printf("%s : %i - Start Semaphores Sync \n", app_name, pid);
    sem_producer = sem_open("SEM_BUFF_PRODUCER", 0);
    sem_consumer = sem_open("SEM_BUFF_CONSUMER", 0);
    sem_last_read = sem_open("SEM_BUF_GLOB_READ_INDEX", 0);
    sem_disable_process = sem_open("SEM_BUF_GLOB_DISABLE_PROCESS", 0);
    sem_finalize = sem_open("SEM_BUF_GLOB_FINALIZER", 0);
    if ( sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED || sem_last_read == SEM_FAILED || 
        sem_disable_process == SEM_FAILED || sem_finalize == SEM_FAILED)
    {
        perror(KRED"Error");
        return EXIT_FAILURE;
    }
    printf("%s : %i - End Semaphores Sync \n", app_name, pid);
    printf("************************************************************ \n");
    return EXIT_SUCCESS;
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
        perror(KRED"Error creating the Shared Memory Object");
        return EXIT_FAILURE;
    }
    printf(KGRN"%s : %i - Created the Shared Memory Object \n", app_name, pid);
    ptr_buff_glob_var = (Global_Var *)mmap(NULL, sizeof(Global_Var), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_var == MAP_FAILED){
        perror(KRED"Error during mapping process");
        return EXIT_FAILURE;
    }
    printf(KGRN"%s : %i - Shared Memory Object Mapped \n", app_name, pid);
    //Read the message buffer size (max of message)
    message_count = ptr_buff_glob_var->buffer_message_size;

    //Global Message Buffer
    printf(KNRM"************************************************************ \n");
    printf(KNRM"%s : %i - Opening the Global Message Buffer \n", app_name, pid);
    shm_fd = shm_open(buffer_message_name, O_RDONLY, 0666);
    if (shm_fd == -1)
    {
        perror(KRED"Error creating the Shared Memory Object");
        return EXIT_FAILURE;
    } 
    printf(KGRN"%s : %i - Created the Shared Memory Object \n", app_name, pid);
    ptr_buff_glob_mess = (Global_Message *)mmap(NULL, (message_count * sizeof(Global_Message)), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_mess == MAP_FAILED){
        perror(KRED"Error during mapping process");
        return EXIT_FAILURE;
    }
    printf(KGRN"%s : %i - Shared Memory Object Mapped \n", app_name, pid);
    printf(KNRM"************************************************************ \n");
    return EXIT_SUCCESS;
}

/**
 * Read Message from buffer
*/
short int ReadMessage()
{
    //Start if the buffer has message
    sem_wait_timed(sem_consumer, &blocked_timer);
    //Validar si hay mensaje de finalizacion
    if(ptr_buff_glob_var->finalize == 1)
    {
        return -1;
    }
    printf(KBLU"%s : %i - Message Detected \n", app_name, pid);
    //Block others consumers
    sem_wait_timed(sem_last_read, &blocked_timer);
    printf(KNRM"************************************************************ \n");
    int last_read_position = ptr_buff_glob_var->last_read_position;
    //Validate if the last position was the end of the buffer, if true reset to start position
    if (last_read_position == (max_message - 1))
    {
        last_read_position = -1;
    }
    int positon_to_read = last_read_position + 1;
    //Process the message readed
    short int magic_number = ptr_buff_glob_mess[positon_to_read].magic_number;
    pid_t message_pit = ptr_buff_glob_mess[positon_to_read].pid;
    time_t message_time = ptr_buff_glob_mess[positon_to_read].date_time;
    int active_producers =  ptr_buff_glob_var->active_productors;
    int active_consumers   = ptr_buff_glob_var->active_consumers;
    total_message_readed++;
    printf(KGRN"%s : %i - Read Buffer Position: %i \n", app_name, pid, positon_to_read);
    printf(KCYN"\t Active Consumers: %i \n", active_consumers);
    printf("\t Active Producers: %i \n", active_producers);
    printf("\t DateTime : ");
    PrintDateTime(message_time);
    printf("\t Process PID: %i \n", message_pit);
    printf("\t Magic Number: %i \n", magic_number);
    printf(KNRM"************************************************************ \n");
    //Set the new last read position index
    ptr_buff_glob_var->last_read_position = positon_to_read;
    //Release for other consumer process
    sem_post(sem_last_read);
    //Producers process can write one more message
    sem_post(sem_producer);
    return magic_number;
}

void AutomatedConsumerProcess()
{
    short int magicNumber = 0;
    while(flag)
    {
        unsigned int sleep = poissonRandom(mean_seconds);
        printf(KCYN"%s : %i - Waiting %u s \n", app_name, pid, sleep/1000000);
        sleep_timer += ((double) sleep)/1000000;; //process total sleep time
        //Seelp the Process
        usleep(sleep);
        printf(KCYN"%s : %i - Process Wake Up\n", app_name, pid);
        if(ptr_buff_glob_var->finalize == 1)
        {
            //Finalize process by Finalizaer Global Var
            printf(KRED"%s : %i - Start Finalize Process | Reason: Global Var Finalize Process \n", app_name, pid);
            break;
        }
        magicNumber = ReadMessage();
        if(magicNumber == pid % 6)
        {
            //Finalize process by Special Message (magic number)
            printf(KRED"%s : %i - Start Finalize Process | Reason: Magic Number \n", app_name, pid);
            exit_by_key = 1;
            break;
        }
        else if (magicNumber == -1) //Read Finalizer Global Var during read message process
        {
            printf(KRED"%s : %i - Start Finalize Process | Reason: Global Var Finalize Process \n", app_name, pid);
            break;
        }
    }
    flag = false;
}

void ManualConsumerProcess()
{
    short int magicNumber = 0;
    while(flag)
    {  
        if(ptr_buff_glob_var->finalize == 1)
        {
            //Finalize process by Finalizaer Global Var
            printf(KRED"%s : %i - Start Finalize Process | Reason: Global Var Finalize Process \n", app_name, pid);
            break;
        }
        printf(KRED"%s : %i - Please Press Enter... \n", app_name, pid);
        if(getchar() == 10)
        {  
            magicNumber = ReadMessage();
        }
        if(magicNumber == pid % 6)
        {
            //Finalize process by Special Message (magic number)
            printf(KRED"%s : %i - Start Finalize Process | Reason: Magic Number \n", app_name, pid);
            exit_by_key = 1;
            break;
        }
        else if(magicNumber == -1) //Read Finalizer Global Var during read message process
        {
            printf(KRED"%s : %i - Start Finalize Process | Reason: Global Var Finalize Process \n", app_name, pid);
            break;
        }
    }
    flag = false;
}


/**
 * Exit Sequence
 * **/
void ExitProcess(double elapsed_time,double process_time, double sys_time, double usr_time )
{
    sem_wait_timed(sem_disable_process, &blocked_timer);
    if (exit_by_key == 1)
    {
        ptr_buff_glob_var->consumers_delete_by_key++;
        printf(KGRN"%s : %i - Increase Consumers Deleted By Key Count \n", app_name, pid);
    }

    ptr_buff_glob_var->total_block_time += blocked_timer; // Total time process was blocked
    ptr_buff_glob_var->total_wait_time += sleep_timer; //Total time process was sleeping

    ptr_buff_glob_var->total_cpu_time += elapsed_time;
    ptr_buff_glob_var->total_kernel_time  += sys_time;
    ptr_buff_glob_var->total_user_time += usr_time;
    printf(KGRN"%s : %i - All Global Statistics Sync \n", app_name, pid);
    //If active_productors = 0 and activer_consumers = 1, I am the last one
    if(ptr_buff_glob_var->active_productors == 0 && ptr_buff_glob_var->active_consumers == 1)
    {
        printf(KNRM"%s : %i - Last process closed \n", app_name, pid);
        sem_post(sem_finalize);
    }
    ptr_buff_glob_var->active_consumers--;
    sem_post(sem_disable_process);
    sem_post(sem_consumer);
}


int main(int argc, char *argv[]){ 
    //Time Vars
    srand(time(0));
    clock_t beginProcess,endProcess;
    struct tms start_tms, end_tms;
    struct timeval  tv1, tv2;
    
    // Start timers
    beginProcess = clock();  
    times(&start_tms);
    gettimeofday(&tv1, NULL);
    // End Time Vars

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
    printf(KGRN"%s : %i - Starting Consumer Process... \n", app_name, pid);
    int value, option_index = 0;	
    char *mode = "";
    /* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "b:s:m:h", long_options, &option_index)) != -1) {
		switch (value) {
			case 'b':
				buffer_message_name = strdup(optarg);
				break;
			case 's':
                sscanf(optarg, "%lf", &mean_seconds);
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
    if (buffer_message_name == NULL || strcmp("", buffer_message_name) == 0 || mean_seconds <= 0 || strcmp("", mode) == 0 || ( strcmp("A", mode) != 0 && strcmp("M", mode) != 0) )
    {
        printf(KRED"%s : %i - Please use -h to see right parameters format \n", app_name, pid);
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
        printf(KRED"%s : %i - Please Run Initializer Process Fisrt \n", app_name, pid);
        return EXIT_FAILURE;
    }
    //Open Buffer
    if(SyncBuffer()  == 1)
    {
        printf(KRED"%s : %i - Please Run Initializer Process Fisrt \n", app_name, pid);
        return EXIT_FAILURE;
    }
    max_message = ptr_buff_glob_var->buffer_message_size;
    //Review if the finalize global var is raised
    if(ptr_buff_glob_var->finalize == 1)
    {
        printf(KNRM"************************************************************ \n");
        printf(KRED"%s : %i - Finalizer Process is running... \n", app_name, pid);
        printf("%s : %i - Please wait until finish and run Initializer Process First, after that run this process again. \n", app_name, pid);
        printf(KNRM"************************************************************ \n");
        return EXIT_FAILURE;
    }
    //Select mode
    //Increment active consumers
    ptr_buff_glob_var->active_consumers++;
    ptr_buff_glob_var->historical_consumers++;

    if(isAutoMode)
    {
        AutomatedConsumerProcess();
    }
    else
    {
        ManualConsumerProcess();
    }
 
    // End timers
    times(&end_tms);
    endProcess = clock();
    gettimeofday(&tv2, NULL);

    // Calculating times
    double elapsed_time = (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
    double process_time = (double)(endProcess - beginProcess) / CLOCKS_PER_SEC;
    double sys_time = (double)(end_tms.tms_stime - start_tms.tms_stime)/100;
    double usr_time = (double)(end_tms.tms_utime - start_tms.tms_utime)/100;
    double suspended_time = elapsed_time - process_time;

    ExitProcess(elapsed_time,process_time, sys_time, usr_time );

    //Release resources (mem, etc)
    sem_close(sem_consumer);
    sem_close(sem_producer);
    sem_close(sem_disable_process);
    sem_close(sem_finalize);
    munmap(ptr_buff_glob_var, sizeof(Global_Var));
    munmap(ptr_buff_glob_mess, (message_count*sizeof(Global_Var)));
    printf(KNRM"************************************************************ \n");
    printf(KCYN"%s : %i - Statistics: \n", app_name, pid);
    printf(KBLU"%s : %i - Total time %f \n", app_name, pid,  elapsed_time);
    printf("%s : %i - Suspended time %f \n", app_name, pid, suspended_time);
    printf("%s : %i \t- Wait time %f \n", app_name, pid, sleep_timer);  
    printf("%s : %i \t- Blocked time %f \n", app_name, pid, blocked_timer);  
    printf("%s : %i \t- Other Time %f \n", app_name, pid, (suspended_time - sleep_timer - blocked_timer));  
    printf("%s : %i - Processing time %f \n", app_name, pid, process_time);
    printf("%s : %i \t- System time %f \n", app_name, pid, sys_time);
    printf("%s : %i \t- User time %f \n",  app_name, pid, usr_time);
    printf("%s : %i - Total Message Read: %i \n", app_name, pid, total_message_readed);
    printf(KNRM"************************************************************ \n");
    printf(KRED"%s : %i - Consumer Process Ends \n", app_name, pid);

    return 0; 
} 