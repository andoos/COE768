/* time_server.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#define	BUFSIZE 100

struct pdu {
char type;
char data[100];
} cpdu, spdu;

/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	struct  sockaddr_in fsin;	/* the from address of a client	*/
	char	buf[100];		/* "input" buffer; any size > 0	*/
	char    *pts;
	int	sock;			/* server socket		*/
	time_t	now;			/* current time			*/
	int	alen;			/* from-address length		*/
	struct  sockaddr_in sin; /* an Internet endpoint address         */
        int     s, type;        /* socket descriptor and socket type    */
	int 	port=3000;
	char pdu_type;
	char data[101];
	FILE * fp; 
	struct stat st;
	int size; 
                                                                                

	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "can't creat socket\n");
                                                                                
    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n",port);
        listen(s, 5);	
	alen = sizeof(fsin);

	// while (1) {
		
	// 	if (recvfrom(s, buf, sizeof(buf), 0,
	// 			(struct sockaddr *)&fsin, &alen) < 0)
	// 		fprintf(stderr, "recvfrom error\n");

	// 	(void) time(&now);
    	//     	pts = ctime(&now);

	// 	(void) sendto(s, pts, strlen(pts), 0,
	// 		(struct sockaddr *)&fsin, sizeof(fsin));
	// }

	while (1) {
		recvfrom(s, data, BUFSIZE, 0,(struct sockaddr *)&fsin, &alen);
		cpdu.type = data[0];
		for (int i = 0; i < BUFSIZE; i++) {
			cpdu.data[i] = data[i + 1];
		}	
		if (cpdu.type == 'C') {
			fp = fopen(cpdu.data, "r");
			if (fp == NULL) {
				spdu.type = 'E';
				strcpy(spdu.data, "File not found");
				sendto(s, &spdu, strlen(spdu.data)+1, 0,(struct sockaddr *)&fsin, sizeof(fsin));
			} else {
				lstat(cpdu.data, &st);
				size = st.st_size;
				while(size > BUFSIZE) {
					spdu.type = 'D';
					fgets(spdu.data, BUFSIZE, fp);
					sendto(s, &spdu, strlen(spdu.data)+1, 0,(struct sockaddr *)&fsin, sizeof(fsin));
					size -= strlen(spdu.data);
				} 
				spdu.type = 'F';
				fgets(spdu.data, size+1, fp);
				sendto(s, &spdu, strlen(spdu.data)+1, 0,(struct sockaddr *)&fsin, sizeof(fsin));
				fclose(fp);
			}
		}
	}
}
