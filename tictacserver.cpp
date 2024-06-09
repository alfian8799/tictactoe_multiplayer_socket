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
#include <iphlpapi.h>
#include <atomic>
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

std::atomic<bool> running(true);

std::vector<std::pair<std::string, std::string>> getIPAddressFromIpConfig() {
    std::vector<std::pair<std::string, std::string>> adapterList;

    // Execute ipconfig command and get its output
    std::string ipConfigOutput;
    std::string ipConfigCommand = "ipconfig";
    FILE* pipe = _popen(ipConfigCommand.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error executing ipconfig command." << std::endl;
        return adapterList;
    }
    char buffer[128];
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            ipConfigOutput += buffer;
    }
    _pclose(pipe);

    // Parse the ipconfig output
    std::istringstream iss(ipConfigOutput);
    std::string line;
    std::vector<std::string> tupleTemp;

    while (std::getline(iss, line)) {
        if (line.find("Wireless LAN adapter") != std::string::npos || line.find("Ethernet adapter") != std::string::npos) {
            tupleTemp.clear();
            if (line.find("Wireless LAN adapter") != std::string::npos) {
                std::string adapterName = line.substr(line.find("Wireless LAN adapter") + 20);
                tupleTemp.push_back(adapterName);
            }
        }
        if (line.find("IPv4 Address") != std::string::npos) {
            std::string ip = line.substr(line.find(":") + 2);
            if (tupleTemp.size() == 1) {
                tupleTemp.push_back(ip);
                adapterList.push_back(std::make_pair(tupleTemp[0], tupleTemp[1]));
            }
        }
    }

    return adapterList;
}

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
                failedMoveMessage =  "Sudah terisi, pilih yang lain!";
                failedMove = true;
            }
        } else {
            failedMoveMessage = "Silakan pilih angka antara 1-9!";
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
                << ", " << checkCurrentPlayerName() << ", menang" << std::endl;
            } else {
                std::cout << std::endl << "Player #" << checkOppositePlayerID()
                << ", " << checkOppositePlayerName() << ", menang" << std::endl;
            }
            // printf("A straight line at square %d, %d, and %d!\n",
            // winningSquares[0], winningSquares[1], winningSquares[2]);
        } else if (drawState){
            std::cout << std::endl << "Tidak ada ruang lagi, seri!" << std::endl << std::endl;
        }
    }

    void printCheckPlayer(){
        printf("Saat ini giliran Pemain #%d (%s)!\n", checkCurrentPlayerID(), checkCurrentPlayerName().c_str());
    }

    void printAdditionalInfo(){
        if (failedMove){
            std::cout << failedMoveMessage << std::endl;
            failedMove = false;
        }
    }

    void printTitle(){
        std::cout << "TicTacToe" << std::endl << std::endl;
        printf("Player 1 #%d (%s): %s   /   Player 2 #%d (%s): %s \n\n", 
        firstPlayer.getID(), firstPlayer.getName().c_str(), firstPlayer.getSymbol().c_str(),
        secondPlayer.getID(), secondPlayer.getName().c_str(), secondPlayer.getSymbol().c_str());
    }

    void printBoard(){
        std::cout << "|-----------------|" << std::endl;
        printf("|   %c    %c    %c   |\n", chr(1), chr(2), chr(3));
        std::cout << "|-----------------|" << std::endl;
        printf("|   %c    %c    %c   |\n", chr(4), chr(5), chr(6));
        std::cout << "|-----------------|" << std::endl;
        printf("|   %c    %c    %c   |\n", chr(7), chr(8), chr(9));
        std::cout << "|-----------------|" << std::endl << std::endl;
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
int serverPlayerID;

void receiveMessages(SOCKET ClientSocket) {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;

    while (running) {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
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
            if (publicBoard.checkCurrentPlayerID() == serverPlayerID) {
                if (!publicBoard.isGameOver()){
                    std::cout << std::endl;
                    publicBoard.printTitle();
                    publicBoard.printBoard();
                    publicBoard.printCheckPlayer();
                    publicBoard.printAdditionalInfo(); // tell if the player fails to mark before
                    std::cout << "Masukan nomor untuk menempatkan tanda: ";
                } else {
                    publicBoard.printWinner(true);
                }
            }
            // std::cout << "Enter message to send (type 'exit' to quit): ";
        } else if (iResult == 0) {
            std::cout << "\nConnection closing..." << std::endl;
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

    serverPlayerID = player1.getID();

    std::vector<std::pair<std::string, std::string>> ipAddresses = getIPAddressFromIpConfig();

    if (ipAddresses.empty()) {
        std::cerr << "No active IP address found." << std::endl;
        return 1;
    }

    std::cout << "IP Addresses of Wireless LAN Adapters:" << std::endl;
    for (const auto& adapter : ipAddresses) {
        std::cout << "Adapter Name: " << adapter.first << ", IP Address: " << adapter.second << std::endl;
    }

    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
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
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Start a thread to receive messages from the client
    std::thread recvThread(receiveMessages, ClientSocket);

    // Loop to send messages repeatedly
    while (running) {
        if (!publicBoard.isGameOver()){
            std::cout << std::endl;
            publicBoard.printTitle();
            publicBoard.printBoard();
            publicBoard.printCheckPlayer();
            publicBoard.printAdditionalInfo(); // tell if the player fails to mark before
            if (publicBoard.checkCurrentPlayerID() == serverPlayerID) {
                std::cout << "Ketik nomor untuk menempatkan tanda : ";
            } else {
                std::cout << "Tunggu, giliran pemain lain    ";
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

        if (publicBoard.checkCurrentPlayerID() == serverPlayerID) {
            publicBoard.placeMark(sendbuf);
            publicBoard.checkWinner();
            publicBoard.checkDraw();
            int iSendResult = send(ClientSocket, sendbuf.c_str(), (int)sendbuf.length(), 0);
            if (iSendResult == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                running = false;
                break;
            }
        } else {

        }
    }

    // Cleanup
    recvThread.join();
    closesocket(ClientSocket);
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}
