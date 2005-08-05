// **********************************************************************
//
// Copyright (c) 2003-2005 ZeroC, Inc. All rights reserved.
//
// This copy of Ice-E is licensed to you under the terms described in the
// ICEE_LICENSE file included in this distribution.
//
// **********************************************************************


#include "stdafx.h"
#include "IceE/SafeStdio.h"
#include "Router.h"
#include "ChatClient.h"
#include "ChatConfigDlg.h"

#ifdef ICEE_HAS_ROUTER


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class ChatCallbackI : public Demo::ChatCallback
{
public:

    ChatCallbackI(LogIPtr log)
        : _log(log)
    {
    }

    virtual void
    message(const std::string& data, const Ice::Current&)
    {
        _log->message(data);
    }

private:

    LogIPtr _log;
};

CChatConfigDlg::CChatConfigDlg(const Ice::CommunicatorPtr& communicator, const LogIPtr& log, 
			       CChatClientDlg* mainDiag, const CString& user, const CString& password,
			       const CString& host, const CString& port, CWnd* pParent /*=NULL*/) :
    CDialog(CChatConfigDlg::IDD, pParent), _communicator(communicator), _log(log), _mainDiag(mainDiag),
    _user(user), _password(password), _host(host), _port(port)
{
    _hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void
CChatConfigDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CChatConfigDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_LOGIN, OnLogin)
END_MESSAGE_MAP()

BOOL
CChatConfigDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    // when the application's main window is not a dialog
    SetIcon(_hIcon, TRUE);         // Set big icon
    SetIcon(_hIcon, FALSE);        // Set small icon

    //
    // Retrieve the text input edit control.
    //
    _useredit = (CEdit*)GetDlgItem(IDC_USER);
    _passedit = (CEdit*)GetDlgItem(IDC_PASSWORD);
    _hostedit = (CEdit*)GetDlgItem(IDC_HOST);
    _portedit = (CEdit*)GetDlgItem(IDC_PORT);

    //
    // Fill the windows with last info entered.
    //
    _useredit->SetWindowText(_user);
    _passedit->SetWindowText(_password);
    _hostedit->SetWindowText(_host);
    _portedit->SetWindowText(_port);

    //
    // Set the focus to the username input
    //
    _useredit->SetFocus();
 
    return FALSE; // return FALSE because we explicitly set the focus
}

void
CChatConfigDlg::OnCancel()
{
    CDialog::OnCancel();
}

//
// If you add a minimize button to your dialog, you will need the code below
// to draw the icon.  For MFC applications using the document/view model,
// this is automatically done for you by the framework.
//
void
CChatConfigDlg::OnPaint() 
{
#ifdef _WIN32_WCE
    CDialog::OnPaint();
#else
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, _hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
#endif
}

//
// The system calls this function to obtain the cursor to display while the user drags
// the minimized window.
//
HCURSOR
CChatConfigDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(_hIcon);
}


void
CChatConfigDlg::OnLogin()
{
    //
    // Retrieve the entered info from the edit controls.
    //
    _useredit->GetWindowText(_user);
    _passedit->GetWindowText(_password);
    _hostedit->GetWindowText(_host);
    _portedit->GetWindowText(_port);

    std::string user;
    std::string password;
    std::string host;
    std::string port;
#ifdef _WIN32_WCE
    char buffer[64];
    wcstombs(buffer, _user, 64);
    user = buffer;

    wcstombs(buffer, _password, 64);
    password = buffer;

    wcstombs(buffer, _host, 64);
    host = buffer;

    wcstombs(buffer, _port, 64);
    port = buffer;
#else
    user = _user;
    password = _password;
    host = _host;
    port = _port;
#endif

    bool success = false;
    try
    {
    	std::string routerStr = Ice::printfToString("Glacier2/router:tcp -p %s -h %s", port.c_str(), host.c_str());

        Glacier2::RouterPrx router = Glacier2::RouterPrx::checkedCast(_communicator->stringToProxy(routerStr));
        if(router)
	{
	    _communicator->setDefaultRouter(router);

	    Ice::PropertiesPtr properties = _communicator->getProperties();
	    properties->setProperty("Chat.Client.Router", routerStr);
	    properties->setProperty("Chat.Client.Endpoints", "");

	    Demo::ChatSessionPrx session = Demo::ChatSessionPrx::uncheckedCast(router->createSession(user, password));

            std::string category = router->getServerProxy()->ice_getIdentity().category;
            Ice::Identity callbackReceiverIdent;
            callbackReceiverIdent.name = "callbackReceiver";
            callbackReceiverIdent.category = category;

            Ice::ObjectAdapterPtr adapter = _communicator->createObjectAdapter("Chat.Client");
            Demo::ChatCallbackPrx callback = Demo::ChatCallbackPrx::uncheckedCast(
                adapter->add(new ChatCallbackI(_log), callbackReceiverIdent));
            adapter->activate();

            session->setCallback(callback);
	    _mainDiag->setSession(session, _user, _password, _host, _port);
	    success = true;
	}
	else
        {
            AfxMessageBox(CString("Configured router is not a Glacier2 router"), MB_OK|MB_ICONEXCLAMATION);
        }

    }
    catch(const Glacier2::CannotCreateSessionException& ex)
    {
        AfxMessageBox(CString(ex.reason.c_str()), MB_OK|MB_ICONEXCLAMATION);
    }
    catch(const Ice::Exception& ex)
    {
        AfxMessageBox(CString(ex.toString().c_str()), MB_OK|MB_ICONEXCLAMATION);
    }
    
    if(success)
    {
        EndDialog(0);
    }
}

#endif // ICEE_HAS_ROUTER
