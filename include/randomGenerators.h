#ifndef __RANDOMGENERATORS_H
#define __RANDOMGENERATORS_H

double expGenerator(double expectedValue);

unsigned int poissonGenerator(double expectedValue);

void rand_tester(void);

unsigned int poissonRandom(double meanSeconds);

unsigned int expRandom(double meanSeconds);

short int magicRandom(void);
#endif