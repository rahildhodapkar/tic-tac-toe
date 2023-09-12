#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

#define PORT 15000

volatile int sigRun;
// these two functions are the same as ttts
void signalHandler(int sigNum) {
    if (sigNum == SIGINT || sigNum == SIGTERM) {
        sigRun = 1;
    }
}

char *receiveMessage(int sock) {
    static char buffer[150];
    memset(buffer, 0, sizeof(buffer));

    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        perror("recv");
        return NULL;
    }
    buffer[bytes] = '\0';
    return buffer;
}

/**
 * This function is responsible for sending and receiving the main bulk of messages. It parses through received messages, determines whether it is
 * a resignation, draw, or move, and appropriately handles each case. 
*/
int loop(int sock, char role) {
    int isValid = 0;
    while (isValid == 0 && !sigRun) {
        // receive message
        char *recvMsg = receiveMessage(sock), *token;
        char buffer[128], sendMsg[150];
        memset(buffer, 0, sizeof(buffer));
        memset(sendMsg, 0, sizeof(sendMsg));
        // print out the received message
        fprintf(stdout, "%s\n", recvMsg);
        token = strtok(recvMsg, "|");
        // if we receive OVER, we know we can just terminate
        if (strcmp(token, "OVER") == 0) {
            return 1;
        } else if (strcmp(token, "DRAW") == 0) {
            token = strtok(NULL, "|");
            token = strtok(NULL, "|");
            // this handles if an opponent suggests a draw. We prompt the user to enter y to accept the draw, or n to decline
            if (strcmp(token, "S") == 0) {
                fprintf(stdout, "Your opponent is suggesting a draw, type y to accept, or n to decline:\n");
                int bytes = read(0, buffer, sizeof(buffer) - 1);
                if (bytes < 0) {
                    perror("ERROR: read");
                    return 1;
                }
                buffer[bytes] = '\0';
                if (buffer[0] == 'y') {
                    send(sock, "DRAW|2|A|", 9, 0);
                } else {
                    send(sock, "DRAW|2|R|", 9, 0);
                }
            } else if (strcmp(token, "R") == 0) {
                // we print out a message letting our user know that their draw request was denied. We then prompt them to enter a new command
                // this snippet of code is present several times in the function, hence we will explain it only once
                // It prints out a prompt to either enter coordinates, q to resign, or d to suggest a draw
                fprintf(stdout, "Draw is rejected\n");
                fprintf(stdout, "Please either enter a valid move [x,y], q to resign, or d to offer a draw:\n");
                int bytes = read(0, buffer, sizeof(buffer) - 1);
                if (bytes < 0) {
                    perror("ERROR: read");
                    return 1;
                }
                buffer[bytes] = '\0';
                // if q, we simply send a resign message
                if (buffer[0] == 'q') {
                    send(sock, "RSGN|0|", 7, 0);
                } else if (buffer[0] == 'd') { // if d, we send a draw suggestion message, and let the user know that it is being sent
                    fprintf(stdout, "Sending draw request\n");
                    send(sock, "DRAW|2|S|", 9, 0);
                } else { // otherwise, we send the coordinates. we are not afraid of dealing with invalid commands, as ttts handles nonsense messages
                    snprintf(sendMsg, 150, "MOVE|6|%c|%s|", role, buffer);
                }
                send(sock, sendMsg, strlen(sendMsg), 0);
                return 0;
            }
        } else if (strcmp(token, "MOVD") == 0) { // this means we received a move update
            token = strtok(NULL, "|");
            token = strtok(NULL, "|");
            char check = token[0];
            // if the movd message is showing our own move, we just skip
            if (check == role) {
                return 0;
            } else {
                fprintf(stdout, "Please either enter a valid move [x,y], q to resign, or d to offer a draw:\n");
                int bytes = read(0, buffer, sizeof(buffer) - 1);
                if (bytes < 0) {
                    perror("ERROR: read");
                    return 1;
                }
                buffer[bytes] = '\0';
                if (buffer[0] == 'q') {
                    send(sock, "RSGN|0|", 8, 0);
                } else if (buffer[0] == 'd') {
                    fprintf(stdout, "Sending draw request\n");
                    send(sock, "DRAW|2|S|", 10, 0);
                } else {
                    snprintf(sendMsg, 150, "MOVE|6|%c|%s|", role, buffer);
                }
                send(sock, sendMsg, strlen(sendMsg), 0);
                return 0;
            }
        }  else {
            fprintf(stdout, "Please either enter a valid move [x,y], q to resign, or d to offer a draw:\n");
                    int bytes = read(0, buffer, sizeof(buffer) - 1);
                    if (bytes < 0) {
                        perror("ERROR: read");
                        return 1;
                    }
                    buffer[bytes] = '\0';
                    if (buffer[0] == 'q') {
                        send(sock, "RSGN|0|", 7, 0);
                    } else if (buffer[0] == 'd') {
                        fprintf(stdout, "Sending draw request\n");
                        send(sock, "DRAW|2|S|", 9, 0);
                    } else {
                        snprintf(sendMsg, 150, "MOVE|6|%c|%s|", role, buffer);
                    }
                    send(sock, sendMsg, strlen(sendMsg), 0);
                    return 0;
        }
    }
    // if we ever exit out of the loop, something went wrong
    fprintf(stderr, "Message error, terminating\n");
    return 1;
}
/**
 * This is the function that initializes the connection to ttts. Again, some of the code is based off of the source code our professor gave us
 * We also establish the names of the players, which role they are, and we also start the first move. If you are X, you have the first move,
 * otherwise if you are O, you wait until X makes a move. 
*/
void serverConnect(char *domain, char *port) {
    struct sigaction act;
    act.sa_handler = signalHandler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    struct addrinfo hints, *infoList, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int sock, error;
    error = getaddrinfo(domain, port, &hints, &infoList);
    if (error != 0) {
        perror("Error");
        return;
    }
    for (info = infoList; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock < 0) {
            continue;
        }
        error = connect(sock, info->ai_addr, info->ai_addrlen);
        if (error != 0) {
            perror("\n");
            close(sock);
            continue;
        }
        break;
    }
    if (info == NULL) {
        fprintf(stderr, "Connection failure\n");
        return;
    }
    freeaddrinfo(infoList);
    // welcome message
    fprintf(stdout, "Welcome! Please enter your name (64 character limit) to begin:\n");
    char name[128], buffer[150];
    int bytes = read(0, name, sizeof(name) - 1);
    if (bytes < 0) {
        perror("ERROR: read");
        return;
    }
    for (int i = 0; name[i] != '\0'; i++) {
        if (name[i] == '\n') {
            name[i] = '\0';
        }
    }
    // we take care of the PLAY message for the client. All they do is enter their name.
    snprintf(buffer, 150, "PLAY|%ld|%s|", strlen(name), name);
    printf("NAME|%ld|%s|\n", strlen(name), name);
    send(sock, buffer, strlen(buffer), 0);
    char *recvMsg = receiveMessage(sock);
    fprintf(stdout, "%s\n", recvMsg);
    char *token = strtok(recvMsg, "|");
    // if the name is invalid, we go into an input loop that will only exit when a valid name is entered
    if (strcmp(token, "INVL") == 0) {
        int isValid = 0;
        while (isValid == 0) {
            memset(name, 0, sizeof(name));
            memset(recvMsg, 0, sizeof(char) * 150);
            memset(buffer, 0, sizeof(buffer));
            fprintf(stdout, "Please enter a valid name:\n");
            bytes = read(0, name, sizeof(name) - 1);
            if (bytes < 0) {
                perror("ERROR: read");
                return;
            }
            for (int i = 0; name[i] != '\0'; i++) {
                if (name[i] == '\n') {
                    name[i] = '\0';
                }
            }
            snprintf(buffer, 150, "PLAY|%ld|%s|", strlen(name), name);
            fprintf(stdout, "NAME|%ld|%s|\n", strlen(name), name);
            send(sock, buffer, strlen(buffer), 0);
            recvMsg = receiveMessage(sock);
            fprintf(stdout, "%s\n", recvMsg);
            char *t = strtok(recvMsg, "|");
            if (strcmp(t, "INVL") == 0) {
                continue;
            } else {
                isValid = 1;
            }
        }
    }
    memset(recvMsg, 0, sizeof(char) *150);
    recvMsg = receiveMessage(sock);
    fprintf(stdout, "%s", recvMsg);
    char *t = strtok(recvMsg, "|");
    t = strtok(NULL, "|");
    t = strtok(NULL, "|");
    char role;
    // sets the appropriate role
    if (strcmp(t, "X") == 0) {
        role = 'X';
    } else {
        role = 'O';
    }
    fprintf(stdout, "\nYou are player %c\n", role);
    char buffer1[128], sendMsg[150];
    memset(sendMsg, 0, sizeof(sendMsg));
    memset(buffer1, 0, sizeof(buffer1));
    // if you are x, you get to input a move
    if (role == 'X') {
        fprintf(stdout, "You go first, please input the coordinates of your move [x,y]:\n");
        int bytes = read(0, buffer1, sizeof(buffer) - 1);
        if (bytes < 0) {
            perror("ERROR: read");
            return;
        }
        buffer1[bytes] = '\0';
        snprintf(sendMsg, 150, "MOVE|6|X|%s|", buffer1);
        send(sock, sendMsg, strlen(sendMsg), 0);
    } else {
        fprintf(stdout, "You go second, please wait until a move is sent by your opponent\n");
    }
    // if loop() ever returns 1, we terminate 
    while (!sigRun) {
        if((loop(sock, role)) == 1) {
            break;
        }
    }
    close(sock);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Error: no port number and/or domain name\n");
        return 1;
    }
    int portTest = atoi(argv[2]);
    if (portTest < 0 || portTest > 65535) {
        fprintf(stderr, "Error: invalid port number\n");
        return 1;
    }
    serverConnect(argv[1], argv[2]);
    return 0;
}