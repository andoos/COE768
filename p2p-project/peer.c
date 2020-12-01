#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSIZE 100

struct pdu {
    char type;
    char data[100];
} request, tmp_pdu;

char* int_to_string(int x);
void stuffString(char arr[]);

int main (int argc, char** argv) {

    int s; //UDP server socket
    int port;
    char *host;
    char user_name[100], content_name[100];
    char command;
    struct sockaddr_in server;
    int server_len;
    struct hostent *phe;
    char data[101];
    int n;
    const char delim[] = "$"; 
    char * tmp;

    switch(argc) {
        case 3:
            host = argv[1];
            port = atoi(argv[2]); 
            break;

        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(1);
    }

    memset(&server, 0 , sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server_len = sizeof(server);

    // Map host name to IP Address
    if  (phe = gethostbyname(host)) {
        memcpy(&server.sin_addr, phe->h_addr, phe->h_length);
    } else if ((server.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        fprintf(stderr, "Cant get host entry \n");
    }

    // Allocate a socket

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        fprintf(stderr, "Can't create socket\n");
    }

    // Connect the socket
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
        fprintf(stderr, "Can't connect ot %s \n", host);


    printf("Choose a user name\n");
    fgets(user_name, sizeof(user_name), stdin);

    while(1) {
        command = '\0';
        printf("Command:\n");
        scanf("%c", &command);
	    fflush(stdin);
        switch(command) {
            case 'R':
                // What we need to do is ask for a peer name and content name 
                // This information, along with the IP address, is sent to the index server 
                // The index server writes back a message containing the content name and port, verifying registration
                printf("Enter the content name\n");
                n = read(0, content_name, BUFSIZE);
                
                // check if peer actually has the content its referencing

                request.type = 'R';
                
                strtok(user_name, "\n");
                strcat(user_name, delim);
                strcpy(request.data, user_name);
                
                strtok(content_name, "\n");
                strcat(content_name, delim);
                strcat(request.data, content_name);
                
                strcat(request.data, int_to_string(server.sin_addr.s_addr)); // This var holds the IP address
                strcat(request.data, delim);

                if (write(s, &request, sizeof(request.data) + 1) < 0) {
                    fprintf(stderr, "Writing failed.");
                }              

                read(s, data, BUFSIZE);
                
                tmp_pdu.type = data[0];
                for (int i = 0; i < BUFSIZE; i++) {
                    tmp_pdu.data[i] = data[i + 1];
                }                
               
               tmp = strtok(tmp_pdu.data, delim);
               printf("Content name: %s\n", tmp);
               tmp = strtok(NULL, delim);
               printf("Port number: %s\n", tmp);

               break;
            case 'T':
                break;
            case 'D':
                break;
            case 'O':
                break;
            case 'Q':
                break;
            case '?':
            	printf("R - Register Content\nT - Deregister Content\n");
            default:
                fprintf(stderr, "Invalid command type: %c\n", command);
                break;
        }
    }
}

char* int_to_string(int x) {
    int length = snprintf(NULL, 0, "%d", x);
    char *str = malloc(length + 1);
    snprintf(str, length + 1, "%d", x);
    return str;
}
