
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

#define KNRM  "\033[0m"
#define KRED  "\033[31m"
#define KGRN  "\033[32m"
#define KYEL  "\033[33m"
#define KBLU  "\033[34m"
#define KMAG  "\033[35m"
#define KCYN  "\033[36m"
#define KWHT  "\033[37m"



#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))
#define to_red() printf("\033[1;31m")
#define to_yellow() printf("\033[1;33m")
#define reset() printf("\033[0m")




int main(int argc, char *argv[]){ 
  clock_t beginProcess = clock();
  time_t beginTime = time(NULL);
  struct tms start_tms;
  times(&start_tms);

  // Stuff
  int c;
  for (int i =0;i<10000; ++i){
    for (int j = 0; j<1000; ++j){

      c = i*j+j+i;
      printf("T");
    }
    usleep(10);
    clear();
  }

  struct tms end_tms;
  times(&end_tms);
  clock_t endProcess = clock();
  time_t endTime = time(NULL);

  time_t time_Time = endTime - beginTime;
  clock_t time_process = (endProcess - beginProcess) / CLOCKS_PER_SEC;
  clock_t cpu_time = end_tms.tms_cutime - start_tms.tms_cutime;
  clock_t utime = end_tms.tms_utime - start_tms.tms_utime;


  printf("************************************************************ \n");
  printf("Testing times Ends \n");

  printf("total time %lu\n\n",  time_Time);
  printf("clock time %jd\n\n",  (intmax_t)time_process);
  printf("cpu time %jd\n\n",  (intmax_t)cpu_time);
  printf("cpu Utime %jd\n\n", (intmax_t)utime);

  return 0;
}