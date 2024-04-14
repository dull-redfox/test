#include "pch.h"
#include"ClientSocket.h"
#include "ClientController.h"
#include"resource.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance=NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientController();
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] =
		{ 
			{WM_SEND_STATUS,&CClientController::OnSendStatus},
			{WM_SEND_WATCH,&CClientController::OnSendWatcher},
			{(UINT)-1,NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>
				(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0,
		&CClientController::threadEntry,
		this, 0, &m_nThreadID);//CreateThread
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL)return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, 
		(WPARAM)&info, (LPARAM)hEvent);
	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);
	return info.result;
}

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength,WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	return pClient->SendPacket(hWnd,CPacket(nCmd, pData, nLength),bAutoClose,wParam);
}

int GetImage(CImage & image) {
	CClientSocket* pClient = CClientSocket::getInstance();
	return CEdouyunTool::Bytes2Image(image, pClient->GetPacket().strData);
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成!"));
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL, strPath,
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		CString strLocal = dlg.GetPathName();
		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile = NULL) {
			AfxMessageBox(_T("本地没有权限报错该文件,或者文件无法创建！！！"));
			return;
		}
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
		//	return -1;
		//}
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);
	CWatchDialog dlg(&m_remoteDlg);
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	dlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {
			std::list<CPacket> lstPacks;
			int ret=SendCommandPacket(m_watchDlg.GetSafeHwnd(),6,true,NULL,0);
			//TODO
			//TODO
			if (ret == 6) {
				if (CEdouyunTool::Bytes2Image(m_watchDlg.GetImage(), lstPacks.front().strData) == 0) {
					m_watchDlg.SetImageStatus(true);
				}
				else {
					TRACE("获取图片失败！ret=%d\r\n", ret);
				}
			}
		}
		Sleep(1);
	}
}

void CClientController::threadWatchScreen(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadDownloadFile()
{
	FILE* pFile = fopen(m_strLocal, "wb+");
	if (pFile = NULL) {
		AfxMessageBox(_T("本地没有权限报错该文件,或者文件无法创建！！！"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	do {
		int ret = SendCommandPacket(m_remoteDlg,4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(),(WPARAM)pFile);
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
		if (nLength == 0) {
			AfxMessageBox("文件长度为零或者无法读取文件！！！");
			break;
		}
		long long nCount = 0;

		while (nCount < nLength) {
			ret = pClient->DealCommand();
			if (ret < 0) {
				AfxMessageBox("传输失败！！");
				TRACE("传输失败，ret=%d\r\n", ret);
				break;
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCount += pClient->GetPacket().strData.size();
		}
	} while (false);
	fclose(pFile);
	pClient->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成!"));
}

void CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, 
					pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

unsigned _stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

LRESULT CClientController::OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
