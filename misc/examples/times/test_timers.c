
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

#include <sys/times.h> // User Time System Time
#include <sys/time.h>  // Elapsed Time

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
  //Time Vars      -------------------------
  clock_t beginProcess,endProcess;
  struct tms start_tms, end_tms;
  struct timeval  tv1, tv2;

  // Start timers
  beginProcess = clock();  
  times(&start_tms);
  gettimeofday(&tv1, NULL);
  // End Time Vars -------------------------


  // Stuff ------------------------------------------
  int list [100];
  int c,d;
  for (int i =0;i<10000; ++i){
    for (int j = 0; j<100; ++j){

      c = ((((i*j+j+i) % 1713)*j*+10000) % 5317)%3;
      c = (c*c)/2;
      d = ((((i*c+c+5000) % 1713)*c*+10000) % 5317)%100;
      list[d] = c;     
      printf("%d %d",c,d);
      //printf("AB");
    }
    c += d;
    
    clear();
  }
  usleep(1500000);
  // End Stuff ------------------------------------------


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

  printf("************************************************************ \n");
  printf("Testing times Ends \n");

 

  printf("Total time %f \n\n",  elapsed_time);
  printf("- Suspended time %f \n\n",  suspended_time);  
  printf("- Processing time %f \n\n",  process_time);
  printf("  - System time %f \n\n",  sys_time);
  printf("  - User time %f \n\n", usr_time);


  return 0;
}