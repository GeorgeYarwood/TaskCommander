// TaskCommander.cpp : Defines the entry point for the application.
//
#include "framework.h"
#include "TaskCommander.h"
#include <psapi.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <windowsx.h>
#include <map>
#include <shellapi.h>
#include <chrono>
#include <thread>
#define MAX_LOADSTRING 100

#define KILL 1
#define FORCE_KILL 2

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HWND m_hWnd;
HWND t_hWnd;

bool running = false;

std::map<HTREEITEM, DWORD> processMap;

HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel)
{
	TVITEM tvi;
	TVINSERTSTRUCT tvins;
	static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
	static HTREEITEM hPrevRootItem = NULL;
	static HTREEITEM hPrevLev2Item = NULL;
	HTREEITEM hti;

	tvi.mask = TVIF_TEXT | TVIF_IMAGE
		| TVIF_SELECTEDIMAGE | TVIF_PARAM;

	// Set the text of the item. 
	tvi.pszText = lpszItem;
	tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);

	// Assume the item is not a parent item, so give it a 
	// document image. 
	//tvi.iImage = g_nDocument;
	//tvi.iSelectedImage = g_nDocument;

	// Save the heading level in the item's application-defined 
	// data area. 
	tvi.lParam = (LPARAM)nLevel;
	tvins.item = tvi;
	tvins.hInsertAfter = TVI_SORT;

	// Set the parent item based on the specified level. 
	if (nLevel == 1)
		tvins.hParent = TVI_ROOT;
	else if (nLevel == 2)
		tvins.hParent = hPrevRootItem;
	else
		tvins.hParent = hPrevLev2Item;

	// Add the item to the tree-view control. 
	hPrev = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM,
		0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

	if (hPrev == NULL)
		return NULL;

	// Save the handle to the item. 
	if (nLevel == 1)
		hPrevRootItem = hPrev;
	else if (nLevel == 2)
		hPrevLev2Item = hPrev;

	// The new item is a child item. Give the parent item a 
	// closed folder bitmap to indicate it now has child items. 
	if (nLevel > 1)
	{
		hti = TreeView_GetParent(hwndTV, hPrev);
		tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.hItem = hti;
		//tvi.iImage = g_nClosed;
		//tvi.iSelectedImage = g_nClosed;
		TreeView_SetItem(hwndTV, &tvi);
	}

	return hPrev;
}

HWND CreateTreeView(HWND hwndParent)
{
	RECT rcClient;  // dimensions of client area 
	HWND hwndTV;    // handle to tree-view control 

	// Ensure that the common control DLL is loaded. 
	InitCommonControls();

	// Get the dimensions of the parent window's client area, and create 
	// the tree-view control. 
	GetClientRect(hwndParent, &rcClient);
	hwndTV = CreateWindowEx(0,
		WC_TREEVIEW,
		TEXT("Tree View"),
		WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES,
		0,
		0,
		rcClient.right,
		rcClient.bottom,
		hwndParent,
		(HMENU)2, //TODO
		hInst,
		NULL);

	return hwndTV;
}

void AddProcessToTree(DWORD processID)
{
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

	if (NULL != hProcess)
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if (EnumProcessModulesEx(hProcess, &hMod, sizeof(hMod), &cbNeeded, LIST_MODULES_ALL))
		{
			GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
			wchar_t buf[512];
			swprintf_s(buf, L"%s %d", szProcessName, processID);
			LPWSTR str = LPWSTR(buf);
			HTREEITEM newItem = AddItemToTree(t_hWnd, str, 0);
			processMap.emplace(newItem, processID);
		}

		CloseHandle(hProcess);
	}
}

void GetAllProcesses(DWORD* allProcesses, DWORD& pCount, int bufSize)
{
	DWORD cbNeeded;

	if (!EnumProcesses(allProcesses, bufSize, &cbNeeded))
	{
		return;
	}

	pCount = cbNeeded / sizeof(DWORD);
}

void UpdateLoop()
{
	while (running)
	{
		//Get all processes
		//Check all existing processes against new list
		//If we already have this process, continue
		//If this process needs to be added, add it 
		//If this process is no longer in the new list, remove it

		DWORD allProcesses[2048], pCount;
		GetAllProcesses(allProcesses, pCount, 2048);

		std::map<HTREEITEM, DWORD>::const_iterator it;
		for (it = processMap.begin(); it != processMap.end(); ++it)
		{
			DWORD pid = it->second;
			bool alreadyHave = false;
			for (int i = 0; i < pCount; i++)
			{
				if (allProcesses[i] != 0)
				{
					if (allProcesses[i] == pid) //We have this process already, move to next
					{
						alreadyHave = true;
						break;
					}
				}
			}

			if (!alreadyHave) //We don't have this process, remove it
			{
				TreeView_DeleteItem(t_hWnd, it->first);
				processMap.erase(it);
				it = processMap.begin();
			}
		}

		for (int i = 0; i < pCount; i++)
		{
			if (allProcesses[i] != 0)
			{
				bool dontAdd = false;
				for (it = processMap.begin(); it != processMap.end(); ++it)
				{
					DWORD pid = it->second;
					if (allProcesses[i] == pid)
					{
						dontAdd = true;
						break;
					}
				}

				if (!dontAdd) //This process isn't in the old tree, we need to add it
				{
					AddProcessToTree(allProcesses[i]);
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(2000));
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_TASKCOMMANDER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TASKCOMMANDER));
	MSG msg;

	running = true;

	std::thread updateThread = std::thread(UpdateLoop);
	updateThread.detach();

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	running = false;
	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TASKCOMMANDER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_TASKCOMMANDER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	m_hWnd = hWnd;
	t_hWnd = CreateTreeView(hWnd);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

BOOL TerminateProcess(int mode, DWORD pid)
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
		ShellExecute(m_hWnd, L"open", L"cmd", buf, NULL, SW_HIDE);
		break;
	}
	}

	return false;
}

void OnSelectItem(int item)
{
	if (item == 0) 
	{
		//No selection made
		return;
	}
	HTREEITEM sel = TreeView_GetSelection(t_hWnd);

	if (!sel)
	{
		return;
	}

	DWORD pid = processMap[sel];

	if (!pid)
	{
		return;
	}

	TerminateProcess(item, pid);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CONTEXTMENU:
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		HMENU menu = CreatePopupMenu();
		AppendMenu(menu, MF_STRING, KILL, L"Terminate");
		AppendMenu(menu, MF_STRING, FORCE_KILL, L"Terminate (Force)");
		OnSelectItem(TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, xPos, yPos, 0, hWnd, NULL));
		break;
	}

	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
