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
    char *data;
} rpdu, dpdu, spdu, tpdu, opdu, apdu, epdu, cpdu, recievedpdu, qpdu;

//Peers
struct peer {
    char *ip;
    int port;
    char *content_name;
} peer;


// For cpdu, the data field is the size of the content. Since the size of the content
// could be larger than the max size of the TCP packet, the data field must be broken
// up into a series of packets

// Content PDU
int len (struct peer reg_peers[]);

int main (int argc, char** argv) {

    int port; //port number
    int s; //UDP server socket
    struct sockaddr_in server, client;
    char data[101];
    int client_len;
    struct peer registered_peers[1000]; 
    int registered_peer_count;


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
        fprintf(stderr, "Can't create socket\n");
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
                rpdu.type = 'R';
                rpdu.data = recievedpdu.data;

                //warning: s_addr type mismatch with .ip
                registered_peers[registered_peer_count].ip = client.sin_addr.s_addr;
                registered_peers[registered_peer_count].port =  client.sin_port;
                registered_peers[registered_peer_count].content_name = recievedpdu.data;
                
                // fix itoa
                char *port_and_content_name = strcat(itoa(registered_peers[registered_peer_count].port), "\n");
                port_and_content_name = strcat(port_and_content_name, registered_peers[registered_peer_count].content_name);
                rpdu.data = port_and_content_name; 

                registered_peer_count += 1;

                sendto(s, &rpdu, strlen(rpdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));

                break;

            case 'T':
            //"Content Deregistration"
                tpdu.type = 'T';
                tpdu.data = recievedpdu.data;

                for (int i = 0; i < len(registered_peers); i++) {
                    if (strcmp(registered_peers[i].ip, "deregister") != 0) {
                        if (strcmp(registered_peers[i].content_name, tpdu.data) == 0) {
                            // remove registered_peers[i]
                            registered_peers[i].ip = "deregister";
                        }
                    }  
                }

                sendto(s, &tpdu, strlen(tpdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));

                break;

            case 'L':
            //
                
                break;

            case 'D':
            // Download 
                dpdu.type = 'D';
                dpdu.data = recievedpdu.data;

                // Look for registered peer with correct content
                for (int i = 0; i < len(registered_peers); i++) {
                    if (strcmp(registered_peers[i].ip, "deregister") != 0) {
                        if (strcmp(registered_peers[i].content_name, dpdu.data) == 0) {
                            // return ith peer to client
                            // fix itoa
                            dpdu.data = itoa(i);
                            sendto(s, &dpdu, strlen(dpdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                        }
                    }  
                }

                break;

            case 'O':
            // return all registered content
                opdu.type = 'O';
                char contents[100];
                for (int i = 0; i < len(registered_peers); i++){
                    if (strcmp(registered_peers[i].ip, "deregister") != 0) {
                        strcat(contents, registered_peers[i].content_name);
                        strcat(contents, "\n");
                    }
                }

                opdu.data = contents;
                sendto(s, &opdu, strlen(opdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                break;

            case 'Q':
            // Deregisters all registered peers and leaves while loop
                qpdu.type = 'Q';

                for (int i = 0; i < len(registered_peers); i++) {
                    registered_peers[i].ip = "deregister";
                }

                sendto(s, &qpdu, strlen(qpdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                break;

            default:
                fprintf(stderr, "Invalid PDU type: %d\n", recievedpdu.type);
                break;
        }

        
    }
}

int len (struct peer reg_peers[]) {
    return (sizeof(reg_peers)/sizeof(reg_peers[0]));
}



