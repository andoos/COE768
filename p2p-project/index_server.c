#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/stat.h>

#define SERVER_TCP_PORT 3000
#define BUFSIZE 100

// 8 pdu types
//R - content registration
//D - content download request
//S - Search for content and the associated content server
//T - Content De-registration
//C - Content Data
//O - list of on-line registered content
//A - acknowledgement
//E - error

// Non-content PDUs
struct pdu {
    char type;
    char data[100];
} rpdu, dpdu, spdu, tpdu, opdu, apdu, epdu, cpdu, recievedpdu;

struct peer {
    char *ip;
    int port;
    char *content_name;
} peer;


// For cpdu, the data field is the size of the content. Since the size of the content
// could be larger than the max size of the TCP packet, the data field must be broken
// up into a series of packets

// Content PDU

int main (int argc, char** argv) {

    int port; //port number
    int s; //UDP server socket
    struct sockaddr_in server, client;
    char data[101];
    int client_len;
    struct peer tcp_connections[5]; 


    switch(argc) {
        case 1:
            port = SERVER_TCP_PORT;
            break;

        case 2:
            port = atoi(argv[1]); 
            break;
        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
    }

    //have our port

    //Create UDP socket for server

    if (s = socket(AF_INET, SOCK_DGRAM, 0) < 0) {
        fprint(stderr, "Can't create socket\n");
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    //Bind the socket
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
        fprintf(stderr, "Cant bind to %d port\n", port);

    listen(s, 5);
    client_len = sizeof(client);

    while(1) {
        // Recieve a datagram from a peer - 
        recvfrom(s, data, BUFSIZE, 0, (struct sockaddr *)&client, &client_len);
        recievedpdu.type = data[0];

        for (int i = 0; i < BUFSIZE; i++) {
            recievedpdu.data[i] = data[i + 1];
        }

        // Now we have recievedpdu with properties type and data

        switch(recievedpdu.type) {
            case 'R':
            // "Registering Content" - We need to establish TCP connection with peer
            // in order to add it to the on-line registered content list.
                
                // Save name and port
                peer.ip = client.sin_addr.s_addr;
                peer.port =  client.sin_port;
                peer.content_name = recievedpdu.data;
                break;

            case 'T':
                
                break;
            case 'L':
                
                break;
            case 'D':
                
                break;
            case 'O':
            // On-line list of contents

                
                break;
            case 'Q':

                break;
            default:
                fprintf(stderr, "Invalid PDU type: %s\n", recievedpdu.type);
    }

        
    }




    
    


}



