#pragma once
#include "framework.h"
#include "ProcessInfo.h"
#include <vector>

class ProcessManager
{
public:
	BOOL TerminateProcess(HWND hWnd, enum TERM_MODE mode, DWORD pid);
	std::vector<ProcessInfo*> GetAllProcesses();
	void RegisterProcess(ProcessInfo* pInfo);
	void UnregisterProcess(ProcessInfo* pInfo);
	bool GetNameFromPid(int pid, WCHAR* buf);
	std::vector<ProcessInfo*> processVec;
	ProcessManager();
private:
	void Init();
};