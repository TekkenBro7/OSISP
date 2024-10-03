#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

struct ProcessInfo
{
	PROCESS_INFORMATION procInfo;
	std::string processPath;
	bool isRunning;
};

std::vector<ProcessInfo> processes;

std::wstring ConvertToWideString(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
	return wstr;
}

bool CreateNewProcess(const char* processPath)
{
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	std::wstring wideProcessPath = ConvertToWideString(processPath);
	if (!CreateProcess(
		wideProcessPath.c_str(), 
		NULL,           // Аргументы командной строки
		NULL,           // Атрибуты процесса
		NULL,           // Атрибуты потока
		FALSE,          // Наследование дескрипторов
		0,              // Флаги создания
		NULL,           // Переменные окружения
		NULL,           // Рабочая директория
		&si,            // Структура STARTUPINFO
		&pi))			// Структура PROCESS_INFORMATION
	{         
		std::cerr << "Ошибка создания процесса: " << GetLastError() << '\n';
		return false;
	}
	CloseHandle(pi.hThread);
	ProcessInfo newProcess = { pi, processPath, true };
	processes.push_back(newProcess);
	std::cout << "Процесс запущен: " << processPath << '\n';
	return true;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD procId;
	GetWindowThreadProcessId(hwnd, &procId);
	if (procId == (DWORD)lParam)
	{
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		return FALSE;
	}
	return TRUE;
}

void CloseProcessWindow(DWORD processId)
{
	EnumWindows(EnumWindowsProc, (LPARAM)processId);
}

void TerminateProcessById(int processIndex)
{
	if (processIndex < 0 || processIndex >= processes.size())
	{
		std::cerr << "Неверный индекс процесса." << '\n';
		return;
	}
	ProcessInfo& process = processes[processIndex];
	if (process.isRunning)
	{
		DWORD processId = GetProcessId(process.procInfo.hProcess);
		CloseProcessWindow(processId);
		std::cout << "Отправлено сообщение WM_CLOSE процессу: " << process.processPath << '\n';
		DWORD waitResult = WaitForSingleObject(process.procInfo.hProcess, 5000);
		if (waitResult == WAIT_OBJECT_0)
		{
			process.isRunning = false;
			std::cout << "Процесс успешно завершился: " << process.processPath << '\n';
			CloseHandle(process.procInfo.hProcess);
		}
		else if (waitResult == WAIT_TIMEOUT) 
		{
			std::cerr << "Процесс не завершился за отведенное время." << '\n';
		}
		else 
		{
			std::cerr << "Ошибка при ожидании завершения процесса: " << GetLastError() << '\n';
		}
	}
	else 
	{
		std::cerr << "Процесс уже завершен." << '\n';
	}
}

void ShowProcesses()
{
	for (int i = 0; i < processes.size(); i++)
	{
		std::cout << i << ": " << processes[i].procInfo.dwProcessId << " " << processes[i].processPath
			<< " - " << (processes[i].isRunning ? "Выполняется" : "Завершен") << '\n';
	}
}

int main()
{
	setlocale(LC_ALL, "ru");
	std::string input;
	while (true)
	{
		std::cout << "\nМеню:\n"
			<< "1. Запустить процесс\n"
			<< "2. Показать процессы\n"
			<< "3. Завершить процесс\n"
			<< "4. Выход\n"
			<< "Выберите действие: ";
		std::cin >> input;
		if (input == "1")
		{
			std::string processPath;
			std::cout << "Введите путь к исполняемому файлу: ";
			std::cin >> processPath;

			if (!CreateNewProcess(processPath.c_str())) {
				std::cerr << "Не удалось запустить процесс." << '\n';
			}
		}
		else if (input == "2")
		{
			ShowProcesses();
		}
		else if (input == "3")	
		{
			int index;
			std::cout << "Введите индекс процесса для завершения: ";
			std::cin >> index;
			TerminateProcessById(index);
		}
		else if (input == "4")
		{
			break;
		}
		else
		{
			std::cout << "Неверный выбор, попробуйте снова." << '\n';
		}
	}
	return 0;
}