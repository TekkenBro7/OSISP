#include <windows.h>
#include <iostream>
#include <vector>

void OutputData() 
{
    HANDLE hPipe;
    DWORD dwRead;
    std::vector<int> data(10);
    // Подключение к каналу
    while (true) 
    {
        hPipe = CreateFile(
            TEXT("\\\\.\\pipe\\SortDataPipe"),
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        if (hPipe == INVALID_HANDLE_VALUE) 
        {
            std::cerr << "O: CreateFile failed: " << GetLastError() << '\n';
        }
        else 
        {
            std::cout << "O: Connected to the sort pipe." << '\n';
            break;
        }
        Sleep(100);
    }
    // Чтение данных
    if (!ReadFile(hPipe, data.data(), data.size() * sizeof(int), &dwRead, NULL)) 
    {
        std::cerr << "O: ReadFile sort failed: " << GetLastError() << '\n';
        CloseHandle(hPipe);
        return;
    }
    // Вывод данных
    std::cout << "O: ";
    for (int num : data) 
    {
        std::cout << num << " ";
    }
    std::cout << '\n';
    CloseHandle(hPipe);
}

//int main() {
//    OutputData();
//    return 0;
//}