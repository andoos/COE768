/* A simple echo server using TCP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <strings.h>


#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		100	/* buffer length */

char *read_file_name(int sd);
int read_file(int);
void reaper(int);

int main(int argc, char **argv)
{
	int 	sd, new_sd, client_len, port;
	struct	sockaddr_in server, client;

	switch(argc){
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void) signal(SIGCHLD, reaper);

	while(1) {
	  client_len = sizeof(client);
	  new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
	  if(new_sd < 0){
	    fprintf(stderr, "Can't accept client \n");
	    exit(1);
	  }
	  switch (fork()){
	  case 0:		/* child */
		(void) close(sd);
		exit(read_file(new_sd));
	  default:		/* parent */
		(void) close(new_sd);
		break;
	  case -1:
		fprintf(stderr, "fork: error\n");
	  }
	}
}

// 

int read_file(int sd) {
	char	buf[BUFLEN], *file_name;
	int 	n;
	FILE * fp;
	FILE * fp2;
	read(sd, buf, BUFLEN);

	file_name = buf;

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		printf("The file was not found.");
	} else {
		// Write the file back to the client side
		while (fgets(buf, BUFLEN, fp) != NULL) {
			// buf is the line by line of the file
			write(sd, buf, BUFLEN);
		}
	}

	fclose(fp);
	fclose(fp2);
	close(sd);

	return 0;
}

/*	read the file name from the client program	*/
// char *read_file_name(int sd) {
// 	char	*bp, buf[BUFLEN];
// 	int 	n;
// 	char *file_name = malloc(sizeof (char) * BUFLEN);

// 	n = read(sd, buf, BUFLEN);

// 	file_name = buf;
// 	return file_name;
// }

/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
