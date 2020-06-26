#define _GNU_SOURCE     // mremap
#include <stdio.h>      // standard input output
#include <stdlib.h>     // EXIT_FAILURE
#include <string.h>     // memcpy 
#include <sys/shm.h>    // shm
#include <sys/mman.h>   // mmap
#include <fcntl.h>      // shared memory permission
#include <stdint.h>     // for int variants
#include <unistd.h> 
#include <getopt.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/times.h>

#include "../include/randomGenerators.h"
#include "../include/common.h"

bool flag = true;
char *app_name = NULL;
double middleSeconds = -1;
char *buffer_message_name = NULL;
bool isAutoMode = false;
Global_Var *ptr_buff_glob_var = NULL;
Global_Message *ptr_buff_glob_mess = NULL;
pid_t pid = 0;
int message_count = 0;

//Begin Semaphore Region
char *semBufferName = "sem_buffer";
sem_t *semBuffer = NULL;
// End Semaphore Region


//Begin Semaphore Region
sem_t *sem_finalize = NULL;
sem_t *sem_consumer = NULL;
sem_t *sem_producer = NULL;
sem_t *sem_last_wrote = NULL;
sem_t *sem_disable_process = NULL;

// -------------------------------------------------------------------------------------
// Creating Producer


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
  printf("\n");
}

// End Semaphore Region
int InitilizeSemaphores()
{
    printf("************************************************************ \n");
    printf("%s : %i - Start Semaphores Sync \n", app_name, pid);
    sem_producer = sem_open("SEM_BUFF_PRODUCER", 0);
    sem_consumer = sem_open("SEM_BUFF_CONSUMER", 0);
    sem_last_wrote = sem_open("SEM_BUF_GLOB_WRITE_INDEX", 0);
    sem_disable_process = sem_open("SEM_BUF_GLOB_DISABLE_PROCESS", 0);
    sem_finalize = sem_open("SEM_BUF_GLOB_FINALIZER", 0);
    if ( sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED || sem_last_wrote == SEM_FAILED || 
        sem_disable_process == SEM_FAILED || sem_finalize == SEM_FAILED)
    {
        perror("Error");
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
    printf("%s : %i - Sync Global Var Buffer \n", app_name, pid);
    int shm_fd = shm_open(buffer_var_name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error creating the Shared Memory Object: Global Vars Buffer");
        return EXIT_FAILURE;
    } 
    ptr_buff_glob_var = (Global_Var *)mmap(NULL, sizeof(Global_Var), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_var == MAP_FAILED){
        perror("Error during mapping process: Global Vars Buffer");
        return EXIT_FAILURE;
    }
    //Read the message buffer size (max of message)
    int message_count = ptr_buff_glob_var->buffer_message_size;
    printf("%i \n", message_count);

    //Global Message Buffer
    printf("%s : %i - Sync the Global Message Buffer \n", app_name, pid);
    shm_fd = shm_open(buffer_message_name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error creating the Shared Memory Object: Global Message Buffer");
        return EXIT_FAILURE;
    } 
    ptr_buff_glob_mess = (Global_Message *)mmap(NULL, (message_count * sizeof(Global_Message)), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr_buff_glob_mess == MAP_FAILED){
        perror("Error during mapping process: Global Message Buffer");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// Creating Producer
// -------------------------------------------------------------------------------------

/**
 * Exit Sequence
 * **/
void ExitProcess()
{
  sem_wait(sem_disable_process);
  printf("%s : %i - Closing Process \n", app_name, pid);
  //If active_productors = 1 and activer_consumers = 0, I am the last one
  if(ptr_buff_glob_var->active_productors == 1 && ptr_buff_glob_var->active_consumers == 0)
  {
      printf("%s : %i - Last process closed \n", app_name, pid);
      sem_post(sem_finalize);
  }
  ptr_buff_glob_var->active_productors--;
  sem_post(sem_disable_process);
  sem_post(sem_producer);
}

/**
 * Write Message to buffer
*/
short int WriteMessage(int max_messages)
{
    printf("Call WriteMessage Function\n");
    //Start if the buffer has message
    sem_wait(sem_producer);
    //Block others consumers
    sem_wait(sem_last_wrote);
    printf("************************************************************ \n");
    printf("New Message to Write: \n");

    // READING/WRITING GLOBAL BUFFER

    int historical_buffer_messages = ptr_buff_glob_var->historical_buffer_messages;
    ptr_buff_glob_var->historical_buffer_messages = historical_buffer_messages +1;

    int active_productors =  ptr_buff_glob_var->active_productors;
    int active_consumers   = ptr_buff_glob_var->active_consumers;
    
    //Validate if the last position was the end of the buffer, if true reset to start position
    int last_write_position = ptr_buff_glob_var->last_write_position;    
    if (last_write_position == (max_messages - 1)){
        last_write_position = -1;
    }
    int positon_to_write = last_write_position + 1;
    //Set the new last read position index
    ptr_buff_glob_var->last_write_position = positon_to_write;
    //Release for other producer process    
    sem_post(sem_last_wrote);

    // WRITING TO MESSAGE BUFFER

    printf("Position to write %d\n",positon_to_write);
    //Process the message readed
    short int magic_number = magicRandom();
    ptr_buff_glob_mess[positon_to_write].magic_number = magic_number;
    ptr_buff_glob_mess[positon_to_write].pid = pid;
    time_t now = time(NULL);
    ptr_buff_glob_mess[positon_to_write].date_time = now;
    struct tm tm = *localtime(&now);
    printf("Time: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    printf("\t Magic Number: %i \n",magic_number);
    printf("Active Consumers %d \n", active_consumers);
    printf("Active Productors %d \n", active_productors);
    
    //Producers process can write one more message
    sem_post(sem_consumer);
    return magic_number;
}

int addProducerCounter(Global_Var *ptr_buff_glob_var){
  int active_productors = ptr_buff_glob_var->active_productors;
  ptr_buff_glob_var->active_productors = active_productors + 1;

  int historical_productor = ptr_buff_glob_var->historical_productor;
  ptr_buff_glob_var->historical_productor = historical_productor + 1;

  return 0;
}

int processloop(Global_Var *ptr_buff_glob_var , Global_Message *ptr_buff_glob_mess, double wait_mean){
  srand(time(0));
  addProducerCounter(ptr_buff_glob_var);
  unsigned int wait_time = expRandom(wait_mean);
  int max_messages = ptr_buff_glob_var->buffer_message_size;
  //short int finalizeFlag = ptr_buff_glob_var->finalize;
  while(flag){
    if(ptr_buff_glob_var->finalize == 1){
      flag = false;
      //Finalize process by Finalizaer Global Var
      printf("%s : %i - Start Finalize Process | Reason: Global Var Finalize Process \n", app_name, pid);
      break;
    }else{
      wait_time = expRandom(wait_mean);
      printf("Waiting %u \n",wait_time );
      usleep(wait_time);
      WriteMessage(max_messages);
    }
  }
  return 0;
}

int main(int argc, char *argv[]){ 
  clock_t beginProcess = clock();
  time_t beginTime = time(NULL);
  struct tms start_tms;
  times(&start_tms);

  static struct option long_options[] = {
    {"buffer_name", required_argument, 0, 'b'},
    {"seconds", required_argument, 0, 's'},
    {"help", no_argument, 0, 'h'},    
    {NULL, 0, 0, 0}
  };
  app_name = argv[0];
  app_name = app_name+2; //Delete ./ From the app Name
  pid = getpid();
  int value, option_index = 0;  
  /* Try to process all command line arguments */
  while ((value = getopt_long(argc, argv, "b:s:h", long_options, &option_index)) != -1) {
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
      default:
        break;
    }
  }
  if (buffer_message_name == NULL || strcmp("", buffer_message_name) == 0 || middleSeconds <= 0 )
  {
    printf("%s : %i - Please use -h to see right parameters format \n", app_name, pid);
    return EXIT_FAILURE;
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

  processloop(ptr_buff_glob_var , ptr_buff_glob_mess, middleSeconds);


  //Exit rutine (save statistics, etc)
  ExitProcess();
  struct tms end_tms;
  times(&end_tms);
  clock_t endProcess = clock();
  time_t endTime = time(NULL);

  time_t time_Time = endTime - beginTime;
  clock_t time_process = (endProcess - beginProcess) / CLOCKS_PER_SEC;
  clock_t cpu_time = end_tms.tms_cutime - start_tms.tms_cutime;
  clock_t utime = end_tms.tms_utime - start_tms.tms_utime;

  //Release resources (mem, etc)
  sem_close(sem_consumer);
  sem_close(sem_producer);
  sem_close(sem_disable_process);
  sem_close(sem_finalize);
  munmap(ptr_buff_glob_var, sizeof(Global_Var));
  munmap(ptr_buff_glob_mess, (message_count*sizeof(Global_Var)));
  printf("************************************************************ \n");
  printf("%s : %i - Producer Process Ends \n", app_name, pid);

  printf("total time %jd\n\n",  time_Time);
  printf("clock time %jd\n\n",  (intmax_t)time_process);
  printf("cpu time %jd\n\n",  (intmax_t)cpu_time);
  printf("cpu Utime %jd\n\n", (intmax_t)utime);

  return 0; 
} 