// server.cpp
// This program listens for a TCP connection. If a client sends the "screenshot" command,
// it then receives the subsequent PNG file and saves it as "received_screenshot.png".

#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <string>
#include <vector>

// Link the Winsock library (ws2_32.lib) for network functions.
#pragma comment(lib, "ws2_32.lib") 

using namespace std;

const int PORT = 4444;            // The port on which the server will listen.
const int BUFFER_SIZE = 4096;     // The size of the buffer used for receiving data.

/**
 * @brief A helper function to ensure all specified bytes are received from the socket.
 * TCP is a stream protocol, so a single send() call by the client might require
 * multiple recv() calls on the server to get all the data.
 * @param sock The client socket to receive data from.
 * @param buf The buffer to store the received data.
 * @param len The total number of bytes to receive.
 * @return The number of bytes received, or a negative value on error.
 */
int recv_all(SOCKET sock, char* buf, int len) {
    int total_received = 0;
    while (total_received < len) {
        // Receive data into the buffer at the correct offset.
        int received = recv(sock, buf + total_received, len - total_received, 0);
        if (received <= 0) {
            // An error occurred or the connection was closed.
            return received; 
        }
        total_received += received;
    }
    return total_received;
}


int main() {
    // 1. INITIALIZE WINSOCK
    WSADATA wsaData; // Structure to hold Winsock implementation details.
    SOCKET listenSock, clientSock; // Sockets for listening and for the client connection.
    sockaddr_in serverAddr = {}, clientAddr = {}; // Address structures for server and client.
    int clientAddrSize = sizeof(clientAddr);

    cout << "Initializing Winsock..." << endl;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return -1;
    }

    // 2. CREATE A LISTENING SOCKET
    cout << "Creating listening socket..." << endl;
    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // AF_INET = IPv4, SOCK_STREAM = TCP
    if (listenSock == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup(); // Clean up Winsock.
        return -1;
    }

    // 3. BIND THE SOCKET TO AN IP ADDRESS AND PORT
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address.
    serverAddr.sin_port = htons(PORT);       // Convert port number to network byte order.

    cout << "Binding socket to port " << PORT << "..." << endl;
    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed." << endl;
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }

    // 4. LISTEN FOR INCOMING CONNECTIONS
    cout << "Listening for incoming connections..." << endl;
    listen(listenSock, 1); // Listen, with a backlog of 1 pending connection.

    // 5. ACCEPT A CLIENT CONNECTION
    // This is a blocking call; the program will wait here until a client connects.
    clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSock == INVALID_SOCKET) {
        cerr << "Accept failed." << endl;
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }
    cout << "Client connection accepted." << endl;
    
    // The listening socket is no longer needed as we are only handling one client.
    closesocket(listenSock); 

    // 6. RECEIVE THE COMMAND FROM THE CLIENT
    char cmdBuf[32] = {};
    // Receive data, leaving one byte for the null terminator.
    int cmdLen = recv(clientSock, cmdBuf, sizeof(cmdBuf) - 1, 0); 
    if (cmdLen <= 0) {
        cerr << "Could not receive command or connection was closed." << endl;
    } else {
        string command(cmdBuf, cmdLen); // Convert the received buffer to a string.
        cout << "Received command: " << command << endl;

        // 7. IF COMMAND IS "screenshot", PROCEED WITH FILE RECEPTION
        if (command == "screenshot") {
            cout << "Waiting for file transfer..." << endl;
            long long fileSize = 0;

            // 7a. First, receive the file size (as a long long, 8 bytes).
            // We use recv_all to ensure we get all 8 bytes reliably.
            int received_size = recv_all(clientSock, (char*)&fileSize, sizeof(fileSize));

            if (received_size != sizeof(fileSize)) {
                cerr << "Error: Did not receive full file size. Received: " << received_size << " bytes." << endl;
            } else {
                cout << "Receiving file of size: " << fileSize << " bytes." << endl;
                
                // 7b. Open the output file in binary mode.
                ofstream outFile("received_screenshot.png", ios::binary);
                if (!outFile) {
                    cerr << "Error: Could not create file received_screenshot.png." << endl;
                } else {
                    char buffer[BUFFER_SIZE];
                    long long totalReceived = 0;
                    
                    // 7c. Loop until the entire file is received.
                    while (totalReceived < fileSize) {
                        // Calculate how many bytes to receive in this iteration.
                        int bytesToReceive = min((long long)BUFFER_SIZE, fileSize - totalReceived);
                        int bytes = recv(clientSock, buffer, bytesToReceive, 0);
                        
                        if (bytes <= 0) {
                            cerr << "Connection lost while receiving file." << endl;
                            break; // Exit the loop on error or disconnection.
                        }
                        
                        outFile.write(buffer, bytes); // Write the received chunk to the file.
                        totalReceived += bytes;
                    }

                    outFile.close(); // Close the file stream.

                    if (totalReceived == fileSize) {
                        cout << "File received successfully and saved as 'received_screenshot.png'." << endl;
                    } else {
                        cerr << "Error: File received is incomplete. Expected: " << fileSize << ", Received: " << totalReceived << endl;
                    }
                }
            }
        } else {
            cout << "Unknown command: " << command << endl;
        }
    }

    // 8. CLEAN UP
    cout << "Closing connection." << endl;
    closesocket(clientSock); // Close the client socket.
    WSACleanup();            // Terminate the use of the Winsock DLL.
    return 0;
}
