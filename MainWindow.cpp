/*————————————————————————————
E-debug   分析界面交互的接口
————————————————————————————*/
#include "stdafx.h"
#include "E-Debug.h"
#include "MainWindow.h"
#include "EAnalyEngine.h"
#include <windows.h>

CMainWindow *pMaindlg;		//一份窗口对应一份窗口指针
vector<CDialog*> Tab_HWND;	//窗口控件指针,这样可以传递到所有类了

IMPLEMENT_DYNAMIC(CMainWindow, CDialog)

CMainWindow::CMainWindow(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_MainWindow, pParent)
{
	
}

CMainWindow::~CMainWindow()
{

}

void CMainWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_Tab);
	DDX_Control(pDX, IDC_LOG, m_output);
}


BEGIN_MESSAGE_MAP(CMainWindow, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CMainWindow::OnTcnSelchangeTab1)
	ON_LBN_SELCHANGE(IDC_LOG, &CMainWindow::OnLbnSelchangeLog)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CMainWindow 消息处理程序


BOOL CMainWindow::OnInitDialog() {
	CDialog::OnInitDialog();
	
	pMaindlg = this;   //保存窗口指针
	g_hInstace = AfxGetInstanceHandle();  //保存实例,留以后用

	HICON hIcon = LoadIcon(g_hInstace, MAKEINTRESOURCE(IDI_ICON));	//设置窗口图标
	SetIcon(hIcon, FALSE);
	SetIcon(hIcon, TRUE);

	CRect rc;
	m_Tab.GetClientRect(&rc);
	rc.top += 26;
	rc.left += 4;
	rc.right -= 6;
	rc.bottom -= 6;

	ULONG uBase, uSize;
	Getdisassemblerrange(&uBase, &uSize);
	outputInfo("->开始分析当前区段....  分析地址: % 08X  内存大小: % 08X", uBase, uSize);

	pEAnalysisEngine = new EAnalysis(uBase);   //初始化内存
	
	Tab_HWND.clear();
	m_Tab.InsertItem(0, _T("特征库"));	//初始化特征库窗口
	m_page0.Create(IDD_PAGE0, &m_Tab);	
	m_page0.MoveWindow(&rc);
	//m_page0.ShowWindow(SW_SHOW);
	Tab_HWND.push_back(m_page0.GetHwnd());		//第一个窗口必定为特征库

	m_Tab.InsertItem(1, _T("函数识别")); //初始化函数识别窗口
	m_page1.Create(IDD_PAGE1, &m_Tab);
	m_page1.MoveWindow(&rc);
	m_page1.ShowWindow(SW_SHOW);
	Tab_HWND.push_back(m_page1.GetHwnd());		//第二个窗口必定为函数识别

	m_Tab.SetCurSel(1);
	return true;
	INT nPos;

		
		m_Tab.InsertItem(2, _T("API命令"));
		if (!pEAnalysisEngine->EStaticLibInit()) {
			outputInfo("%s", "该程序可能不是易语言静态编译程序!");
			return true;
		}
		vector<string> krnlCmd =
		{ "错误回调", "DLL命令", "三方支持库命令", "核心支持库命令",
			"读取组件属性", "设置组件属性", "分配内存", "重新分配内存",
			"释放内存", "结束", "窗口消息循环", "载入启动窗口", "初始化" };

		nPos = outputInfo("->  用户自写代码段开始 : %08X     ", pEAnalysisEngine->dwUsercodeStart);
		m_output.SetItemData(nPos, pEAnalysisEngine->dwUsercodeStart);
		Insertname(pEAnalysisEngine->dwUsercodeStart, NM_COMMENT, "用户代码段开始");

		if (!pEAnalysisEngine->GetUserEntryPoint()) {
			pEAnalysisEngine->dwUsercodeEnd = uBase + uSize - 1;
			nPos = outputInfo("->  用户自写代码段结束 : %08X     ", pEAnalysisEngine->dwUsercodeEnd);
		}
		else {
			nPos = outputInfo("->  用户自写代码段结束 : %08X     ", pEAnalysisEngine->dwUsercodeEnd);
			m_output.SetItemData(nPos, pEAnalysisEngine->dwUsercodeEnd);
			Insertname(pEAnalysisEngine->dwUsercodeEnd, NM_COMMENT, "易语言程序入口");
		}

		DWORD	dwKrnlEntry = pEAnalysisEngine->pEnteyInfo->dwEString;
		if (dwKrnlEntry == 0) {
			dwKrnlEntry = pEAnalysisEngine->pEnteyInfo->dwEWindow;
		}
		DWORD	dwPoint;

		UINT index = pEAnalysisEngine->FindOriginSection(dwKrnlEntry);

		if (index == -1) {           //区段有可能被切割,加入新的区段
			index = pEAnalysisEngine->AddSection(dwKrnlEntry);
		}

		int proadd = 100 / krnlCmd.size();
		for (int i = krnlCmd.size() - 1; i >= 0; i--)
		{
			Progress(promile = promile + proadd, "正在识别核心支持库命令");
			dwKrnlEntry -= sizeof(DWORD);
			dwPoint = pEAnalysisEngine->GetPoint(pEAnalysisEngine->O2V(dwKrnlEntry, index));
			if (i == 1) {  //获取DLL命令调用地址
				pEAnalysisEngine->DLLCALL = dwKrnlEntry;
			}
			Insertname(dwPoint, NM_LABEL, (char*)krnlCmd[i].c_str());
		}

		m_page1.Create(IDD_PAGE1, &m_Tab);
		m_page1.MoveWindow(&rc);

		m_page2.Create(IDD_PAGE2, &m_Tab);
		m_page2.MoveWindow(&rc);

		m_Tab.SetCurSel(1);
		m_page1.ShowWindow(true);
	//}
	//else if (AnalysisMode == CMode)
	{
		m_Tab.InsertItem(0, _T("函数"));
		m_Tab.InsertItem(1, _T("特征库"));

		MessageBoxA(NULL, "To Be Continued", "错误",MB_ICONINFORMATION);
		return true;
		//m_page1.Create(IDD_PAGE1, &m_Tab);
		//m_page1.MoveWindow(&rc);
	}
	
}

void CMainWindow::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	int nCursel = m_Tab.GetCurSel();
	
	for (UINT n = 0;n < Tab_HWND.size();n++) {
		Tab_HWND[n]->ShowWindow(SW_HIDE);
	}
	Tab_HWND[nCursel]->ShowWindow(SW_SHOW);
	*pResult = 0;
}

int CMainWindow::outputInfo(char *formatText, ...)
{
	USES_CONVERSION;
	char buf[10000] = { 0 };
	va_list st;
	va_start(st, formatText);
	vsprintf_s(buf, formatText, st);
	va_end(st);
	return m_output.InsertString(-1, A2W(buf));
}


void CMainWindow::OnLbnSelchangeLog()
{
	int		nCurSel;
	DWORD	dwItemData;
	nCurSel = m_output.GetCurSel();
	dwItemData = m_output.GetItemData(nCurSel);
	if (dwItemData != 0)
	{
		Setcpu(NULL, dwItemData, NULL, NULL, CPU_NOFOCUS);
	}
}




void CMainWindow::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (MessageBox(L"确定要退出分析窗口吗？", L"退出提示", MB_ICONINFORMATION | MB_YESNO) == IDNO)
		return; 

	pEAnalysisEngine->~EAnalysis();

	CDialog::OnClose();
}
