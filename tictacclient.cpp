#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
#elif _WIN32_WINNT < 0x0600
    #undef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
#endif

#include <iostream>
#include "mingw.thread.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <vector>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

std::atomic<bool> running(true);

class Player {
    public:
    Player(){};
    void initPlayer(std::string tobename, int tobeid, std::string tobesymbol){
        name = tobename;
        id = tobeid;
        symbol = tobesymbol;
    }

    Player(std::string tobename, int tobeid, std::string tobesymbol){
        initPlayer(tobename, tobeid, tobesymbol);
    }

    std::string getSymbol(){ return symbol;}
    std::string getName(){ return name;}
    int getID(){return id;}

    private:
    std::string name;
    int id;
    std::string symbol;
};

class TicTacToeBoard {
    public:
    TicTacToeBoard(){}
    TicTacToeBoard(Player& firstP, Player& secondP){
        firstPlayer.initPlayer(firstP.getName(), firstP.getID(),firstP.getSymbol());
        secondPlayer.initPlayer(secondP.getName(), secondP.getID(),secondP.getSymbol());
        initialized = true;
    }

    bool isInitialized(){ return initialized; }
    int getSquareValueFromNumber(int squareNumber){
        // from left to right, top to bottom, 1 to 9
        return squareValues[(squareNumber - 1)]; // because of zero index
    }

    // check the owner of a square, and display the mark if any
    char chr(int squareNumber){
        int squareValue = getSquareValueFromNumber(squareNumber);
        std::string temp;
        switch(squareValue){
            case 0:
                // display the square number if there's no mark
                temp = std::to_string(squareNumber);
                break;
            case 1:
                temp = firstPlayer.getSymbol(); // First player own this square
                break;
            case 2:
                temp = secondPlayer.getSymbol(); // Second player own this square    
                break;
        };
        auto printedchar = temp.c_str()[0];
        return printedchar;
    }

    bool isGameOver(){
        if (winningState || drawState) return true;
        else return false;
    }

    bool checkSingleDigitInput(const std::string &input){;
    if (input.length() != 1) return false; // Input has more than one character

    char c = input[0];
    if (c >= '1' && c <= '9') return true; // Comparing ascii value
    else return false;
    }

    void togglePlayerTurn(){
        playerTurn = (playerTurn == 1) ? 2 : 1; // toggle between player 1 and 2
    }

    void placeMark(std::string inputNumber){
        if (checkSingleDigitInput(inputNumber)){
            int squareNumber = stoi(inputNumber); // std::string to Integer
            int squareValue = getSquareValueFromNumber(squareNumber);
            if (squareValue == 0){
                // the selected square is now marked by the current player
                squareValues[(squareNumber - 1)] = playerTurn; 
                togglePlayerTurn();
                failedMove = false;
            } else { // placing mark on occupied square
                failedMoveMessage = "That space is already occupied, choose another!";
                failedMove = true;
            }
        } else {
            failedMoveMessage = "Please choose number between 1-9!";
            failedMove = true; // input isn't between 1-9
        }
    }

    void checkLine(int firstSquare, int secondSquare, int thirdSquare){
        if (!winningState) {
            int square1st_val = getSquareValueFromNumber(firstSquare);
            int square2nd_val = getSquareValueFromNumber(secondSquare);
            int square3rd_val = getSquareValueFromNumber(thirdSquare);
            if (square1st_val == square2nd_val
                && square2nd_val == square3rd_val
                && (square1st_val +square2nd_val + square3rd_val) != 0){ // it's not an empty row
                    winningState = true;
                    winningSquares = {firstSquare, secondSquare, thirdSquare};
            }
        }
    }

    void checkWinner(){
        checkLine(1,2,3); // top row
        checkLine(4,5,6); // middle row
        checkLine(7,8,9); // bottom row

        checkLine(1,4,7); // left column
        checkLine(2,5,8); // middle column
        checkLine(3,6,9); // right column

        checkLine(1,5,9); // diagonal top-left to bottom-right
        checkLine(3,5,7); // diagonal top-right to bottom-left
    }

    void checkDraw(){
        if (!winningState){
            int checkDraw = 1;
            for (int v: squareValues) { // iterate on every square
                // draw in tictactoe will happen if the board is full but no line
                // if one of the square isn't marked, then the board is not full
                if (v == 0){
                    checkDraw = 0;
                    break;
                }
            }
            if (checkDraw == 1) drawState = true;
        }
    }

    std::string checkCurrentPlayerName() {
        std::string playername = (playerTurn == 1) ? firstPlayer.getName() : secondPlayer.getName();
        return playername;
    }

    std::string checkOppositePlayerName() {
        std::string playername = (playerTurn == 1) ? secondPlayer.getName() : firstPlayer.getName();
        return playername;
    }

    int checkCurrentPlayerID() {
        int playerID = (playerTurn == 1) ? firstPlayer.getID() : secondPlayer.getID();
        return playerID;
    }

    int checkOppositePlayerID() {
        int playerID = (playerTurn == 1) ? secondPlayer.getID() : firstPlayer.getID();
        return playerID;
    }

    void printWinner(bool opposite = false){
        if (winningState){
            if (!opposite) {
                std::cout << std::endl << "Player #" << checkCurrentPlayerID()
                << ", " << checkCurrentPlayerName() << ", is the winner!" << std::endl;
            } else {
                std::cout << std::endl << "Player #" << checkOppositePlayerID()
                << ", " << checkOppositePlayerName() << ", is the winner!" << std::endl;
            }
            printf("A straight line at square %d, %d, and %d!\n",
            winningSquares[0], winningSquares[1], winningSquares[2]);
        } else if (drawState){
            std::cout << std::endl << "No more space left, it's a draw!" << std::endl << std::endl;
        }
    }

    void printCheckPlayer(){
        printf("Currently it's Player #%d (%s's) turn!\n", checkCurrentPlayerID(), checkCurrentPlayerName().c_str());
    }

    void printAdditionalInfo(){
        if (failedMove){
            std::cout << failedMoveMessage << std::endl;
            failedMove = false;
        }
    }

    void printTitle(){
        std::cout << "Ether's TicTacToe" << std::endl << std::endl;
        printf("Player #%d (%s): %s   /   Player #%d (%s): %s \n\n", 
        firstPlayer.getID(), firstPlayer.getName().c_str(), firstPlayer.getSymbol().c_str(),
        secondPlayer.getID(), secondPlayer.getName().c_str(), secondPlayer.getSymbol().c_str());
    }

    void printBoard(){
        std::cout << "||||   ||   ||   ||   ||||" << std::endl;
        printf("||||   %c    %c    %c    ||||\n", chr(1), chr(2), chr(3));
        printf("||||   %c    %c    %c    ||||\n", chr(4), chr(5), chr(6));
        printf("||||   %c    %c    %c    ||||\n", chr(7), chr(8), chr(9));
        std::cout << "||||   ||   ||   ||   ||||" << std::endl << std::endl;
    }

    private:
    bool initialized = false;
    int playerTurn = 1; // 1 means first player's turn, 2 means second player's
    bool failedMove = false;
    bool winningState = false;
    bool drawState = false;
    std::string failedMoveMessage;
    Player firstPlayer;
    Player secondPlayer;

    // the number is player who marked the square currently
    // 1 for player 1 (@), 2 for player 2 (#)
    // 0 means no one marked the square yet
    std::vector<int> squareValues = {0,0,0,0,0,0,0,0,0};
    // list of squares that make a line for the winner
    std::vector<int> winningSquares = {0,0,0};
};

TicTacToeBoard publicBoard;
int clientPlayerID;

void receiveMessages(SOCKET ConnectSocket) {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;

    while (running) {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            std::string message = std::string(recvbuf, iResult);
            // std::cout << "Receive mark?";
            if (publicBoard.isInitialized()){
                // if (publicBoard.checkSingleDigitInput(message)){
                //     publicBoard.chan
                // }
                // std::cout << "Receiving mark...";
                publicBoard.placeMark(message);
                publicBoard.checkWinner();
                publicBoard.checkDraw();
            }
            std::cout << "\nReceived: " << message << std::endl;
            if (publicBoard.checkCurrentPlayerID() == clientPlayerID) {
                if (!publicBoard.isGameOver()){
                    std::cout << std::endl;
                    publicBoard.printTitle();
                    publicBoard.printBoard();
                    publicBoard.printCheckPlayer();
                    publicBoard.printAdditionalInfo(); // tell if the player fails to mark before
                    std::cout << "Type a number to put your mark there: ";
                } else {
                    publicBoard.printWinner(true);
                }
            }
            // std::cout << "Enter message to send (type 'exit' to quit): ";
        } else if (iResult == 0) {
            std::cout << "\nConnection closed" << std::endl;
            running = false;
            break;
        } else {
            std::cerr << "\nrecv failed: " << WSAGetLastError() << std::endl;
            running = false;
            break;
        }
    }
}

int main() {
    std::string markNumber;
    Player player1("ServerPlayer", 1234, "O");
    Player player2("ClientPlayer", 6969, "X");
    TicTacToeBoard board(player1, player2);
    publicBoard = board;

    clientPlayerID = player2.getID();

    std::string customip;
    std::cout << "Insert Server's IP address (keep empty for localhost) \n > ";
    std::getline(std::cin, customip);
    if (customip.empty()) { customip = "127.0.0.1"; }

    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    int iResult;

    std::string sendbuf;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(customip.c_str(), DEFAULT_PORT, &hints, &result); // customip.c_str()
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // Attempt to connect to the first address returned by the call to getaddrinfo
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

    // Start a thread to receive messages from the server
    std::thread recvThread(receiveMessages, ConnectSocket);

    // Loop to send messages repeatedly
    while (running) {
        if (!publicBoard.isGameOver()){
            std::cout << std::endl;
            publicBoard.printTitle();
            publicBoard.printBoard();
            publicBoard.printCheckPlayer();
            publicBoard.printAdditionalInfo(); // tell if the player fails to mark before
            if (publicBoard.checkCurrentPlayerID() == clientPlayerID) {
                std::cout << "Type a number to put your mark there: ";
            } else {
                std::cout << "Wait for other player's turn     ";
            }
        } else {
            publicBoard.printWinner(true);
        }
        // std::cout << "Enter message to send (type 'exit' to quit): ";
        std::getline(std::cin, sendbuf);

        if (sendbuf == "exit") {
            running = false;
            break;
        }

        if (publicBoard.checkCurrentPlayerID() == clientPlayerID) {
            // std::cout << "Placing mark...";
            publicBoard.placeMark(sendbuf);
            publicBoard.checkWinner();
            publicBoard.checkDraw();
            int iResult = send(ConnectSocket, sendbuf.c_str(), (int)sendbuf.length(), 0);
            if (iResult == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                running = false;
                break;
            }
        } else {

        }
    }

    // Cleanup
    recvThread.join();
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
