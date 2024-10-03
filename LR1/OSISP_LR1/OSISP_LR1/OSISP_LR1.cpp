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
		NULL,           // ��������� ��������� ������
		NULL,           // �������� ��������
		NULL,           // �������� ������
		FALSE,          // ������������ ������������
		0,              // ����� ��������
		NULL,           // ���������� ���������
		NULL,           // ������� ����������
		&si,            // ��������� STARTUPINFO
		&pi))			// ��������� PROCESS_INFORMATION
	{         
		std::cerr << "������ �������� ��������: " << GetLastError() << '\n';
		return false;
	}
	CloseHandle(pi.hThread);
	ProcessInfo newProcess = { pi, processPath, true };
	processes.push_back(newProcess);
	std::cout << "������� �������: " << processPath << '\n';
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
		std::cerr << "�������� ������ ��������." << '\n';
		return;
	}
	ProcessInfo& process = processes[processIndex];
	if (process.isRunning)
	{
		DWORD processId = GetProcessId(process.procInfo.hProcess);
		CloseProcessWindow(processId);
		std::cout << "���������� ��������� WM_CLOSE ��������: " << process.processPath << '\n';
		DWORD waitResult = WaitForSingleObject(process.procInfo.hProcess, 5000);
		if (waitResult == WAIT_OBJECT_0)
		{
			process.isRunning = false;
			std::cout << "������� ������� ����������: " << process.processPath << '\n';
			CloseHandle(process.procInfo.hProcess);
		}
		else if (waitResult == WAIT_TIMEOUT) 
		{
			std::cerr << "������� �� ���������� �� ���������� �����." << '\n';
		}
		else 
		{
			std::cerr << "������ ��� �������� ���������� ��������: " << GetLastError() << '\n';
		}
	}
	else 
	{
		std::cerr << "������� ��� ��������." << '\n';
	}
}

void ShowProcesses()
{
	for (int i = 0; i < processes.size(); i++)
	{
		std::cout << i << ": " << processes[i].procInfo.dwProcessId << " " << processes[i].processPath
			<< " - " << (processes[i].isRunning ? "�����������" : "��������") << '\n';
	}
}

int main()
{
	setlocale(LC_ALL, "ru");
	std::string input;
	while (true)
	{
		std::cout << "\n����:\n"
			<< "1. ��������� �������\n"
			<< "2. �������� ��������\n"
			<< "3. ��������� �������\n"
			<< "4. �����\n"
			<< "�������� ��������: ";
		std::cin >> input;
		if (input == "1")
		{
			std::string processPath;
			std::cout << "������� ���� � ������������ �����: ";
			std::cin >> processPath;

			if (!CreateNewProcess(processPath.c_str())) {
				std::cerr << "�� ������� ��������� �������." << '\n';
			}
		}
		else if (input == "2")
		{
			ShowProcesses();
		}
		else if (input == "3")	
		{
			int index;
			std::cout << "������� ������ �������� ��� ����������: ";
			std::cin >> index;
			TerminateProcessById(index);
		}
		else if (input == "4")
		{
			break;
		}
		else
		{
			std::cout << "�������� �����, ���������� �����." << '\n';
		}
	}
	return 0;
}