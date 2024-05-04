#pragma once
#include "framework.h"
#include <psapi.h>
#include <CommCtrl.h>
#include <thread>
class ProcessInfo;

class UIManager
{
#define MAX_LOADSTRING 100
public:
	const WCHAR* TREE_VIEW_NAME = L"Process View";
	const WCHAR* TERMINATE_TEXT = L"Terminate";
	const WCHAR* FORCE_TERMINATE_TEXT = L"Terminate (Force)";
	UIManager(HINSTANCE hInstance);
	~UIManager();
	static UIManager* GetInstance() { return instance; }

	HTREEITEM AddItemToTree(LPTSTR lpszItem, HTREEITEM parent);
private:
	void UpdatePCount(int count);
	HMENU toolbar = NULL;
	void ClearSearch();
	bool MatchesSearchTerm(ProcessInfo* pInfo);
	bool ForceRoot(ProcessInfo* pInfo);
	WCHAR titleText[MAX_LOADSTRING];                  // The title bar text
	WCHAR windowClassName[MAX_LOADSTRING];            // the main window class name
	std::thread updateThread;
	void RemoveProcess( ProcessInfo* pInfo);
	void AddProcessToTree(ProcessInfo* pInfo, HTREEITEM parent);
	BOOL Init(HINSTANCE hInstance);
	bool running = false;
	static UIManager* instance;
	HINSTANCE hInst;
	HWND hWnd;
	HWND CreateTreeView(HWND hwndParent);
	HWND tv_hWnd;
	void OnSelectItem(int item);

	static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR SearchProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	void UpdateLoop();
	class ProcessManager* pMan;
	bool resetPending = false;
};

