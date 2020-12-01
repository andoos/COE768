#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>

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
} request, response, dpdu, spdu, tpdu, opdu, apdu, epdu, cpdu, qpdu;

//Peers
struct Content {
    char name[10];
    char content_name[10];
    char ip[10];
    int port;
} peer, tmp_peer;


// For cpdu, the data field is the size of the content. Since the size of the content
// could be larger than the max size of the TCP packet, the data field must be broken
// up into a series of packets

// Content PDU
char* int_to_string(int x);

int main (int argc, char** argv) {

    int port; //port number
    int s; //UDP server socket
    struct sockaddr_in server, client;
    char data[101];
    int client_len;
    struct Content registered_contents[1000]; 
    int registered_contents_count = 0;
    const char delim[] = "$"; 
    char * tmp;


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

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        fprintf(stderr, "Can't create socket\n");
    }

    //Bind the socket
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
        fprintf(stderr, "Cant bind to %d port\n", port);


    listen(s, 5);
    client_len = sizeof(client);
    
    setbuf(stdout, NULL);
    while(1) {
        // Recieve a datagram from a peer - 
        recvfrom(s, data, sizeof(data), 0, (struct sockaddr *)&client, &client_len);
        printf("Raw Data: %s\n", data);
        request.type = data[0];

        for (int i = 0; i < BUFSIZE; i++) {
            request.data[i] = data[i + 1];
        }

        // Now we have recievedpdu with properties type and data

        switch(request.type) {
            case 'R':
            	// Content Registration
            	// When a peer makes a request to register some content, we need to save the name, content name, ip address, and port for download 
            	// The user can write the first 3 parameters and all that needs to be done is parsing 
            	// The port number can be decided by the server or specified by the peer
            	// After all information has been gathered, we will write back a message to the peer with the port number 
                printf("PDU Type: %c\n", request.type);
                printf("PDU Data: %s\n", request.data);
		
		        // Get name 
                tmp = strtok(request.data, delim); // I used $ as a delimiter on the peer side to split up the name, content and IP
                strcpy(tmp_peer.name, tmp);
                printf("name: %s\n", tmp_peer.name);
                
                // Get content name 
                tmp = strtok(NULL, delim);
                strcpy(tmp_peer.content_name, tmp);
                printf("content: %s\n", tmp_peer.content_name);
                
                // Get IP - This will probably always be localhost 
                tmp = strtok(NULL, delim);
                strcpy(tmp_peer.ip, tmp);
                printf("ip: %s\n", tmp_peer.ip);
                
                // Set port number - hard code for now 
                tmp_peer.port = 8000;
                
                // Add peer information to registered peers list 
                registered_contents[registered_contents_count] = tmp_peer;

                // Write the content name and port number to the peer 
                response.type = 'A';
                strcpy(response.data, tmp_peer.content_name);
                strcat(response.data, delim);
                strcat(response.data, int_to_string(tmp_peer.port));
                strcat(response.data, delim);
		
                sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
		
		        // Increase registered peers count
                registered_contents_count++;

                break;
            /*case 'D':
            // Content Download Request 
                dpdu.type = 'D';
                dpdu.data = recievedpdu.data;

                // Look for registered peer with correct content
                for (int i = 0; i < registered_peer_count; i++) {
                    if (strcmp(registered_peers[i].ip, "deregister") != 0) {
                        if (strcmp(registered_peers[i].content_name, dpdu.data) == 0) {
                            // return ith peer to client
                            // fix itoa
                            dpdu.data = int_to_string(i);
                            sendto(s, &dpdu, strlen(dpdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                        }
                    }  
                }
                break;*/
            case 'S':
                // Search for Content and Associated Content Server 
                break;
            case 'T':
                // Content De-Registration
                tpdu.type = 'T';
                //tpdu.data = recievedpdu.data;

                /*for (int i = 0; i < registered_peer_count; i++) {
                    if (strcmp(registered_peers[i].ip, "deregister") != 0) {
                        if (strcmp(registered_peers[i].content_name, tpdu.data) == 0) {
                            // remove registered_peers[i]
                            registered_peers[i].ip = "deregister";
                        }
                    }  
                }*/

                sendto(s, &tpdu, strlen(tpdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                break;
            case 'O':
                // List of Online Registered Content 
                memset(response.data, 0, 100);

                for (int i = 0; i < registered_contents_count; i++){
                    if (strcmp(registered_contents[i].content_name, "deregister") != 0) {
                        strcat(response.data, registered_contents[i].content_name);
                        strcat(response.data, "\n");
                    }
                }

                response.type = 'A';

                sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));

                break;
            /*case 'Q':
            // Deregisters all registered peers and leaves while loop
                qpdu.type = 'Q';

                for (int i = 0; i < registered_peer_count; i++) {
                    registered_peers[i].ip = "deregister";
                }

                sendto(s, &qpdu, strlen(qpdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                break;*/
            default:
                fprintf(stderr, "Invalid PDU type: %d\n", request.type);
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
