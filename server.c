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

#include <my_global.h>
#include <mysql.h>

#define MAX_DATA 1024

struct cli_sock
{
	int src;
	int dest;
};


void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);        
}

void show_online_users(MYSQL *con, char *myres) {
	
	strcpy(myres, "Users online: ");
	if (mysql_query(con, "SELECT username FROM users WHERE status=1")) 
	{
	  finish_with_error(con);
	}

	MYSQL_RES *result = mysql_store_result(con);

	if (result == NULL) 
	{
	  finish_with_error(con);
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result))) 
	{ 		
	      strcat(myres, row[0]);
	      strcat(myres, " ");
	}

	mysql_free_result(result);
}

int get_sock(MYSQL *con, char *username, int i) { //i=0 for src_socket, i=1 for dest_socket

	char query[150]="", temp[150]="";
	int no_of_rows;

	if(i==0) {
		sprintf(query, "SELECT src_socket FROM users WHERE username='%s'", username);
		//printf("\nFirst Case\n");
	} else if(i==1) {
		sprintf(query, "SELECT dest_socket FROM users WHERE username='%s'", username);
		//printf("\nSecond case\n");
	}

	
	if (mysql_query(con, query)) 
	{
	  finish_with_error(con);
	}

	MYSQL_RES *result = mysql_store_result(con);

	if (result == NULL) 
	{
	 	finish_with_error(con);
	}

	no_of_rows = mysql_num_rows(result);

	if(no_of_rows>0) {
		MYSQL_ROW row;

		row = mysql_fetch_row(result);

		//printf("\nyoyoy100\n");

		sprintf(temp, "%s", row[0]);

		//printf("\nyoyoy200\n");
		return atoi(temp);
	} 
	return -1;
}

void *my_recv(void *user_n)
{
    // Store the value argument passed to this thread
    char *username = (char *)user_n;
    char mesg[150] = "", msg[MAX_DATA]="", temp[100]="", dest_user[MAX_DATA], query[150];
    char my_username[MAX_DATA]; //There will be a major issue without this
    int data_len;
    struct cli_sock cli;
    MYSQL *con = mysql_init(NULL);

	if (con == NULL) {
	  fprintf(stderr, "mysql_init() failed\n");
	  exit(1);
	}  

	if (mysql_real_connect(con, "localhost", "root", "password", "socket_prog", 0, NULL, 0) == NULL) {
	  finish_with_error(con);
	} 

	cli.src = get_sock(con, username, 0); //src_socket
	cli.dest = get_sock(con, username, 1); //dest_socket


	//printf("\nsrc and dest: %d %d\n", cli.src, cli.dest);

	cli.src = get_sock(con, username, 0); //src_socket
	strcpy(my_username, username);

	while(1){
		strcpy(mesg, "Received from client: ");
		strcpy(msg, "");

		//printf("\nDATA_LEN_1 %d\n", cli.src);
		
		int data_len = recv(cli.src, msg, MAX_DATA, 0);

		if(data_len == -1) {
			//printf("\nDATA_LEN_2 %d\n", cli.src);
			break;
		}

		if(data_len>0) {
			msg[data_len-1] = '\0';
			strcpy(mesg, strcat(mesg,msg));
			write(1, mesg, strlen(mesg));
			printf("\n");
			
			if(strcmp(msg, ":disp") == 0) {
				show_online_users(con, temp);
				strcat(temp, "\n");
				send(cli.src, temp, strlen(temp), 0);
			} else if(strcmp(msg, ":start") == 0) {
				strcpy(mesg, "Enter username with whom you want to communicate");
				send(cli.src, mesg, strlen(mesg), 0);

				data_len = recv(cli.src, dest_user, MAX_DATA, 0);

				if(data_len) {
					dest_user[data_len-1] = '\0';
					write(1, dest_user, strlen(dest_user)); //printf("\nLength: %d", (int)strlen(dest_user));
					
					//printf("\nyoyoyoyoyoyo1\n");

					cli.dest = get_sock(con, dest_user, 0);

					if(cli.dest == -1) {
						strcpy(mesg, "No such user available. Please try again.");
						send(cli.src, mesg, strlen(mesg), 0);
					} else {
						sprintf(query, "UPDATE users SET dest_socket=%d WHERE username='%s'", cli.dest, my_username); //printf("Query: %s\n", query);

						if (mysql_query(con, query)) {
							finish_with_error(con);
						}

						sprintf(query, "UPDATE users SET dest_socket=%d WHERE username='%s' AND dest_socket=-1", cli.src, dest_user); //printf("Query: %s\n", query);

						if (mysql_query(con, query)) {
							finish_with_error(con);
						}

						strcpy(mesg, "Connected");
						send(cli.src, mesg, strlen(mesg), 0);
					}
				}

			} 

			else if(strcmp(msg, ":exit") == 0){
				send(cli.src, msg, strlen(msg), 0);
				close(cli.src);

				sprintf(query, "SELECT src_socket FROM users WHERE dest_socket=%d", cli.src);

				if (mysql_query(con, query)) {
				  finish_with_error(con);
				}

				MYSQL_RES *result = mysql_store_result(con);

				if (result == NULL) {
				  finish_with_error(con);
				}

				MYSQL_ROW row;

				while(row = mysql_fetch_row(result)){
					send(atoi(row[0]), msg, strlen(msg), 0);
					close(atoi(row[0]));
				}

				//make users offline
				sprintf(query, "UPDATE users SET src_socket=-1, dest_socket=-1, status=0 WHERE dest_socket=%d", cli.src); //printf("Query: %s\n", query);

				if (mysql_query(con, query)) {
					finish_with_error(con);
				}

				sprintf(query, "UPDATE users SET src_socket=-1, dest_socket=-1, status=0 WHERE src_socket=%d", cli.src); //printf("Query: %s\n", query);

				if (mysql_query(con, query)) {
					finish_with_error(con);
				}
				

				//close sockets
			} 
			else {
				cli.dest = get_sock(con, my_username, 1); //dest_socket
				strcpy(mesg, my_username);
				sprintf(mesg, "%s>>",my_username);
				strcat(mesg, msg);
				send(cli.dest, mesg, strlen(mesg), 0);
			}	
		}
	}
}

int main (int argc, char *argv[]) {
	int sock, cli, i=0;
	// struct cli_sock csock[2];
	struct sockaddr_in server, client;
	int sockaddr_len;
	char mesg[500], username[MAX_DATA], password[MAX_DATA];
	char outgoing[MAX_DATA], temp[100]="";
	int data_len;
	int no_of_rows;
	char query[500];

	pthread_t tid;

	/*Connect to database*/
	MYSQL *con = mysql_init(NULL);

	if (con == NULL) {
	  fprintf(stderr, "mysql_init() failed\n");
	  exit(1);
	}  

	if (mysql_real_connect(con, "localhost", "root", "password", "socket_prog", 0, NULL, 0) == NULL) {
	  finish_with_error(con);
	} 

	sprintf(query, "UPDATE users SET status=0, src_socket=-1,dest_socket=-1 where 1");

	if (mysql_query(con, query)) {
		finish_with_error(con);
	}

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket: ");
		exit(-1);
	} 
	
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[1]));
	server.sin_addr.s_addr = INADDR_ANY;
	bzero(&server.sin_zero, 8);
	
	sockaddr_len = sizeof(struct sockaddr_in);

	if((bind(sock, (struct sockaddr *)&server, sockaddr_len)) == -1) {
		perror("bind: ");
		exit(-1);
	}
	
	if((listen(sock, 2)) == -1) {
		perror("listen: ");
		exit(-1);
	}
	


	while(1) {
	//for(i=0;i<3;i++) {
		if((cli = accept(sock, (struct sockaddr *)&client, &sockaddr_len)) == -1) {
			perror("accept: ");
			exit(-1);
		}
		printf("New client connected from port no %d and IP %s\n",ntohs(client.sin_port), inet_ntoa(client.sin_addr));
		

		strcpy(mesg, "Enter username: ");
		send(cli, mesg, strlen(mesg), 0);
		
		data_len = recv(cli, username, MAX_DATA, 0);

		if(data_len) {
			username[data_len-1] = '\0';
			write(1, username, strlen(username));
		}

		strcpy(mesg, "Enter password: ");
		send(cli, mesg, strlen(mesg), 0);

		data_len = recv(cli, password, MAX_DATA, 0);

		if(data_len) {
			password[data_len-1] = '\0';
			write(1, password, strlen(password));
		}
		
		sprintf(query, "SELECT * FROM users where username='%s' AND password='%s'", username, password);

		/*mysql_query returns 0 for success, non-zero if error occured*/
		if (mysql_query(con, query)) {
			finish_with_error(con);
		}

		MYSQL_RES *result = mysql_store_result(con);

		if (result == NULL) 
		{
			finish_with_error(con);
		}

		no_of_rows = mysql_num_rows(result);

		if(!no_of_rows) {
			strcpy(mesg, "Incorrect username or password! Please try again.");
			send(cli, mesg, strlen(mesg), 0);
			close(cli);
			i--;
		} else {
			printf("Correct\n");
			
			sprintf(query, "UPDATE users SET status=1, src_socket=%d where username='%s'", cli, username);

			if (mysql_query(con, query)) {
				finish_with_error(con);
			}

			sprintf(mesg, "Logged In.\n");

			show_online_users(con, temp);
			strcat(mesg, temp);

			strcat(mesg, "\nAt any point of time enter \":disp\" to see the online users.\nTo start a communication enter: \":start\"\nTo logout enter: \":exit\"\n");
			send(cli, mesg, strlen(mesg), 0);

			pthread_create(&tid, NULL, my_recv, (void *)username);
		}
		
	}	
	
	while(1);
}	
