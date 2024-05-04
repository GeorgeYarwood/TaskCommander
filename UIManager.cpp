#include "UIManager.h"
#include "ProcessManager.h"
#include "Windows.h"
#include "Common.h"
#include <windowsx.h>
#include <tlhelp32.h>
#include <chrono>
#include "resource.h"
#include <shlwapi.h>
#include <search.h>

UIManager* UIManager::instance = NULL;

WCHAR searchReq[MAX_PATH];
bool caseSensitive = false;

HTREEITEM killReq = NULL;
int killMode = 0;

UIManager::UIManager(HINSTANCE hInstance)
{
	if (instance)
	{
		return;
	}
	if (Init(hInstance))
	{
		pMan = new ProcessManager();
		running = true;
		updateThread = std::thread([&] {this->UpdateLoop(); });
		updateThread.detach();
		instance = this;
	}
}

BOOL UIManager::Init(HINSTANCE hInstance)
{
	LoadStringW(hInstance, IDS_APP_TITLE, titleText, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_TASKCOMMANDER, windowClassName, MAX_LOADSTRING);

	//Register main window class
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = this->WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDM_TASKCOMMANDER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_TASKCOMMANDER);
	wcex.lpszClassName = windowClassName;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDM_TASKCOMMANDER));

	if (!RegisterClassExW(&wcex))
	{
		return FALSE;
	}

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindowW(windowClassName, titleText, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 500, 750, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	tv_hWnd = CreateTreeView(hWnd);

	ShowWindow(hWnd, 1);
	UpdateWindow(hWnd);

	toolbar = GetMenu(hWnd);

	return TRUE;
}

void UIManager::UpdatePCount(int count) 
{
	//Display number of running processes on the menu bar
	wchar_t buf[100];
	swprintf_s(buf, L"%i processes", count);

	MENUITEMINFO menuitem = { sizeof(MENUITEMINFO) };
	menuitem.fMask = MIIM_TYPE | MIIM_DATA;
	//GetMenuItemInfo(toolbar, ID_PCOUNT, false, &menuitem);
	menuitem.dwTypeData = buf;
	SetMenuItemInfo(toolbar, ID_PCOUNT, false, &menuitem);
}

void UIManager::ToggleClearSearchBtn(bool toggle)
{
	MENUITEMINFO menuitem = { sizeof(MENUITEMINFO) };
	menuitem.fMask = MIIM_STATE;
	//GetMenuItemInfo(toolbar, ID_CLEARSEACH, false, &menuitem);
	menuitem.fState = toggle ? MFS_ENABLED : MFS_DISABLED;
	SetMenuItemInfo(toolbar, IDM_CLEARSEARCH, false, &menuitem);
}

void UIManager::UpdateToolbar() 
{
	UpdatePCount(pMan->processVec.size());
	ToggleClearSearchBtn(wcslen(searchReq) != 0);
	DrawMenuBar(hWnd);
}

HWND UIManager::CreateTreeView(HWND hwndParent)
{
	RECT rcClient;
	HWND hwndTV;

	InitCommonControls();

	GetClientRect(hwndParent, &rcClient);
	hwndTV = CreateWindowEx(0,
		WC_TREEVIEW,
		TREE_VIEW_NAME,
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

LRESULT UIManager::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
	{
		if (!instance)
		{
			break;
		}
		//Get root window rect
		RECT rootRect;
		GetClientRect(instance->hWnd, &rootRect);

		//Scale treeview control with 
		SetWindowPos(instance->tv_hWnd, NULL, rootRect.left, rootRect.top, rootRect.right - rootRect.left,
			rootRect.bottom - rootRect.top, SWP_SHOWWINDOW);
		break;
	}
	case WM_CONTEXTMENU:
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		RECT treeRect;

		TVHITTESTINFO hitInfo = { 0 };
		GetWindowRect(instance->tv_hWnd, &treeRect);
		hitInfo.pt.x = xPos - treeRect.left;
		hitInfo.pt.y = yPos - treeRect.top;

		//If we hit an item with rclick, force it as the selection and show the context menu
		HTREEITEM hitItem = TreeView_HitTest(instance->tv_hWnd, &hitInfo);
		if (hitItem != NULL)
		{
			TreeView_SelectItem(instance->tv_hWnd, hitItem);
			HMENU menu = CreatePopupMenu();
			AppendMenu(menu, MF_STRING, KILL, instance->TERMINATE_TEXT);
			AppendMenu(menu, MF_STRING, FORCE_KILL, instance->FORCE_TERMINATE_TEXT);
			instance->OnSelectItem(TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, xPos, yPos, 0, hWnd, NULL));
		}

		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(instance->hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, instance->AboutProc);
			break;
		case IDM_SEARCH:
			DialogBox(instance->hInst, MAKEINTRESOURCE(IDD_SEARCHBOX), hWnd, instance->SearchProc);
			break;
		case IDM_CLEARSEARCH:
			instance->ClearSearch();
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

INT_PTR UIManager::AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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

INT_PTR UIManager::SearchProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDC_OK:
		{
			GetDlgItemText(hDlg, IDC_SEARCH, searchReq, MAX_PATH);

			caseSensitive = IsDlgButtonChecked(hDlg, IDC_CASESENS);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			//Checkbox and close button have the same ID???
			if (caseSensitive != IsDlgButtonChecked(hDlg, IDC_CASESENS))
			{
				caseSensitive = !caseSensitive;
				break;
			}
			EndDialog(hDlg, LOWORD(wParam));
			break;
		}
		}
	}
	}
	return (INT_PTR)FALSE;
}

void UIManager::OnSelectItem(int item)
{
	if (item == 0)
	{
		//No selection made
		return;
	}
	killReq = TreeView_GetSelection(tv_hWnd);
	killMode = item;
}

HTREEITEM UIManager::AddItemToTree(LPTSTR lpszItem, HTREEITEM parent)
{
	TVITEM tvi;
	TVINSERTSTRUCT tvins;
	static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
	HTREEITEM hti;

	tvi.mask = TVIF_TEXT | TVIF_IMAGE
		| TVIF_SELECTEDIMAGE | TVIF_PARAM;

	tvi.pszText = lpszItem;
	tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);

	tvi.lParam = (LPARAM)parent == NULL ? 0 : 1;
	tvins.item = tvi;
	tvins.hInsertAfter = TVI_SORT;

	if (parent == NULL)
		tvins.hParent = TVI_ROOT;
	else
		tvins.hParent = parent;

	hPrev = (HTREEITEM)SendMessage(tv_hWnd, TVM_INSERTITEM,
		0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

	if (hPrev == NULL)
	{
		return NULL;
	}

	if (parent != NULL)
	{
		hti = TreeView_GetParent(tv_hWnd, hPrev);
		tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.hItem = hti;

		TreeView_SetItem(tv_hWnd, &tvi);
	}

	return hPrev;
}

void UIManager::RemoveProcess(ProcessInfo* pInfo)
{
	if (pInfo->item != NULL)
	{
		TreeView_DeleteItem(tv_hWnd, pInfo->item);
	}
	pMan->UnregisterProcess(pInfo);
}

void UIManager::ClearSearch()
{
	memset(searchReq, 0, MAX_PATH);
}

bool UIManager::MatchesSearchTerm(ProcessInfo* pInfo)
{
	WCHAR req[MAX_PATH];
	StrCpyW(req, searchReq);

	WCHAR pName[MAX_PATH];
	StrCpyW(pName, pInfo->name);

	if (!caseSensitive)
	{
		CharLower(req);
		CharLower(pName);
	}

	return !StrNCmp(pName, req, wcslen(req)) || wcslen(req) == 0;
}

void UIManager::UpdateLoop()
{
	//TODO refactor this whole thing
	while (running)
	{
		UpdateToolbar();

		if (killReq != NULL)
		{
			DWORD pid = NULL;

			for (int i = 0; i < pMan->processVec.size(); i++)
			{
				if (pMan->processVec[i]->item == killReq)
				{
					pid = pMan->processVec[i]->pid;
					break;
				}
			}

			if (pid)
			{
				pMan->KillProcess(hWnd, (TERM_MODE)killMode, pid);
			}

			killReq = NULL;
		}

		std::vector<ProcessInfo*> newProcesses = pMan->GetAllProcesses();

		for (int i = 0; i < newProcesses.size(); i++)
		{
			ProcessInfo* pInfo = newProcesses.at(i);
			if (!MatchesSearchTerm(pInfo))
			{
				delete pInfo;
				newProcesses.erase(newProcesses.begin() + i);
				i = 0;
			}
		}

		for (int i = 0; i < pMan->processVec.size(); i++)
		{

			ProcessInfo* current = pMan->processVec.at(i);

			//Lazy way of stopping duplicate entries that can happen from search timing issues
			for (int j = 0; j < pMan->processVec.size(); j++)
			{
				if (pMan->processVec[j] == current)
				{
					continue;
				}

				if (pMan->processVec[j]->pid == current->pid)
				{
					RemoveProcess(pMan->processVec[j]);
					j = 0;
				}
			}

			bool match = MatchesSearchTerm(current);
			bool alreadyHave = false;

			if (match)
			{
				for (int k = 0; k < newProcesses.size(); k++)
				{
					if (newProcesses[k]->pid == current->pid) //We have this process already, move to next
					{
						newProcesses[k]->alreadyHave = true; //We already have this in the main vec so delete the copy when we're done
						alreadyHave = true;
						break;
					}
				}
			}

			if ((!alreadyHave || !match)) //We don't have this anymore process, remove it
			{
				//If this process had children, delete the children first
				for (int j = 0; j < pMan->processVec.size(); j++)
				{
					ProcessInfo* child = pMan->processVec.at(j);
					if (current->item != NULL && child->parentItem != NULL && child->parentItem == current->item)
					{
						RemoveProcess(pMan->processVec[j]);
						j = 0;
					}
				}

				RemoveProcess(current);
				i = 0;
			}
		}

		for (int i = 0; i < newProcesses.size(); i++)
		{
			if (newProcesses[i]->alreadyHave)
			{
				continue;
			}
			if (newProcesses[i]->parentPid == 0)
			{
				newProcesses[i]->isRoot = true;
				continue;
			}

			for (int j = 0; j < newProcesses.size(); j++)
			{
				if (newProcesses[i]->pid == newProcesses[j]->parentPid)
				{
					newProcesses[i]->isRoot = true;
					break;
				}
			}
		}

		//Check new snapshot against existing list
		for (int i = 0; i < newProcesses.size(); i++)
		{
			if (!newProcesses[i]->alreadyHave && (newProcesses[i]->isRoot || ForceRoot(newProcesses[i])))
			{
				AddProcessToTree(newProcesses[i], NULL);
				newProcesses[i]->isRoot = true;
			}
		}

		for (int i = 0; i < newProcesses.size(); i++)
		{
			if (!newProcesses[i]->alreadyHave && !newProcesses[i]->isRoot)
			{
				//Get parent item
				for (int j = 0; j < pMan->processVec.size(); j++)
				{
					if (pMan->processVec[j]->item != NULL)
					{
						ProcessInfo* childInfo = newProcesses[i];
						ProcessInfo* parentInfo = pMan->processVec[j];
						DWORD pid = childInfo->pid;
						if (parentInfo->pid == childInfo->parentPid)
						{
							AddProcessToTree(childInfo, parentInfo->item);
						}
					}
				}
			}
		}

		//Delete any processes we didn't end up adding to the list
		for (int i = 0; i < newProcesses.size(); i++)
		{
			if (newProcesses[i]->item == NULL)
			{
				delete newProcesses[i];
			}
		}

		newProcesses.clear();

		std::this_thread::sleep_for(std::chrono::microseconds(50000));
	}
}

bool UIManager::ForceRoot(ProcessInfo* pInfo)
{
	//We haven't set it's parent ProcessInfo yet, so we must rely on the Pid for this intervention

	if (!pInfo)
	{
		return false;
	}

	WCHAR name[MAX_PATH];
	if (!pMan->GetNameFromPid(pInfo->parentPid, name))
	{
		return false;
	}

	LPWSTR fileName = PathFindFileNameW(name);

	bool ret = false;
	if (!wcscmp(fileName, L"explorer.exe"))
	{
		ret = true;
	}

	/*if (!StrCmpW(it, L"conhost"))
	{
		delete[] name;
		return true;
	}*/

	return ret;
}

void UIManager::AddProcessToTree(ProcessInfo* pInfo, HTREEITEM parent)
{
	wchar_t buf[512];
	swprintf_s(buf, L"%s %d", pInfo->name, pInfo->pid);

	LPWSTR str = LPWSTR(buf);
	HTREEITEM newItem = AddItemToTree(str, parent);
	if (newItem != NULL)
	{
		pInfo->item = newItem;
	}
	pInfo->parentItem = parent;
	pMan->RegisterProcess(pInfo);
}

UIManager::~UIManager()
{
	running = false;

	if (pMan)
	{
		delete pMan;
	}
}