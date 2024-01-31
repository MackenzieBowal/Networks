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
#include <math.h>
#include <sys/time.h>

#define PORTNUM 9571
#define MSGLEN 3000

/* Main program for currency converter
Implements the functionality of the converter. Creates a UDP socket and listens for data, then reads it in 
the format "amount source dest". If the source and dest currencies are the same, it simply returns the original
value. If they are different, it converts the source amount to CAD and then to the destination currency. Sends
back the return value as a float to the same client.
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

    char msgIn[MSGLEN] = {0};
    char msgOut[MSGLEN];
    char *v;
    float value = 0;
    char *source;
    char *dest;
    char *c;
    float result;
    float valueCopy;

    // Loop for data
    while (1) {

        printf("Listening...\n");

        recvfrom(sockfd, msgIn, MSGLEN, MSG_WAITALL, (struct sockaddr*)&client, &len);
        printf("Converting %s...\n", msgIn);
        
        // Parse string into amount, source, dest
        c = strtok(msgIn, " ");
        v = c;
        value = atof(v);
        c = strtok(NULL, " ");
        source = c;
        c = strtok(NULL, " ");
        dest = c;
        strcpy((dest + 3), "\0");
        strcpy((dest + 4), "\0");

        valueCopy = value;

        // Convert to CAD
        if (strcasecmp(source, "USD") == 0) {
            value = 1.23 * value;
        } else if (strcasecmp(source, "EUR") == 0) {
            value = 1.44 * value;
        } else if (strcasecmp(source, "GBP") == 0) {
            value = 1.70 * value;
        } else if (strcasecmp(source, "BTC") == 0) {
            value = 82198.67 * value;
        }

        // Convert to dest currency
        if(strcasecmp(dest, "CAD") == 0) {
            result = 1.0;
        } else if(strcasecmp(dest, "USD") == 0) {
            result = 0.81;
        } else if(strcasecmp(dest, "EUR") == 0) {
            result = 0.70;
        } else if(strcasecmp(dest, "GBP") == 0) {
            result = 0.59;
        } else if(strcasecmp(dest, "BTC") == 0) {
            result = 0.000013;
        }

        // Calculate result
        result = result * value;

        if (strcasecmp(source, dest) == 0) {
            result = valueCopy;
        }

        // Send back to client
        sprintf(msgOut, "%.2f", result);
        printf("The result is %s...sending back\n", msgOut);

        sendto(sockfd, msgOut, sizeof(msgOut), 0, (const struct sockaddr *) &client, sizeof(client));

    }

    close(sockfd);
    return 0;
}