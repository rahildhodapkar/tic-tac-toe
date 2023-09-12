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
#include <pthread.h>

#define PORT 15000

volatile int sigRun;

void signalHandler(int sigNum) {
    if (sigNum == SIGINT || sigNum == SIGTERM) {
        sigRun = 1;
    }
}

/**
 * This is our function we use to receive messages from the server, just makes our code look a little more neat
*/
char *receiveMessage(int socket) {
    static char buffer[150];
    memset(buffer, 0, sizeof(buffer));

    int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        perror("recv");
        return NULL;
    }
    buffer[bytes] = '\0';
    return buffer;
}

/**
 * This function is responsible for game logic
 * It starts by figuring out the names of each player. If a player inputs an invalid name, then the while loop continuously prompts the client to 
 * enter a proper name. When the client does, the validInput flag is set, allowing us to proceed with the rest of the code. This while loop also sends 
 * the play message.
 * In the second while loop, this is where we have the actual game logic. This is where we parse through the message, checking which type of command it is.
 * If it is a resignation, we send the appropriate message to each player. If it is a move, we ensure that the coordinates are valid. And if it is a draw,
 * we properly handle that message as well here.
*/
int gameHandler(int clS_X, int clS_O, char *board) {
    char name_X[64] = {0};
    char name_O[64] = {0};
    for (int i = 0; i < 2; i++) {
        int currSock, validInput = 0;
        if (i == 0) {
            currSock = clS_X;
        } else {
            currSock = clS_O;
        }
        while (validInput == 0) {
            char *recvMsg = receiveMessage(currSock);
            if (recvMsg == NULL) {
                return 1;
            } else {
                const char temp[2] = "|";
                char *token = strtok(recvMsg, temp);
                if (strcmp(token, "PLAY") == 0) {
                    token = strtok(NULL, temp); 
                    token = strtok(NULL, temp); 
                    if (strlen(token) < 64) { 
                        if (i == 0) {
                            strcpy(name_X, token);
                        } else {
                            strcpy(name_O, token);
                        }
                        if ((strcmp(name_X, name_O) == 0) && currSock == clS_O){
                            send(currSock, "INVL|38|Must use different name than opponent|", 47, 0);
                            memset(name_O, 0, sizeof(char) * 64);
                            continue;
                        } 
                        validInput = 1;
                        send(currSock, "WAIT|0|", 7, 0); 
                    } else {
                        send(currSock, "INVL|17|Name is too long|", 24, 0); 
                    } 
                }
            }
        }
    }
    // Send BEGN statement
    char destX[150], destO[150];
    snprintf(destX, 150, "BEGN|%ld|X|%s|", strlen(name_O) + 2, name_O);
    snprintf(destO, 150, "BEGN|%ld|O|%s|", strlen(name_X) + 2, name_X);
    send(clS_X, destX, strlen(destX), 0);
    send(clS_O, destO, strlen(destO), 0);

    int gameStatus = 0; // if the game is still running, gameStatus is 0.
    char currentPlayer = 'X'; // by default, currentPlayer is set to x
    int currSock = 0, oppSock = 0, drawRejected = 0;
    int win = 0;
    int start = 0;
    while (gameStatus == 0 && !sigRun) {
        int currSock, oppSock;
        // if a draw is rejected, this flag is set to 1, allowing us to skip the code that switches the currSock and oppSock
        // this allows the player who suggested the draw to then continue with their turn
        if (drawRejected != 1) { 
            if (start != 0) {currentPlayer = currentPlayer == 'X' ? 'O' : 'X';}
            if (currentPlayer == 'X') {
                currSock = clS_X;
                oppSock = clS_O;
            } else {
                currSock = clS_O;
                oppSock = clS_X;
            }
        }
        // this start flag just sets whether the game has started or not
        start++;
        char buffer[150];
        memset(buffer, 0, sizeof(buffer));
        int isValid = 0;
        // loop that does not exit till a valid move is made
        while (isValid == 0) {
            char *recvMsg = receiveMessage(currSock);
            char *token = strtok(recvMsg, "|");
            if (strcmp(token, "MOVE") == 0) {
                int x, y;
                token = strtok(NULL, "|");
                token = strtok(NULL, "|");
                if (strchr(token, currentPlayer) == NULL) {
                    send(currSock, "INVL|29|Please enter a valid command|", 37, 0);
                    continue;
                }
                token = strtok(NULL, "|");
                int xFlag = 0;
                // simple parsing loop that extracts coordinates
                for (int i = 0; token[i] != '\0'; i++) {
                    if (token[i] != ' ' && token[i] != ',') {
                        if (xFlag == 0) {
                            x = token[i] - '0';
                            xFlag = 1;
                        } else {
                            y = token[i] - '0';
                            break;
                        }
                    }
                }
                // valid check of coordinates
                if (x < 1 || x > 3 || y > 3) {
                    send(currSock, "INVL|29|Please enter a valid command|", 38, 0);
                    continue;
                }
                // minus one, as arrays start at 0
                y--;
                x--;
                // equation to calculate corresponding 1D array index
                int index = x * 3 + y;
                if (board[index] != 'X' && board[index] != 'O') {
                    board[index] = currentPlayer;
                } else {
                    send(currSock, "INVL|29|Please enter a valid command|", 38, 0);
                    continue;
                }
                // next three for-loops check whether there is a win horizontally, vertically, or diagonally
                for (int i = 0; i < 3; i++) {
                    if (board[i * 3] != '.' &&
                        board[i * 3] == board[i * 3 + 1] &&
                        board[i * 3] == board[i * 3 + 2]) {
                        win = 1;
                        break;
                    }
                }
                if (win == 0) {
                    for (int i = 0; i < 3; i++) {
                        if (board[i] != '.' &&
                            board[i] == board[3 + i] &&
                            board[i] == board[2 * 3 + i]) {
                            win = 1;
                            break;
                        }
                    }
                }
                if (win == 0) {
                    if (board[0] != '.' && board[0] == board[4] && board[0] == board[8]) {
                        win = 1;
                    } else if (board[2] != '.' && board[2] == board[4] && board[2] == board[6]) {
                        win = 1;
                    }
                }
                // we increment the win by 1 if a win is detected
                if (win == 1) {
                    char buffer[150];
                    if (currentPlayer == 'X') {
                        snprintf(buffer, 150, "OVER|%ld|W|%s has won|", 10 + strlen(name_X), name_X);
                        send(currSock, buffer, strlen(buffer), 0);
                        snprintf(buffer, 150, "OVER|%ld|L|%s has won|", 10 + strlen(name_X), name_X);
                        send(oppSock, buffer, strlen(buffer), 0);
                    } else {
                        snprintf(buffer, 150, "OVER|%ld|W|%s has won|", 10 + strlen(name_O), name_O);
                        send(currSock, buffer, strlen(buffer), 0);
                        snprintf(buffer, 150, "OVER|%ld|L|%s has won|", 10 + strlen(name_O), name_O);
                        send(oppSock, buffer, strlen(buffer), 0);
                    }
                    isValid = 1;
                    gameStatus = 1;
                    return 1;
                }
                // a draw flag is initialized. if we are at this point, there are no winners, however there may be a full board,
                // so we just check if there is by incrementing draw for each non-empty space. if draw is 9, we know that the board is full
                int draw = 0;
                for (int i = 0; i < 9; i++) {
                    if (board[i] != '.') {
                        draw++;
                    }
                }
                if (draw == 9) {
                    send(currSock, "OVER|14|D|Board is full|", 25, 0);
                    send(oppSock, "OVER|14|D|Board is full|", 25, 0);
                    isValid = 1;
                    gameStatus = 1;
                    return 1;
                }
                char buffer[150];
                snprintf(buffer, 150, "MOVD|%d|%c|%d,%d|%s|", 18, currentPlayer, x + 1, y + 1, board);
                send(currSock, buffer, strlen(buffer), 0);
                send(oppSock, buffer, strlen(buffer), 0);
                isValid = 1;
            } else if (strcmp(token, "RSGN") == 0) {
                char buffer[150];
                send(currSock, "OVER|20|L|You have resigned|", 29, 0);
                if (currentPlayer == 'X') {
                    snprintf(buffer, 150, "OVER|%ld|W|%s has resigned|", strlen(name_X) + 2 + 13, name_X);
                } else {
                    snprintf(buffer, 150, "OVER|%ld|W|%s has resigned|", strlen(name_O) + 2 + 13, name_O);
                }
                send(oppSock, buffer, strlen(buffer), 0);
                isValid = 1;
                gameStatus = 1;
            } else if (strcmp(token, "DRAW") == 0) {
                token = strtok(NULL, "|");
                token = strtok(NULL, "|");
                if (strcmp(token, "S") == 0) {
                    send(oppSock, "DRAW|2|S|", 10, 0);
                    isValid = 1;
                } else if (strcmp(token, "A") == 0) {
                    send(currSock, "OVER|15|D|Draw accepted|", 25, 0);
                    send(oppSock, "OVER|15|D|Draw accepted|", 25, 0);
                    isValid = 1;
                    gameStatus = 1;
                    return 1;
                } else {
                    // this goes back to the drawRejected flag, which we set to 1, indicating that the player who sent the draw should still be currSock
                    send(oppSock, "DRAW|2|R|", 10, 0);
                    drawRejected = 1;
                    isValid = 1;
                }
            } else {
                // anything else is regarded as a nonsensical message
                send(currSock, "INVL|29|Please enter a valid command|", 38, 0);
            }
        }
        drawRejected = 0;
        win = 0;
    }
    close(currSock);
    close(oppSock);
    return 1;
}
/**
 * This function initializes the server.
 * Some of this code is based off of source code provided by our professor
 * For the most part, nothing very out of the ordinary is happening here
 * We tried to experiment with setsockopt to allow the server to always be run on the same terminal with no bind issues, but we were met with failure
*/
void serverInitialize(char *port) {
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
    hints.ai_flags = AI_PASSIVE;
    int e, s;
    e = getaddrinfo("0.0.0.0", port, &hints, &infoList);
    for (info = infoList; info != NULL; info = info->ai_next) {
        s = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (s == -1) {
            continue;
        }
        printf("Socket established...\n");
        e = bind(s, info->ai_addr, info->ai_addrlen);
        if (e) {
            close(s);
            continue;
        }
        printf("Bind established...\n");
        e = listen(s, SOMAXCONN);
        if (e) {
            close(s);
            continue;
        }
        printf("Listening successfully...\n");
        break;
    }
    freeaddrinfo(infoList);
    if (info == NULL) {
        fprintf(stderr, "Oh no, we cannot bind!\n");
        return;
    }
    struct sockaddr_storage clLen;
    socklen_t adLen = sizeof(clLen);
    int clS_X, clS_O;
    // we malloc a 1d board array here, and fill it with "."
    char *board = malloc(sizeof(char) * 10);
    for (int i = 0; i < 9; i++) {
        board[i] = '.';
    }
    board[9] = '\0';
    while (!sigRun) {
        for (int i = 0; i < 2; i++) {
            int clS = accept(s, (struct sockaddr *)&clLen, &adLen);
            printf("Accepted...\n");
            if (clS == -1) {
                perror("accept");
                continue;
            }
            if (i == 0) {
                clS_X = clS;
                printf("Connection with Player X established...\n");
            } else {
                clS_O = clS;
                printf("Connection with Player O established...\n");
            }
        }
        printf("Starting game...\n");
        // if gameHandler ever returns 1, we know that either there was an error, or the game is finished. For both cases, we simply terminate the code.
        if ((gameHandler(clS_X, clS_O, board)) == 1) {
            break;
        }
    }
    close(s);
    free(board);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: no port number\n");
        return 1;
    }
    int portTest = atoi(argv[1]);
    // google said that this is the range of valid port numbers
    if (portTest < 0 || portTest > 65535) {
        fprintf(stderr, "Error: invalid port number\n");
        return 1;
    }
    printf("Initializing server...\n");
    serverInitialize(argv[1]);
    printf("Terminating server...\n");
    return 0;
}
