#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <winsock2.h> 
#include <windows.h> 
#include <string> 
#include <iostream> 
#pragma comment(lib, "ws2_32.lib") 
#pragma warning(disable: 4996) 
using namespace std;

SOCKET Connection;

enum Packet {
    PACKET_MESSAGE,
    PACKET_NICKNAME,
    PACKET_JOIN,
    PACKET_LEAVE,
    PACKET_PRIVATE
};


DWORD WINAPI ClientThread(LPVOID lpParam) {
    SOCKET clientSocket = *(SOCKET*)lpParam;
    Packet packetType;

    setlocale(LC_ALL, "Russian");

    while (true) {
        int bytesReceived = recv(clientSocket, (char*)&packetType, sizeof(Packet), 0);

        if (bytesReceived <= 0) {
            cout << "Соединение с сервером потеряно." << endl;
            break;
        }

        if (packetType == PACKET_MESSAGE) {
            int nickSize;
            recv(clientSocket, (char*)&nickSize, sizeof(int), 0);
            char* nick = new char[nickSize + 1];
            nick[nickSize] = '\0';
            recv(clientSocket, nick, nickSize, 0);

            int msgSize;
            recv(clientSocket, (char*)&msgSize, sizeof(int), 0);
            char* msg = new char[msgSize + 1];
            msg[msgSize] = '\0';
            recv(clientSocket, msg, msgSize, 0);

            cout << "[" << nick << "]: " << msg << endl;

            delete[] nick;
            delete[] msg;
        }
        else if (packetType == PACKET_JOIN) {
            int nickSize;
            recv(clientSocket, (char*)&nickSize, sizeof(int), 0);
            char* nick = new char[nickSize + 1];
            nick[nickSize] = '\0';
            recv(clientSocket, nick, nickSize, 0);

            cout << "Пользователь " << nick << " присоединился к чату" << endl;
            delete[] nick;
        }
        else if (packetType == PACKET_LEAVE) {
            int nickSize;
            recv(clientSocket, (char*)&nickSize, sizeof(int), 0);
            char* nick = new char[nickSize + 1];
            nick[nickSize] = '\0';
            recv(clientSocket, nick, nickSize, 0);

            cout << "Пользователь " << nick << " покинул чат" << endl;
            delete[] nick;
        }
        else if (packetType == PACKET_PRIVATE) {
            int nickSize;
            recv(clientSocket, (char*)&nickSize, sizeof(int), 0);
            char* nick = new char[nickSize + 1];
            nick[nickSize] = '\0';
            recv(clientSocket, nick, nickSize, 0);

            int msgSize;
            recv(clientSocket, (char*)&msgSize, sizeof(int), 0);
            char* msg = new char[msgSize + 1];
            msg[msgSize] = '\0';
            recv(clientSocket, msg, msgSize, 0);

            cout << "[Приватно от " << nick << "]: " << msg << endl;

            delete[] nick;
            delete[] msg;
        }
    }
    closesocket(clientSocket);
    return 0;
}

int main() {
    setlocale(LC_ALL, "Russian");

    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        cout << "Ошибка инициализации Winsock" << endl;
        return 1;
    }

    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(123);
    addr.sin_family = AF_INET;

    Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
        cout << "Ошибка подключения к серверу" << endl;
        return 1;
    }

    cout << "Подключено к серверу!" << endl;
    cout << "Введите ваш ник: ";

    string nickname;
    getline(cin, nickname);

    Packet nickPacket = PACKET_NICKNAME;
    send(Connection, (char*)&nickPacket, sizeof(Packet), 0);

    int nickSize = nickname.size();
    send(Connection, (char*)&nickSize, sizeof(int), 0);
    send(Connection, nickname.c_str(), nickSize, 0);

    CreateThread(NULL, NULL, ClientThread, &Connection, NULL, NULL);

    while (true) {
        string message;
        getline(cin, message);

        if (message.find('/') == 0) {
            size_t spacePos = message.find(' ');
            if (spacePos != string::npos) {
                string targetNick = message.substr(1, spacePos - 1);
                string privMessage = message.substr(spacePos + 1);

                Packet privPacket = PACKET_PRIVATE;
                send(Connection, (char*)&privPacket, sizeof(Packet), 0);

                int targetSize = targetNick.size();
                send(Connection, (char*)&targetSize, sizeof(int), 0);
                send(Connection, targetNick.c_str(), targetSize, 0);

                int msgSize = privMessage.size();
                send(Connection, (char*)&msgSize, sizeof(int), 0);
                send(Connection, privMessage.c_str(), msgSize, 0);

                continue;
            }
        }

        Packet msgPacket = PACKET_MESSAGE;
        send(Connection, (char*)&msgPacket, sizeof(Packet), 0);

        int msgSize = message.size();
        send(Connection, (char*)&msgSize, sizeof(int), 0);
        send(Connection, message.c_str(), msgSize, 0);
    }

    return 0;
}
