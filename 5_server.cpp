#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

//массив сокетов активных клиентов
SOCKET Connections[100];
// число активных клиентов
int Counter = 0;
// допустимые типы пакетов
enum Packet { Pack, Test, Private, UserList, UserConnected, UserDisconnected };

struct ClientInfo {
    SOCKET socket;
    string name;
};

map<int, ClientInfo> clientInfoMap;

/* передача типа, объема и содержания информационного пакета */
bool SendPacket(SOCKET socket, Packet type, const string& message) {
    int msg_size = message.size();

    if (send(socket, (char*)&type, sizeof(Packet), 0) == SOCKET_ERROR)
        return false;

    if (send(socket, (char*)&msg_size, sizeof(int), 0) == SOCKET_ERROR)
        return false;

    if (send(socket, message.c_str(), msg_size, 0) == SOCKET_ERROR)
        return false;

    return true;
}

void BroadcastUserConnected(const string& username) {
    for (auto& pair : clientInfoMap) {
        SendPacket(pair.second.socket, UserConnected, username);
    }
}

void BroadcastUserDisconnected(const string& username) {
    for (auto& pair : clientInfoMap) {
        SendPacket(pair.second.socket, UserDisconnected, username);
    }
}

void BroadcastUserList() {
    string userList;
    for (auto& pair : clientInfoMap) {
        if (!userList.empty()) userList += ",";
        userList += pair.second.name;
    }
    for (auto& pair : clientInfoMap) {
        SendPacket(pair.second.socket, UserList, userList);
    }
}

// функционал потока отдельного клиента
DWORD WINAPI ServerThread(LPVOID lpParam) {

    Packet packetType;
    // определение номера сокета клиента
    int index = *(int*)lpParam;
    cout << "socket number= " << index << endl;
    SOCKET clientSocket = Connections[index];
    string clientName = clientInfoMap[index].name;

    // общение с клиентом:
    while (true) {
        //получение информации от клиента
        int result = recv(clientSocket, (char*)&packetType, sizeof(Packet), 0);
        
        //сообщение об отключении клиента от сервера
        if (result <= 0) {
            cout << "Клиент " << clientName << " покинул чат\n";
            break;
        }

        int msg_size;
        // определение объема пакета
        result = recv(clientSocket, (char*)&msg_size, sizeof(int), 0);
        if (result <= 0) break;

        /* резервирование буфера нужного размера для принятия пакета */
        char* msg = new char[msg_size + 1];
        msg[msg_size] = '\0';

        // получение пакета
        result = recv(clientSocket, msg, msg_size, 0);
        if (result <= 0) {
            delete[] msg;
            break;
        }

        string message(msg);
        delete[] msg;

        // определение типа полученного пакета
        /* if (packettype == Pack) */
        switch (packetType) {

        case Pack: {
            string fullMsg = clientName + ": " + message;
            for (auto& pair : clientInfoMap) {
                if (pair.first != index) {
                    SendPacket(pair.second.socket, Pack, fullMsg);
                }
            }
            cout << fullMsg << '\n';
            break;
        }
        case Private: {
            size_t pos = message.find('|');
            if (pos != string::npos) {
                string recipient = message.substr(0, pos);
                string privateMsg = message.substr(pos + 1);

                bool found = false;
                for (auto& pair : clientInfoMap) {
                    if (pair.second.name == recipient) {
                        string fullMsg = "{Получено личное сообщение от пользователя " + clientName + "}: " + privateMsg;
                        SendPacket(pair.second.socket, Private, fullMsg);
                        found = true;
                        break;
                    }
                }

                string status = found ? "Сообщение отправлено" : "Пользователь " + recipient + " не найден";
                SendPacket(clientSocket, Private, status);
                cout << "Приватное сообщение отправлено пользователем" << clientName << " пользователю " << recipient << '\n';
            }
            
            break;
        }
                    // получен неопознанный пакет!
        default:
            cout << "Unknown packet type: " << packetType << endl;
        }
    }

    closesocket(clientSocket);
    Connections[index] = 0;
    clientInfoMap.erase(index);
    Counter--;
    BroadcastUserDisconnected(clientName);
    BroadcastUserList();
    delete (int*)lpParam;
    return 0;
}

int main() {
    setlocale(LC_ALL, "Russian");

    // Инициализация Winsock
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sListen == INVALID_SOCKET) {
        cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    /* сохранение в слушающем сокете информации о сервере */
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(123);

    if (bind(sListen, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Bind failed\n";
        closesocket(sListen);
        WSACleanup();
        return 1;
    }

    listen(sListen, SOMAXCONN);
    cout << "Сервер ожидает подключения\n";

    // режим прослушивания, организация очереди
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        // извлечение запросов из очереди
        SOCKET newConnection = accept(sListen, (SOCKADDR*)&clientAddr, &clientAddrSize);

        if (newConnection == INVALID_SOCKET) {
            cerr << "Accept failed\n";
            continue;
        }

        Packet packetType;
        int bytesReceived = recv(newConnection, (char*)&packetType, sizeof(Packet), 0);
        if (bytesReceived <= 0 || packetType != Pack) {
            closesocket(newConnection);
            continue;
        }

        int nameSize;
        recv(newConnection, (char*)&nameSize, sizeof(int), 0);
        char* nameBuffer = new char[nameSize + 1];
        nameBuffer[nameSize] = '\0';
        recv(newConnection, nameBuffer, nameSize, 0);
        string clientName(nameBuffer);
        delete[] nameBuffer;

        int index = -1;
        for (int i = 0; i < 100; i++) {
            if (Connections[i] == 0) {
                index = i;
                break;
            }
        }

        if (index == -1) {
            SendPacket(newConnection, Pack, "Server is full");
            closesocket(newConnection);
            continue;
        }


        /* сохранение сокета клиента в массиве участников чата */
        Connections[index] = newConnection;
        ClientInfo clientInfo;
        clientInfo.socket = newConnection;
        clientInfo.name = clientName;
        clientInfoMap[index] = clientInfo;
        Counter++;

        cout << "Пользователь " << clientName << " присоединился к чату" << endl;
        SendPacket(newConnection, Pack, "Добро пожаловать в чат, " + clientName + "!");

        BroadcastUserConnected(clientName);
        BroadcastUserList();

        int* clientIndex = new int(index);
        CreateThread(NULL, NULL, ServerThread, clientIndex, NULL, NULL);
    }

    closesocket(sListen);
    WSACleanup();
    return 0;
}
