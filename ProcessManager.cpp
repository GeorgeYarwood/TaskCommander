#include "ProcessManager.h"
#include "Common.h"
#include <shellapi.h>
#include <TlHelp32.h>
#include <Psapi.h>

ProcessManager::ProcessManager()
{
}

BOOL ProcessManager::TerminateProcess(HWND hWnd, TERM_MODE mode, DWORD pid)
{
	switch (mode)
	{
	case KILL:
	{
		HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess)
		{
			return ::TerminateProcess(hProcess, 1);
		}
		break;
	}
	case FORCE_KILL:
	{
		//TODO replace this with programmatic taskkill https://stackoverflow.com/questions/70178895/need-win32-api-c-code-that-is-equivalent-to-taskkill-t-f-pid-xxx
		wchar_t buf[512];
		swprintf_s(buf, L"/C taskkill /pid %d /f", pid);
		ShellExecute(hWnd, L"open", L"cmd", buf, NULL, SW_HIDE);
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
			processVec.erase(processVec.begin() + p);
			delete pInfo;
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
