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