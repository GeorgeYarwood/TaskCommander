#include "ProcessInfo.h"
#include <Shlwapi.h>

ProcessInfo::ProcessInfo(DWORD setPid, DWORD setParentPid, DWORD setCurrThreads, DWORD setCurrUsage, WCHAR* setName)
{
	pid = setPid;
	parentPid = setParentPid;
	currThreads = setCurrThreads;
	currUsage = setCurrUsage;
	name = new WCHAR[MAX_PATH];
	StrCpyW(name, setName);
}

ProcessInfo::~ProcessInfo()
{
	delete[]name;
}
