#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/randomGenerators.h"

double expGenerator(double expectedValue){ //expectedValue = 1/lambda
    double u;
    u = rand() / (RAND_MAX + 1.0);
    return -log(u) * expectedValue;
}

unsigned int poissonGenerator(double expectedValue) {
  unsigned int k = 0; //counter of iteration
  double limit; 
  double p;  //pseudo random number
  limit = exp(-expectedValue);
  p = rand() / (RAND_MAX + 1.0); 
  while (p > limit) {
    k++;
    p *= rand() / (RAND_MAX + 1.0);
  }
  return k;
}

void rand_tester(void){
    srand((unsigned)time(NULL));
    double mean = 0;
    
    printf("Generating exponential distribution\n");
    for (int i=0; i<20; i++)
      printf("%f\n", expGenerator(mean));
  

    printf("Generating Poisson distribution\n");
    for (int i=0; i<100; i++)
      printf("%d\n", poissonGenerator(mean));  
}

unsigned int poissonRandom(double meanSeconds){
  unsigned int random_ = poissonGenerator((double)meanSeconds*100);
  return random_*10000;
}

unsigned int expRandom(double meanSeconds){
  double random_ = expGenerator((double)meanSeconds);
  return (unsigned int) (random_*1000000);
}

short int magicRandom(void){
  short int magic_num =  (short int)(rand() % 7);
  return magic_num;
}