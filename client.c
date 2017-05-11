#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h> /*for bzero() */
#include <errno.h> /*for perror() */
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#define MAX_DATA 1024


void *my_send(void *sock_des)
{
 	int sock = (int *)sock_des, data_len;
    char outgoing[MAX_DATA],incoming[MAX_DATA],dest_user[MAX_DATA];

    while(1) {
    	fgets(outgoing, MAX_DATA, stdin);

    	//Handle :start
    	if(strcmp(outgoing, ":start") == 0) {
    		data_len = recv(sock, incoming, MAX_DATA, 0);
	
			if(data_len) {	
				incoming[data_len] = '\0';
				write(1, incoming, strlen(incoming));

				fgets(dest_user, MAX_DATA, stdin);
				send(sock, dest_user, strlen(dest_user), 0);
			}
    	} 

    	send(sock, outgoing, strlen(outgoing), 0);
	}
}

int main (int argc, char *argv[]) {
	
	int sock;
	struct sockaddr_in remote_server;
	int sockaddr_len;
	char incoming[MAX_DATA], username[MAX_DATA], password[MAX_DATA];
	
	int data_len;
	

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket: ");
		exit(-1);
	} 
	

	remote_server.sin_family = AF_INET;
	remote_server.sin_port = htons(atoi(argv[2]));
	remote_server.sin_addr.s_addr = inet_addr(argv[1]);
	bzero(&remote_server.sin_zero, 8);
	
	sockaddr_len = sizeof(struct sockaddr_in);

	if((connect(sock, (struct sockaddr *)&remote_server, sockaddr_len)) == -1) {
		perror("connect: ");
		exit(-1);
	}

	data_len = recv(sock, incoming, MAX_DATA, 0);

	if(data_len) {	
		incoming[data_len] = '\0';
		write(1, incoming, strlen(incoming));

		fgets(username, MAX_DATA, stdin);
		send(sock, username, strlen(username), 0);
	}

	data_len = recv(sock, incoming, MAX_DATA, 0);

	if(data_len) {	
		incoming[data_len] = '\0';
		write(1, incoming, strlen(incoming));

		fgets(password, MAX_DATA, stdin);
		send(sock, password, strlen(password), 0);
	}
	
	pthread_t tid;
	pthread_create(&tid, NULL, my_send, (void *)sock);

	while(1) {
		
	    data_len = recv(sock, incoming, MAX_DATA, 0);

		if(data_len) {	
			incoming[data_len] = '\0';

			if(strcmp(incoming, ":exit") == 0) {
				close(sock);
				exit(1);
			}
    	
			write(1, incoming, strlen(incoming));
			printf("\n");
			//fflush(stdout);
		}
	}

	close(sock);
}	