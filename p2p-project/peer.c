#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/stat.h>

#define BUFSIZE 100

struct pdu {
    char type;
    char *data;
} pdu;

int main (int argc, char** argv) {

    int s; //UDP server socket
    int port;
    char *host;
    char *user_name, *content_name;
    char command;
    struct sockaddr_in server;
    int server_len;
    struct hostent *phe;
    char data[101];

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


    printf("Choose a user name");
    scanf("%s", user_name);

    while(1) {
        printf("Command:\n");
        scanf("%s", command);

        switch(command) {
            case 'R':
                printf("Enter the content name");
                scanf("%s", content_name);

                // check if peer has the content
                

                pdu.type = 'R';
                pdu.data = content_name;
                write(s, &pdu, strlen(pdu.data) + 1);
                read(s, data, BUFSIZE);
                print(data);

                break;
            case 'T':
                break;
            case 'D':
                break;
            case 'O':
                break;
            case 'Q':
                break;
            default:
                fprintf(stderr, "Invalid command type: %s\n", command);
                break;
        }
    }



}