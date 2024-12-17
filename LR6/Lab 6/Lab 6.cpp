#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>

void PrintRegistryTree(HKEY hKey, const std::wstring& subKey, std::ofstream& outputFile, const std::wstring& searchKeyPattern, const std::wstring& searchValuePattern);
std::wstring ConvertDataToRegFormat(DWORD type, const BYTE* data, DWORD dataSize);

int main() 
{
    setlocale(LC_ALL, "ru");

    std::string searchKey, searchValue;
    std::cout << "Введите ключ: ";
    std::getline(std::cin, searchKey);
    std::cout << "Введите значение: ";
    std::getline(std::cin, searchValue);

    HKEY hKey;
    std::wstring str = std::wstring(searchKey.begin(), searchKey.end());

    if (RegOpenKeyEx(HKEY_CURRENT_USER, NULL, 0, KEY_READ, &hKey) == ERROR_SUCCESS) 
    {
        std::ofstream outputFile("output.reg");
        if (outputFile.is_open()) 
        {
            outputFile << "Версия редактора реестра Windows 5.00\n\n";

            PrintRegistryTree(hKey, str, outputFile, std::wstring(searchKey.begin(), searchKey.end()), std::wstring(searchValue.begin(), searchValue.end()));

            outputFile.close();
            std::cout << "Результат был записан в output.reg." << '\n';
        }
        else 
        {
            std::cerr << "Ошибка при открытии файла для записи" << '\n';
        }
        RegCloseKey(hKey);
    }
    else 
    {
        std::cerr << "Ошибка при открытии реестра" << '\n';
    }

    return 0;
}

void PrintRegistryTree(HKEY hKey, const std::wstring& subKey, std::ofstream& outputFile, const std::wstring& searchKeyPattern, const std::wstring& searchValuePattern) {
    HKEY hSubKey;
    //std::cout << RegOpenKeyEx(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey);
    if (RegOpenKeyEx(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) 
    {
        wchar_t keyName[256];
        DWORD keyNameSize;
        DWORD index = 0;
        FILETIME lastWriteTime;

        // Перебор значений текущего ключа
        wchar_t valueName[256];
        DWORD valueNameSize;
        BYTE data[16384];  // Буфер для данных
        DWORD dataSize;
        DWORD type;
        DWORD valueIndex = 0;

        // Если это не корневой ключ, выводим информацию о нём
        if (!subKey.empty()) 
        {
            std::wstring fullKeyPath = L"HKEY_CURRENT_USER\\" + subKey;
            if (searchKeyPattern.empty() || fullKeyPath.find(searchKeyPattern) != std::wstring::npos) 
            {
                outputFile << "[HKEY_CURRENT_USER\\" << std::string(subKey.begin(), subKey.end()) << "]\n";
                std::cout << "Найден ключ: HKEY_CURRENT_USER\\" << std::string(subKey.begin(), subKey.end()) << '\n';

                // Перебор значений в ключе
                while (true) 
                {
                    valueNameSize = sizeof(valueName) / sizeof(valueName[0]);
                    dataSize = sizeof(data);
                    if (RegEnumValue(hSubKey, valueIndex++, valueName, &valueNameSize, NULL, &type, data, &dataSize) == ERROR_SUCCESS) 
                    {
                        std::wstring valueStr(valueName);

                        if (searchValuePattern.empty() || valueStr.find(searchValuePattern) != std::wstring::npos) 
                        {
                            std::cout << "Найдено значение: " << std::string(valueStr.begin(), valueStr.end()) << '\n';

                            std::wstring dataString = ConvertDataToRegFormat(type, data, dataSize);

                            outputFile << "\"" << std::string(valueStr.begin(), valueStr.end()) << "\"=" << std::string(dataString.begin(), dataString.end()) << "\n";
                        }
                    }
                    else 
                    {
                        break;
                    }
                }
                outputFile << "\n";
            }
        }
        while (true) 
        {
            keyNameSize = sizeof(keyName) / sizeof(keyName[0]);
            if (RegEnumKeyEx(hSubKey, index++, keyName, &keyNameSize, NULL, NULL, NULL, &lastWriteTime) == ERROR_SUCCESS) 
            {
                std::wstring fullSubKey = subKey.empty() ? keyName : subKey + L"\\" + keyName;
                PrintRegistryTree(hKey, fullSubKey, outputFile, searchKeyPattern, searchValuePattern);
            }
            else 
            {
                break; 
            }
        }
        RegCloseKey(hSubKey);
    }
}

// Функция для преобразования данных значения в формат, подходящий для reg-файлов
std::wstring ConvertDataToRegFormat(DWORD type, const BYTE* data, DWORD dataSize) {
    std::wstringstream ss;
    switch (type) 
    {
        case REG_SZ:  
            ss << L"\"" << std::wstring((wchar_t*)data) << L"\"";
            break;
        case REG_DWORD:  
            ss << L"dword:" << std::hex << std::setw(8) << std::setfill(L'0') << *(DWORD*)data;
            break;
        case REG_BINARY:  
            ss << L"hex:";
            for (DWORD i = 0; i < dataSize; i++) 
            {
                if (i > 0) ss << L",";
                ss << std::hex << std::setw(2) << std::setfill(L'0') << (int)data[i];
            }
            break;
        default:
            ss << L"неизвестно";
            break;
    }
    return ss.str();
}