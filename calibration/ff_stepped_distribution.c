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
#include <math.h>
#include <libgen.h>
#include <sys/ipc.h>
#include <stdint.h>
#include <sys/wait.h>
#include <sched.h>
#include "./cacheutils.h"

#define ITERATIONS (200000)

long long count = 0;
int cached_var = 0;
int non_cached_var = 0;
int cached_delta_arr[ITERATIONS];
int non_cached_delta_arr[ITERATIONS];
int core_id = 0;
int no_of_hogs = 1;
int child_hog_pid[12];

int set_affinity(long thread_id, int cpu_id){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id,&mask);
    pthread_setaffinity_np(thread_id,sizeof(cpu_set_t),&mask);
}

void *cached_in_core(){
    set_affinity(pthread_self(),core_id);
    cached_var = cached_var%10;
    for(int i=0;i<100;i++){
        flush(&i);
        cached_var += i;
        i=i+(i%2);
    }
}

int hogcpu ()
{
    while(1)
        sqrt(rand());
    return 0;
}

int hogio ()
{
    while(1)
        sync();
    return 0;
}

int hogvm ()
{
    long long bytes = 256*1024*1024;
    long long stride = 4096;
    long long i;
    char *ptr = 0;
    char c;
    int do_malloc = 1;
    while (1)
    {
        if (do_malloc)
        {
            if (!(ptr = (char *) malloc (bytes * sizeof (char))))
            {
                return 1;
            }
        }
        for (i = 0; i < bytes; i += stride)
        ptr[i] = 'Z';           /* Ensure that COW happens.  */

        for (i = 0; i < bytes; i += stride)
        {
            c = ptr[i];
            if (c != 'Z')
            {
                return 1;
            }
        }
        if (do_malloc)
        {
            free (ptr);
        }
    }
    return 0;
}

void run_hogs(){
    no_of_hogs = sysconf(_SC_NPROCESSORS_CONF)/4>0?sysconf(_SC_NPROCESSORS_CONF)/4:1;
    if(no_of_hogs>4)
        no_of_hogs = 4;
    for(int i=0; i<no_of_hogs; i++){
        int pid = fork();
        if(pid < 0)
            printf("fork failed!\n");
        else if(pid == 0){
            hogcpu();
            exit(0);
        }
        else
            child_hog_pid[3*i] = pid;
        pid = fork();
        if(pid < 0)
            printf("fork failed!\n");
        else if(pid == 0){
            hogio();
            exit(0);
        }
        else
            child_hog_pid[3*i+1] = pid;
        pid = fork();
        if(pid < 0)
            printf("fork failed!\n");
        else if(pid == 0){
            hogvm();
            exit(0);
        }
        else 
            child_hog_pid[3*i+2] = pid;
    }
    return;
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

int measure_flush_latency(void* addr){
    size_t time, newtime, delta;
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
    return delta;
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

    if(core_id==-1 && cached){
        measure_access_latency(addr);
        compute_intensive_loop(400);
    }
    else if(cached){
        pthread_t t[1];
        pthread_create(&t[0],NULL,cached_in_core,NULL);
        compute_intensive_loop(400);
        pthread_join(t[0],NULL);
    }
    else
        compute_intensive_loop(400);

    size_t delta = measure_flush_latency(addr);

    if(count%10)
        count++;
    else if (delta >= 0 && delta <= 1000 && count/10 < ITERATIONS)
    {
	if(cached)
            cached_delta_arr[count/10] = delta;
        else
            non_cached_delta_arr[count/10] = delta;
        count++;
    }
    else if(count/10 >= ITERATIONS){
        count /= 10;
        return 1;
    }
   return 0;  
}

int main()
{
    void* addr1;
    addr1 = &cached_var;
    void* addr2;
    addr2 = &non_cached_var;
    int fd = open("ff_latency_freq.csv",O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    char str[100];
    int core_id_arr[4] = {-1, 0, sysconf(_SC_NPROCESSORS_CONF)/2, sysconf(_SC_NPROCESSORS_CONF)/4};
    char test_type[4][100] = {"Same program","Same logical core","Same physical core","Different physical cores"};
    int wait_time = 0;
    printf("This program may take 2-5 minutes to complete.\n");
    run_hogs();
    printf("The program will spin off worker processes which might make the system slow for the duration of the program. They will gracefully exit with the program.\n");

    for(int i = 0; i < 4; i++){
        sleep(5);
        set_affinity(pthread_self(),0);
        core_id = core_id_arr[i];
        count = 0;
        size_t time_remaining = rdtsc();
        while(1)
            if(attacker(addr1, 1))  break;
        count = 0;
        time_remaining = rdtsc() - time_remaining;
        sleep(5);
        while(1)
            if(attacker(addr2, 0))  break;
        while(count>0){
            sprintf(str,"%lld,%d,%d\n",ITERATIONS-count+1,cached_delta_arr[ITERATIONS-count],non_cached_delta_arr[ITERATIONS-count]);
            if(write(fd,str,strlen(str))<0)
                printf("Write error!\n");
            count--;
        }
        wait_time = (int)(((float)(time_remaining/3400000000)+5)*(6-2*i)+1);
        if(!i)
            wait_time *= 1.8;
        printf("Test type: %s, Status: done, please wait for approximately %d seconds.\n",test_type[i], wait_time);
    }

    for(int i = 0; i < 3*no_of_hogs; i++){
        kill(child_hog_pid[i],SIGTERM);
    }
    wait(NULL);
    close(fd);
    return 0;
}
