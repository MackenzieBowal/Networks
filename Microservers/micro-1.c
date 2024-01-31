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

#define PORTNUM 8725
#define MSGLEN 3000

/* Main program for translator
Implements the functionality of the English-French translator. Creates a UDP socket and listens for data, then reads it in 
the format "word". Translates the word, then sends back the French equivalent to the client.
*/

int main() {

    // Create UDP socket
    int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (sockfd == -1) {
        printf("Socket() call failed\n");
        exit(1);
    }

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

    char msgIn[MSGLEN];
    char msgOut[MSGLEN];
    bzero(msgIn, MSGLEN);
    bzero(msgOut, MSGLEN);

    // Loop listening for data
    while (1) {

        printf("Listening...\n");

        recvfrom(sockfd, msgIn, MSGLEN, MSG_WAITALL, (struct sockaddr*)&client, &len);

        printf("Translating %s...\n", msgIn);

        // Translate the input word
        if (strcasecmp(msgIn, "Hello") == 0) {
            strcpy(msgOut, "Bonjour");
        } else if (strcasecmp(msgIn, "Goodbye") == 0) {
            strcpy(msgOut, "Au revoir");
        } else if (strcasecmp(msgIn, "Computer") == 0) {
            strcpy(msgOut, "Ordinateur");
        } else if (strcasecmp(msgIn, "Ostrich") == 0) {
            strcpy(msgOut, "Autruche");
        } else if (strcasecmp(msgIn, "Wine") == 0) {
            strcpy(msgOut, "Vin");
        } else {
            strcpy(msgOut, "Undefined");
        }

        // Send response back
        sendto(sockfd, msgOut, sizeof(msgOut), 0, (const struct sockaddr *) &client, sizeof(client));

        printf("Translation sent back\n");
    }
    
    close(sockfd);
    return 0;
}