// #define _GNU_SOURCE
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

#define ITERATIONS (10000)

int no_of_hogs = 1;
int child_hog_pid[12];
int CMI[3] = {0, 0, 0};

int set_affinity(long thread_id, int cpu_id){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id,&mask);
    pthread_setaffinity_np(thread_id,sizeof(cpu_set_t),&mask);
}

int hogcpu ()
{
    if (!CMI[0]) return 0;
    while(1)
        sqrt(rand());
    return 0;
}

int hogvm ()
{
    if (!CMI[1]) return 0;
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

int hogio ()
{
    if (!CMI[2]) return 0;
    while(1)
        sync();
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
            child_hog_pid[i] = pid;
        pid = fork();
        if(pid < 0)
            printf("fork failed!\n");
        else if(pid == 0){
            hogvm();
            exit(0);
        }
        else 
            child_hog_pid[no_of_hogs+i] = pid;
        pid = fork();
        if(pid < 0)
            printf("fork failed!\n");
        else if(pid == 0){
            hogio();
            exit(0);
        }
        else
            child_hog_pid[2*no_of_hogs+i] = pid;
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

int main(int argc, char* argv[])
{
    if (argc < 4) {
        printf("Usage: ./noise <enable-compute> <enable-mem> <enable-IO>\n");
        exit(1);
    }
    else {
        CMI[0] = atoi(argv[1]);
        CMI[1] = atoi(argv[2]);
        CMI[2] = atoi(argv[3]);
    }

    run_hogs();

    while(1);

    for(int i = 0; i < 3*no_of_hogs; i++){
        if (CMI[i/no_of_hogs]) {
            kill(child_hog_pid[i],SIGTERM);
        }
    }
    wait(NULL);
    return 0;
}
