
// WebServerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "WebServer.h"
#include "WebServerDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWebServerDlg 对话框

//ADD
//临界区代码初始化
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


// CWebServerDlg 消息处理程序

BOOL CWebServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	m_bStart = false;
	localPort.SetWindowText("");

	

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CWebServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CWebServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CWebServerDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CWebServerDlg::OnStartStop()
{
	// TODO: 在此添加控件通知处理程序代码
	this->UpdateData();
	if (!m_bStart)
	{
		StartWebServer();
		m_start.SetWindowTextA("关闭");
		LocalIP.EnableWindow(false);
		localPort.EnableWindow(false);
		rootdir.EnableWindow(false);
		m_exit.EnableWindow(false);
		m_bStart = true;
	}
	else
	{
		StopWebServer();
		m_start.SetWindowTextA("开启");
		LocalIP.EnableWindow(true);
		localPort.EnableWindow(true);
		rootdir.EnableWindow(true);
		m_exit.EnableWindow(true);
		m_bStart = false;
	}
}


//显示日志信息：
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
	//创建监听套接字
	m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	BYTE nFild[4];
	CString sIP;
	LocalIP.GetAddress(nFild[0], nFild[1], nFild[2], nFild[3]);
	sIP.Format("%d.%d.%d.%d", nFild[0], nFild[1], nFild[2], nFild[3]);

	//服务器地址
	sockaddr_in sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.S_un.S_addr = inet_addr(sIP);
	sockAddr.sin_port = htons(m_nPort);

	//初始化content-type和文件后缀对应关系的map
	CreateTypeMap();

	//绑定套接字
	bind(m_listenSocket, (sockaddr*)&sockAddr, sizeof(sockAddr));

	//开始监听
	listen(m_listenSocket, 5);


	//创建监听线程，接收客户连接要求
	m_pListenThread = AfxBeginThread(ListenThread, this);

	//显示服务器启动信息
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

	//初始化ClientNum，创建“no client”事件对象
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

		//创建客户线程
		pClientThread = AfxBeginThread(ClientThread, pReq);
	}
	

	//等待监听线程结束
	WaitForSingleObject((HANDLE)pWebServerDlg->m_hExit, INFINITE);
	//等待所有客户线程结束
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
	//创建“no client”事件对象
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
	//客户计数+1
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

		//分析浏览器的请求信息
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
				//请求的方法是GET
				pWebServerDlg->SendFile(pReq);
			}
			break;
		}

	}
	//客户计数减1
	pWebServerDlg->CountDown();
	pWebServerDlg->Disconnect(pReq);
	delete pReq;
	return 0;

}

void CWebServerDlg::CountUp()
{
	//进入临界区
	m_criSect.Lock();
	ClientNum++;
	//离开零界面
	m_criSect.Unlock();
	//重置为无信号事件对象
	ResetEvent(None);

}

int CWebServerDlg::Analyze(PREQUEST pReq, char* pBuf)
{
	//分析接收到的信息
	char szSeps[] = " \n";
	char *cpToken;
	//判断request的method
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
	//获取Request-URL
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
	//取得文件长度
	DWORD length;
	length = GetFileSize(pReq->hFile, NULL);
	//取得文件的last-modified时间
	char last_modified[60] = "";
	GetLastModified(pReq->hFile, (char*)last_modified);
	//取得文件类型
	char ContenType[50] = "";
	GetContenType(pReq, (char*)ContenType);
	sprintf((char*)Header, "HTTP/1.0 %s\r\nDate: %s\r\nServer: %s\r\nContent-Type: %s\r\nContent-Length:%d\r\nLast-Modified: %s\r\n\r\n",
		"200 OK",
		curTime,
		"My Http Server",
		ContenType,
		length,
		last_modified);
	
	//发送头部
	send(pReq->Socket, Header, strlen(Header), 0);


}

void CWebServerDlg::SendFile(PREQUEST pReq)
{
	int n = FileExist(pReq);
	//若文件不存在，则返回
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
	//读写数据直到完成
	while (true)
	{
		//从file中读入到buf
		ReadFile(pReq->hFile, buf, sizeof(buf), &dwRead, NULL);
		if (dwRead == 0)
		{
			break;
		}
		//将buf内容传送给client
		send(pReq->Socket, buf, sizeof(buf), 0);

	}
	//关闭文件
	CloseHandle(pReq->hFile);
}

void CWebServerDlg::Disconnect(PREQUEST pReq)
{
	//关闭套接字，释放资源
	closesocket(pReq->Socket);
	CString *pStr = new CString;
	pStr->Format("Closing socket:%d", pReq->Socket);
	SendMessage(LOG_MSG, (UINT)pStr, NULL);

}

void CWebServerDlg::CountDown()
{
	//进入临界区
	m_criSect.Lock();
	if (ClientNum > 0)
	{
		ClientNum--;
	}
	//离开临界区
	m_criSect.Unlock();
	if (ClientNum < 1)
	{
		//重置有信号事件对象
		SetEvent(None);
	}

}

int CWebServerDlg::FileExist(PREQUEST pReq)
{
	pReq->hFile = CreateFile(pReq->szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	//如果文件不存在，则返回出错信息
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
//格林时间星期转换
char *week[] = { "Sun,","Mon,","Tue,","Wed,","Thu,","Fri,","Sat,", };
//月份转换
char *month[] = { "Jan","Feb","Mar","Apr","May","Jun","jul","Aug","Sep","Oct","Nov","Dec" ,};

void CWebServerDlg::GetCurentTime(LPSTR lpszString)
{
	//活动本地时间
	SYSTEMTIME st;
	GetLocalTime(&st);
	//事件格式化
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[st.wDayOfWeek], st.wDay, month[st.wMonth - 1],
		st.wYear, st.wMinute, st.wSecond);

}

bool CWebServerDlg::GetLastModified(HANDLE hFile, LPSTR lpszString)
{
	//获得文件的last-modified时间
	FILETIME ftCreat, ftAccess, ftWrite;
	SYSTEMTIME stCreate;
	FILETIME ftime;
	//获取文件的last-modified的UTC时间
	if (!GetFileTime(hFile, &ftCreat, &ftAccess, &ftWrite))
	{
		return false;
	}
	FileTimeToLocalFileTime(&ftWrite, &ftime);
	//UTC手机将转化为本地时间
	FileTimeToSystemTime(&ftime, &stCreate);
	//时间格式化
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[stCreate.wDayOfWeek],
		stCreate.wDay, month[stCreate.wMonth - 1], stCreate.wYear, stCreate.wHour,
		stCreate.wMinute, stCreate.wSecond);

	
}

bool CWebServerDlg::GetContenType(PREQUEST pReq, LPSTR type)
{
	//取得文件类型
	CString cpToken;
	cpToken = strstr(pReq->szFileName, ".");
	strcpy(pReq->postfix, cpToken);
	//遍历搜索该文件类型对应的content-type
	map<CString, char*>::iterator it = m_typeMap.find(pReq->postfix);
	if (it != m_typeMap.end())
	{
		wsprintf(type, "%s", (*it).second);
	}
	return true;
}

void CWebServerDlg::CreateTypeMap()
{
	// 初始化map
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
