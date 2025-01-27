// DLLInjector1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>
#include <stdlib.h>
#include <string>

using namespace std;

HANDLE GetProcessByName(PCWSTR name)
{
    DWORD pid = 0;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(process));
    process.dwSize = sizeof(process);

    if (Process32First(snapshot, &process))
    {
        do
        {
            if (_wcsicmp(process.szExeFile, name) == 0) 
            {
                pid = process.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &process));
    }

    CloseHandle(snapshot);

    if (pid != 0)
    {
        return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }

    // Not found


    return NULL;
}

int main()
{
    PCWSTR processName = L"TestConsoleApp1.exe";
	PCSTR dllPath = "C:\\Users\\ryana\\Documents\\Dev\\DLL Injection\\Test1\\TestDLL1\\x64\\Release\\TestDLL1.dll";

	HANDLE process = GetProcessByName(processName);
    if (!process || process == INVALID_HANDLE_VALUE)
    {
        cout << "Process not found" << endl;
        return 1;
    }

	void* allocatedMemory = VirtualAllocEx(process, 0, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(process, allocatedMemory, dllPath, strlen(dllPath), NULL);
    HANDLE thread = CreateRemoteThread(process, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibraryA, allocatedMemory, NULL, NULL);
    CloseHandle(thread);
    cout << "DLL Injected." << endl;
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
