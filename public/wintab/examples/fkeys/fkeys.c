/*------------------------------------------------------------------------------
FKeys - using fkey input.
RICO 4/29/93
------------------------------------------------------------------------------*/

#include <windows.h>
#include "msgpack.h"
#include <wintab.h>
#define PACKETDATA	0
#define PACKETMODE	0
#define PACKETFKEYS PKEXT_ABSOLUTE
#include <pktdef.h>
#include "fkeys.h"		    

HANDLE hInst;
WTPKT FKeysMask;
UINT FKeysCat;


/* -------------------------------------------------------------------------- */
int PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HANDLE hInstance;			     
HANDLE hPrevInstance;			     
LPSTR lpCmdLine;			     
int nCmdShow;				     
{
    MSG msg;				     

    if (!hPrevInstance)			 
	if (!InitApplication(hInstance)) 
	    return (FALSE);		 

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,	   
	    NULL,		   
	    0,		   
	    0))		   
	{
	TranslateMessage(&msg);	   
	DispatchMessage(&msg);	   
    }
    return (msg.wParam);	   
}
/* -------------------------------------------------------------------------- */
BOOL InitApplication(hInstance)
HANDLE hInstance;			       
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = 0;                    
    wc.lpfnWndProc = MainWndProc;       
                                        
    wc.cbClsExtra = 0;                  
    wc.cbWndExtra = 0;                  
    wc.hInstance = hInstance;           
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "FKeysMenu";   
    wc.lpszClassName = "FKeysWClass"; 

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}
/* -------------------------------------------------------------------------- */
UINT ScanExts(UINT wTag)
{
	UINT i;
	UINT wScanTag;

	/* scan for wTag's info category. */
	for (i = 0; WTInfo(WTI_EXTENSIONS + i, EXT_TAG, &wScanTag); i++) {
		 if (wTag == wScanTag) {
			/* return category offset from WTI_EXTENSIONS. */
			return i;
		}
	}
	/* return error code. */
	return 0xFFFF;
}
/* -------------------------------------------------------------------------- */
BOOL FKeysInit(void)
{
	FKeysCat = ScanExts(WTX_FKEYS);
	if (FKeysCat != 0xFFFF) {
		WTInfo(WTI_EXTENSIONS + FKeysCat, EXT_MASK, &FKeysMask);
		return TRUE;
	}
	return FALSE;
}
/* -------------------------------------------------------------------------- */
BOOL InitInstance(hInstance, nCmdShow)
    HANDLE          hInstance;          
    int             nCmdShow;           
{
    HWND            hWnd;               
	char buf[50];

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

	/* check if WinTab available. */
	if (!WTInfo(0, 0, NULL)) {
		MessageBox(NULL, "WinTab Services Not Available.", "WinTab",
					MB_OK | MB_ICONHAND);
		return FALSE;
	}

	/* check if WinTab FKeys Extension available. */
	if (!FKeysInit()) {
		MessageBox(NULL, "WinTab Function Keys Extension Not Available.",
					"WinTab", MB_OK | MB_ICONHAND);
		return FALSE;
	}

    /* Create a main window for this application instance.  */

	wsprintf(buf, "FKeys:%x", hInst);
    hWnd = CreateWindow(
        "FKeysWClass",                
        "FKeys Sample Application",   
        WS_OVERLAPPEDWINDOW,            
        CW_USEDEFAULT,                  
        CW_USEDEFAULT,                  
        CW_USEDEFAULT,                  
        CW_USEDEFAULT,                  
        NULL,                           
        NULL,                           
        hInstance,                      
        NULL                            
    );

    /* If window could not be created, return "failure" */

    if (!hWnd)
        return (FALSE);

    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hWnd, nCmdShow);  
    UpdateWindow(hWnd);          
    return (TRUE);               

}
/* -------------------------------------------------------------------------- */
HCTX static NEAR TabletInit(HWND hWnd)
{
	LOGCONTEXT lcMine;
	AXIS a;

	/* get default region */
	WTInfo(WTI_DEFCONTEXT, 0, &lcMine);

	/* modify the digitizing region */
	wsprintf(lcMine.lcName, "FKeys %x", hInst);
	lcMine.lcOptions |= CXO_MESSAGES;
	lcMine.lcPktData = PACKETDATA | FKeysMask;
	lcMine.lcPktMode = PACKETMODE;
	lcMine.lcMoveMask = PACKETDATA | FKeysMask;
	lcMine.lcBtnUpMask = lcMine.lcBtnDnMask = 0;

	lcMine.lcInOrgX = lcMine.lcInOrgY = lcMine.lcInOrgZ = 0;
	WTInfo(WTI_DEVICES + lcMine.lcDevice, DVC_X, &a);
	lcMine.lcInExtX = a.axMax;
	WTInfo(WTI_DEVICES + lcMine.lcDevice, DVC_Y, &a);
	lcMine.lcInExtY = a.axMax;
	if (WTInfo(WTI_DEVICES + lcMine.lcDevice, DVC_Z, &a))
		lcMine.lcInExtZ = a.axMax;

	/* open the region */
	return WTOpen(hWnd, &lcMine, TRUE);

}
/* -------------------------------------------------------------------------- */
LRESULT FAR PASCAL MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;				  
unsigned message;			  
WPARAM wParam;				  
LPARAM lParam;				  
{
    FARPROC lpProcAbout;		  
	static HCTX hTab = NULL;
	static inOverlap;
	PACKET pkt;

    switch (message) {
	case WM_CREATE:
		hTab = TabletInit(hWnd);
		if (!hTab) {
			MessageBox(NULL, " Could Not Open Tablet Context.", "WinTab",
						MB_OK | MB_ICONHAND);

			SendMessage(hWnd, WM_DESTROY, 0, 0L);
		}
		break;

	case WT_PACKET:
		if (WTPacket((HCTX)lParam, wParam, &pkt)) {
			static char buf[100];

			if (pkt.pkFKeys) {
				MessageBeep(0);
			}
			wsprintf(buf, "FKeys:%x %d", hInst, pkt.pkFKeys);
			SetWindowText(hWnd, buf);
		}
		break;

	case WT_CTXOVERLAP:
		if (!inOverlap && !(lParam & CXS_ONTOP)) {
			inOverlap = TRUE;
			WTOverlap(hTab, TRUE);
			inOverlap = FALSE;
		}
		break;

	case WM_COMMAND:	   
	    if (GET_WM_COMMAND_ID(wParam, lParam) == IDM_ABOUT) {
		lpProcAbout = MakeProcInstance(About, hInst);

		DialogBox(hInst,		 
		    "AboutBox",			 
		    hWnd,			 
		    lpProcAbout);		 

		FreeProcInstance(lpProcAbout);
		break;
	    }
	    else			    
		return (DefWindowProc(hWnd, message, wParam, lParam));

	case WM_DESTROY:		  
		if (hTab)
			WTClose(hTab);
	    PostQuitMessage(0);
	    break;

	default:			  
	    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return 0;
}
/* -------------------------------------------------------------------------- */
BOOL FAR PASCAL About(hDlg, message, wParam, lParam)
HWND hDlg;                                
unsigned message;                         
WPARAM wParam;                              
LPARAM lParam;
{
    switch (message) {
	case WM_INITDIALOG:		   
	    return (TRUE);

	case WM_COMMAND:		      
	    if (GET_WM_COMMAND_ID(wParam, lParam) == IDOK                
                || GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL) {      
		EndDialog(hDlg, TRUE);	      
		return (TRUE);
	    }
	    break;
    }
    return (FALSE);			      
}
/* -------------------------------------------------------------------------- */

