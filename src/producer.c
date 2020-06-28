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
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>      // Date Time
#include <sys/times.h> // User Time System Time
#include <sys/time.h>  // Elapsed Time

#include "../include/randomGenerators.h"
#include "../include/common.h"

bool flag = true;
char *app_name = NULL;
double meanSeconds = -1;
double blocked_timer = 0.0;
double sleep_timer = 0.0;
char *buffer_message_name = NULL;
bool isAutoMode = false;
Global_Var *ptr_buff_glob_var = NULL;
Global_Message *ptr_buff_glob_mess = NULL;
pid_t pid = 0;
int num_messages = 0;
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
    printf(KNRM"************************************************************ \n");
    printf("%s : %i - Start Semaphores Sync \n", app_name, pid);
    sem_producer = sem_open("SEM_BUFF_PRODUCER", 0);
    sem_consumer = sem_open("SEM_BUFF_CONSUMER", 0);
    sem_last_wrote = sem_open("SEM_BUF_GLOB_WRITE_INDEX", 0);
    sem_disable_process = sem_open("SEM_BUF_GLOB_DISABLE_PROCESS", 0);
    sem_finalize = sem_open("SEM_BUF_GLOB_FINALIZER", 0);
    if ( sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED || sem_last_wrote == SEM_FAILED || 
        sem_disable_process == SEM_FAILED || sem_finalize == SEM_FAILED)
    {
        perror(KRED"Error initializing semaphores");
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
    perror(KRED"Error creating the Shared Memory Object: Global Vars Buffer");
    return EXIT_FAILURE;
  } 
  printf(KGRN"%s : %i - Created the Shared Memory Object \n", app_name, pid);    
  ptr_buff_glob_var = (Global_Var *)mmap(NULL, sizeof(Global_Var), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr_buff_glob_var == MAP_FAILED){
    perror(KRED"Error during mapping process: Global Vars Buffer");
    return EXIT_FAILURE;
  }
  printf(KGRN"%s : %i - Shared Memory Object Mapped \n", app_name, pid);
  
  //Read the message buffer size (max of message)
  int num_messages = ptr_buff_glob_var->buffer_message_size;


  //Global Message Buffer
  printf(KNRM"************************************************************ \n");
  printf(KNRM"%s : %i - Sync the Global Message Buffer \n", app_name, pid);
  shm_fd = shm_open(buffer_message_name, O_RDWR, 0666);
  if (shm_fd == -1)
  {
    perror(KRED"Error creating the Shared Memory Object: Global Message Buffer");
    return EXIT_FAILURE;
  } 
  printf(KGRN"%s : %i - Created the Shared Memory Object \n", app_name, pid);    
  ptr_buff_glob_mess = (Global_Message *)mmap(NULL, (num_messages * sizeof(Global_Message)), PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr_buff_glob_mess == MAP_FAILED){
    perror(KRED"Error during mapping process: Global Message Buffer");
    return EXIT_FAILURE;
  }
  printf(KGRN"%s : %i - Shared Memory Object Mapped \n", app_name, pid);
  printf(KNRM"************************************************************ \n");    
  return EXIT_SUCCESS;
}

// Creating Producer
// -------------------------------------------------------------------------------------

/**
 * Exit Sequence
 * **/
void ExitProcess(double elapsed_time,double process_time, double sys_time, double usr_time )
{
  sem_wait_timed(sem_disable_process,&blocked_timer);
  printf("%s : %i - Closing Process \n", app_name, pid);
  //If active_productors = 1 and activer_consumers = 0, I am the last one
  if(ptr_buff_glob_var->active_productors == 1 && ptr_buff_glob_var->active_consumers == 0)
  {
      printf(KNRM"%s : %i - Last process closed \n", app_name, pid);
      sem_post(sem_finalize);
  }
  ptr_buff_glob_var->active_productors--;

  ptr_buff_glob_var->total_cpu_time += elapsed_time;
  ptr_buff_glob_var->total_wait_time += sleep_timer;
  ptr_buff_glob_var->total_block_time += blocked_timer;
  ptr_buff_glob_var->total_kernel_time  += sys_time;
  ptr_buff_glob_var->total_user_time += usr_time;

  printf(KGRN"%s : %i - All Global Statistics Sync \n", app_name, pid);    
  sem_post(sem_disable_process);
  sem_post(sem_producer);
}

void PrintDateTime(time_t time)
{
    time_t t = time;
    struct tm tm = *localtime(&t);
    printf("%d-%02d-%02d %02d:%02d:%02d\n",
         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

/**
 * Write Message to buffer
*/
short int WriteMessage(int max_messages)
{
    printf("Call WriteMessage Function\n");
    //Start if the buffer has message
    sem_wait_timed(sem_producer,&blocked_timer);
    //Block others consumers
    sem_wait_timed(sem_last_wrote,&blocked_timer);
    
    printf("New Message to Write: \n");

    // READING/WRITING GLOBAL BUFFER

    ptr_buff_glob_var->historical_buffer_messages++;
    message_count++;

    int active_productors =  ptr_buff_glob_var->active_productors;
    int active_consumers  =  ptr_buff_glob_var->active_consumers;
    
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
    short int magic_number = magicRandom();
    ptr_buff_glob_mess[positon_to_write].magic_number = magic_number;
    ptr_buff_glob_mess[positon_to_write].pid = pid;
    time_t now = time(NULL);
    ptr_buff_glob_mess[positon_to_write].date_time = now;
    

    printf(KGRN"\t Write Buffer Position: %d\n",positon_to_write);
    printf(KGRN"\t DateTime: ");
    PrintDateTime(now);
    printf(KGRN"\t Magic Number: %i \n",magic_number);
    printf(KCYN"Active Consumers: %d \n", active_consumers);
    printf(KCYN"Active Producers: %d \n", active_productors);
    printf(KNRM"************************************************************ \n");
    //Producers process can write one more message
    sem_post(sem_consumer);
    return magic_number;
}

int processloop(Global_Var *ptr_buff_glob_var , Global_Message *ptr_buff_glob_mess, double wait_mean){
  srand(time(0));
  ptr_buff_glob_var->active_productors += 1;
  int productor_number = ptr_buff_glob_var->historical_productor++;
  unsigned int wait_time;
  int max_messages = ptr_buff_glob_var->buffer_message_size;
  //short int finalizeFlag = ptr_buff_glob_var->finalize;
  while(flag){
    printf(KNRM"************************************************************ \n");
    printf("- Producer #%d\n", productor_number);
    if(ptr_buff_glob_var->finalize == 1){
      flag = false;
      //Finalize process by Finalizaer Global Var
      printf(KRED"%s : %i - Start Finalize Process | Reason: Global Var Finalize Process \n", app_name, pid);
      break;
    }else{      
      wait_time = expRandom(wait_mean);
      sleep_timer += ((double) wait_time)/1000000;
      printf(KCYN"%s : %i - Waiting %u s \n", app_name, pid, wait_time/1000000);        
      usleep(wait_time);
      printf(KCYN"%s : %i - Process Wake Up\n", app_name, pid);        
      if(ptr_buff_glob_var->finalize == 1){
        flag = false;
        //Finalize process by Finalizaer Global Var
        printf(KRED"%s : %i - Start Finalize Process | Reason: Global Var Finalize Process \n", app_name, pid);
        break;
      }
      WriteMessage(max_messages);
    }
  }
  return 0;
}

int main(int argc, char *argv[]){ 
  //Time Vars      -------------------------
  clock_t beginProcess,endProcess;
  struct tms start_tms, end_tms;
  struct timeval  tv1, tv2;

  // Start timers
  beginProcess = clock();  
  times(&start_tms);
  gettimeofday(&tv1, NULL);
  // End Time Vars -------------------------

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
        sscanf(optarg, "%lf", &meanSeconds);
        break;
      case 'h':
        print_help();
        return EXIT_SUCCESS;
      default:
        break;
    }
  }
  if (buffer_message_name == NULL || strcmp("", buffer_message_name) == 0 || meanSeconds <= 0 )
  {
    printf(KRED"%s : %i - Please use -h to see right parameters format \n", app_name, pid);
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
  //Review if the finalize global var is raised
  if(ptr_buff_glob_var->finalize == 1)
  {
    printf(KNRM"************************************************************ \n");
    printf(KRED"%s : %i - Finalizer Process is running... \n", app_name, pid);
    printf("%s : %i - Please wait until finish and run Initializer Process First, after that run this process again. \n", app_name, pid);
    printf(KNRM"************************************************************ \n");
    return EXIT_FAILURE;
  }

  processloop(ptr_buff_glob_var , ptr_buff_glob_mess, meanSeconds);

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

  //Exit rutine (save statistics, etc)
  ExitProcess(elapsed_time,process_time, sys_time, usr_time );

  //Release resources (mem, etc)
  sem_close(sem_consumer);
  sem_close(sem_producer);
  sem_close(sem_disable_process);
  sem_close(sem_finalize);
  munmap(ptr_buff_glob_var, sizeof(Global_Var));
  munmap(ptr_buff_glob_mess, (num_messages*sizeof(Global_Var)));
  printf(KNRM"************************************************************ \n");
  printf(KCYN"%s : %i - Statistics: \n", app_name, pid);    
  printf(KMAG"\n%s : %i Total time %f \n",app_name, pid,  elapsed_time);
  printf("%s : %i - Suspended time %f \n", app_name, pid, suspended_time);
  printf("%s : %i   - Wait time %f \n", app_name, pid, sleep_timer);  
  printf("%s : %i   - Blocked time %f \n",  app_name, pid, blocked_timer);   
  printf("%s : %i   - Other time %f \n",app_name, pid,  suspended_time - sleep_timer - blocked_timer);    
  printf("%s : %i - Processing time %f \n",  app_name, pid, process_time);
  printf("%s : %i   - System time %f \n", app_name, pid, sys_time );
  printf("%s : %i   - User time %f \n", app_name, pid, usr_time);


  printf(KNRM"************************************************************ \n");
  printf(KRED"%s : %i - Producer Process Ends \n", app_name, pid);

  return 0; 
} 