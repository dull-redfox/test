#pragma once
#include "Resource.h"
#include<map>
#include<atlimage.h>
#include<direct.h>
#include"ServerSocket.h"
#include<stdio.h>
#include<io.h>
#include<list>
#include"EdouyunTool.h"
#include "LockinfoDialog.h"
#pragma warning(disable:4966)//fopen sprintf strcpy strstr
class CCommand
{
public:
	CCommand();
	~CCommand(){}
	int ExcuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)();//��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction;//������ŵ����ܵ�ӳ��
	CLockinfoDialog dlg;
	unsigned threadid;
protected:
	static unsigned __stdcall threadLockDlg(void* arg)
	{
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}

	void threadLockDlgMain() {
		TRACE("Cthreadid=%d\r\n", GetCurrentThreadId());
		dlg.Create(IDD_DIALOG_INFO, NULL);
		dlg.ShowWindow(SW_SHOW);
		//�ڱκ�̨����
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		rect.right *= LONG(rect.right * 0.2);
		rect.bottom *= LONG(rect.bottom * 0.2);
		dlg.MoveWindow(rect);
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText) {
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width() / 2;
			int x = (rect.right - nWidth) / 2;
			int nHeight = rtText.Height();
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//�����ö�
		//���������Χ�͹���
		ShowCursor(false);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//����������
		dlg.GetWindowRect(rect);
		//���������Χ
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect);
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN) {
				TRACE("msg:%08X wparm:%08x lparm:%08X\r\n", msg.message, msg.wParam, msg.lParam);
				if (msg.wParam == 0x41) {//����ESC�˳�
					break;
				}
			}
		}
		ClipCursor(NULL);
		//�ָ����
		ShowCursor(true);
		//�ָ�������
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);//�ָ�������
		dlg.DestroyWindow();
	}

	int MakeDriverInfo() {//1==>A 2==>B 3==>C ... 26==>Z               //�޸Ĺ�
		std::string result;
		for (int i = 1; i <= 26; i++) {
			if (_chdrive(i) == 0) {
				if (result.size() > 0)
					result += ',';
				result += 'A' + i - 1;
			}
		}
		CPacket pack(1, (BYTE*)result.c_str(), result.size());
		CEdouyunTool::Dump((BYTE*)pack.Data(), pack.Size());
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int MakeDirectoryInfo() {
		std::string strPath;
		//std::list<FILEINFO>listFileInfos;
		if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
			OutputDebugString(_T("��ǰ��������ǻ�ȡ�ļ��б��������������"));
			return -1;
		}
		if (_chdir(strPath.c_str()) != 0) {
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			OutputDebugString(_T("û��Ȩ�޷���Ŀ¼��"));
			return -2;
		}
		_finddata_t fdata;
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1) {
			OutputDebugString(_T("û���ҵ��κ��ļ���"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			return -3;
		}
		int count = 0;
		do {
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			//listFileInfos.push_back(finfo);
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			count++;
		} while (!_findnext(hfind, &fdata));
		TRACE("server:count=%d\r\n", count);
		//������Ϣ�����ƶ�
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int RunFile() {
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		CPacket pack(3, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}


	int DownloadFile() {
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		long long data = 0;
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0) {
			CPacket pack(4, (BYTE*)&data, 8);
			CServerSocket::getInstance()->Send(pack);
			return -1;
		}
		if (pFile != NULL) {
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			CPacket head(4, (BYTE*)&data, 8);
			CServerSocket::getInstance()->Send(head);
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do {
				rlen = fread(buffer, 1, 1024, pFile);
				CPacket pack(4, (BYTE*)&buffer, rlen);
				CServerSocket::getInstance()->Send(pack);
			} while (rlen >= 1024);
			fclose(pFile);
		}
		CPacket pack(4, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int MouseEvent()
	{
		MOUSEEV mouse;
		if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
			DWORD nFlags = 0;
			switch (mouse.nButton) {
			case 0://���
				nFlags = 1;
				break;
			case 1://�Ҽ�
				nFlags = 2;
				break;
			case 2://�м�
				nFlags = 4;
				break;
			case 4://û�а���
				nFlags = 8;
				break;
			}
			if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
			switch (mouse.nAction)
			{
			case 0://����
				nFlags |= 0x10;
				break;
			case 1://˫��
				nFlags |= 0x20;
				break;
			case 2://����
				nFlags |= 0x40;
				break;
			case 3://�ſ�
				nFlags |= 0x80;
				break;
			default:
				break;
			}
			switch (nFlags)
			{
			case 0x21://���˫��
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x11://�������
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x41://�������
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x81://����ſ�
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x22://�Ҽ�˫��
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x12://�Ҽ�����
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x42://�Ҽ�����
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x82://�Ҽ��ſ�
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x24://�м�˫��
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x14://�м�����
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x44://�м�����
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x84://�м��ſ�
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x08://��������ƶ�
				mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
				break;
			}
			CPacket pack(4, NULL, 0);
			CServerSocket::getInstance()->Send(pack);
		}
		else {
			OutputDebugString(_T("��ȡ����������ʧ��!"));
			return -1;
		}
		return 0;
	}

	int SendScreen()
	{
		CImage screen;//GDI ȫ���豸�ӿ�
		HDC hScreen = ::GetDC(NULL);//��þ��
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		screen.Create(nWidth, nHeight, nBitPerPixel);
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);//��ͼ���Ƶ�screen��
		ReleaseDC(NULL, hScreen);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//�ɱ仯�ڴ�ľ��
		if (hMem == NULL)return -1;
		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (ret == S_OK) {
			screen.Save(pStream, Gdiplus::ImageFormatPNG);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			PBYTE pData = (PBYTE)GlobalLock(hMem);
			SIZE_T nSize = GlobalSize(hMem);
			CPacket pack(6, pData, nSize);
			CServerSocket::getInstance()->Send(pack);
			GlobalUnlock(hMem);
		}

		//screen.Save(_T("test2024.png"), Gdiplus::ImageFormatPNG);
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}

	int LockMachine()
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
			TRACE("threadid=%d\r\n", threadid);
		}
		CPacket pack(7, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int UnlockMachine()
	{
		//dlg.SendMessage(WM_KEYDOWN, 0x41, 00010001);
		//::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 00010001);
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
		CPacket pack(8, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int TestConnect()
	{
		CPacket pack(1981, NULL, 0);
		bool ret = CServerSocket::getInstance()->Send(pack);
		TRACE("Send ret=%d\r\n", ret);
		return 0;
	}

	int DeleteLocalFile()
	{
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size()); ������������
		MultiByteToWideChar(
			CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
			sizeof(sPath) / sizeof(TCHAR));
		DeleteFile(sPath);
		CPacket pack(9, NULL, 0);
		bool ret = CServerSocket::getInstance()->Send(pack);
		TRACE("Send ret=%d\r\n", ret);
		return 0;
	}
};
