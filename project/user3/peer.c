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
} request, response, tmp_pdu;

struct Content {
    char name[10];
    char content_name[10];
    char ip[10];
    int port;
} peer, tmp_peer;

char* int_to_string(int x);
void stuffString(char arr[]);
void print_options();
void pad_string(char str[], int padding_amount);

int main (int argc, char** argv) {

    int s; //UDP server socket
    int port;
    char *host;
    char user_name[10], content_name[10], to_deregister[10], old_content_name[10];
    char command;
    struct sockaddr_in server;
    int server_len;
    struct hostent *phe;
    char data[101];
    int n;
    const char delim[] = "$"; 
    char * tmp;
    FILE *fptr;
    char file_name[20];
    char path[100];
    char local_string[10];
    // Each peer can at most register 100 contents
    char local_registers[100][10];
    int local_register_count = 0;
    int found_local;

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
    print_options();

    // Add check for conflicting user name?
    while(1) {
        
        command = getchar();
        printf("\n");

        memset(response.data, 0, 100);
        
        //Reset the request.data and populate the first 10 chars with the username
        memset(request.data, 0, 100);

        // Username
        char user[10];
        strcpy(user, user_name);
        int user_padding = 10 - strlen(user);
        pad_string(user, user_padding);
        strcat(request.data, user);

        //IP
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &server.sin_addr, ip, INET_ADDRSTRLEN);
        int ip_padding = 16 - strlen(ip);
        pad_string(ip, ip_padding);
        strcat(request.data, ip);

        //Port String, next 8 bytes - **port cannot be larger than 7 digits
        char* port_str = int_to_string(port);
        int port_padding = 8 - strlen(port_str);
        pad_string(port_str, port_padding);
        strcat(request.data, port_str);
        
        memset(data, 0, 101);
        
        switch(command) {
            case 'R':
                // Content Registration
                // What we need to do is ask for a peer name and content name 
                // This information, along with the IP address, is sent to the index server 
                // The index server writes back a message containing the content name and port, verifying registration
                request.type = 'R';
                
                memset(content_name, 0, sizeof(content_name));
                printf("Enter the content name\n");
                n = read(0, content_name, BUFSIZE);

                // Check if the content is in the local registers list already
                found_local = 0;
                for (int i = 0; i < local_register_count; i++) {
                    memset(local_string, 0, sizeof(local_string));
                    for (int j = 0; j < 10; j++) {
                        local_string[j] = local_registers[i][j];
                        if (j == 9) {
                            if (strcmp(local_string, content_name) == 0) {
                                found_local = 1;
                                break;
                            }
                        }
                    }
                }

                if (found_local == 1) {
                    printf("The content has already been registered by this peer.\n");
                    print_options();
                    break;
                }
                //Check if the content name is in the directory
                
                memset(file_name, 0, sizeof(file_name));
                strcpy(file_name, content_name);
                strtok(file_name, "\n");

                printf("The file name is: %s\n", file_name);

                memset(path, 0, sizeof(path));
                strcat(path, "//home//marvin//coe768//project//user3//");
                strcat(path, file_name);
                printf("The path we are looking for is: %s\n", path);

                if ((fptr = fopen(path, "r")) == NULL) {
                    printf("No file matching the given content.\n");
                    print_options();
                    break;
                } else {
                    printf("Successfully found the file!\n");
                    fclose(fptr);
                }

                strcpy(old_content_name, content_name);
                // Content

                int content_padding = 10 - n;
                pad_string(content_name, content_padding);
                strcat(request.data, content_name);
                
                if (write(s, &request, sizeof(request.data) + 1) < 0) {
                    fprintf(stderr, "Writing failed.");
                }              

                // Recieve acknowledgement from server
                read(s, data, BUFSIZE);
                
                response.type = data[0];
                for (int i = 0; i < BUFSIZE; i++) {
                    response.data[i] = data[i + 1];
                }                
                
                if (response.type == 'A') {
                    tmp = strtok(response.data, delim);
                    printf("Content has been successfully registered.\nContent name: %s\n", tmp);
                    tmp = strtok(NULL, delim);
                    printf("Port number: %s\n", tmp);

                    // Add content to local contents list
                    for (int i = 0; i < sizeof(old_content_name); i++){
                        local_registers[local_register_count][i] = old_content_name[i];
                    }
                    local_register_count++;
                }
                else {
                    printf("Registration Unsuccessful.\n");
                }
                print_options();
                break;

            case 'D':
                // Content Download Request
                // The user inputs the name of the content they want to download 
                // A pdu with type S and data containing the peer name and content name is sent to the index server 
                // The index server responds with an S type pdu containing the ip and port or Error 
                // The port and ip are used to establish a tcp connection with the content server 
                // After connection is established, peer sends a D type PDU with content name to initiate the download 
                // Content server responds with a C type PDU, or multiple depending on the size
                // Once all data is sent, the TCP connection is terminated 
                // A message is sent to the content server to register the peer as the new content server for the downloaded content 
                
                print_options();
                break;
            case 'S':
                // Search for Content and Associated Content Server
                printf("Enter the content name\n");
                n = read(0, content_name, BUFSIZE);

                request.type = 'S';
                printf("%s", request.data);
                
                // Delim bug - appending more than one delimiter messing up the 
                
                strtok(user_name, "\n");
                strcat(user_name, delim);
                strcpy(request.data, user_name);
                
                printf("The content name is: %s", content_name);
                strtok(content_name, "\n");
                strcat(content_name, delim);
                strcat(request.data, content_name);

                if (write(s, &request, sizeof(request.data) + 1) < 0) {
                    fprintf(stderr, "Writing failed.");
                }              

                read(s, data, BUFSIZE);
                
                response.type = data[0];
                for (int i = 0; i < BUFSIZE; i++) {
                    response.data[i] = data[i + 1];
                }  

                if (response.type == 'S') {
                    printf("Content found!\n");
                    printf("%s\n", response.data);
                }
                else {
                    printf("%s\n", response.data);
                }

                print_options();
                break;
            case 'T':
                // Content De-Registration
                printf("Enter the content name\n");
                n = read(0, to_deregister, BUFSIZE);

                // Deregister from local
                
                found_local = 0;
                for (int i = 0; i < local_register_count; i++) {
                    memset(local_string, 0, sizeof(local_string));
                    for (int j = 0; j < 10; j++) {
                        local_string[j] = local_registers[i][j];
                        if (j == 9) {
                            if (strcmp(local_string, to_deregister) == 0) {
                                // found match
                                for (int k = 0; k < 9; k ++) {
                                    local_registers[i][k] = '\0';
                                }
                                break;
                            }
                        }
                    }
                }

                // Deregister from online
                request.type = 'T';
                
                strtok(to_deregister, "\n");
                strcat(request.data, to_deregister);

                if (write(s, &request, sizeof(request.data) + 1) < 0) {
                    fprintf(stderr, "Writing failed.");
                }     

                read(s, data, BUFSIZE);
                
                response.type = data[0];
                for (int i = 0; i < BUFSIZE; i++) {
                    response.data[i] = data[i + 1];
                }                
               
                if (response.type == 'A') {
                        printf("Content has been successfully de-registered.\n");
                        printf("%s\n", response.data);
                }
                else {
                    printf("De-Registration Unsuccessful.\n");
                    printf("%s\n", response.data);
                }

                print_options();
                break;
            case 'C':
                // Content Data (download)
                break;
            case 'O':
                // List of Online Registered Content 
                request.type = 'O';

                if (write(s, &request, sizeof(request.data) + 1) < 0) {
                    fprintf(stderr, "Writing failed.");
                } 

                read(s, data, BUFSIZE);

                response.type = data[0];
                for (int i = 0; i < BUFSIZE; i++) {
                    response.data[i] = data[i + 1];
                }  

                if (response.type == 'O') { 
                    printf("Online Content List:\n%s", response.data);
                }
                else {
                     printf("Error processing O command.\n");
                }

                print_options();
                break;
            case 'L':
                // Print all the locally registered content
                printf("Locally Registered Content:\n");
                for (int i = 0; i < local_register_count; i++) {
                    for (int j = 0; j < 10; j++) {
                        printf("%c", local_registers[i][j]);
                        if (j == 9 && local_registers[i][0] != '\0') {
                            printf("\n");
                        }
                    }
                }

                print_options();
                break;
            // default:
            //     fprintf(stderr, "Invalid command type: %c\n", command);
                //break;
        }
    }
}

char* int_to_string(int x) {
    int length = snprintf(NULL, 0, "%d", x);
    char *str = malloc(length + 1);
    snprintf(str, length + 1, "%d", x);
    return str;
}

void print_options() {
    printf("\nR - Register Content\nD - Content Download Request\nS - Search for Content and Associated Content Server\nT - Content De-Registration\nO - List of Online Registered Content\nL - List of Locally Registered Content\n");
}

void pad_string(char str[], int padding_amount) {
    strtok(str, "\n");
    for (int i = 0; i < padding_amount; i++) {
        strcat(str, "$");
    }
}
