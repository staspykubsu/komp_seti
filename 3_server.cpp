#include <iostream>
#include <sstream>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

int main() {
    setlocale(LC_ALL, "rus");
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAStartup выдал ошибку: " << result << endl;
        return result;
    }

    struct addrinfo* addr = NULL;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, "8080", & hints,  & addr);
    if (result != 0) {
        cout << "getaddrinfo выдал ошибку: " << result << endl;
        WSACleanup();
        return 1;
    }

    SOCKET listen_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        cout << "Ошибка в сокете: " << WSAGetLastError() << endl;
        freeaddrinfo(addr);
        WSACleanup();
        return 1;
    }

    result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);
    if (result == SOCKET_ERROR) {
        cout << "Связываение сокета к IP-адресу вызвало ошибку: " << WSAGetLastError() << endl;
        freeaddrinfo(addr);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Ошибка инициализации слушания следующего сокета: " << WSAGetLastError() << endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    const int max_client_buffer_size = 1024;
    char buf[max_client_buffer_size];
    SOCKET client_socket = INVALID_SOCKET;

    cout << "Сервер запущен на localhost:8080" << endl;

    while (true) {
        client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            cout << "Ошибка сокета: " << WSAGetLastError() << endl;
            closesocket(listen_socket);
            WSACleanup();
            return 1;
        }

        stringstream response;
        stringstream response_body;
        result = recv(client_socket, buf, max_client_buffer_size, 0);
        if (result == SOCKET_ERROR) {
            cout << "recv выдал ошибку: " << WSAGetLastError() << endl;
            closesocket(client_socket);
        }
        else if (result == 0) {
            cout << "Соединение закрыто" << endl;
        }
        else if (result > 0) {
            buf[result] = '\0';
            cout << "Получен ответ:" << endl << buf << endl;

            response_body << "<!DOCTYPE html>\n"
                << "<html>\n"
                << "<head>\n"
                << "<title>HTTP C++ Server</title>\n"
                << "</head>\n"
                << "<body>\n"
                << "<h1>TEST PAGE</h1>\n"
                << "<p>Alina:)</p>\n"
                << "<pre>" << buf << "</pre>\n"
                << "</body>\n"
                << "</html>\n";

            response << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: text/html; charset=utf-8\r\n"
                << "Content-Length: " << response_body.str().length()
                << "\r\n\r\n"
                << response_body.str();

            result = send(client_socket, response.str().c_str(), response.str().length(), 0);
            if (result == SOCKET_ERROR) {
                cout << "Ошибка отправки сокета: " << WSAGetLastError() << endl;
            }
        }
        closesocket(client_socket);
    }
    closesocket(listen_socket);
    freeaddrinfo(addr);
    WSACleanup();
}
