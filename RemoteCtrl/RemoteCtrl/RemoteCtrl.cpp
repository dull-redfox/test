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

#define INVOKE_PATH

CWinApp theApp;
using namespace std;
//修改注册表方式

bool ChooseAutoInvoke(const CString& strPath) {
	TCHAR wcsSystem[MAX_PATH] = _T("");
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
		if (!CEdouyunTool::WriteStartupDir(strPath))
		{
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL) {
		return false;
	}
	return true;
}



int main()
{
	if (CEdouyunTool::IsAdmin()) {
		if (!CEdouyunTool::Init())return 1;

		/*if (ChooseAutoInvoke(INVOKE_PATH)) {
		 	CCommand cmd;
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
		switch (ret) {
		case -1:
			MessageBox(NULL, _T("网络初始化异常，未能成功初始hi，请检查网络设置！"), _T("网络初始化错误！"), MB_OK | MB_ICONERROR);
			break;
		case -2:
			MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
			break;
		}
			::exit(0);
		}*/
		
	}
	else {
		if (CEdouyunTool::RunAsAdmin()==false) {
			CEdouyunTool::ShowError();
			return 1;
		}
	}
	return 0;
}
