#include <windows.h>
#include <iostream>


int main() 
{
    // Создаем три процесса для каждого этапа обработки
    STARTUPINFO si[3];
    PROCESS_INFORMATION pi[3];
    std::wstring commands[3] = 
    { 
        L"GenerateData.exe", 
        L"SortData.exe", 
        L"OutputData.exe" 
    };
    for (int i = 0; i < 3; ++i) {
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        ZeroMemory(&pi[i], sizeof(pi[i]));
        // Запуск процессов
        if (!CreateProcess(NULL, const_cast<LPWSTR>(commands[i].c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si[i], &pi[i])) 
        {
            std::cerr << "CreateProcess failed: " << GetLastError() << " " << i << '\n';
            return 1;
        }
    }
    for (int i = 0; i < 3; ++i) 
    {
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }
    return 0;
}