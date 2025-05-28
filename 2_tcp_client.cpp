// Клиент:
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <sstream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>

using namespace std;

bool isValidGrade(int grade) {
    return grade >= 2 && grade <= 5;
}

int main() {
    setlocale(LC_ALL, "Russian");
    while (true) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "Ошибка инициализации WinSock!" << endl;
            return -1;
        }

        SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Ошибка создания сокета!" << endl;
            WSACleanup();
            return -1;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(8080);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        string studentSurname;
        int studentGrades[4];
        bool validGrades = true;

        cout << "Введите фамилию студента: ";
        cin >> studentSurname;
        cout << "Введите 4 оценки (от 2 до 5): ";
        
        for (int i = 0; i < 4; ++i) {
            cin >> studentGrades[i];
            if (!isValidGrade(studentGrades[i])) {
                cout << "Некорректные данные! Оценка должна быть от 2 до 5!" << endl;
                validGrades = false;
                break;
            }
        }

        if (!validGrades) {
            closesocket(clientSocket);
            WSACleanup();
            continue;
        }

        ostringstream messageStream;
        messageStream << studentSurname;
        for (int i = 0; i < 4; ++i) {
            messageStream << " " << studentGrades[i];
        }
        string message = messageStream.str();

        if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "Ошибка подключения!" << endl;
            closesocket(clientSocket);
            WSACleanup();
            return -1;
        }

        if (send(clientSocket, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
            cerr << "Ошибка отправки данных!" << endl;
            closesocket(clientSocket);
            WSACleanup();
            return -1;
        }

        char response[1024];
        int bytesReceived = recv(clientSocket, response, sizeof(response), 0);
        if (bytesReceived > 0) {
            cout << "Ответ сервера: " << string(response, bytesReceived) << endl;
        }
        else {
            cerr << "Ошибка получения данных от сервера!" << endl;
        }

        closesocket(clientSocket);
        WSACleanup();
    }
    return 0;
}
