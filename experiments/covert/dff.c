#define _GNU_SOURCE
#include <stdio.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include "../../calibration/cacheutils.h"
#define SUM_KEY 23
const int length = 4*256*1024;//8MB shm
#define PORT 8080 

size_t start = 0;
size_t kpause = 0;
size_t diff_time = 0;
unsigned long long iter_no = 0;

int dff_phase_3(void *addr){
	iter_no++;
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
	if(delta < 150)
		iter_no += 5000;
	if(delta >= 650 && iter_no < 500)
	{
		return 0;
	}
	else if(delta >= 370 && iter_no < 5000){
		return 0;
	}
	else if(delta >= 180 && iter_no > 5000){
		return 0;
	}
	else{
		return 1;
	}

}

int main(int argc, char const *argv[])
{
	// SET PROC TO CPU2
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	int result = sched_setaffinity(2, sizeof(mask), &mask);

	//SHARED MEMORY INIT
	int shm_id = shmget(SUM_KEY, length*sizeof(size_t), 0777 | IPC_CREAT);
	size_t* shm = shmat(shm_id, 0, 0);
	*(long long int*)shm = 0;

	 int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[1024] = {0}; 
    char *hello = "Hello from server"; 
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                       (socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 
    int time_tot = 0;
	int ones=0;
	int zeros=0;
	int key_len = 1000;
	int code[1000] = {0,1,1,1,1,1,0,0,0,1,1,1,0,1,1,0,0,0,1,1,0,0,0,0,1,1,1,0,1,0,0,1,1,1,0,0,0,0,0,0,1,1,1,1,1,0,1,1,0,1,1,0,1,1,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,0,1,0,0,1,0,1,0,0,1,0,0,1,0,0,0,1,0,1,0,0,0,1,1,1,0,1,1,0,0,1,1,1,1,1,0,0,0,0,0,1,0,0,1,1,0,1,0,1,0,0,1,1,1,0,0,0,1,0,0,1,1,1,0,0,1,0,0,1,1,0,1,1,0,0,0,1,1,1,0,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,1,0,1,1,0,1,0,1,0,1,0,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,0,1,0,0,0,1,1,0,0,1,1,0,1,0,0,0,1,0,1,1,1,0,0,0,1,1,0,1,1,0,0,1,1,0,1,0,1,0,0,0,0,0,0,1,0,0,1,1,0,1,0,1,1,0,0,1,0,0,0,1,0,0,1,1,0,0,1,0,1,0,0,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,0,0,1,0,0,1,1,0,0,1,1,0,0,1,0,1,1,0,0,1,0,0,1,1,1,1,0,1,1,0,1,0,1,0,0,1,0,1,1,0,0,1,0,1,0,0,0,0,0,0,1,0,0,0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1,1,1,0,1,0,1,1,1,0,1,1,1,1,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0,0,1,1,1,0,1,0,0,1,1,0,1,1,1,1,0,1,1,0,0,0,0,1,1,0,0,1,0,0,0,0,1,1,0,0,1,1,1,1,0,0,1,0,1,1,1,1,0,0,1,0,0,0,1,1,0,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1,1,1,0,0,1,1,0,0,1,0,1,1,0,1,0,1,0,1,1,1,1,0,1,0,1,0,0,1,0,0,0,1,1,1,1,1,0,1,1,1,1,0,0,0,0,0,1,0,1,0,1,0,1,1,0,0,1,1,1,1,1,0,0,1,1,0,0,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,0,0,0,0,1,1,0,1,0,0,1,1,1,1,1,0,1,0,0,0,0,1,1,1,1,1,1,0,0,0,1,0,0,1,1,1,0,1,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,0,0,1,0,1,0,0,0,1,1,1,1,0,1,0,0,1,1,0,1,1,1,1,1,1,0,0,0,1,1,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1,0,0,1,1,1,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,1,0,1,1,0,0,1,0,1,1,0,1,1,1,0,1,1,1,1,0,1,0,1,1,1,1,1,1,0,1,1,1,1,0,0,1,0,1,0,1,0,0,0,1,1,1,0,0,0,0,1,1,0,0,1,1,1,0,0,1,1,1,0,0,0,0,0,0,1,0,0,1,0,0,1,1,1,1,1,1,1,0,1,1,0,0,1,0,0,1,1,1,1,0,1,1,0,1,1,0,1,1,1,1,0,0,0,1,0,0,1,1,0,0,1,1,0,0,1,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,1,0,1,1,0,1,1,1,0,0,1,1,1,1,1,0,1,1,0,0,1,1,0,1,0,0,0,1,0,0,1,1,1,0,0,1,1,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,1,0,0,0,0,1,0,1,1,1,0,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,0,1,1,1,1,0,1,0,1,1,1,0,0,1,0,0,0,1,1,1,0,1,1,0,1,1,0,1,0,1,0,0,0,1,1,0,1,1,0,1,0,1,1,1,0,1,1,0,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,1,1,0,1,1,1,1,0,0,0,1,1,0,1,0};
	int new_code[1000] = {0};
    for (int z = 0; z < key_len; ++z)
    {
		flush(&shm[0]);
		valread = 			read( new_socket , buffer, 1024);
		int res = 			dff_phase_3(&shm[0]);
		if(!res)			new_code[z] = 1;
		else if(res==1)	 	new_code[z] = 0;
		else				z--;
		send(new_socket , hello , strlen(hello) , 0 ); 
	}
	int acc = 0;
	for (int i = 0; i < key_len; ++i)
	{
		if(code[i] == new_code[i])
			acc++;

	}
	int tp=0, fp=0, tn=0, fn=0; // tp is 1 classified as 1
	for (int i = 0; i < key_len; ++i)
	{
		if(code[i] == new_code[i]){
			if(new_code[i] == 1)
				tp++;
			else
				tn++;
		}
		else{
			if(new_code[i] == 1)
				fp++;
			else
				fn++;
		}

	}
	printf("TP: %d\n", tp);
	printf("TN: %d\n", tn);
	printf("FP: %d\n", fp);
	printf("FN: %d\n", fn);
	printf("Accuracy : %f\n", (float)acc/(float)(key_len) );
	shmctl(shm_id, IPC_RMID, NULL);
	shmdt(shm);
	return 0;
}


