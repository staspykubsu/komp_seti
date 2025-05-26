#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(666);
    //server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        cerr << "Invalid address/Address not supported\n";
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Connection failed\n";
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server. Type messages to send (type 'exit' to quit):\n";

    char buffer[1024];
    while (true) {
        cout << "> ";
        cin.getline(buffer, sizeof(buffer));

        if (send(client_socket, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
            cerr << "Send failed\n";
            break;
        }

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            cerr << "Server disconnected\n";
            break;
        }

        buffer[bytes_received] = '\0';
        cout << "Server echo: " << buffer << endl;
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
