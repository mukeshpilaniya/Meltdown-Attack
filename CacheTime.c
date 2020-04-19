#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <emmintrin.h>
#include <x86intrin.h>

#define PAGESIZE 4096

uint8_t array[12*PAGESIZE];

int main(int argc, const char **argv) {
  int tmp=0;
  register uint64_t time1, time2;
  volatile uint8_t *addr;
  int i;
  
  // Initialize the array by 1 to load in main memory
  for(i=0; i<12; i++) array[i*PAGESIZE]=1;

  // FLUSH the complete array from the CPU cache to make sure that array index is  not cached
  for(i=0; i<12; i++) _mm_clflush(&array[i*4096]);

  // Access some of the array items so that that index is cached
  array[4*PAGESIZE] = 500;
  array[8*PAGESIZE] = 600;

  //measuring timing diifference to observed that array index 4 and 8 CPU cycle is less than other index 
  for(i=0; i<12; i++) {
    addr = &array[i*PAGESIZE];
    time1 = __rdtscp(&tmp);                
    tmp = *addr;
    time2 = __rdtscp(&tmp) - time1;       
    printf("Access time for array[%d*PAGESIZE]: %d CPU cycles\n",i, (int)time2);
  }
  return 0;

}
