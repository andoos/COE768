/* A simple echo client using TCP */
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>

#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		100	/* buffer length */

int main(int argc, char **argv)
{
	int 	n, i, bytes_to_read;
	int 	sd, port;
	struct	hostent		*hp;
	struct	sockaddr_in server;
	char	*host, *bp, rbuf[BUFLEN], sbuf[BUFLEN];
	FILE *fp;
	char buf[BUFLEN];

	switch(argc){
	case 2:
		host = argv[1];
		port = SERVER_TCP_PORT;
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (hp = gethostbyname(host)) 
	  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect \n");
	  exit(1);
	}

	// printf("Transmit: \n");
	// while(n=read(0, sbuf, BUFLEN)){	/* get user message */
	//   write(sd, sbuf, n);		/* send it out */
	//   printf("Receive: \n");
	//   bp = rbuf;
	//   bytes_to_read = n;
	//   while ((i = read(sd, bp, bytes_to_read)) > 0){
	// 	bp += i;
	// 	bytes_to_read -=i;
	//   }
	//   write(1, rbuf, n);
	//   printf("Transmit: \n");
	// }
	
	char file_name[BUFLEN];
	printf("Enter a file name\n");
	scanf("%s", file_name);
	strtok(file_name, "\n");
	
	// write file name to server
	write(sd, file_name, sizeof(file_name));

	read(sd, buf, 1);
	if (strcmp(buf, "0") == 0) {
		read(sd, buf, BUFLEN);
		printf("%s\n", buf);
	} else {
		// Open a file with the same file name for writing
		fp = fopen(file_name, "w");
		// Copy contents line by line and write it to the file
		while (read(sd, buf, BUFLEN)) {
			fputs(buf, fp);
		}
	}

	close(sd);
	return(0);
}
