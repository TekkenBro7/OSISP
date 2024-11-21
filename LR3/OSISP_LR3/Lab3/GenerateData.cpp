#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdlib> 
#include <ctime>

#define BUFFER_SIZE 512

void GenerateData() 
{
    HANDLE hPipeGenerate;
    DWORD dwWritten;
    int dataCount = 10;
    std::vector<int> data(dataCount);
    std::cout << "G: Before create generate pipe" << '\n';
    hPipeGenerate = CreateNamedPipe(
        TEXT("\\\\.\\pipe\\GenerateDataPipe"),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        BUFFER_SIZE,
        BUFFER_SIZE,
        0,
        NULL);
    if (hPipeGenerate == INVALID_HANDLE_VALUE) 
    {
        std::cerr << "G: CreateNamedPipe generate failed: " << GetLastError() << '\n';
        return;
    }
    std::cout << "G: After create generate pipe" << '\n';
    // Ожидание подключения клиента
    std::cout << "G: Waiting for client to connect generate..." << '\n';
    if (!ConnectNamedPipe(hPipeGenerate, NULL)) 
    {
        std::cerr << "G: ConnectNamedPipe generate failed: " << GetLastError() << '\n';
        CloseHandle(hPipeGenerate);
        return;
    }
    std::cout << "G: Client connected to generate." << '\n';
    // Генерация данных
    std::cout << "G: Generating data..." << '\n';
    std::srand(static_cast<unsigned int>(std::time(0)));
    for (int& num : data) 
    {
        num = std::rand() % 100;
    }
    std::cout << "G: Data has been generated" << '\n';
    if (!WriteFile(hPipeGenerate, data.data(), data.size() * sizeof(int), &dwWritten, NULL)) 
    {
        std::cerr << "G: WriteFile failed: " << GetLastError() << '\n';
    }
    else 
    {
        std::cout << "G: Data has been sent." << '\n';
    }
    CloseHandle(hPipeGenerate);
}

//int main() {
//    GenerateData();
//    return 0;
//}