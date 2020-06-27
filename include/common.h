#include <sys/types.h>
#include <semaphore.h>
#include <sys/time.h>

#define BUFFER_GLOB_SUFIX "_GLOBAL"

#define MESSAGE_SIZE 1

#define KNRM  "\033[0m"
#define KRED  "\033[31m"
#define KGRN  "\033[32m"
#define KYEL  "\033[33m"
#define KBLU  "\033[34m"
#define KMAG  "\033[35m"
#define KCYN  "\033[36m"
#define KWHT  "\033[37m"

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
    char message[MESSAGE_SIZE];
} Global_Message;

int sem_wait_timed(sem_t *sem, double *timer){
    struct timeval  tv1, tv2;
    int result;
    gettimeofday(&tv1, NULL);
    result = sem_wait(sem);
    gettimeofday(&tv2, NULL);
    double elapsed_time = (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
    *timer += elapsed_time;
    return result;
}