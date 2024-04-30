#pragma once
#include <Windows.h>
#include <CommCtrl.h>
class ProcessInfo
{
public:
	DWORD pid;
	DWORD parentPid;
	DWORD currThreads;
	DWORD currUsage;
	WCHAR* name;
	HTREEITEM item = NULL;
	HTREEITEM parentItem = NULL;

	bool isRoot = false;
	bool alreadyHave = false;

	ProcessInfo(DWORD setPid, DWORD setParentPid, DWORD setCurrThreads, DWORD setCurrUsage, WCHAR* setName);
	~ProcessInfo();
};

