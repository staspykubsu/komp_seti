// Сервер:
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

bool checkForFailingGrades(const vector<int>& grades) {
    for (int grade : grades) {
        if (grade == 2) return true;
    }
    return false;
}

string calculateStudentStatus(const vector<int>& grades) {
    double sum = 0;
    for (int grade : grades) {
        sum += grade;
    }
    double average = sum / grades.size();

    if (checkForFailingGrades(grades)) {
        return "имеет задолженности и не получает стипендию";
    }
    else if (average == 5.0) {
        return "получает повышенную стипендию 5000 рублей";
    }
    else if (average >= 4.0) {
        return "получает стипендию 3000 рублей";
    }
    else {
        return "не имеет задолженностей, но стипендию не получает";
    }
}

void processClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    
    if (bytesReceived == SOCKET_ERROR) {
        cerr << "Ошибка получения данных!" << endl;
        return;
    }

    string data(buffer, bytesReceived);
    istringstream stream(data);
    string surname;
    vector<int> grades(4);
    
    stream >> surname;
    for (int i = 0; i < 4; ++i) {
        stream >> grades[i];
    }

    string result = "Студент " + surname + " " + calculateStudentStatus(grades);
    send(clientSocket, result.c_str(), result.length(), 0);
    closesocket(clientSocket);
}

int main() {
    setlocale(LC_ALL, "Russian");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Ошибка инициализации WinSock!" << endl;
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Ошибка создания сокета!" << endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Ошибка привязки!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Ошибка прослушивания!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    cout << "Сервер запущен и ожидает подключений..." << endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Ошибка принятия подключения!" << endl;
            continue;
        }
        cout << "Клиент подключен!" << endl;
        processClient(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
