
// WebServerDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include <map>
using namespace std;

//连接的client信息：
typedef struct REQUEST
{
	HANDLE	hExit;
	SOCKET Socket;
	int nMethod;
	HANDLE hFile;
	char szFileName[254];
	char postfix[10];
	char StatuCodeReason[100];
	void *pWebServerDlg;
}REQUEST, *PREQUEST;


// CWebServerDlg 对话框
class CWebServerDlg : public CDialogEx
{
// 构造
public:
	CWebServerDlg(CWnd* pParent = NULL);	// 标准构造函数

	//ADD
	bool m_bStart;
	HANDLE m_hExit;
	SOCKET m_listenSocket;
	CWinThread* m_pListenThread;
	static HANDLE None;
	static UINT ClientNum;
	static CCriticalSection m_criSect;
	map<CString, char*> m_typeMap;
	//ADD

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WEBSERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	//ADD
	void StartWebServer();
	void StopWebServer();
	static UINT ListenThread(LPVOID param);
	static UINT ClientThread(LPVOID param);
	HANDLE InitClientCount();
	void DeleteClientCount();
	void CountUp();
	void CountDown();
	void Disconnect(PREQUEST pReq);
	int Analyze(PREQUEST pReq, char* pBuf);
	void SendHeader(PREQUEST pReq);
	int FileExist(PREQUEST pReq);
	void GetCurentTime(LPSTR lpszString);
	bool GetLastModified(HANDLE hFile, LPSTR lpszString);
	void CreateTypeMap();
	void SendFile(PREQUEST pReq);
	bool GetContenType(PREQUEST pReq, LPSTR type);
	//ADD

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	//ADD
	afx_msg LRESULT AddLog(WPARAM wParam, LPARAM IPatam);
	//ADD
public:
	afx_msg void OnBnClickedButton2();
	CIPAddressCtrl LocalIP;
	CEdit localPort;
	UINT m_nPort;
	CEdit rootdir;
	CString m_strRootDir;
	CButton m_start;
	CListBox m_list;
	CButton m_exit;
	afx_msg void OnStartStop();
};

