/*
This main server accepts a telnet client and displays a simple text-based menu. The services 
provided (see micro-servers) include an English-French translator, a currency converter, and a voting system. Each of these 
three services is handled by a microserver program, which are connected to via UDP. The interserver receives the 
client’s commands and forwards them to the corresponding microserver, which processes it and sends back a response. The 
translator has a 5-word vocabulary, which is specified to the client. The converter can convert between any of 5 specified 
currencies. The voting service can show candidates, accept a vote (if the client hasn’t voted already), and 
show the election results (if the client has voted).
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>

#define CLIENTPORTNUM 9000
#define MSERVER1 8725
#define MSERVER2 9571
#define MSERVER3 8552
#define MSGLEN 3000

/* Main program for interserver
Implements the interserver functionality by creating a UDP socket for communication with the microservers and
a TCP connection with a client. It enters into a loop prompting the client for a service, and forwards the relevant
information back and forth between the client and the microservers.
*/

int main() {

    char msgIn[MSGLEN];
    char msgOut[MSGLEN];
    bzero(msgIn, MSGLEN);
    bzero(msgOut, MSGLEN);
    char *c;
    char command[20];
    char word[20];
    char *value;
    char *source;
    char *dest;
    char msg[MSGLEN];
    int voted = 0;
    int key;
    int clientVote;


    struct sockaddr_in server, clientAddr, mServer;
    int client, serverSocket, clientSocket, pid, num;

    // timeval struct for setting socket timers
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* Create the UPD socket */

    memset(&clientAddr,0,sizeof(clientAddr));
    socklen_t cLen = sizeof(clientAddr);

    // Initialize server sockaddr structure
    memset(&mServer, 0, sizeof(mServer));
    mServer.sin_family = AF_INET;
    inet_pton(AF_INET,"136.159.5.25",&mServer.sin_addr);
    socklen_t len = sizeof(mServer);

    // Create socket for communicating with microservers
    if((clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("Error creating socket\n");
        exit(1);
    }

    // Set the timer for UDP connections at 1 second
    if(setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) == -1) {
        printf("Setsockopt failed\n");
        exit(1);
    }


    /* Connect to telnet client */

    // Initialize server sockaddr structure
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(CLIENTPORTNUM);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // set up the transport-level end point to use TCP
    if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
	    fprintf(stderr, "Server socket() call failed\n");
	    exit(1);
    }

    // bind a specific address and port to the end point
    if(bind(serverSocket, (struct sockaddr *)&server, sizeof(struct sockaddr_in) ) == -1 ) {
        fprintf(stderr, "Server bind() call failed\n");
	    exit(1);
    }

    // start listening for incoming connections from clients
    if(listen(serverSocket, 5) == -1 ) {
	    fprintf(stderr, "Server listen() call failed\n");
	    exit(1);
    }

    printf("Listening for clients on port %d...\n", CLIENTPORTNUM);

    // Loop forever listening for clients and forking upon connection
    while (1) {

        if((client = accept(serverSocket, (struct sockaddr*)&clientAddr, &cLen)) != -1 ) {

            pid = fork();

            if(pid < 0) {
                printf("fork() call failed!\n");
                exit(1);
            }

            // Child process deals with client
            else if (pid == 0) {

                // Get client's IP address
                char * clientIP = inet_ntoa(clientAddr.sin_addr);

                close(serverSocket);

                // Present menu
                strcpy(msgOut, "\nWelcome! We have three services for you:\n1. An English-French translator (command <translate>)\n2. A currency converter (command <convert>)\n3. A voting service (command <vote>)\n\nPlease make your selection.\n>> ");
                send(client, msgOut, MSGLEN, 0);
                bzero(msgOut, MSGLEN);
                recv(client, msgIn, MSGLEN, 0);
                c = strtok(msgIn, "\r\n");
                strcpy(command, c);

                // Some simple input validation
                while ((strcasecmp(command, "translate") != 0) && (strcasecmp(command, "convert") != 0) && (strcasecmp(command, "vote") != 0)) {
                    bzero(msgIn, MSGLEN);
                    bzero(msgOut, MSGLEN);
                    bzero(command, 20);
                    strcpy(msgOut,"Please enter a valid command (translate, convert, or vote)\n>> ");
                    send(client, msgOut, MSGLEN, 0);
                    recv(client, msgIn, MSGLEN, 0);
                    c = strtok(msgIn, "\r\n");
                    strcpy(command, c);
                }

                // Loop with the client until they quit the program
                while (strcasecmp(command, "exit") != 0) {

                    printf("\nClient requested service %s\n", msgIn);
                    bzero(msgIn, MSGLEN);
                    bzero(msgOut, MSGLEN);

                    // Client requested translator service
                    if (strcasecmp(command, "translate") == 0) {

                        // Present menu and get English word
                        strcpy(msgOut,"You can translate the following words to French: \n\"Hello\", \"Goodbye\", \"Computer\", \"Ostrich\", \"Wine\"\n> Enter an English word: ");
                        send(client, msgOut, MSGLEN, 0);
                        recv(client, word, sizeof(word), 0);
                        c = strtok(word, "\r\n");
                        strcpy(word, c);
                        printf("Client chose to convert %s to French\n", word);

                        // Send word to microserver and receive response
                        mServer.sin_port = htons(MSERVER1);
                        if(sendto(clientSocket, word, sizeof(word), 0, (const struct sockaddr *) &mServer, sizeof(mServer)) < 0) {
                            bzero(msgOut, MSGLEN);
                            strcpy(msgOut, "This service is temporarily unavailable. Please try again later.\n");
                            send(client, msgOut, MSGLEN, 0);
                            printf("Failed to send to microserver.\n");
                        }
                        if(recvfrom(clientSocket, msgIn, MSGLEN, MSG_WAITALL, (struct sockaddr*)&mServer, &len) > 0) {

                            // UDP worked - send response back to client
                            printf("Received response from translator: %s\nForwarding to client...\n", msgIn);
                            bzero(msgOut, MSGLEN);
                            strcpy(msgOut, "> French translation: ");
                            strcat(msgOut, msgIn);
                            strcat(msgOut, "\n");
                            send(client, msgOut, MSGLEN, 0);
                        } else {
                            bzero(msgOut, MSGLEN);
                            strcpy(msgOut, "This service is temporarily unavailable. Please try again later.\n");
                            send(client, msgOut, MSGLEN, 0);
                            printf("Failed to receive from microserver.\n");
                        }
                    }

                    // Client chose converter service
                    else if (strcasecmp(command, "convert") == 0) {

                        // Present menu and get input
                        strcpy(msgOut,"You can convert between any of the following currencies: \nCAD, USD, EUR, GBP, BTC\nUse the following format:\n>> <amount> <source currency> <dest currency>\n>> ");
                        send(client, msgOut, MSGLEN, 0);
                        recv(client, msgIn, MSGLEN, 0);
                        bzero(msgOut, MSGLEN);
                        c = strtok(msgIn, "\r\n");
                        strcpy(msgOut, c);
                        strcpy(msg, msgOut);
                        printf("Client requested %s\n", msgOut);

                        // Send input to microserver
                        mServer.sin_port = htons(MSERVER2);
                        bzero(msgIn, MSGLEN);
                        if(sendto(clientSocket, msgOut, sizeof(msgOut), 0, (const struct sockaddr *) &mServer, sizeof(mServer)) == -1) {
                            bzero(msgOut, MSGLEN);
                            strcpy(msgOut, "This service is temporarily unavailable. Please try again later.\n");
                            send(client, msgOut, MSGLEN, 0);
                            printf("Failed to send to microserver.\n");
                        } else {
                            if (recvfrom(clientSocket, msgIn, MSGLEN, MSG_WAITALL, (struct sockaddr*)&mServer, &len) != -1) {

                                // UDP worked - send response back to client
                                c = strtok(msg, " ");
                                value = c;
                                c = strtok(NULL, " ");
                                source = c;
                                c = strtok(NULL, " ");
                                dest = c;
                                
                                printf("Received response from currency converter: %s\nForwarding to client...\n", msgIn);
                                bzero(msgOut, MSGLEN);
                                sprintf(msgOut, "%s %s is %s %s\n", value, source, msgIn, dest);
                                send(client, msgOut, MSGLEN, 0);
                            } else {
                                bzero(msgOut, MSGLEN);
                                strcpy(msgOut, "This service is temporarily unavailable. Please try again later.\n");
                                send(client, msgOut, MSGLEN, 0);
                                printf("Failed to receive from microserver.\n");
                            }
                            
                        }

                    }

                    // Client requested voting service
                    else if (strcasecmp(command, "vote") == 0) {

                        // Present menu and get next command
                        strcpy(msgOut,"\nYou can choose any of the following:\n>> show\n>> vote\n>> summary\n\n>> ");
                        send(client, msgOut, MSGLEN, 0);
                        bzero(msgIn, MSGLEN);
                        recv(client, msgIn, MSGLEN, 0);
                        bzero(msgOut, MSGLEN);
                        c = strtok(msgIn, "\r\n");
                        strcpy(msg, c);
                        printf("Client requested %s\n", msg);

                        // Send client command and IP address to microserver
                        mServer.sin_port = htons(MSERVER3);
                        bzero(msgIn, MSGLEN);

                        // Loop within the voting service until client chooses to exit
                        while(strcasecmp(msg, "exit") != 0) {

                            bzero(msgOut, MSGLEN);
                            sprintf(msgOut, "%s %s", msg, clientIP);
                            bzero(msgIn, MSGLEN);

                            // Send command and IP address to microserver
                            if(sendto(clientSocket, msgOut, sizeof(msgOut), 0, (const struct sockaddr *) &mServer, sizeof(mServer)) == -1) {
                                bzero(msgOut, MSGLEN);
                                strcpy(msgOut, "This service is temporarily unavailable. Please try again later.\n");
                                send(client, msgOut, MSGLEN, 0);
                                printf("Failed to send to microserver.\n");
                            } else {
                                if (recvfrom(clientSocket, msgIn, MSGLEN, MSG_WAITALL, (struct sockaddr*)&mServer, &len) == -1) {
                                    bzero(msgOut, MSGLEN);
                                    strcpy(msgOut, "This service is temporarily unavailable. Please try again later.\n");
                                    send(client, msgOut, MSGLEN, 0);
                                    printf("Failed to receive from microserver.\n");
                                }
                            }

                            // Next steps depend on client command

                            // Client requested show
                            if(strstr(msg, "show") != NULL) {
                                printf("Received list of candidates, forwarding to client...\n", msgIn);
                                bzero(msgOut, MSGLEN);
                                strcpy(msgOut, msgIn);
                                send(client, msgOut, MSGLEN, 0);
                            }
                            
                            // Client requested vote
                            else if(strstr(msg, "vote") != NULL) {
                                // Microserver response is "N" if user has voted already
                                if (strcmp(msgIn, "N") == 0) {
                                    printf("This client has already voted\n");
                                    bzero(msgOut, MSGLEN);
                                    strcpy(msgOut, "You can only vote once.\n");
                                    send(client, msgOut, MSGLEN, 0);
                                } else {
                                    // Microserver returns encryption key...get candidate ID from client
                                    printf("Encryption key is %s\n", msgIn);
                                    key = atoi(msgIn);
                                    strcpy(msgOut, "Enter the ID of the candidate you would like to vote for: ");
                                    send(client, msgOut, MSGLEN, 0);
                                    bzero(msgIn, MSGLEN);
                                    bzero(msgOut, MSGLEN);
                                    recv(client, msgIn, MSGLEN, 0);
                                    clientVote = atoi(msgIn);
                                    clientVote = clientVote * key;
                                    sprintf(msgOut, "%d", clientVote);

                                    // Send encrypted vote to microserver
                                    if(sendto(clientSocket, msgOut, sizeof(msgOut), 0, (const struct sockaddr *) &mServer, sizeof(mServer)) == -1) {
                                        bzero(msgOut, MSGLEN);
                                        strcpy(msgOut, "This service is temporarily unavailable. Please try again later.\n");
                                        send(client, msgOut, MSGLEN, 0);
                                        printf("Failed to send to microserver.\n");
                                    } else {
                                        if (recvfrom(clientSocket, msgIn, MSGLEN, MSG_WAITALL, (struct sockaddr*)&mServer, &len) == -1) {
                                            bzero(msgOut, MSGLEN);
                                            strcpy(msgOut, "This service is temporarily unavailable. Please try again later.\n");
                                            send(client, msgOut, MSGLEN, 0);
                                            printf("Failed to receive from microserver.\n");
                                        } else {
                                            // Vote counted or not
                                            if(strcmp(msgIn, "vote counted") == 0) {
                                                printf("Vote counted\n");
                                            } else {
                                                printf("Vote not counted\n");
                                                bzero(msgOut, MSGLEN);
                                                strcpy(msgOut, "Your vote could not be processed. Please try again later.\n");
                                                send(client, msgOut, MSGLEN, 0);
                                            }
                                        }
                                    }
                                }

                            // Client requested summary
                            } else if(strstr(msg, "summary") != NULL) {
                                if(strcmp(msgIn, "N") == 0) {
                                    // Client can't see summary yet
                                    strcpy(msgOut, "You can't see the election results until you have voted\n");
                                    send(client, msgOut, MSGLEN, 0);
                                } else {
                                    // Send summary to client
                                    printf("Received summary results, forwarding to client...\n", msgIn);
                                    bzero(msgOut, MSGLEN);
                                    strcpy(msgOut, msgIn);
                                    send(client, msgOut, MSGLEN, 0);
                                }
                                
                            }

                            // Ask for next instruction
                            bzero(msgOut, MSGLEN);
                            strcpy(msgOut,"\nYou can choose any of the following:\n>> show\n>> vote\n>> summary\n>> exit\n\n>> ");
                            send(client, msgOut, MSGLEN, 0);
                            bzero(msgIn, MSGLEN);
                            recv(client, msgIn, MSGLEN, 0);
                            bzero(msgOut, MSGLEN);
                            c = strtok(msgIn, "\r\n");
                            strcpy(msg, c);
                        }

                    }

                    // Ask for and verify next command
                    bzero(msgOut, MSGLEN);
                    strcpy(msgOut, "\nPlease choose another service (translate, convert, vote) or enter \"exit\" to leave:\n>> ");
                    send(client, msgOut, MSGLEN, 0);
                    bzero(msgOut, MSGLEN);
                    recv(client, msgIn, MSGLEN, 0);
                    c = strtok(msgIn, "\r\n");
                    strcpy(command, c);

                    while ((strcasecmp(command, "translate") != 0) && (strcasecmp(command, "convert") != 0) && (strcasecmp(command, "vote") != 0) && (strcasecmp(command, "exit") != 0)) {
                        bzero(msgIn, MSGLEN);
                        bzero(msgOut, MSGLEN);
                        bzero(command, 20);
                        strcpy(msgOut,"Please enter a valid command (translate, convert, vote, or exit)\n>> ");
                        send(client, msgOut, MSGLEN, 0);
                        recv(client, msgIn, MSGLEN, 0);
                        c = strtok(msgIn, "\r\n");
                        strcpy(command, c);
                    }
                }

                strcpy(msgOut, "Thank you for your time.\n");
                send(client, msgOut, MSGLEN, 0);
                close(client);
                exit(1);
            }

            else if (pid > 0) {
                close(client);
            }
            
        }
    }
    close(client);
    close(serverSocket);
    return 0;
}
