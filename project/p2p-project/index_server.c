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
} request, response;

//Peers
typedef struct Content {
    char name[10];
    char content_name[10];
    char ip[16];
    char port[8];
} Content;


// For cpdu, the data field is the size of the content. Since the size of the content
// could be larger than the max size of the TCP packet, the data field must be broken
// up into a series of packets

// Content PDU
char* int_to_string(int x);
char* get_attribute(char* str, int num_bytes, int start);
void content_peer_to_string(Content tmp_peer);
int content_equals(Content c1, Content c2);

int main (int argc, char** argv) {

    int port; //port number
    int s; //UDP server socket
    struct sockaddr_in server, client;
    char data[101];
    int client_len;
    char to_deregister[10];
    Content registered_contents[100000];
    int registered_contents_count = 0;
    const char delim[] = "$"; 
    char * tmp;
    int found = 0;
    int duplicate_check = 0;
    int found_duplicate = 0;

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
        Content tmp_peer;
        memset(request.data, 0, 100);
        memset(response.data, 0, 100);
        memset(data, 0, 101);
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
		
		        // Get name - first 9 bytes
                get_attribute(tmp_peer.name, 9, 0);

                // Get IP - next 16 bytes
                get_attribute(tmp_peer.ip, 16, 9);

                // Get Port number - next 8 bytes
                get_attribute(tmp_peer.port, 8, 25);

                // Get content name - next 9 bytes
                get_attribute(tmp_peer.content_name, 9, 33);
                content_peer_to_string(tmp_peer);

                // Check duplicates
                found_duplicate = 0;
                for (int i = 0; i < registered_contents_count; i ++) {
                    duplicate_check = content_equals(registered_contents[i], tmp_peer);
                    if (duplicate_check == 1) {
                        found_duplicate = 1;
                        break;
                    }
                }

                // if (found_duplicate == 1) {
                //     found_duplicate = 0;
                //     response.type = 'E';
                //     sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                //     break;
                // }

                // Add peer information to registered peers list 
                registered_contents[registered_contents_count] = tmp_peer;

                // Send an acknowledgement back to the peer.
                response.type = 'A';
                strcpy(response.data, tmp_peer.content_name);
                strcat(response.data, delim);
                strcat(response.data, tmp_peer.port);
                strcat(response.data, delim);
		
                sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
		
		        // Increase registered peers count
                registered_contents_count++;

                break;
            case 'D':
                // Content Download Request 
                // I think all the index server D type has to do is swap port, name, and content name of the downloaded content 
                //dpdu.type = 'D';
                //dpdu.data = recievedpdu.data;

                // Look for registered peer with correct content
                /*for (int i = 0; i < registered_peer_count; i++) {
                    if (strcmp(registered_peers[i].ip, "deregister") != 0) {
                        if (strcmp(registered_peers[i].content_name, dpdu.data) == 0) {
                            // return ith peer to client
                            // fix itoa
                            dpdu.data = int_to_string(i);
                            sendto(s, &dpdu, strlen(dpdu.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                        }
                    }  
                }*/
                break;
            case 'S':
                 // Search for Content and Associated Content Server 
                 printf("PDU Type: %c\n", request.type);
                 printf("PDU Data: %s\n", request.data);

                 char search_content[10];
                
                 get_attribute(search_content, sizeof(search_content), 33);
                 printf("The search content is: %s\n", search_content);

                int max_i = 0;
                 for (int i = 0; i < registered_contents_count; i++){
                     if (strcmp(registered_contents[i].content_name, search_content) == 0) {
                        if (i > max_i) {
                            max_i = i;
                        } 
                     }
                 }

                strcat(response.data, registered_contents[max_i].port);
                found = 1;

                 if (found == 1) {
                     found = 0;
                     response.type = 'S';
                 }
                 else {
                     response.type = 'E';
                     strcpy(response.data, "Content was not found.\n");
                 }

                 sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));

                 break;
            case 'T':
                // Content De-Registration
                //get the content string you need to compare with from the request.data
                memset(to_deregister, 0, sizeof(to_deregister));
                get_attribute(to_deregister, 9, 33);
                printf("The to_deregister string is: %s\n", to_deregister);

                for (int i = 0; i < registered_contents_count; i++) {
                    if (strcmp(registered_contents[i].content_name, "deregister") != 0) {
                        if (strcmp(registered_contents[i].content_name, to_deregister) == 0) {
                            registered_contents[i].name[0] = '\0'; 
                            strcpy(registered_contents[i].content_name, "deregister");
                            registered_contents[i].ip[0] = '\0';
                            registered_contents[i].port[0] = '\0';
                            found = 1;
                        }
                    }  
                }

                if (found == 1) {
                    found = 0;
                    response.type = 'A';
                    strcpy(response.data, "Content was successfully de-registered.\n");
                }
                else {
                    response.type = 'E';
                    strcpy(response.data, "Content was not found.\n");
                }

                sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));

                break;
            case 'O':
                // List of Online Registered Content 
                for (int i = 0; i < registered_contents_count; i++){
                    if (strcmp(registered_contents[i].content_name, "deregister") != 0) {
                        strcat(response.data, registered_contents[i].content_name);
                        strcat(response.data, "\n");
                    }
                }

                response.type = 'O';

                sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                break;
            case 'Q':
                // Not going to reset online content on individual peer quit
                // memset(registered_contents, 0, sizeof(registered_contents));
                // registered_contents_count = 0;

                response.type = 'Q';
                strcpy(response.data, "Quitting...");
                sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&client, sizeof(client));
                break;

            default:
                fprintf(stderr, "Invalid PDU type: %c\n", request.type);
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

char* get_attribute(char* str, int num_bytes, int start) {
    for (int i = 0; i < num_bytes; i++) {
        str[i] = request.data[start + i];
    }
    strtok(str, "$");
}

void content_peer_to_string(Content tmp_peer) {
    printf("The content peer's username is: %s\n", tmp_peer.name);
    printf("The content peer's IP is: %s\n", tmp_peer.ip);
    printf("The content peer's port number is: %s\n", tmp_peer.port);
    printf("The content peer's content name: %s\n", tmp_peer.content_name);
}

int content_equals(Content c1, Content c2) {
    if ((strcmp(c1.ip, c2.ip) && strcmp(c1.name, c2.name) && strcmp(c1.port, c2.port)) == 0) {
        return 1;
    } else {
        return 0;
    }
}
