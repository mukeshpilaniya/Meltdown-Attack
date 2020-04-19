#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <emmintrin.h>
#include <x86intrin.h>

#define PAGESIZE 4096
//Cache hit CPU cycle assumed
#define CACHE_HIT_THRESHOLD (80)
#define DELTA 1024

uint8_t array[256*PAGESIZE];
int temp;
char secretValue = 105;

void flushSideChannel()
{
  int i;

  // Bring the array index into RAM
  for (i = 0; i < 256; i++) array[i*PAGESIZE+DELTA] = 1;

  //flush the values of the array from cache to make sure that array index is not cahed
  for (i = 0; i < 256; i++) _mm_clflush(&array[i*PAGESIZE+DELTA]);
}

void reloadSideChannel() 
{
  int tmp1=0;
  register uint64_t time1, time2;
  volatile uint8_t *addr;
  int i;
  for(i = 0; i < 256; i++){
     addr = &array[i*PAGESIZE+DELTA];
     time1 = __rdtscp(&tmp1);
     tmp1 = *addr;
     time2 = __rdtscp(&tmp1) - time1;
     if (time2 <= CACHE_HIT_THRESHOLD){
         printf("array[%d*PAGESIZE + %d] is in cache.\n",i,DELTA);
         printf("The secretValue = %d.\n",i);
     }
  } 
}

void victim()
{
  temp = array[secretValue*PAGESIZE+DELTA];
}


int main(int argc, const char **argv) 
{
  flushSideChannel();
  temp = array[secretValue*PAGESIZE+DELTA];
  reloadSideChannel();
  return (0);
}
