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
#include<sched.h>
#include "./cacheutils.h"

size_t start = 0;
long long sumdelta=0;
long long count=0;
int var;

int set_affinity(long thread_id, int cpu_id){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id,&mask);
    pthread_setaffinity_np(thread_id,sizeof(cpu_set_t),&mask);
}

void *cached_in_c1(){
    set_affinity(pthread_self(),1);
    if(count%2==0)  var++;
    else var--;  
    for(int i=0;i<100;i++)
        i=i+(i%2);
}

void *cached_in_c2(){
    set_affinity(pthread_self(),2);
    if(count%2==0)  var++;
    else var--;  
    for(int i=0;i<100;i++)
        i=i+(i%2);
}

void *cached_in_c3(){
    set_affinity(pthread_self(),3);
    if(count%2==0)  var++;
    else var--;  
    for(int i=0;i<100;i++)
        i=i+(i%2);
}

//Compute intensive loop to be run before calibration begins

void compute_intensive_loop(unsigned long long iters){
    long x, y;
    x = 5;
    y = 6;
    unsigned long long max_i;
    if(!iters)
        max_i = 100000000;
    else
        max_i = iters;
    for (unsigned long long i = 0; i < max_i; ++i){
        // Flush so the code doesn't get optimized out
        flush(&max_i);
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

int attacker(void* addr)
{
    flush(addr);
    pthread_t t[3];
    pthread_create(&t[0],NULL,cached_in_c1, NULL);
    pthread_create(&t[1],NULL,cached_in_c2, NULL);
    pthread_create(&t[2],NULL,cached_in_c3, NULL);
    compute_intensive_loop(100);
    for (int x = 0; x < 3; c++)
        pthread_join(t[x]);

    size_t time,newtime,delta;
    asm volatile ("mfence");
    asm volatile ("lfence");
    asm volatile ("rdtsc" : "=a" (time), "=d" (delta));
    time = (delta<<32) | time;
    asm volatile ("lfence");
    asm volatile ("clflush 0(%0)\n"
    :
    : "c" (addr)
    : "rax");
    asm volatile ("mfence");
    asm volatile ("lfence");
    asm volatile ("rdtsc" : "=a" (newtime), "=d" (delta));
    newtime = (delta<<32) | newtime;
    asm volatile ("lfence");
    delta = newtime - time;
    if (delta >= 0 && delta <= 2000)
    {
	    count++;
	    if(count>10000){
	    	printf("%f\n",(float)(sumdelta)/(float)(10000));
		    return 1;	
	    }
	    sumdelta+=delta;
    }
    return 0;
}

int main()
{
    set_affinity(pthread_self(),0);
    compute_intensive_loop(0);
    void* addr;
    addr = &var;
    start = rdtsc();
    while(1)
	    if(attacker(addr))  break;
    return 0;
}
