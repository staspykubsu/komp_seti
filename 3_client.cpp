#include <iostream>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

int main() {
    setlocale(LC_ALL, "rus");

    // ОТПРАВЛЯЕМ ЗАПРОС
    WSADATA winSock;
    if (WSAStartup(MAKEWORD(2, 2), &winSock) != 0) {
        cout << "WSAStartup выдал ошибку" << endl;
        return 1;
    }

    struct addrinfo hints, * result = nullptr;
    ZeroMemory(& hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int iResult = getaddrinfo("library.ru", "80", &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo выдал ошибку: " << iResult << endl;
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Ошибка создания сокета: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cout << "Ошибка подключения: " << WSAGetLastError() << endl;
        closesocket(ConnectSocket);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    string request = "GET / HTTP/1.1\r\n"
        "Host: library.ru\r\n"
        "Connection: close\r\n"
        "\r\n";

    iResult = send(ConnectSocket, request.c_str(), (int)request.size(), 0);
    if (iResult == SOCKET_ERROR) {
        cout << "Ошибка отправки запроса: " << WSAGetLastError() << endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    cout << "Запрос отправлен, ожидайте..." << endl;;

    // ПОЛУЧАЕМ ОТВЕТ
    char buffer[4096];
    int bytesReceived;
    string response;

    do {
        bytesReceived = recv(ConnectSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            response.append(buffer, bytesReceived);
        }
        else if (bytesReceived == 0) {
            cout << "Соединение закрыто" << endl;
        }
        else {
            cerr << "Recv выдал ошибку: " << WSAGetLastError() << endl;
        }
    } 
    while (bytesReceived > 0);

    cout << "Ответ получен:" << endl << endl;
    cout << response << endl;

    closesocket(ConnectSocket);
    WSACleanup();
}
