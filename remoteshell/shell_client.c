// Name: Rediet Negash
// ID: 111820799

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define SERV_ADDR  "127.0.0.1"
#define SERV_PORT   6000
#define MAXLINE     1024

void repeat(int sfd){
	char buf[MAXLINE];
	int n;
	fd_set fds;
	int w;
	//TODO: using FD_ZERO, FD_SET, FD_ISSET, and select
	//	copy the contents from stdin (0) to sfd
	//	copy the contents from sfd to stdout (1)

	while(1){
		FD_ZERO(&fds);
		FD_SET(sfd, &fds);
		FD_SET(0, &fds);
	
		int selectedfd = select(sfd+1 , &fds , NULL , NULL , NULL);

		if ((selectedfd < 0) )   
		{   
		    printf("select error");   
		}  
		if(FD_ISSET(sfd, &fds)){   
			n = read(sfd, buf, MAXLINE-1);
			if(n == 0){
				printf("Disconnected from server\n");
				close(sfd);
				break;
			}
			if(n < 0){
			    perror("Error reading");			
			}
			w = write(1, buf, n);
			if (w < 0){perror ("Error writing");}
		} 
		if(FD_ISSET(0, &fds)){
		    
		    n = read(0, buf, MAXLINE-1);
		    if(n == 0){
			printf("Disconnected from server\n");
			close(sfd);
			break;
		    }		
		    if(n < 0){
			perror("Error reading");			
		    }
		    w = write(sfd, buf, n);
		    if (w < 0){perror ("Error writing");}
		}
	}
}

int main (int argc, char **argv){
	int sfd, ret;
	struct sockaddr_in saddr;
	// TODO: create a socket, connect to server
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0){
		perror("Socket failed");
		exit(EXIT_FAILURE);	
	}
	printf("Client Socket is created.\n");
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(SERV_ADDR);
	saddr.sin_port = htons(SERV_PORT);
	ret = connect (sfd, (struct sockaddr*)&saddr, sizeof(saddr));
	if(ret < 0){
		printf("Error in connection.\n");
		exit(1);
	}
	printf("Connected to Server.\n");
	repeat(sfd);
	close(sfd);
	return 0;

}



