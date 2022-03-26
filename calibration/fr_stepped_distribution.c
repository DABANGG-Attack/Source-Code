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
#include <string.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <stdint.h>
#include<sched.h>
#include "./cacheutils.h"

#define ITERATIONS (10000)

long long count = 0;
int cached_var = 0;
int non_cached_var = 0;
int cached_delta_arr[ITERATIONS];
int non_cached_delta_arr[ITERATIONS];
int core_id = 0;

int thread_running = 0;
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;

int set_affinity(long thread_id, int cpu_id){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id,&mask);
    pthread_setaffinity_np(thread_id,sizeof(cpu_set_t),&mask);
}

void *cached_in_core(){
    set_affinity(pthread_self(),core_id);
    cached_var = cached_var%10;
    pthread_mutex_lock(&mutex);
    thread_running = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    for (int i = 0; i < 100; i++){
        flush(&i);
        cached_var += i;
        i=i+1+(i%2);
        if (thread_running == 0) {
            break;
        }
    }
}

void compute_intensive_loop(int max_i){
    long x, y;
    x = 5;
    y = 6;
    for (int i = 0; i < max_i; ++i){
        // Flush so the code doesn't get optimized out
        flush(&max_i);
        x *= y;
        y *=x;
        if(i%7==0){
            x = y/5.2;
            y = x*0.81;
        }
        else if(i%20==0){
            x = y%13;
            y = x%6;
        }
        else{
            x = y/(x+1);
            y = 4*x/(7+3*x);
        }
    }
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

int attacker(void* addr, int cached)
{
    flush(addr);
    pthread_t t[1];

    if(core_id==-1 && cached){
        measure_access_latency(addr);
        compute_intensive_loop(400);
    }
    else if(cached){
        pthread_create(&t[0],NULL,cached_in_core,NULL);
        pthread_mutex_lock(&mutex);
        while (thread_running == 0) pthread_cond_wait(&cond, &mutex);
    }
    else
        compute_intensive_loop(400);

    size_t delta = measure_access_latency(addr);

    if (core_id != -1 && cached) {
        thread_running = 0;
        pthread_mutex_unlock(&mutex);
        pthread_join(t[0],NULL);
    }
    if (delta >= 0 && delta <= 1000 && count < ITERATIONS)
    {
	    if(cached)
            cached_delta_arr[count] = delta;
        else
            non_cached_delta_arr[count] = delta;
        count++;
    }
    else if(count >= ITERATIONS)
        return 1;
   return 0;  
}

int main()
{
    void* addr1;
    addr1 = &cached_var;
    void* addr2;
    addr2 = &non_cached_var;
    int fd = open("fr_latency_freq.csv",O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    char str[100];

    int core_id_arr[4] = {-1, 0, sysconf(_SC_NPROCESSORS_CONF)/2, sysconf(_SC_NPROCESSORS_CONF)/4};
    char test_type[4][100] = {"Same program","Same logical core","Same physical core","Different physical cores"};

    for(int i = 0; i < 4; i++){
        sleep(5);
        printf("Test type: %s\n", test_type[i]);
        set_affinity(pthread_self(),0);
        core_id = core_id_arr[i];
        count = 0;
        printf("Part 1");
        while(1) {
            if(attacker(addr1, 1))  break;
            if (count%(ITERATIONS/10) == 0) {
                printf(".");
            }
        }
        printf("Done\n");
        count = 0;
        sleep(5);
        printf("Part 2");
        while(1) {
            if(attacker(addr2, 0))  break;
            if (count%(ITERATIONS/10) == 0) {
                printf(".");
            }
        }
        printf("Done\n");
        while(count>0){
            sprintf(str,"%lld,%d,%d\n",ITERATIONS-count+1,
                    cached_delta_arr[ITERATIONS-count],non_cached_delta_arr[ITERATIONS-count]);
            if(write(fd,str,strlen(str))<0)
                printf("Write error!\n");
            count--;
        }
    }
    close(fd);
    return 0;
}
