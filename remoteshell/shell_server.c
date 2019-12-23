// Name: Rediet Negash
// ID: 111820799
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"

#define SERV_PORT 6000
#define ERRQUIT(msg)  (err_quit((msg), _FILE_, _func_, _LINE_))
#define CHKBOOLQUIT(exp, msg)((exp) || ERRQUIT(msg))

int main(int argc, char **argv){
	int sfd;
	struct sockaddr_in saddr;
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0){
		perror("Socket failed");
		exit(EXIT_FAILURE);	
	}
	
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(SERV_PORT);
	
	if(bind(sfd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0){
		perror("ERROR on binding");
		exit(1);	
	};
	listen(sfd, 1024);
	printf("server is listening...\n");
	
	while(1){
		struct sockaddr_in caddr;
		int cfd, clen = sizeof(caddr);
		//TODO: accept a connection
		cfd= accept(sfd, (struct sockaddr*) &caddr, &clen);
		
		if(cfd < 0) perror("ERROR on accept");
		printf("server %d: connection accepted \n", getpid());
		// TODO: execute shell after duping cfd for 0: stdin, 
		//  1:stdout, and 2: stderr
		if(fork() == 0){	
			dup2(cfd, 0);
			dup2(cfd, 1);
			dup2(cfd, 2);
			//gets in an infinite loop
			while(1){
				//reads a shell command from the client socket
				close(cfd);
		 	        close(sfd);				
				execve("./shell", argv , NULL);
			}
		}else{
			close(cfd);
		}
	}
	return 0;
}



