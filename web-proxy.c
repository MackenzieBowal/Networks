/*
This program is a web censorship proxy that interacts with a blocking client (to specify
censored words) and one or more browser clients. It receives HTTP requests from the browser
and checks the URL for inappropriate content (as defined by the blocking client) before forwarding
the request on to the web server. If the URL is inappropriate, it is replaced by a request for the 
error page instead. The web server's response is then sent back to the browser.
An additional feature (checking html for inappropriate content) has also been implemented, and can be turned on and
off by setting Bonus to 1 or 0.
Works best on Firefox with http (not https)
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

// Global constants
#define MSG_LENGTH 3000
#define WEB_CLIENT_PORT 9001
#define TEL_PORT 9000
#define SERVER_PORT 80

// Struct for dynamically updating censorship
struct censored_words {
    int numWords;
    char badWords[10][50];
} BadList;

// Set to 1 for use of the bonus feature, 0 otherwise
int Bonus = 1;

int handleClientRequest(char URL[300], char msgIn[MSG_LENGTH], char errorURL[100], int replaceURL, int clientSocket);
int checkCensorUpdates(int telSocket, int telServerSocket);
int doBonus(char * webCode, char webServerReply[MSG_LENGTH], char msgIn[MSG_LENGTH], char errorURL[100], int proxyClientSocket);

/* Main program for proxy
Implements most of the functionality of the proxy, and calls the other functions when necessary.
It starts by connecting to the blocking client, then creates a server socket to listen for browser clients.
The program then loops, alternating updating the list of censored words and checking for new browser clients.
When a browser client connects, the program forks into a child process which manages the browser connection.
*/
int main() {

    BadList.numWords = 0;
    char msgIn[MSG_LENGTH];
    char msgOut[MSG_LENGTH];
    char errorURL[100] = "http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/error.html";
    struct sockaddr_in server, telServer;
    int clientSocket, proxyServerSocket; 
    int telSocket, telServerSocket;
    int bytesReceived, pid;
    char webServerReply[MSG_LENGTH];

    // timeval struct for setting socket timers
    struct timeval timeOut;
    timeOut.tv_usec = 10;
    timeOut.tv_sec = 0;

    // initialize message strings just to be safe
    bzero(msgIn, MSG_LENGTH);
    bzero(msgOut, MSG_LENGTH);


    /* Connect to the telnet client */

    // Initialize telServer structure
    memset(&telServer, 0, sizeof(telServer));
    telServer.sin_family = AF_INET;
    telServer.sin_port = htons(TEL_PORT);
    telServer.sin_addr.s_addr = htonl(INADDR_ANY);

    // set up server socket for telnet client
    if((telServerSocket = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) {
	    fprintf(stderr, "telServer socket() call failed\n");
	    exit(1);
    }

    // bind a specific address and port to the end point
    if(bind(telServerSocket, (struct sockaddr *)&telServer, sizeof(struct sockaddr_in) ) == -1 ) {
        fprintf(stderr, "telServer bind() call failed\n");
	    exit(1);
    }

    // start listening for a connection from a client
    if(listen(telServerSocket, 5) == -1 ) {
	    fprintf(stderr, "telServer listen() call failed\n");
	    exit(1);
    }

    printf("\nServer for blocking client listening on port %d...\n\n", TEL_PORT);

    // Accept the telnet client
    if((telSocket = accept(telServerSocket, NULL, NULL)) == -1) {
        printf("telServer accept() call failed\n");
        exit(1);
    }

    strcpy(msgOut, "\nWelcome! You can block up to 10 censored words. Here are the supported commands:\nBLOCK <word>\tSet <word> as censored\nUNBLOCK\t\tClear the list of censored words\n\n>> ");
    send(telSocket, msgOut, MSG_LENGTH, 0);

    // Set the timer for recv() to 10 usec
    if (setsockopt(telSocket, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(struct timeval)) == -1) {
        printf("setsockopt failed\n");
        exit(1);
    }


    /* Create the web client socket */

    // Initialize server sockaddr structure
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(WEB_CLIENT_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // set up the transport-level end point to use TCP
    if((proxyServerSocket = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) {
	    fprintf(stderr, "Server socket() call failed\n");
	    exit(1);
    }

    // bind a specific address and port to the end point
    if(bind(proxyServerSocket, (struct sockaddr *)&server, sizeof(struct sockaddr_in) ) == -1 ) {
        fprintf(stderr, "Server bind() call failed\n");
	    exit(1);
    }

    // start listening for incoming connections from clients
    if(listen(proxyServerSocket, 5) == -1 ) {
	    fprintf(stderr, "Server listen() call failed\n");
	    exit(1);
    }

    // Set the timer for the accept() call to 10 microseconds
    if (setsockopt(proxyServerSocket, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(struct timeval)) == -1) {
        printf("setsockopt failed\n");
        exit(1);
    }

    fprintf(stderr, "Server for web client listening on TCP port %d...\n\n", WEB_CLIENT_PORT);
    
    // Main loop: server loops forever listening for requests and updating censored words
    while(1) {

        checkCensorUpdates(telSocket, telServerSocket);

        if((clientSocket = accept(proxyServerSocket, NULL, NULL)) != -1 ) {
		
	        // start another process if a browser client was accepted
            pid = fork();

            if(pid < 0) {
                printf("fork() call failed!\n");
                exit(1);
            } 
            // Child process (deal with client)
            else if (pid == 0) {

		        // Parent listening socket no longer needed
                close(proxyServerSocket);
                bytesReceived = recv(clientSocket, msgIn, MSG_LENGTH, 0);

                printf("Connected to web client\n");

                if (bytesReceived > 0) {
                    printf("Client request: %s\n", msgIn);
                }

		        // Parse request for URL
                char inCopy[MSG_LENGTH];
                char * splitMessage;
                char URL[300];
                memcpy(inCopy, msgIn, MSG_LENGTH);
                splitMessage = strtok(inCopy, " ");
                splitMessage = strtok(NULL, " ");
                strcpy(URL, splitMessage);

		        // Check URL for bad content and update request if necessary
                handleClientRequest(URL, msgIn, errorURL, 0, clientSocket);

		        // Parse request for the host name
                char * hostName = (strstr(msgIn, "Host: ")+6);
                memcpy(inCopy, hostName, MSG_LENGTH);
                hostName = strtok(inCopy, "\r\n");
                
                // Create a socket for communicating with server
                int proxyClientSocket;
                proxyClientSocket = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in webServer;

		        // Initialize sockaddr structure by using the host name
                struct hostent * he;
                he = gethostbyname(hostName);
                webServer.sin_addr.s_addr = *((unsigned long *) he->h_addr_list[0]);
                webServer.sin_family = AF_INET;
                webServer.sin_port = htons(SERVER_PORT);

                // Connect to the web server
                if (connect(proxyClientSocket, (struct sockaddr *)&webServer, sizeof(webServer)) < 0)
                {
                    printf("Client connect() call failed\n");
                    exit(1);
                }
                
                printf("Sending client request to server...\n");

		        // Send request to web server
                if (send(proxyClientSocket, msgIn, strlen(msgIn), 0) < 0) {
                    printf("Client send() call failed\n");
                    exit(1);
                }

                //printf("Web server response: ");

		        // Receive server's reply
                bzero(webServerReply, MSG_LENGTH);
                bytesReceived = recv(proxyClientSocket, webServerReply, MSG_LENGTH, 0);
                //printf("%s\n", webServerReply);

                // Bonus:
                if (Bonus) {
			        // Check that the URL is not already the error page
                    if (strcmp(URL, errorURL) != 0) {
			            // Check that the received message is an html file
                        char *contentType = strstr(webServerReply, "text/html");
                        if (contentType != NULL) {
				            // Get the html code separate from the headers and check it for bad content, updating the 
				            // request if necessary
                            char *webCode = strstr(contentType, "<html>");
                            doBonus(webCode, webServerReply, msgIn, errorURL, proxyClientSocket);

                            // Connect to web server using a new socket
                            int pCS = socket(AF_INET, SOCK_STREAM, 0);
                            struct sockaddr_in wS;
                            struct hostent * hb;
                            hb = gethostbyname(hostName);
                            wS.sin_addr.s_addr = *((unsigned long *) hb->h_addr_list[0]);
                            wS.sin_family = AF_INET;
                            wS.sin_port = htons(SERVER_PORT);
 
                            if (connect(pCS, (struct sockaddr *)&wS, sizeof(wS)) < 0)
                            {
                                printf("Client connect() call failed\n");
                                exit(1);
                            }
            			    // Send updated request to the web server
                            int num = send(pCS, msgIn, strlen(msgIn), 0);
                            if (num <= 0) {
                                printf("Error sending to web server\n");
                                exit(1);
                            }
                            bzero(webServerReply, MSG_LENGTH);
			                // Receive new web server response
                            num = recv(pCS, webServerReply, MSG_LENGTH, 0);
                            if (num <= 0) {
                                printf("Error receiving from web server\n");
                                exit(1);
                            }
                        }
                    }
                }
                
		// Check the size of the server response
                char *content = strstr(webServerReply, "Content-Length: ");
                if (content != NULL) {

                    int size = atoi(content+16);
                    int count = bytesReceived;
                    printf("Sending response back to web client\n");

                    // Loop for the whole image/website
                    while (count < size) {
                        send(clientSocket, webServerReply, bytesReceived, 0);
                        bzero(webServerReply, MSG_LENGTH);
                        bytesReceived = recv(proxyClientSocket, webServerReply, MSG_LENGTH, 0);
                        count += bytesReceived;
                    }
                }

                // Send bytes from the last/only recv call
                send(clientSocket, webServerReply, MSG_LENGTH, 0);

                printf("Finished sending to web client, closing socket\n\n");

                bytesReceived = 0;
                bzero(msgIn, strlen(msgIn));
                bzero(msgOut, strlen(msgOut));

                // Close child socket and terminate process
                close(clientSocket);
                exit(0);
            }
	    // Parent process does this part
	    else {
		// Close unecessary child socket
            close(clientSocket);
            }
        } 
    }
    close(proxyServerSocket);
    return 0;
}

/* checkCensorUpdates
 * Calls the recv() function on the telnet (blocking) socket to check if any updates to
 * the censored words list are available. If the response is "BLOCK <word>", then <word> is added
 * to the censored words list. If the response is "UNBLOCK", then the list of censored words is 
 * zeroed. If the response is anything else, nothing happens.
 */

int checkCensorUpdates(int telSocket, int telServerSocket) {

    char recvMsg[MSG_LENGTH];
    char sendMsg[MSG_LENGTH];
    bzero(recvMsg, MSG_LENGTH);
    bzero(sendMsg, MSG_LENGTH);

    // Check for updates
    if (recv(telSocket, recvMsg, MSG_LENGTH, 0) != -1) {

	// Parse the incoming command
        char * splitMsg = strtok(recvMsg, " ");
        char * command = splitMsg;
        splitMsg = strtok(NULL, " ");
        char * word = splitMsg;
        word = strtok(word, "\n\r");

        // Command was "BLOCK"
        if (strcmp(command, "BLOCK") == 0) {
            strcpy(BadList.badWords[BadList.numWords], word);
            BadList.numWords++;
        }
        // Command was "UNBLOCK"
        else {
            command = strtok(command, "\n\r");
            if (strcmp(command, "UNBLOCK") == 0) {
                for (int i = 0; i < 10; i++) {
                    strcpy(BadList.badWords[i], "");
                }
                BadList.numWords = 0;
            }
        }

        strcpy(sendMsg, ">> ");
        send(telSocket, sendMsg, MSG_LENGTH, 0);
    }
}

/* handleClientRequest
 * Takes the URL in the web client request and scans it for bad content. If the URL is inappropriate, the function splits the incoming
 * request and replaces the old URL with the error page URL.
 */

int handleClientRequest(char URL[300], char msgIn[MSG_LENGTH], char errorURL[100], int replaceURL, int clientSocket) {

    printf("Checking the URL\n");

    // Check each word in the bad words list
    for (int i = 0; i < BadList.numWords; i++) {

	// Replace the URL if it has a bad word or the calling code wants it explicitly replaced
        if (strstr(URL, BadList.badWords[i]) != NULL || replaceURL) {

            strcpy(URL, errorURL);

	    // Splits the incoming request into the command, the URL, and the rest
            char * restOfMsg = strstr(msgIn, " HTTP");
            char * splitMsg = strtok(msgIn, " ");
            char request[MSG_LENGTH];
            strcpy(request, splitMsg);
            
            // End child process if the request is anything other than "GET"
            if (strcmp(request, "GET") != 0) {
                close(clientSocket);
                exit(0);
            }

	    // Combine the request pieces back into a single request with the URL replaced
            strcat(request, " ");
            strcat(request, URL);
            strcat(request, restOfMsg);
            
            memcpy(msgIn, request, MSG_LENGTH);
        }
    }
    return 0;
}


/* doBonus
 * Checks the html code of a web server response for bad content. If it contains censored words, the request is modified to request the
 * error page instead.
 */


int doBonus(char * webCode, char webServerReply[MSG_LENGTH], char msgIn[MSG_LENGTH], char errorURL[100], int proxyClientSocket) {

    // check html code for bad words
    for (int i = 0; i < BadList.numWords; i++) {
        if (strstr(webCode, BadList.badWords[i]) != NULL) {
            
            printf("The bad word detected was %s\n", BadList.badWords[i]);

            // send to handleClientRequest with msgIn & replaceURL = 1
            char bURL[300] = "N/A";
            handleClientRequest(bURL, msgIn, errorURL, 1, proxyClientSocket);

        }
    }

    return 0;
}