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
#include <errno.h>
#include <time.h>

#define BUFSIZE 100

struct pdu {
    char type;
    char data[100];
} request, response, tmp_pdu;

typedef struct content_pdu {
    char type;
    char data[100];
} content_pdu;

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
int random_number_in_range (int lower, int upper) ;
int read_file(int sd, char file_name[]);

int main (int argc, char** argv) {

    int s; //UDP server socket
    int port;
    char *host;
    char user_name[10], content_name[10], to_deregister[10], old_content_name[10], tmp_user_name[10], to_download[10], save_to_download[10];
    char save_sendData_data[10];
    char command;
    struct sockaddr_in server;
    int server_len;
    struct hostent *phe;
    content_pdu sendData;
    content_pdu recievedData;
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
    pid_t pid;
    int sd;
    char content_server_data[101];

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
        port = random_number_in_range(5000, 50000);
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

                memset(path, 0, sizeof(path));
                strcat(path, "./");
                strcat(path, file_name);

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

                // Open up a tcp connection 
                pid = fork();
                if (pid == 0) {
                    struct sockaddr_in content_server_addr, content_client_addr;
                    socklen_t content_client_len; 

                    memset(&content_server_addr, 0, sizeof(content_server_addr));
                    memset(&content_client_addr, 0, sizeof(content_client_addr));

                    // Create a socket stream 
                    int sd; 
                    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                        printf("socker error\n");
                    }

                    // Bind an address to the socket
                    bzero((char*)&content_server_addr, sizeof(struct sockaddr_in));
                    content_server_addr.sin_family = AF_INET; 
                    content_server_addr.sin_port = htons(port);
                    content_server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 

                    if (bind(sd, (struct sockaddr*)&content_server_addr, sizeof(content_server_addr)) == -1) {
                        printf("bind error\n");
                        printf("error code: %d\n", errno);
                    }

                    listen(sd, 5);

                    content_client_len = sizeof(content_client_addr);
                    while (1) {
                        int clientSocket = accept(sd, (struct sockaddr*)&content_client_addr, &content_client_len);
                        if (clientSocket < 0) {
                            printf("can't accept client\n");
                        }

                        char contentData[101];
                        memset(contentData, '\0', sizeof(contentData));
                        read(clientSocket, contentData, sizeof(contentData));
                        printf("Got: %s\n", contentData);

                        recievedData.type = contentData[0];
                        for (int i = 0; i < BUFSIZE; i++) {
                            recievedData.data[i] = contentData[i + 1];
                        }

                        // now we have file name
                        char to_open_file_path[20];
                        strcpy(to_open_file_path, "./");
                        strcat(to_open_file_path, recievedData.data);
                        printf("The file path is: %s\n", to_open_file_path);

                        read_file(clientSocket, to_open_file_path);
                        break;
                    }
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
                
                memset(to_download, 0, sizeof(to_download));
                memset(save_to_download, 0, sizeof(save_to_download));
                printf("What content would you like to download?\n");
                n = read(0, to_download, sizeof(to_download));
                strcpy(save_to_download, to_download);

                //Check if the content is in the local registers list already
                found_local = 0;
                for (int i = 0; i < local_register_count; i++) {
                    memset(local_string, 0, sizeof(local_string));
                    for (int j = 0; j < 10; j++) {
                        local_string[j] = local_registers[i][j];
                        if (j == 9) {
                            if (strcmp(local_string, to_download) == 0) {
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

                //Send an S type PDU to index_server.c
                request.type = 'S';

                int to_download_padding = 10 - n;
                pad_string(to_download, to_download_padding);
                strcat(request.data, to_download);

                if (write(s, &request, sizeof(request.data) + 1) < 0) {
                    fprintf(stderr, "Writing failed.");
                }              

                // Recieve acknowledgement from server
                read(s, data, sizeof(data));

                response.type = data[0];
                for (int i = 0; i < BUFSIZE; i++) {
                    response.data[i] = data[i + 1];
                }

                if (response.type == 'S') {
                    printf("Found the content!\n");
                    printf("The current port is: %d\n", port);
                    printf("The port to set the TCP connection with is: %d\n", atoi(response.data));

                    //SET UP TCP Connection
                    struct hostent *content_server;
                    struct sockaddr_in content_server_addr; 

                    memset(&content_server_addr, 0 , sizeof(content_server_addr));
                    content_server_addr.sin_family = AF_INET;
                    content_server_addr.sin_port = htons(atoi(response.data));
                    printf("%s\n", response.data);
                    char contentAddr[9] = "localhost";
                    content_server = gethostbyname(contentAddr);
                    bcopy((char *)content_server -> h_addr, (char *) &content_server_addr.sin_addr.s_addr, content_server -> h_length);

                    int fileClientSocket; 
                    if ((fileClientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        printf("socket error\n");
                        //exit(1);
                    }
                    if (connect(fileClientSocket, (struct sockaddr*) &content_server_addr, sizeof(content_server_addr)) == -1) {
                        printf("connect error\n");
                        //exit(1);
                    }

                    // After connection is established, peer sends a D type PDU with content name to initiate the download 
                    memset(&sendData, 0, sizeof(sendData));
                    sendData.type = 'D';
                    strcpy(sendData.data, save_to_download);
                    printf("Sent: %s\n", sendData.data);
                    write(fileClientSocket, &sendData, sizeof(sendData.data) + 1);

                    char buf[1];
                    memset(buf, 0, sizeof(buf));
                    read(fileClientSocket, buf, sizeof(buf));

                    if (strcmp(buf, "0") == 0) {
                        printf("File not open succesfully\n");
                    } else {
                        printf("File open successfully\n");
                        FILE *fp;

                        strcpy(save_sendData_data, sendData.data);
                        
                        strtok(sendData.data, "\n");
                        fp = fopen(sendData.data, "w");
                        
                        char file_line[BUFSIZE];

                        while (read(fileClientSocket, file_line, sizeof(file_line))) {
                            fseek(fp, 0, SEEK_CUR);
                            fputs(file_line, fp);
                        }

                        close(fileClientSocket);

                        //register the new content to this peer
                        request.type = 'R';
                        pad_string(sendData.data, 10 - strlen(sendData.data));
                        strcat(request.data, sendData.data);

                        if (write(s, &request, sizeof(request.data) + 1) < 0) {
                            fprintf(stderr, "Writing failed.");
                        }

                        memset(data, 0, sizeof((data)));
                        read(s, data, BUFSIZE);
                        printf("RAW data: %s\n", data);
                
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
                            for (int i = 0; i < sizeof(save_sendData_data); i++){
                                local_registers[local_register_count][i] = save_sendData_data[i];
                            }
                            local_register_count++;

                            pid_t new_pid;
                            
                            new_pid = fork();
                            if (new_pid == 0) {
                                struct sockaddr_in content_server_addr, content_client_addr;
                                socklen_t content_client_len; 

                                memset(&content_server_addr, 0, sizeof(content_server_addr));
                                memset(&content_client_addr, 0, sizeof(content_client_addr));

                                // Create a socket stream 
                                int sd; 
                                memset(&sd, 0, sizeof(sd));
                                if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                                    printf("socker error\n");
                                }

                                // Bind an address to the socket
                                bzero((char*)&content_server_addr, sizeof(struct sockaddr_in));
                                content_server_addr.sin_family = AF_INET; 
                                content_server_addr.sin_port = htons(atoi(tmp));
                                content_server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 

                                if (bind(sd, (struct sockaddr*)&content_server_addr, sizeof(content_server_addr)) == -1) {
                                    printf("bind error\n");
                                    printf("error code: %d\n", errno);
                                }

                                listen(sd, 5);

                                content_client_len = sizeof(content_client_addr);
                                while (1) {
                                    int clientSocket = 0; 
                                    clientSocket = accept(sd, (struct sockaddr*)&content_client_addr, &content_client_len);
                                    if (clientSocket < 0) {
                                        printf("can't accept client\n");
                                    }

                                    printf("clientSocket: %d\n", clientSocket);
                                    char d_data[101];
                                    memset(d_data, '\0', sizeof(d_data));
                                    read(clientSocket, d_data, sizeof(d_data));
                                    
                                    // printf("Got: %s\n", d_data);

                                    // recievedData.type = d_data[0];
                                    // for (int i = 0; i < BUFSIZE; i++) {
                                    //     recievedData.data[i] = d_data[i + 1];
                                    // }

                                    // // now we have file name
                                    // char to_open_file_path[20];
                                    // strcpy(to_open_file_path, "./");
                                    // strcat(to_open_file_path, recievedData.data);
                                    // printf("The file path is: %s\n", to_open_file_path);

                                    // read_file(clientSocket, to_open_file_path);
                                    // break;
                                }
                            }
                        }
                        else {
                            printf("Registration Unsuccessful.\n");
                        }
                    }                    

                } else {
                    printf("%s\n", response.data);
                }
                

                print_options();
                break;
             case 'S':
                 // Search for Content and Associated Content Server
                 memset(content_name, 0, sizeof(content_name));
                 memset(request.data, 0, 100);
                 printf("Enter the content name\n");
                 n = read(0, content_name, BUFSIZE);

                 request.type = 'S';
                 //printf("%s", request.data);
                
                 // Delim bug - appending more than one delimiter messing up the 
                strcpy(tmp_user_name, user_name); // temp fix, will change to use other logic later

                 strtok(tmp_user_name, "\n");
                 strcat(tmp_user_name, delim);
                 strcpy(request.data, tmp_user_name);
                
                 //printf("The content name is: %s", content_name);
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
                            if (strcmp(local_string, strtok(to_deregister, "\n")) == 0) {
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
                    printf("%s\n", response.data);
                }
                else {
                    printf("%s\n", response.data);
                }

                print_options();
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
            case 'Q':
                request.type = 'Q';
                
                if (write(s, &request, sizeof(request.data) + 1) < 0) {
                    fprintf(stderr, "Writing failed.");
                }

                read(s, data, BUFSIZE);

                response.type = data[0];
                for (int i = 0; i < BUFSIZE; i++) {
                    response.data[i] = data[i + 1];
                }

                printf("%s\n", response.data);
                exit(0);

            // default:
            //     print_options();
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
    printf("\nR - Register Content\nT - Content De-Registration\nO - List of Online Registered Content\nL - List of Locally Registered Content\nD - Content Download Request\nQ - Quit\n");
}

void pad_string(char str[], int padding_amount) {
    strtok(str, "\n");
    for (int i = 0; i < padding_amount; i++) {
        strcat(str, "$");
    }
}

int random_number_in_range (int lower, int upper) {
    srand(time(NULL));
    return (rand() % (upper - lower + 1)) + lower;
}

int read_file(int sd, char file_name[]) {
    char buf[BUFSIZE];
    FILE *fp;
    strtok(file_name, "\n");
    fp = fopen(file_name, "r");
    content_pdu file_io;

    if (fp == NULL) {
        write(sd, "0", 1);
    } else {
        printf("File is there!\n");
        write(sd, "1", 1);
        while (fgets(buf, BUFSIZE, fp) != NULL) {
            write(sd, buf, BUFSIZE);
        }
    }

    fclose(fp);
    close(sd);
    printf("TCP Connection closed \n");
}

void create_content_server() {
    
}
