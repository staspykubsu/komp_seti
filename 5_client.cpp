//CLIENT_CHAT
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")
// #pragma warning(disable: 4996)
using namespace std;

SOCKET Connection;
// типы пакетов
enum Packet { Pack, Test, Private, UserList, UserConnected, UserDisconnected };
vector<string> onlineUsers;

/* дополнительный поток клиента для отправки сообщений */
DWORD WINAPI ClientThread(LPVOID lpParam) {
    SOCKET sock = *(SOCKET*)lpParam;
    Packet packetType;

    // получение пакетов от сервера
    while (true) {
        int result = recv(sock, (char*)&packetType, sizeof(Packet), 0);
        if (result <= 0) {
            cout << "Отключение от сервера\n";
            break;
        }

        // определение объема информационного пакета
        int msg_size;
        recv(sock, (char*)&msg_size, sizeof(int), 0);
        char* msg = new char[msg_size + 1];
        msg[msg_size] = '\0';
        recv(sock, msg, msg_size, 0);

        // прием типа пакета
        /*
        if (packettype == Pack)
        {*/

        switch (packetType) {
        case Pack:
            cout << msg << endl;
            break;
        case Private:
            cout << msg << endl;
            break;
        case UserList: {
            onlineUsers.clear();
            string users(msg);
            size_t pos = 0;
            while ((pos = users.find(',')) != string::npos) {
                onlineUsers.push_back(users.substr(0, pos));
                users.erase(0, pos + 1);
            }
            if (!users.empty()) onlineUsers.push_back(users);
            break;
        }
        case UserConnected:
            cout << "Пользователь " << msg << " присоединился к чату\n";
            onlineUsers.push_back(msg);
            break;
        case UserDisconnected:
            cout << "Пользователь " << msg << " покинул чат\n";
            onlineUsers.erase(remove(onlineUsers.begin(), onlineUsers.end(), msg), onlineUsers.end());
            break;
        default:
            cout << "Unknown packet type: " << packetType << endl;
        }

        delete[] msg;
    }
    return 0;
}

bool SendPacket(Packet type, const string& message) {
    int msg_size = message.size();

    if (send(Connection, (char*)&type, sizeof(Packet), 0) == SOCKET_ERROR)
        return false;

    if (send(Connection, (char*)&msg_size, sizeof(int), 0) == SOCKET_ERROR)
        return false;

    if (send(Connection, message.c_str(), msg_size, 0) == SOCKET_ERROR)
        return false;

    return true;
}

/*
void PrintHelp() {
    cout << "\nCommands:\n"
        << "/help - Show this help\n"
        << "/list - Show online users\n"
        << "/priv username message - Send private message\n"
        << "Any other text - Public message\n\n";
}*/

/*
void PrintUserList() {
    int i = 0;
    cout << "\nАктивные пользователи (" << onlineUsers.size() << "):\n";
    for (const auto& user : onlineUsers) {
        cout << i << ") " << user << '\n';
        i++;
    }
    cout << '\n';
}*/

int main() {
    setlocale(LC_ALL, "Russian");

    cout << "Чтобы написать личное сообщение используйте команду !private <имя пользователя>\nЧтобы просмотреть всех активных пользователей чата используйте команду !online\n";

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }

    Connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Connection == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(123);

    if (connect(Connection, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed" << endl;
        closesocket(Connection);
        WSACleanup();
        return 1;
    }

    cout << "Успешное подключение" << endl;

    string clientName;
    cout << "Введите имя, которое будет отображаться для всех пользователей: ";
    getline(cin, clientName);
    SendPacket(Pack, clientName);

    CreateThread(NULL, NULL, ClientThread, &Connection, NULL, NULL);
    //PrintHelp();
    //cout << "Чтобы написать личное сообщение используйте команду !private <имя пользователя>\nЧтобы просмотреть всех активных пользователей чата используйте команду !online\n";
    string input;
    while (true) {
        getline(cin, input);

        if (input.empty()) continue;

        /*
        if (input == "/help") {
            PrintHelp();
        }*/
        if (input == "!online") {
            int i = 0;
            cout << "\nАктивные пользователи (" << onlineUsers.size() << "):\n";
            for (const auto& user : onlineUsers) {
                cout << i << ") " << user << '\n';
                i++;
            }
            cout << '\n';
        }
        else if (input.find("!private ") == 0) {
            size_t space1 = input.find(' ');
            size_t space2 = input.find(' ', space1 + 1);

            if (space2 != string::npos) {
                string recipient = input.substr(space1 + 1, space2 - space1 - 1);
                string message = input.substr(space2 + 1);
                SendPacket(Private, recipient + "|" + message);
            }
            else {
                cout << "Неправильный формат команды private" << endl;
            }
        }
        else {
            SendPacket(Pack, input);
        }
    }

    closesocket(Connection);
    WSACleanup();
    return 0;
}
