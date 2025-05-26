#include <iostream>
#include <winsock2.h>
#include <string>
#include <windows.h>
#include <ws2tcpip.h>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const u_short MY_PORT = 666;
vector<SOCKET> client_sockets;
HANDLE mutex;

#define PRINTNUSERS \
    if (nclients) cout << "Users on-line: " << nclients << endl; \
    else cout << "No users online"<<endl;

DWORD WINAPI ConToClient(LPVOID client_socket);

// Глобальная переменная с синхронизацией
int nclients = 0;
CRITICAL_SECTION cs;

int main() {
    WSADATA wsaData;
    char buff[1024];

    cout << "TCP ECHO-SERVER WITH SYNCHRONIZATION\n";

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed: " << WSAGetLastError() << endl;
        return -1;
    }

    // Инициализация критической секции
    InitializeCriticalSection(&cs);
    mutex = CreateMutex(NULL, FALSE, NULL);

    // Создание сокета
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }

    // Настройка адреса сервера
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MY_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Привязка сокета
    if (bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Bind failed: " << WSAGetLastError() << endl;
        closesocket(listen_socket);
        WSACleanup();
        return -1;
    }

    // Начало прослушивания
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed: " << WSAGetLastError() << endl;
        closesocket(listen_socket);
        WSACleanup();
        return -1;
    }

    cout << "Server started on port " << MY_PORT << ". Waiting for connections...\n";

    // Основной цикл сервера
    while (true) {
        sockaddr_in client_addr;
        int client_addr_size = sizeof(client_addr);

        SOCKET client_socket = accept(listen_socket, (sockaddr*)&client_addr, &client_addr_size);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Accept failed: " << WSAGetLastError() << endl;
            continue;
        }

        // Синхронизированное увеличение счетчика клиентов
        EnterCriticalSection(&cs);
        nclients++;
        client_sockets.push_back(client_socket);
        LeaveCriticalSection(&cs);

        // Информация о клиенте
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        cout << "+ New connection from: " << client_ip << endl;
        PRINTNUSERS;

        // Создание потока для клиента
        DWORD threadId;
        HANDLE thread = CreateThread(NULL, 0, ConToClient, &client_socket, 0, &threadId);
        if (thread == NULL) {
            cerr << "Thread creation failed: " << GetLastError() << endl;
            closesocket(client_socket);

            EnterCriticalSection(&cs);
            nclients--;
            LeaveCriticalSection(&cs);
        }
        else {
            CloseHandle(thread);
        }
    }

    // Очистка (теоретически, сюда мы никогда не попадем)
    DeleteCriticalSection(&cs);
    CloseHandle(mutex);
    closesocket(listen_socket);
    WSACleanup();
    return 0;
}

DWORD WINAPI ConToClient(LPVOID param) {
    SOCKET client_socket = *((SOCKET*)param);
    char buffer[1024];
    const char* welcome_msg = "Welcome to the echo server! Send 'exit' to quit.\r\n";

    // Отправка приветствия
    send(client_socket, welcome_msg, strlen(welcome_msg), 0);

    // Цикл обработки сообщений
    while (true) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) break;

        buffer[bytes_received] = '\0';

        // Проверка на команду выхода
        if (strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "exit\r\n") == 0) {
            break;
        }

        // Вывод полученного сообщения (с синхронизацией)
        WaitForSingleObject(mutex, INFINITE);
        cout << "Received from client: " << buffer;
        ReleaseMutex(mutex);

        // Эхо-ответ
        send(client_socket, buffer, bytes_received, 0);
    }

    // Закрытие соединения
    WaitForSingleObject(mutex, INFINITE);
    cout << "- Client disconnected\n";
    nclients--;
    PRINTNUSERS;

    // Удаление сокета из вектора
    for (auto it = client_sockets.begin(); it != client_sockets.end(); ++it) {
        if (*it == client_socket) {
            client_sockets.erase(it);
            break;
        }
    }
    ReleaseMutex(mutex);

    closesocket(client_socket);
    return 0;
}
