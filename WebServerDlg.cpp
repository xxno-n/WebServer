
// WebServerDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "WebServer.h"
#include "WebServerDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWebServerDlg �Ի���

//ADD
//�ٽ��������ʼ��
HANDLE CWebServerDlg::None = NULL;
UINT CWebServerDlg::ClientNum = 0;
CCriticalSection CWebServerDlg::m_criSect;
//ADD

CWebServerDlg::CWebServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_WEBSERVER_DIALOG, pParent)
	, m_nPort(0)
	, m_strRootDir(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWebServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS1, LocalIP);
	DDX_Control(pDX, IDC_EDIT1, localPort);
	DDX_Text(pDX, IDC_EDIT1, m_nPort);
	DDX_Control(pDX, IDC_EDIT2, rootdir);
	DDX_Text(pDX, IDC_EDIT2, m_strRootDir);
	DDX_Control(pDX, IDC_BUTTON1, m_start);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_BUTTON2, m_exit);
}

BEGIN_MESSAGE_MAP(CWebServerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON2, &CWebServerDlg::OnBnClickedButton2)
	ON_MESSAGE(LOG_MSG, AddLog)
	ON_BN_CLICKED(IDC_BUTTON1, &CWebServerDlg::OnStartStop)
END_MESSAGE_MAP()


// CWebServerDlg ��Ϣ�������

BOOL CWebServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��
	m_bStart = false;
	localPort.SetWindowText("");

	

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CWebServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CWebServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CWebServerDlg::OnBnClickedButton2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}


void CWebServerDlg::OnStartStop()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	this->UpdateData();
	if (!m_bStart)
	{
		StartWebServer();
		m_start.SetWindowTextA("�ر�");
		LocalIP.EnableWindow(false);
		localPort.EnableWindow(false);
		rootdir.EnableWindow(false);
		m_exit.EnableWindow(false);
		m_bStart = true;
	}
	else
	{
		StopWebServer();
		m_start.SetWindowTextA("����");
		LocalIP.EnableWindow(true);
		localPort.EnableWindow(true);
		rootdir.EnableWindow(true);
		m_exit.EnableWindow(true);
		m_bStart = false;
	}
}


//��ʾ��־��Ϣ��
LRESULT CWebServerDlg::AddLog(WPARAM wParam, LPARAM IPatam)
{
	char szBuf[1024];
	CString *strTemp = (CString *)wParam;
	SYSTEMTIME st;
	GetLocalTime(&st);
	wsprintf(szBuf, "%02d:%02d:%02d.%03d  %s", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, *strTemp);
	m_list.AddString(szBuf);
	m_list.SetTopIndex(m_list.GetCount() - 1);
	delete strTemp;
	strTemp = NULL;
	return 0L;
}

void CWebServerDlg::StartWebServer()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	m_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);
	//���������׽���
	m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	BYTE nFild[4];
	CString sIP;
	LocalIP.GetAddress(nFild[0], nFild[1], nFild[2], nFild[3]);
	sIP.Format("%d.%d.%d.%d", nFild[0], nFild[1], nFild[2], nFild[3]);

	//��������ַ
	sockaddr_in sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.S_un.S_addr = inet_addr(sIP);
	sockAddr.sin_port = htons(m_nPort);

	//��ʼ��content-type���ļ���׺��Ӧ��ϵ��map
	CreateTypeMap();

	//���׽���
	bind(m_listenSocket, (sockaddr*)&sockAddr, sizeof(sockAddr));

	//��ʼ����
	listen(m_listenSocket, 5);


	//���������̣߳����տͻ�����Ҫ��
	m_pListenThread = AfxBeginThread(ListenThread, this);

	//��ʾ������������Ϣ
	CString *pStr = new CString;
	*pStr = "Start WebServer Succeed";
	SendMessage(LOG_MSG, (UINT)pStr, NULL);
	char hostname[255];
	gethostname(hostname, sizeof(hostname));
	CString *pStr1 = new CString;
	pStr1->Format("%s", hostname);
	*pStr1 = *pStr1 + "[" + sIP;
	CString strTmp;
	strTmp.Format(":%d]", m_nPort);
	*pStr1 += strTmp;
	SendMessage(LOG_MSG, (UINT)pStr1, NULL);
	

}

void CWebServerDlg::StopWebServer()
{
	SetEvent(m_hExit);
	closesocket(m_listenSocket);
	int nRet = WaitForSingleObject((HANDLE)m_pListenThread, 10000);
	if (nRet == WAIT_TIMEOUT)
	{
		CString *pStr = new CString;
		*pStr = "TIMEOUT waiting for ListenThread";
		SendMessage(LOG_MSG, (UINT)pStr, NULL);
	}
	CloseHandle(m_hExit);

	CString *pStr1 = new CString;
	*pStr1 = "WebServer Stopped";
	SendMessage(LOG_MSG, (UINT)pStr1, NULL);

}

UINT CWebServerDlg::ListenThread(LPVOID param)
{
	CWebServerDlg *pWebServerDlg = (CWebServerDlg*)param;
	SOCKET socketClient;
	CWinThread *pClientThread;
	sockaddr_in SockAddr;
	PREQUEST pReq;
	DWORD dwRet;

	//��ʼ��ClientNum��������no client���¼�����
	HANDLE hNoClient;
	hNoClient = pWebServerDlg->InitClientCount();
	
	
	while (true)
	{
		int nlen = sizeof(SockAddr);
		socketClient = accept(pWebServerDlg->m_listenSocket, (struct sockaddr*)&SockAddr, &nlen);
		if (socketClient == INVALID_SOCKET)
		{
			break;
		}
		CString *pStr = new CString;
		pStr->Format("Connecting on socket:%d", socketClient);
		pWebServerDlg->SendMessage(LOG_MSG, (UINT)pStr, NULL);
		pReq = new REQUEST;
		pReq->hExit = pWebServerDlg->m_hExit;
		pReq->Socket = socketClient;
		pReq->pWebServerDlg = pWebServerDlg;

		//�����ͻ��߳�
		pClientThread = AfxBeginThread(ClientThread, pReq);
	}
	

	//�ȴ������߳̽���
	WaitForSingleObject((HANDLE)pWebServerDlg->m_hExit, INFINITE);
	//�ȴ����пͻ��߳̽���
	dwRet = WaitForSingleObject(hNoClient, 5000);
	if (dwRet == WAIT_TIMEOUT)
	{
		CString *pStr = new CString;
		*pStr = "One or more client threads did not exit";
		pWebServerDlg->SendMessage(LOG_MSG, (UINT)pStr, NULL);
	}
	pWebServerDlg->DeleteClientCount();
	
	return 0;

}



HANDLE CWebServerDlg::InitClientCount()
{
	ClientNum = 0;
	//������no client���¼�����
	None = CreateEvent(NULL, TRUE, TRUE, NULL);
	return None;
}


void CWebServerDlg::DeleteClientCount()
{
	CloseHandle(None);
}


UINT CWebServerDlg::ClientThread(LPVOID param)
{
	PREQUEST pReq = (PREQUEST)param;
	CWebServerDlg *pWebServerDlg = (CWebServerDlg*)pReq->pWebServerDlg;
	//�ͻ�����+1
	pWebServerDlg->CountUp(); 
	while (1)
	{
		int nRet;
		char buf[1024] = { 0 };
		nRet = recv(pReq->Socket, buf, 1024, 0);
		if (!nRet)
		{
			break;
		}
		buf[nRet] = 0;

		//�����������������Ϣ
		nRet = pWebServerDlg->Analyze(pReq, buf);
		if (nRet)
		{
			break;
		}
		else
		{
			pWebServerDlg->SendHeader(pReq);
			if (pReq->nMethod == 0)
			{
				//����ķ�����GET
				pWebServerDlg->SendFile(pReq);
			}
			break;
		}

	}
	//�ͻ�������1
	pWebServerDlg->CountDown();
	pWebServerDlg->Disconnect(pReq);
	delete pReq;
	return 0;

}

void CWebServerDlg::CountUp()
{
	//�����ٽ���
	m_criSect.Lock();
	ClientNum++;
	//�뿪�����
	m_criSect.Unlock();
	//����Ϊ���ź��¼�����
	ResetEvent(None);

}

int CWebServerDlg::Analyze(PREQUEST pReq, char* pBuf)
{
	//�������յ�����Ϣ
	char szSeps[] = " \n";
	char *cpToken;
	//�ж�request��method
	cpToken = strtok(pBuf, szSeps);
	if (!_stricmp(cpToken, "GET"))
	{
		pReq->nMethod = 0;
		//ADD
		CString *pStr = new CString;
		*pStr = "GET";
		SendMessage(LOG_MSG, (UINT)pStr, NULL);
		//ADD
	}
	else
	{
		strcpy(pReq->StatuCodeReason, "501 Not Implemented");

		//ADD
		CString *pStr = new CString;
		*pStr = "501 Not Implemented";
		SendMessage(LOG_MSG, (UINT)pStr, NULL);
		//ADD

		return 1;
	}
	//��ȡRequest-URL
	cpToken = strtok(NULL, szSeps);
	strcpy(pReq->szFileName, m_strRootDir);
	if (strlen(cpToken) > 1)
	{
		strcat(pReq->szFileName, cpToken);
	}
	else
	{
		strcat(pReq->szFileName, "/index.html");
	}
	return 0;
	 
}

void CWebServerDlg::SendHeader(PREQUEST pReq)
{
	int n = FileExist(pReq);
	if (!n)
	{
		return;
	}
	char Header[2048] = "";
	char curTime[50] = "";
	GetCurentTime((char*)curTime);
	//ȡ���ļ�����
	DWORD length;
	length = GetFileSize(pReq->hFile, NULL);
	//ȡ���ļ���last-modifiedʱ��
	char last_modified[60] = "";
	GetLastModified(pReq->hFile, (char*)last_modified);
	//ȡ���ļ�����
	char ContenType[50] = "";
	GetContenType(pReq, (char*)ContenType);
	sprintf((char*)Header, "HTTP/1.0 %s\r\nDate: %s\r\nServer: %s\r\nContent-Type: %s\r\nContent-Length:%d\r\nLast-Modified: %s\r\n\r\n",
		"200 OK",
		curTime,
		"My Http Server",
		ContenType,
		length,
		last_modified);
	
	//����ͷ��
	send(pReq->Socket, Header, strlen(Header), 0);


}

void CWebServerDlg::SendFile(PREQUEST pReq)
{
	int n = FileExist(pReq);
	//���ļ������ڣ��򷵻�
	if (!n)
	{
		return;
	}
	CString *pStr = new CString;
	*pStr = *pStr + &pReq->szFileName[strlen(m_strRootDir)];
	SendMessage(LOG_MSG, UINT(pStr), NULL);
	char buf[2048];
	DWORD dwRead;
	int flag = 1;
	//��д����ֱ�����
	while (true)
	{
		//��file�ж��뵽buf
		ReadFile(pReq->hFile, buf, sizeof(buf), &dwRead, NULL);
		if (dwRead == 0)
		{
			break;
		}
		//��buf���ݴ��͸�client
		send(pReq->Socket, buf, sizeof(buf), 0);

	}
	//�ر��ļ�
	CloseHandle(pReq->hFile);
}

void CWebServerDlg::Disconnect(PREQUEST pReq)
{
	//�ر��׽��֣��ͷ���Դ
	closesocket(pReq->Socket);
	CString *pStr = new CString;
	pStr->Format("Closing socket:%d", pReq->Socket);
	SendMessage(LOG_MSG, (UINT)pStr, NULL);

}

void CWebServerDlg::CountDown()
{
	//�����ٽ���
	m_criSect.Lock();
	if (ClientNum > 0)
	{
		ClientNum--;
	}
	//�뿪�ٽ���
	m_criSect.Unlock();
	if (ClientNum < 1)
	{
		//�������ź��¼�����
		SetEvent(None);
	}

}

int CWebServerDlg::FileExist(PREQUEST pReq)
{
	pReq->hFile = CreateFile(pReq->szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	//����ļ������ڣ��򷵻س�����Ϣ
	if (pReq->hFile == INVALID_HANDLE_VALUE)
	{
		strcpy(pReq->StatuCodeReason, "404 File can not found!");
		return 0;
	}
	else
	{
		return 1;
	}
}
//����ʱ������ת��
char *week[] = { "Sun,","Mon,","Tue,","Wed,","Thu,","Fri,","Sat,", };
//�·�ת��
char *month[] = { "Jan","Feb","Mar","Apr","May","Jun","jul","Aug","Sep","Oct","Nov","Dec" ,};

void CWebServerDlg::GetCurentTime(LPSTR lpszString)
{
	//�����ʱ��
	SYSTEMTIME st;
	GetLocalTime(&st);
	//�¼���ʽ��
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[st.wDayOfWeek], st.wDay, month[st.wMonth - 1],
		st.wYear, st.wMinute, st.wSecond);

}

bool CWebServerDlg::GetLastModified(HANDLE hFile, LPSTR lpszString)
{
	//����ļ���last-modifiedʱ��
	FILETIME ftCreat, ftAccess, ftWrite;
	SYSTEMTIME stCreate;
	FILETIME ftime;
	//��ȡ�ļ���last-modified��UTCʱ��
	if (!GetFileTime(hFile, &ftCreat, &ftAccess, &ftWrite))
	{
		return false;
	}
	FileTimeToLocalFileTime(&ftWrite, &ftime);
	//UTC�ֻ���ת��Ϊ����ʱ��
	FileTimeToSystemTime(&ftime, &stCreate);
	//ʱ���ʽ��
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[stCreate.wDayOfWeek],
		stCreate.wDay, month[stCreate.wMonth - 1], stCreate.wYear, stCreate.wHour,
		stCreate.wMinute, stCreate.wSecond);

	
}

bool CWebServerDlg::GetContenType(PREQUEST pReq, LPSTR type)
{
	//ȡ���ļ�����
	CString cpToken;
	cpToken = strstr(pReq->szFileName, ".");
	strcpy(pReq->postfix, cpToken);
	//�����������ļ����Ͷ�Ӧ��content-type
	map<CString, char*>::iterator it = m_typeMap.find(pReq->postfix);
	if (it != m_typeMap.end())
	{
		wsprintf(type, "%s", (*it).second);
	}
	return true;
}

void CWebServerDlg::CreateTypeMap()
{
	// ��ʼ��map
	m_typeMap[".doc"] = "application/msword";
	m_typeMap[".bin"] = "application/octet-stream";
	m_typeMap[".dll"] = "application/octet-stream";
	m_typeMap[".exe"] = "application/octet-stream";
	m_typeMap[".pdf"] = "application/pdf";
	m_typeMap[".ai"] = "application/postscript";
	m_typeMap[".eps"] = "application/postscript";
	m_typeMap[".ps"] = "application/postscript";
	m_typeMap[".rtf"] = "application/rtf";
	m_typeMap[".fdf"] = "application/vnd.fdf";
	m_typeMap[".arj"] = "application/x-arj";
	m_typeMap[".gz"] = "application/x-gzip";
	m_typeMap[".class"] = "application/x-java-class";
	m_typeMap[".js"] = "application/x-javascript";
	m_typeMap[".lzh"] = "application/x-lzh";
	m_typeMap[".lnk"] = "application/x-ms-shortcut";
	m_typeMap[".tar"] = "application/x-tar";
	m_typeMap[".hlp"] = "application/x-winhelp";
	m_typeMap[".cert"] = "application/x-x509-ca-cert";
	m_typeMap[".zip"] = "application/zip";
	m_typeMap[".cab"] = "application/x-compressed";
	m_typeMap[".arj"] = "application/x-compressed";
	m_typeMap[".aif"] = "audio/aiff";
	m_typeMap[".aifc"] = "audio/aiff";
	m_typeMap[".aiff"] = "audio/aiff";
	m_typeMap[".au"] = "audio/basic";
	m_typeMap[".snd"] = "audio/basic";
	m_typeMap[".mid"] = "audio/midi";
	m_typeMap[".rmi"] = "audio/midi";
	m_typeMap[".mp3"] = "audio/mpeg";
	m_typeMap[".vox"] = "audio/voxware";
	m_typeMap[".wav"] = "audio/wav";
	m_typeMap[".ra"] = "audio/x-pn-realaudio";
	m_typeMap[".ram"] = "audio/x-pn-realaudio";
	m_typeMap[".bmp"] = "image/bmp";
	m_typeMap[".gif"] = "image/gif";
	m_typeMap[".jpeg"] = "image/jpeg";
	m_typeMap[".jpg"] = "image/jpeg";
	m_typeMap[".tif"] = "image/tiff";
	m_typeMap[".tiff"] = "image/tiff";
	m_typeMap[".xbm"] = "image/xbm";
	m_typeMap[".wrl"] = "model/vrml";
	m_typeMap[".htm"] = "text/html";
	m_typeMap[".html"] = "text/html";
	m_typeMap[".c"] = "text/plain";
	m_typeMap[".cpp"] = "text/plain";
	m_typeMap[".def"] = "text/plain";
	m_typeMap[".h"] = "text/plain";
	m_typeMap[".txt"] = "text/plain";
	m_typeMap[".rtx"] = "text/richtext";
	m_typeMap[".rtf"] = "text/richtext";
	m_typeMap[".java"] = "text/x-java-source";
	m_typeMap[".css"] = "text/css";
	m_typeMap[".mpeg"] = "video/mpeg";
	m_typeMap[".mpg"] = "video/mpeg";
	m_typeMap[".mpe"] = "video/mpeg";
	m_typeMap[".avi"] = "video/msvideo";
	m_typeMap[".mov"] = "video/quicktime";
	m_typeMap[".qt"] = "video/quicktime";
	m_typeMap[".shtml"] = "wwwserver/html-ssi";
	m_typeMap[".asa"] = "wwwserver/isapi";
	m_typeMap[".asp"] = "wwwserver/isapi";
	m_typeMap[".cfm"] = "wwwserver/isapi";
	m_typeMap[".dbm"] = "wwwserver/isapi";
	m_typeMap[".isa"] = "wwwserver/isapi";
	m_typeMap[".plx"] = "wwwserver/isapi";
	m_typeMap[".url"] = "wwwserver/isapi";
	m_typeMap[".cgi"] = "wwwserver/isapi";
	m_typeMap[".php"] = "wwwserver/isapi";
	m_typeMap[".wcgi"] = "wwwserver/isapi";

}
