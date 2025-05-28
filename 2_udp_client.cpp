#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#pragma comment(lib, "ws2_32.lib")
using namespace std; //client

int main() {
    setlocale(LC_ALL, "rus");
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup ошибка запуска\n";
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Ошибка создания сокета\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(8081);

    cout << "UDP - клиент. Введите сообщение серверу\n";

    char buffer[1024];
    while (true) {
        string message;
        cout << "Введите сообщение: ";
        getline(cin, message);

        sendto(clientSocket, message.c_str(), message.size(), 0,
            (sockaddr*)&serverAddr, sizeof(serverAddr));

        sockaddr_in fromAddr;
        int fromAddrSize = sizeof(fromAddr);
        int bytesReceived = recvfrom(clientSocket, buffer, sizeof(buffer), 0,
            (sockaddr*)&fromAddr, &fromAddrSize);
        if (bytesReceived == SOCKET_ERROR) {
            cerr << "Ошибка подключения\n";
            continue;
        }

        buffer[bytesReceived] = '\0';
        cout << "Введите ответ: " << buffer << "\n";
    }

    closesocket(clientSocket);
    WSACleanup();
}
