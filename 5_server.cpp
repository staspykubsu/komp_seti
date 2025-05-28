#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <winsock2.h> 
#include <windows.h> 
#include <string> 
#include <iostream> 
#pragma comment(lib, "ws2_32.lib") 
#pragma warning(disable: 4996) 
using namespace std;

struct ClientInfo {
    SOCKET socket;
    string nickname;
};

ClientInfo clients[100];
int Counter = 0;

enum Packet {
    PACKET_MESSAGE,
    PACKET_NICKNAME,
    PACKET_JOIN,
    PACKET_LEAVE,
    PACKET_PRIVATE
};

DWORD WINAPI ServerThread(LPVOID lpParam) {
    int index = *(int*)lpParam;
    SOCKET clientSocket = clients[index].socket;
    string nickname = clients[index].nickname;

    setlocale(LC_ALL, "Russian");

    while (true) {
        Packet packetType;
        int bytesReceived = recv(clientSocket, (char*)&packetType, sizeof(Packet), 0);

        if (bytesReceived <= 0) {
            cout << "Клиент " << nickname << " отключился." << endl;

            for (int i = 0; i < Counter; i++) {
                if (clients[i].socket != clientSocket) {
                    Packet leavePacket = PACKET_LEAVE;
                    send(clients[i].socket, (char*)&leavePacket, sizeof(Packet), 0);

                    int nickSize = nickname.size();
                    send(clients[i].socket, (char*)&nickSize, sizeof(int), 0);
                    send(clients[i].socket, nickname.c_str(), nickSize, 0);
                }
            }

            for (int i = index; i < Counter - 1; i++) {
                clients[i] = clients[i + 1];
            }
            Counter--;

            closesocket(clientSocket);
            return 0;
        }

        if (packetType == PACKET_MESSAGE) {
            int msg_size;
            recv(clientSocket, (char*)&msg_size, sizeof(int), 0);
            char* msg = new char[msg_size + 1];
            msg[msg_size] = '\0';
            recv(clientSocket, msg, msg_size, 0);

            cout << nickname << ": " << msg << endl;

            for (int i = 0; i < Counter; i++) {
                if (clients[i].socket != clientSocket) {
                    Packet msgType = PACKET_MESSAGE;
                    send(clients[i].socket, (char*)&msgType, sizeof(Packet), 0);

                    int nickSize = nickname.size();
                    send(clients[i].socket, (char*)&nickSize, sizeof(int), 0);
                    send(clients[i].socket, nickname.c_str(), nickSize, 0);

                    send(clients[i].socket, (char*)&msg_size, sizeof(int), 0);
                    send(clients[i].socket, msg, msg_size, 0);
                }
            }
            delete[] msg;
        }
        else if (packetType == PACKET_PRIVATE) {
            int targetNickSize;
            recv(clientSocket, (char*)&targetNickSize, sizeof(int), 0);
            char* targetNick = new char[targetNickSize + 1];
            targetNick[targetNickSize] = '\0';
            recv(clientSocket, targetNick, targetNickSize, 0);

            int msg_size;
            recv(clientSocket, (char*)&msg_size, sizeof(int), 0);
            char* msg = new char[msg_size + 1];
            msg[msg_size] = '\0';
            recv(clientSocket, msg, msg_size, 0);

            cout << "Приватное от " << nickname << " для " << targetNick << ": " << msg << endl;

            bool found = false;
            for (int i = 0; i < Counter; i++) {
                if (clients[i].nickname == string(targetNick)) {
                    found = true;
                    Packet privType = PACKET_PRIVATE;
                    send(clients[i].socket, (char*)&privType, sizeof(Packet), 0);

                    int senderNickSize = nickname.size();
                    send(clients[i].socket, (char*)&senderNickSize, sizeof(int), 0);
                    send(clients[i].socket, nickname.c_str(), senderNickSize, 0);

                    send(clients[i].socket, (char*)&msg_size, sizeof(int), 0);
                    send(clients[i].socket, msg, msg_size, 0);
                    break;
                }
            }

            if (!found) {
                string errorMsg = "Пользователь '" + string(targetNick) + "' не найден!";
                Packet msgType = PACKET_MESSAGE;
                send(clientSocket, (char*)&msgType, sizeof(Packet), 0);

                int nickSize = 6;
                send(clientSocket, (char*)&nickSize, sizeof(int), 0);
                send(clientSocket, "Сервер", nickSize, 0);

                int errorSize = errorMsg.size();
                send(clientSocket, (char*)&errorSize, sizeof(int), 0);
                send(clientSocket, errorMsg.c_str(), errorSize, 0);
            }

            delete[] targetNick;
            delete[] msg;
        }
    }
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

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    cout << "Сервер запущен. Ожидание подключений..." << endl;

    SOCKET newConnection;
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    while (true) {
        newConnection = accept(sListen, (SOCKADDR*)&clientAddr, &clientAddrSize);

        if (newConnection == INVALID_SOCKET) {
            cout << "Ошибка принятия подключения" << endl;
            continue;
        }

        Packet packetType;
        recv(newConnection, (char*)&packetType, sizeof(Packet), 0);

        if (packetType == PACKET_NICKNAME) {
            int nickSize;
            recv(newConnection, (char*)&nickSize, sizeof(int), 0);
            char* nick = new char[nickSize + 1];
            nick[nickSize] = '\0';
            recv(newConnection, nick, nickSize, 0);
            string nickname(nick);
            delete[] nick;

            clients[Counter].socket = newConnection;
            clients[Counter].nickname = nickname;

            cout << "Подключился новый клиент: " << nickname << endl;

            for (int i = 0; i < Counter; i++) {
                Packet joinPacket = PACKET_JOIN;
                send(clients[i].socket, (char*)&joinPacket, sizeof(Packet), 0);

                int nickSize = nickname.size();
                send(clients[i].socket, (char*)&nickSize, sizeof(int), 0);
                send(clients[i].socket, nickname.c_str(), nickSize, 0);
            }

            int clientIndex = Counter;
            Counter++;
            CreateThread(NULL, NULL, ServerThread, &clientIndex, NULL, NULL);
        }
    }

    closesocket(sListen);
    WSACleanup();
    return 0;
}
