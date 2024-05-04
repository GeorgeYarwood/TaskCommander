#include "ProcessManager.h"
#include "Common.h"
#include <shellapi.h>
#include <TlHelp32.h>
#include <Psapi.h>

ProcessManager::ProcessManager()
{
}

BOOL ProcessManager::KillProcess(HWND hWnd, TERM_MODE mode, DWORD pid)
{
	switch (mode)
	{
	case KILL:
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess)
		{
			TerminateProcess(hProcess, 1);
			CloseHandle(hProcess);
		}
		break;
	}
	case FORCE_KILL:
	{
		for (int p = 0; p < processVec.size(); p++)
		{
			ProcessInfo* pInfo = processVec.at(p);
			if(pInfo->parentPid == pid)
			{
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pInfo->pid);
				if (hProcess)
				{
					TerminateProcess(hProcess, 1);
					CloseHandle(hProcess);
				}
			}
		}

		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess)
		{
			TerminateProcess(hProcess, 1);
			CloseHandle(hProcess);
		}

		break;
	}
	}

	return false;
}

std::vector<ProcessInfo*>  ProcessManager::GetAllProcesses()
{
	std::vector<ProcessInfo*> vec;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32* processInfo = new PROCESSENTRY32;
	processInfo->dwSize = sizeof(PROCESSENTRY32);

	while (Process32Next(hSnapshot, processInfo))
	{
		if (processInfo->th32ProcessID == 0)
		{
			continue;
		}

		ProcessInfo* newInfo = new ProcessInfo(processInfo->th32ProcessID, processInfo->th32ParentProcessID, processInfo->cntThreads, processInfo->cntUsage, processInfo->szExeFile);
		vec.push_back(newInfo);
	}

	delete processInfo;
	CloseHandle(hSnapshot);
	return vec;
}

void ProcessManager::RegisterProcess(ProcessInfo* pInfo)
{
	processVec.push_back(pInfo);
}

void ProcessManager::UnregisterProcess(ProcessInfo* pInfo)
{
	for (int p = 0; p < processVec.size(); p++)
	{
		if (pInfo == processVec.at(p))
		{
			delete pInfo;
			processVec.erase(processVec.begin() + p);
			return;
		}
	}
}

bool ProcessManager::GetNameFromPid(int pid, WCHAR* buf)
{
	if (!buf) 
	{
		return false;
	}
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess != NULL)
	{
		GetProcessImageFileNameW(hProcess, buf, MAX_PATH);
		CloseHandle(hProcess);
		return true;
	}
	return false;
}
