#include <stdint.h>
#include <openssl/aes.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include <map>
#include <vector>
#include <unistd.h>
#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <iostream>
#include <bitset>
#define MAX 16 
#define PORT 10000 
#define SA struct sockaddr 

unsigned char key[] =
{
  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
  0xff, 0x00, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99,
};

AES_KEY key_struct;

unsigned char ciphertext[128];

unsigned long long req_rec = 0;

void func(int sockfd) 
{ 
    unsigned char buff[MAX]; 
    int n; 
    // infinite loop for server 
    for (;;) { 
	req_rec++;
	if(req_rec%20000==0)
		printf("%lld requests received\n",req_rec);
        bzero(buff, MAX); 
	    bzero(ciphertext, 128);
        // read the message from client and copy it in buffer 
        read(sockfd, buff, 16);
        // Encrypt the 16B plaintext received
        AES_encrypt(buff, ciphertext, &key_struct);	
        // and send ciphertext to client 
        write(sockfd, ciphertext, 128); 
    } 
} 
  
// Driver function 
int main() 
{ 
    int sockfd, connfd;
    unsigned int len; 
    struct sockaddr_in servaddr, cli; 

    AES_set_encrypt_key(key, 128, &key_struct);
    printf("Secret key:\n");
    for (int i = 0; i < 16; i++) {
        std::cout << std::bitset<8>(key[i]);
    }
    printf("\n");
    // socket create and verification 
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n"); 
  
    func(connfd); 
    close(sockfd); 
} 

