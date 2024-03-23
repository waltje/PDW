//
// printer.c
//
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "pdw.h"
#include "printer.h"


static int printOK = 1;
static HWND PrtCancel_hDlg = NULL;


/* Printer abort function. */
static int CALLBACK
AbortFunc(HDC hdc, int err)
{
    MSG message;

    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
	if (! IsDialogMessage(PrtCancel_hDlg, &message)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
    }

    return printOK;
}


/* Let user kill print process. */
static LRESULT CALLBACK
KillPrint(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
	case WM_INITDIALOG:
		CenterWindow(hdwnd);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDCANCEL:
				printOK = 0;
				DestroyWindow(PrtCancel_hDlg);
				PrtCancel_hDlg = NULL;
				return 1;
		}
		break;
    }

    return 0;
}


/* Initialize PRINTDLG structure. */
static void
PrintInit(PRINTDLG *dlg, HWND hwnd)
{
    dlg->lStructSize = sizeof(PRINTDLG);
    dlg->hwndOwner = hwnd;
    dlg->hDevMode = NULL;
    dlg->hDevNames = NULL;
    dlg->hDC = NULL;
    dlg->Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE;
    dlg->nFromPage = 0;
    dlg->nToPage = 0;
    dlg->nMinPage = 0;
    dlg->nMaxPage = 0;
    dlg->nCopies = 1;
    dlg->hInstance = NULL;
    dlg->lCustData = 0;
    dlg->lpfnPrintHook = NULL;
    dlg->lpfnSetupHook = NULL;
    dlg->lpPrintTemplateName = NULL;
    dlg->lpSetupTemplateName = NULL;
    dlg->hPrintTemplate = NULL;
    dlg->hSetupTemplate = NULL;
}
 

// Print out buffer, 
// Seperate each line if '\n' is detected.
//
void
PrintBuffer(LPSTR lpBuffer)
{
    DOCINFO di;
    TEXTMETRIC tm;
    PRINTDLG dlg; 
    LPSTR s;
    int i, copies;
    int prtX = 0;       // current X output location
    int prtY = 0;       // current Y output location

    /* initialize PRINTDLG struct */
    PrintInit(&dlg, ghWnd);

    if (! PrintDlg(&dlg))
	return;

    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = "Print...";
    di.lpszOutput = NULL;
    di.lpszDatatype = NULL;
    di.fwType = 0;
    StartDoc(dlg.hDC, &di);

    /* get text metrics for printer */
    GetTextMetrics(dlg.hDC, &tm);

    printOK = 1;
    SetAbortProc(dlg.hDC, (ABORTPROC)AbortFunc);

    PrtCancel_hDlg = CreateDialog(ghInstance, "PrCancel", ghWnd, (DLGPROC)KillPrint);

    for (copies = 0; copies < dlg.nCopies; copies++) {
	StartPage(dlg.hDC);
	prtX = 0;
	prtY = 0;
	i = 0;
	s = lpBuffer;

	while (((*s) && (i < 15000))) {
		if (*s == '\n') {
			prtX = 0;
			prtY += tm.tmHeight + tm.tmExternalLeading;
			s++;
			continue;
		}

		if (((*s != ' ') && ((*s < 0x21) || (*s > 0x7E)))) {
			s++;
			continue;
		}

		TextOut(dlg.hDC, prtX, prtY, (LPSTR)s, 1);

		prtX += tm.tmAveCharWidth + tm.tmExternalLeading;
		s++;
		i++;
	}

	EndPage(dlg.hDC);
    }

    if (printOK) {
	if (PrtCancel_hDlg)
		DestroyWindow(PrtCancel_hDlg);
	EndDoc(dlg.hDC);
    }

    DeleteDC(dlg.hDC);
}
