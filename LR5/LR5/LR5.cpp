#include <iostream>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

std::mutex consoleMutex;

void scanPort(const std::string& ip, int port)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Ошибка создания сокета: " << WSAGetLastError() << '\n';
        return;
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

    DWORD timeout = 100;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::cout << "Порт " << port << " закрыт." << '\n';
    }
    else
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::cout << "Порт " << port << " открыт." << '\n';
    }

    closesocket(sock);
}

int main()
{
    setlocale(LC_ALL, "ru");
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "Ошибка WSAStartup" << WSAGetLastError() << '\n';
        return 1;
    }

    std::string ip;
    std::cout << "Введи IP адресс для сканнирования: ";
    std::cin >> ip;

    int startPort, endPort;
    std::cout << "Введите начаро порта: ";
    std::cin >> startPort;
    std::cout << "Введите конец порта: ";
    std::cin >> endPort;

    if (startPort < 1 || endPort > 65535 || startPort > endPort)
    {
        std::cerr << "Недействительный радиус портов." << '\n';
        WSACleanup();
        return 1;
    }
    std::vector<std::thread> threads;
    for (int port = startPort; port <= endPort; ++port)
    {
        threads.emplace_back(scanPort, ip, port);
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    WSACleanup();
    return 0;
}