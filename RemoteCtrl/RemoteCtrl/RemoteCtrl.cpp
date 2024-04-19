// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//#pragma comment( linker,"/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment( linker,"/subsystem:windows /entry:mainCRTStartup")
//#pragma comment( linker,"/subsystem:console /entry:mainCRTStartup")
//#pragma comment( linker,"/subsystem:console /entry:WinMainCRTStartup")
//后台运行设置

// 唯一的应用程序对象

CWinApp theApp;
using namespace std;
//修改注册表方式
void WriteRegisterTable(const CString& strPath)
{
	CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	char sPath[MAX_PATH] = "";
	char sSys[MAX_PATH] = "";
	std::string strExe = "\\RemoteCtrl.exe ";
	GetCurrentDirectoryA(MAX_PATH, sPath);
	GetSystemDirectoryA(sSys, sizeof(sSys));
	std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
	system(strCmd.c_str());
	HKEY hKey = NULL;
	int ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置自动开机失败！是否权限不足？\r\n程序启动失败！"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		::exit(0);
	}

	ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置自动开机失败！是否权限不足？\r\n程序启动失败！"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		::exit(0);
	}
	RegCloseKey(hKey);

}

//放入启动文件夹
void WriteStartupDir(const CString& strPath)
{
	CString strCmd = GetCommandLine();
	strCmd.Replace(_T("\""),_T(""));
	BOOL ret = CopyFile(strCmd, strPath, FALSE);
	if (ret = FALSE) {
		MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		::exit(0);
	}
}

//开机启动的时候，程序的权限是跟谁启动用户的
// 如果两者权限不一致，则会导致程序启动失败
// 开机启动对环境变量有影响，如果依赖dll（动态库），则可能启动失败
// 复制这些dll到system32下面或者sysWOW64下面
// system32下面，多是64位程序，syswow64下面多是32位程序
// 使用静态库，并非动态库
void ChooseAutoInvoke() {
	TCHAR wcsSystem[MAX_PATH] = _T("");
	GetSystemDirectory(wcsSystem, MAX_PATH);
	//CString strPath = CString(wcsSystem) + CString(_T("%SystemRoot%\\system32\\RemoteCtrl.exe"));
	CString strPath;
	if (PathFileExists(strPath)) {
		return;
	}

	CString strInfo = _T("该程序只允许用于合法途径!\n");
	strInfo = _T("继续运行该程序，将使这台机器处于被监控状态\n");
	strInfo = _T("如果你不希望这样，请按“取消”，退出程序\n");
	strInfo = _T("按下“是”，该程序将被复制到你的机器上，并随系统启动而自动运行\n");
	strInfo = _T("按下“否”，程序只运行一次，不会在系统内留下任何东西\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		/*WriteRegisterTable(strPath);*/
		WriteStartupDir(strPath);
	}
	else if (ret == IDCANCEL) {
		exit(0);
	}
	return;
}

int main()
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// 初始化 MFC 并在失败时显示错误       
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			nRetCode = 1;
		}
		else
		{
			// TODO: 在此处为应用程序的行为编写代码。
			CCommand cmd;
			//ChooseAutoInvoke();
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret) {
			case -1:
				MessageBox(NULL, _T("网络初始化异常，未能成功初始hi，请检查网络设置！"), _T("网络初始化错误！"), MB_OK | MB_ICONERROR);
				exit(0);
				break;
			case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
				exit(0);
				break;
			}
		}
	}
	else
	{
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");
		nRetCode = 1;
	}

	return nRetCode;
}
