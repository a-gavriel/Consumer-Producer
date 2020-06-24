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

#define MSGSIZE 16
#define GLOB_SIZE 64
#define BUFFER_GLOB_SUFIX "GLOBAL"

//Begin Region Global Variables 
char *app_name = NULL;
char *buffer_name = NULL;
//End Region Global Varibales

typedef struct Global_Var
{
    int buffer_message_size;
    int active_consumers;
    int historical_consumers;
    int active_productors;
    int historical_productor;
} Global_Var;

/**
 * Initialize the semaphores needed by the system
*/
int InitilizeSemaphores(int messageCount)
{
    if (sem_open("SEM_BUF_MES_ADD", O_CREAT, 0644, messageCount) == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    if (sem_open("SEM_BUF_MES_SUB", O_CREAT, 0644, 0) == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    if (sem_open("SEM_BUF_GLOB_READ_INDEX", O_CREAT, 0644, 0) == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    if (sem_open("SEM_BUF_GLOB_WRITE_INDEX", O_CREAT, 0644, 0) == SEM_FAILED)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int InitializeBuffers()
{ 
    char *buffer_name_global = malloc(strlen(buffer_name) + strlen(BUFFER_GLOB_SUFIX) + 2);
    if(buffer_name_global == NULL)
    {
        return EXIT_FAILURE;
    }
    strcpy(buffer_name_global, buffer_name);
    strcat(buffer_name_global, "_");
    strcat(buffer_name_global, BUFFER_GLOB_SUFIX);
    printf("%s \n",buffer_name_global);

    //Global Varibales Buffer
    printf("A \n");
    int shm_fd = shm_open(buffer_name_global, O_CREAT | O_RDWR, 0666);
    printf("B \n");
    if (shm_fd == -1)
    {
        return EXIT_FAILURE;
    } 
    if(ftruncate(shm_fd, sizeof(Global_Var) == -1))
    {
        return EXIT_FAILURE;
    }
    //TEST
    Global_Var *ptr = (Global_Var *)mmap(0, sizeof(Global_Var), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED){
        perror("MMAP FAILED, Error mmapping the file, Buffer wasn't created!\n");
        return EXIT_FAILURE;
    } 
    printf("TTTT");
    ptr -> active_consumers = 10;
    printf("Active Consumers %i", ptr->active_consumers);

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
		{"size", required_argument, 0, 's'},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}
	};
    app_name = argv[0];
    app_name = app_name+2; //Delete ./ From the app Name
    int pid = getpid();
    int value, option_index = 0;	
    int message_count = 0;
    /* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "b:s:h", long_options, &option_index)) != -1) {
		switch (value) {
			case 'b':
				buffer_name = strdup(optarg);
				break;
			case 's':
                sscanf(optarg, "%i", &message_count);
				break;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			default:
				break;
		}
	}
    if (buffer_name == NULL || strcmp("", buffer_name) == 0 || message_count <= 0 )
    {
        printf("%s : %i - Please use -h to see right parameters format \n", app_name, pid);
        return EXIT_FAILURE;
    }
    InitilizeSemaphores(message_count);
    InitializeBuffers();

    /* name of the shared memory object */
    char buffer_name [10]= ""; 
    printf("Enter buffer name: ");
    scanf("%s",buffer_name);
    
    /* the size (in bytes) of shared memory object */
    printf("Enter the amount of messages the buffer will hold: " );
    uint8_t totmsgs;
    scanf("%" SCNu8, &totmsgs); 

    uint32_t SIZE = GLOB_SIZE + MSGSIZE*totmsgs; 
  
    /* shared memory file descriptor */
    int shm_fd; 
  
    /* pointer to shared memory obect */
    void* ptr; 
  
    /* create the shared memory object */
    shm_fd = shm_open(buffer_name, O_CREAT | O_RDWR, 0666); 
  
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE); 
  
    /* memory map the shared memory object */
    ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0); 
    if (ptr == MAP_FAILED){
        perror("MMAP FAILED, Error mmapping the file, Buffer wasn't created!\n");
        return EXIT_FAILURE;
    }

    // Writes the totmsgs in the beginning of the buffer
    // dest , src , size
    memcpy(ptr,&totmsgs,1);

    // For testing!
    value = 11;
    memcpy(ptr+50,&value,1);
} 