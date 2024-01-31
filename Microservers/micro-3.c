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

#define PORTNUM 8552
#define MSGLEN 3000

/* Main program for voting service
Implements the functionality of the voting service. Creates a UDP socket and listens for data, then reads it in 
the format "command IP". If the command is "vote", it checks that the IP address has not been used to
vote before, and sends an encryption key before receiving/counting the vote and updating the voter list. If it 
is "summary", it checks that the IP address has already voted. If it is "show", it sends back a list of the candidates.
*/

int main() {

    // Create socket for UDP data
    int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (sockfd == -1) {
        printf("Socket() call failed\n");
        exit(1);
    }

    char list[300] = "\nThe candidates are:\nBen Smith (ID 1)\nJessica Narwhal (ID 2)\nKimberly Johnson (ID 3)\nTristan Roberts (ID 4)\n\n";
    char msgIn[MSGLEN] = {0};
    char msgOut[MSGLEN] = {0};
    char key[5] = "5";
    char *c;
    char command[20];
    char summary[MSGLEN];
    int clientVote;
    char voterList[10][20];
    int numVoted = 0;
    int voted = 0;
    char * clientIP;
    char cIP[20] = {0};

    // for each candidate, track ID and number of votes
    int candidates[4][2];

    for (int i = 1; i < 5; i++) {
        candidates[i-1][0] = i;
    }

    // Set votes for each candidate
    candidates[0][1] = 34221;
    candidates[1][1] = 4566;
    candidates[2][1] = 34221;
    candidates[3][1] = 14504;

    // initialize and bind socket
    struct sockaddr_in sock;
    memset(&sock,0,sizeof(sock));
    sock.sin_family = AF_INET;
    sock.sin_port = htons(PORTNUM);
    sock.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd,(struct sockaddr*)&sock,sizeof(sock)) == -1) {
        printf("Bind() call failed\n");
        exit(1);
    }

    struct sockaddr_in client;
    memset(&client,0,sizeof(client));
    socklen_t len = sizeof(client);

    // Loop for UDP data
    while (1) {

        voted = 0;
        printf("Listening...\n");
        bzero(msgIn, MSGLEN);
        recvfrom(sockfd, msgIn, MSGLEN, MSG_WAITALL, (struct sockaddr*)&client, &len);
        printf("Received \"%s\"\n", msgIn);

        // input is one of "vote", "show", "summary" with IP address

        if(strstr(msgIn, "vote") != NULL) {
            // Check that they haven't voted already
            clientIP = msgIn + 5;
            for (int i = 0; i < numVoted; i++) {
                printf("Checking IP %s against list item %s\n", clientIP, voterList[i]);
                if (strcmp(voterList[i], clientIP) == 0) {
                    voted = 1;
                    break;
                }
            }
            if (voted) {
                // Don't send encryption key
                printf("This client has already voted\n\n");
                bzero(msgOut, MSGLEN);
                strcpy(msgOut, "N");
                sendto(sockfd, msgOut, strlen(msgOut), 0, (const struct sockaddr *) &client, sizeof(client));
            } else {
                // Send encryption key
                strcpy(voterList[numVoted], clientIP);
                numVoted++;
                printf("Sending encryption key back\n\n");
                sendto(sockfd, key, strlen(key), 0, (const struct sockaddr *) &client, sizeof(client));

                recvfrom(sockfd, msgIn, MSGLEN, MSG_WAITALL, (struct sockaddr*)&client, &len);
                // Count vote and record IP address of voter
                printf("Counting vote\n\n");
                clientVote = atoi(msgIn);
                clientVote = clientVote / atoi(key);
                candidates[clientVote - 1][1]++;
                bzero(msgOut, MSGLEN);
                strcpy(msgOut, "vote counted");
                sendto(sockfd, msgOut, strlen(msgOut), 0, (const struct sockaddr *) &client, sizeof(client));
            }
            
        } else if (strstr(msgIn, "show") != NULL) {
            // Send back list of candidates
            printf("Sending list of candidates back\n\n");
            sendto(sockfd, list, strlen(list), 0, (const struct sockaddr *) &client, sizeof(client));
        } else if (strstr(msgIn, "summary") != NULL) {

            // Check that client has already voted
            clientIP = msgIn + 8;
            for (int i = 0; i < numVoted; i++) {
                printf("Checking IP %s against list item %s\n", clientIP, voterList[i]);
                if (strcmp(voterList[i], clientIP) == 0) {
                    voted = 1;
                    break;
                }
            }

            if (voted) {
                // Send back results
                printf("Sending voting results back\n\n");
                sprintf(summary, "\nHere are the results:\nBen Smith has %d votes\nJessica Narwhal has %d votes\nKimberly Johnson has %d votes\nTristan Roberts has %d votes\n\n", candidates[0][1], candidates[1][1], candidates[2][1], candidates[3][1]);
                sendto(sockfd, summary, strlen(summary), 0, (const struct sockaddr *) &client, sizeof(client));
            } else {
                // Don't send back results
                printf("This client hasn't voted yet\n\n");
                bzero(msgOut, MSGLEN);
                strcpy(msgOut, "N");
                sendto(sockfd, msgOut, strlen(msgOut), 0, (const struct sockaddr *) &client, sizeof(client));
            }
        }

    }

    close(sockfd);
    return 0;
}