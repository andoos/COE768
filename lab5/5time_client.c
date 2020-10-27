/* time_client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
                                                                                
#include <netdb.h>

#define	BUFSIZE 100

#define	MSG		"Any Message \n"


struct pdu {
char type;
char data[100];
} spdu, cpdu;

/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */
int
main(int argc, char **argv)
{
	char	*host = "localhost";
	int	port = 3000;
	char	now[100];		/* 32-bit integer to hold time	*/ 
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, n, type;	/* socket descriptor and socket type	*/
	char data[101];
	FILE * fp;
	char content[1000];

	switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;                                                                
        sin.sin_port = htons(port);
                                                                                        
    /* Map host name to IP address, allowing for dotted decimal */
        if ( phe = gethostbyname(host) ){
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        }
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "Can't create socket \n");
	
                                                                                
    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "Can't connect to %s %s \n", host, "Time");


	//(void) write(s, MSG, strlen(MSG));

	/* Read the time */

	// n = read(s, (char *)&now, sizeof(now));
	// if (n < 0)
	// 	fprintf(stderr, "Read failed\n");
	// write(1, now, n);
	// exit(0);

	while (1) {
		printf("Enter a file name or quit:\n");
		n=read(0, spdu.data, BUFSIZE);
		if (strcmp(spdu.data, "quit\n") == 0) {
			exit(0);
		} else {
			spdu.type = 'C';
			spdu.data[n-1] = '\0';
			write(s, &spdu, n+1);
			while (n = read(s, data, BUFSIZE)) {
				cpdu.type = data[0];
				for (int i = 0; i < n; i++) {
					cpdu.data[i] = data[i + 1];
				}
				cpdu.data[n-1] = '\0';
				if (cpdu.type == 'E') {
					printf("Error: %s\n", cpdu.data);
					break;
				} else if (cpdu.type == 'D') {
					strcat(content, cpdu.data);
				} else if (cpdu.type == 'F') {
					strcat(content, cpdu.data);
					fp = fopen(spdu.data, "w");
					fputs(content,fp);
					fclose(fp);
					printf("File recieved\n");
					break;
				} else {
					printf("Error: Got bad identifier\n");
					break;
				}
				cpdu.data[0]= '\0';
			}
		}
		spdu.data[0]  = '\0';
		content[0] = '\0';
	}
}
