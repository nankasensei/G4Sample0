// FTconsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include "PDI.h"
#include <fstream>

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <math.h>
#include <locale.h>

#include "time.h"

#include "serial.h"

using namespace std;
typedef basic_string<TCHAR> tstring;
#if defined(UNICODE) || defined(_UNICODE)
#define tcout std::wcout
#else
#define tcout std::cout
#endif

#define BUFFLEN 2048

COMPORT	*hCom1; //COM PORTÇÃégóp
#define PI 3.141592

const char ID1 = 1;
const char ID2 = 2;
const char ID3 = 3;
const float CURRENT1 = 2.0f;//mA
const float CURRENT2 = 2.0f;
const float CURRENT3 = 2.0f;
const float OFFSET = 0.95;
char POLE1;
char POLE2;
char POLE3;
const int TIME_A = 5000;//ms
const int TIME_B = 5000;//ms
const int TIME_C = 5000;//ms

LPCWSTR buf2;

int timeNow, datf1, datf2, datf3;
float dat1, dat2, dat3;
int currentScale1, currentScale2, currentScale3, currentUnit, Max;
char buffer[10];

float dataOfID0;
float offset;
float offsetHead;
int timeStart;
bool flag;

void Init() {
	hCom1 = NULL;
	//
	char text[] = "COM4";
	wchar_t wtext[20];
	mbstowcs(wtext, text, strlen(text) + 1);//Plus null
	LPWSTR ptr = wtext;
	//
	hCom1 = new COMPORT(ptr, 9600); //COM1Å`COM9Ç‹Ç≈

	timeNow = 0;
	dat1 = dat2 = dat3 = 0;
	POLE1 = 0;
	POLE2 = 0;
	POLE3 = 0;

	currentScale1 = (CURRENT1 / 2.0f) * 4095 * OFFSET;
	currentScale2 = (CURRENT2 / 2.0f) * 4095 * OFFSET;
	currentScale3 = (CURRENT3 / 2.0f) * 4095 * OFFSET;

	currentUnit = (0.1f / 2.0f) * 4095 * OFFSET;
	Max = 4095 * OFFSET;


	flag = true;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
CPDIg4		g_pdiDev;
DWORD		g_dwFrameSize;
BOOL		g_bCnxReady;
DWORD		g_dwStationMap;
DWORD		g_dwLastHostFrameCount;
ePDIoriUnits g_ePNOOriUnits;



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

typedef enum
{
	CHOICE_CONT
	, CHOICE_SINGLE
	, CHOICE_QUIT
	, CHOICE_ENTER
	, CHOICE_HUBMAP
	, CHOICE_OPTIONS

	, CHOICE_NONE = -1
} eMenuChoice;

#define ESC	0x1b
#define ENTER 0x0d
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL		Initialize			( VOID );
BOOL		Connect				( VOID );
VOID		Disconnect			( VOID );
BOOL		SetupDevice			( VOID );
VOID		UpdateStationMap	( VOID );
VOID		AddMsg				( tstring & );
VOID		AddResultMsg		( LPCTSTR );
VOID		ShowMenu			( VOID );	
eMenuChoice Prompt				( VOID );
BOOL		StartCont			( VOID );
BOOL		StopCont			( VOID );	
VOID		DisplayCont			( VOID );
VOID		DisplaySingle		( VOID );
VOID		ParseG4NativeFrame	( PBYTE, DWORD );
eMenuChoice		CnxPrompt			( VOID );
VOID SetOriUnits( ePDIoriUnits eO );
VOID SetPosUnits( ePDIposUnits eP );
VOID ShowOptionsMenu( VOID );
VOID OptionsPrompt( VOID );

#define BUFFER_SIZE 0x1FA400   // 30 seconds of xyzaer+fc 8 sensors at 240 hz 
//BYTE	g_pMotionBuf[0x0800];  // 2K worth of data.  == 73 frames of XYZAER
BYTE	g_pMotionBuf[BUFFER_SIZE]; 
TCHAR	g_G4CFilePath[_MAX_PATH+1];
#define DEFAULT_G4C_PATH _T("C:\\Polhemus\\G4 Files\\default.g4c")

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int _tmain(int argc, TCHAR* argv[])
{
	eMenuChoice eChoice = CHOICE_NONE;
	BOOL		bDisconnect = TRUE;

	Init();
	
	if (!(Initialize()))
	{
	}

	else if (CnxPrompt() == CHOICE_QUIT)
	{
		eChoice = CHOICE_QUIT;
		bDisconnect = FALSE;
	}

	//Connect To Tracker
	else if (!( Connect()))
	{
		tcout << _T("Connect ");
		tcout << g_G4CFilePath << _T(" Failed. ") << endl;
	}

	//Configure Tracker
	else if (!( SetupDevice()))
	{
	}
	else
	{
		//Wait for go-ahead for cont mode
		ShowMenu();
		do
		{
			eChoice = Prompt();

			switch (eChoice)
			{
			case CHOICE_CONT:
				//Start Cont Mode
				if (!(StartCont()))
				{
				}
				else
				{
					//Collect/Display Tracker Data until ESC
					DisplayCont();
					StopCont();
				}

				break;

			case CHOICE_SINGLE:
				DisplaySingle();
				break;

			case CHOICE_HUBMAP:
				UpdateStationMap();
				break;

			case CHOICE_OPTIONS:
				OptionsPrompt();
				break;

			default:
				break;
			}

		} while (eChoice != CHOICE_QUIT);


	}

	INT ch;

	if (eChoice == CHOICE_NONE)
	{
		tcout << _T("\n\nPress any key to exit. ") << endl;
		ch = _gettche();
		tcout << (TCHAR)ch << endl;
	}

	//Close Tracker Connection
	if (bDisconnect)
		Disconnect();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL Initialize( VOID )
{
	BOOL	bRet = TRUE;

	g_pdiDev.Trace(TRUE, 7);

	::SetConsoleTitle( _T("G4console") );

	_tcsncpy_s(g_G4CFilePath, _countof(g_G4CFilePath), DEFAULT_G4C_PATH, _tcslen(DEFAULT_G4C_PATH));

	g_bCnxReady = FALSE;
	g_dwStationMap = 0;
	g_ePNOOriUnits = E_PDI_ORI_QUATERNION;

	return bRet;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
eMenuChoice CnxPrompt( VOID )
{
	BOOL bQuit = FALSE;
	eMenuChoice eChoice = CHOICE_NONE;

	do
	{
		tcout << _T("Connecting to G4 using G4 Source Config File: ");
		tcout << g_G4CFilePath << endl;
		tcout << _T("Press\n Enter to continue,\n ESC to quit,\n or type in new .g4c file pathname followed by Enter.") << endl;
		tcout << _T(">> ");

		INT ch;

		ch = _gettche();
		
		switch (ch)
		{
		case ESC:
			eChoice = CHOICE_QUIT;
			bQuit = TRUE;
			break;
		case ENTER:
			eChoice = CHOICE_ENTER;
			break;
		default:
			{
				//_ungettch( ch );

				TCHAR newpath[_MAX_PATH+1];
				size_t newsize;

				newpath[0] = (TCHAR)ch;
				if (0 == _cgetts_s(&newpath[1], _MAX_PATH, &newsize))
				{
					eChoice = CHOICE_ENTER;
					_tcsncpy_s(g_G4CFilePath, newpath, newsize+1);
				}

			}
			break;
		}

	} while (eChoice == CHOICE_NONE);

	return eChoice;
}

VOID ShowOptionsMenu( VOID )
{
	tcout << _T("\n\nPlease select a command option: \n\n");
	tcout << _T("E or e:  Output Ori Euler Angles (degrees)\n");
	tcout << _T("R or r:  Output Ori Euler Angles (radians)\n");
	tcout << _T("Q or q:  Output Ori Quaternion (default)\n");
	tcout << _T("C or c:  Output Pos CM\n");
	tcout << _T("I or i:  Output Pos Inches (default)\n");
	tcout << _T("Enter:   Redisplay Options Menu\n");
	tcout << endl;
	tcout << _T("ESC:     Exit Options Menu\n");
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
VOID OptionsPrompt( VOID )
{
	BOOL bQuit = FALSE;

	ShowOptionsMenu();
	do
	{
		INT ch;

		tcout << _T("\nEe/Rr/Qq/Cc/Ii/ESC>> ");
		ch = _getche();
		ch = toupper( ch );
	
		switch (ch)
		{
		case ESC:
			bQuit = TRUE;
			break;
		case ENTER:
			ShowOptionsMenu();
			break;
		case 'E':
			SetOriUnits(E_PDI_ORI_EULER_DEGREE);
			break;
		case 'R':
			SetOriUnits(E_PDI_ORI_EULER_RADIAN);
			break;
		case 'Q':
			SetOriUnits(E_PDI_ORI_QUATERNION);
			break;
		case 'C':
			SetPosUnits(E_PDI_POS_CM);
			break;
		case 'I':
			SetPosUnits(E_PDI_POS_INCH);
			break;
		default:
			tcout << _T("\nUnsupported Selection\n");
			break;
		}

	} while (bQuit == FALSE);


}

VOID SetOriUnits( ePDIoriUnits eO )
{
	g_pdiDev.SetPNOOriUnits( eO );
	AddResultMsg(_T("SetPNOOriUnits") );

	g_ePNOOriUnits = eO;
}

VOID SetPosUnits( ePDIposUnits eP )
{
	g_pdiDev.SetPNOPosUnits( eP );
	AddResultMsg(_T("SetPNOPosUnits") );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL Connect( VOID )
{
    tstring msg;
    if (!(g_pdiDev.CnxReady()))
    {
 		g_pdiDev.ConnectG4( g_G4CFilePath );

		msg = _T("ConnectG4") + tstring( g_pdiDev.GetLastResultStr() ) + _T("\r\n");

        g_bCnxReady = g_pdiDev.CnxReady();
		AddMsg( msg );		
	}
    else
    {
        msg = _T("Already connected\r\n");
        g_bCnxReady = TRUE;
	    AddMsg( msg );
    }

	return g_bCnxReady;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID Disconnect()
{
    tstring msg;
    if (!(g_pdiDev.CnxReady()))
    {
        //msg = _T("Already disconnected\r\n");
    }
    else
    {
        g_pdiDev.Disconnect();
        msg = _T("Disconnect result: ") + tstring(g_pdiDev.GetLastResultStr()) + _T("\r\n");
		AddMsg( msg );
    }

}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL SetupDevice( VOID )
{
	g_pdiDev.SetPnoBuffer( g_pMotionBuf, BUFFER_SIZE );
	AddResultMsg(_T("SetPnoBuffer") );

	g_pdiDev.StartPipeExport();
	AddResultMsg(_T("StartPipeExport") );

	UpdateStationMap();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID UpdateStationMap( VOID )
{
	g_pdiDev.GetStationMap( g_dwStationMap );
	AddResultMsg(_T("GetStationMap") );

	TCHAR szMsg[100];
	_stprintf_s(szMsg, _countof(szMsg), _T("ActiveStationMap: %#x\r\n"), g_dwStationMap );

	AddMsg( tstring(szMsg) );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID AddMsg( tstring & csMsg )
{
	tcout << csMsg ;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID AddResultMsg( LPCTSTR szCmd )
{
	tstring msg;

	//msg.Format("%s result: %s\r\n", szCmd, m_pdiDev.GetLastResultStr() );
	msg = tstring(szCmd) + _T(" result: ") + tstring( g_pdiDev.GetLastResultStr() ) + _T("\r\n");
	AddMsg( msg );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID ShowMenu( VOID )
{

	tcout << _T("\n\nPlease enter select a data command option: \n\n");
	tcout << _T("C or c:  Continuous Motion Data Display\n");
	tcout << _T("P or p:  Single Motion Data Frame Display\n");
	tcout << _T("H or h:  Update Hub/Station Map\n");
	tcout << _T("O or o:  Options Menu\n");
	tcout << endl;
	tcout << _T("ESC:     Quit\n");
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
eMenuChoice Prompt( VOID )
{
	eMenuChoice eRet = CHOICE_NONE;
	INT			ch;

	do
	{
		tcout << _T("\nCc/Pp/Hh/Oo/ESC>> ");
		ch = _getche();
		ch = toupper( ch );

		switch (ch)
		{
		case 'C':
			eRet = CHOICE_CONT;
			break;
		case 'P':
			eRet = CHOICE_SINGLE;
			break;
		case 'H':
			eRet = CHOICE_HUBMAP;
			break;
		case 'O':
			eRet = CHOICE_OPTIONS;
			break;
		case ESC: // ESC
			eRet = CHOICE_QUIT;
			break;
		default:
			break;
		}
	} while (eRet == CHOICE_NONE);

	return eRet;
}



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
BOOL StartCont( VOID )
{
	BOOL bRet = FALSE;


	if (g_dwStationMap==0)
	{
		UpdateStationMap();
	}

	g_pdiDev.ResetHostFrameCount();
	g_dwLastHostFrameCount = 0;

	if (!(g_pdiDev.StartContPnoG4(0)))
	{
	}
	else
	{
		bRet = TRUE;
		Sleep(1000);  
	}
	AddResultMsg(_T("\nStartContPnoG4") );

	return bRet;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
BOOL StopCont( VOID )
{
	BOOL bRet = FALSE;

	if (!(g_pdiDev.StopContPno()))
	{
	}
	else
	{
		bRet = TRUE;
		Sleep(1000);
	}
	AddResultMsg(_T("StopContPno") );

	return bRet;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
VOID DisplayCont( VOID )
{
	BOOL bExit = FALSE;

	//tcout << _T("\nPress any key to start continuous display\r\n");
	//tcout << _T("\nPress ESC to stop.\r\n");
	//tcout << _T("\nReady? ");
	//_getche();
	//tcout << endl;

	INT  count = 10;
	PBYTE pBuf;
	DWORD dwSize;
	DWORD dwFC;

	do
	{
		if (!(g_pdiDev.LastHostFrameCount( dwFC )))
		{
			AddResultMsg(_T("LastHostFrameCount"));
			bExit = TRUE;
		}
		else if (dwFC == g_dwLastHostFrameCount)
		{
			// no new frames since last peek
		}
		else if (!(g_pdiDev.LastPnoPtr( pBuf, dwSize )))
		{
			AddResultMsg(_T("LastPnoPtr"));
			bExit = TRUE;
		}
		else if ((pBuf == 0) || (dwSize == 0))
		{
		}
		else 
		{
			g_dwLastHostFrameCount = dwFC;
			ParseG4NativeFrame( pBuf, dwSize );
		}

		if ( _kbhit() && ( _getch() == ESC ) ) 
		{
			bExit = TRUE;
		}

		if (count == 0)
			bExit = TRUE;

	} while (!bExit);

}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
VOID DisplaySingle( VOID )
{
	PBYTE pBuf;
	DWORD dwSize;

	cout << endl;

	if (g_dwStationMap==0)
	{
		UpdateStationMap();
	}

	if (!(g_pdiDev.ReadSinglePnoBufG4(pBuf, dwSize)))
	{
		AddResultMsg(_T("ReadSinglePno") );
	}
	else if ((pBuf == 0) || (dwSize == 0))
	{
	}
	else 
	{
		ParseG4NativeFrame( pBuf, dwSize );
	}
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//typedef struct {				// per sensor P&O data
//	UINT32 nSnsID;				//4
//	float pos[3];				//12
//	float ori[4];				//16
//}*LPG4_SENSORDATA,G4_SENSORDATA;//32 bytes
//
//typedef struct {				// per hub P&O data
//	UINT32 nHubID;								//4
//	UINT32 nFrameCount;							//4
//	UINT32 dwSensorMap;							//4
//	UINT32 dwDigIO;								//4
//	G4_SENSORDATA sd[G4_MAX_SENSORS_PER_HUB]; //96
//}*LPG4_HUBDATA,G4_HUBDATA;	//	112 bytes

void ParseG4NativeFrame( PBYTE pBuf, DWORD dwSize )
{
	if ((!pBuf) || (!dwSize))
	{}
	else
	{
		DWORD i= 0;
		LPG4_HUBDATA	pHubFrame;

		while (i < dwSize )
		{
			pHubFrame = (LPG4_HUBDATA)(&pBuf[i]);

			i += sizeof(G4_HUBDATA);

			UINT	nHubID = pHubFrame->nHubID;
			UINT	nFrameNum =  pHubFrame->nFrameCount;
			UINT	nSensorMap = pHubFrame->dwSensorMap;
			UINT	nDigIO = pHubFrame->dwDigIO;

			UINT	nSensMask = 1;

			TCHAR	szFrame[800];

			for (int j=0; j<G4_MAX_SENSORS_PER_HUB; j++)
			{
				if (((nSensMask << j) & nSensorMap) != 0)
				{
					G4_SENSORDATA * pSD = &(pHubFrame->sd[j]);
					if (g_ePNOOriUnits == E_PDI_ORI_QUATERNION)
					{
						_stprintf_s( szFrame, _countof(szFrame), _T("%3d %3d|  %05d|  0x%04x|  % 7.3f % 7.3f % 7.3f| % 8.4f % 8.4f % 8.4f % 8.4f\r\n"),
								pHubFrame->nHubID, pSD->nSnsID,
								pHubFrame->nFrameCount, pHubFrame->dwDigIO,
								pSD->pos[0], pSD->pos[1], pSD->pos[2],
								pSD->ori[0], pSD->ori[1], pSD->ori[2], pSD->ori[3] );

						if (dat1 != 0) {

							if (pSD->nSnsID == 1 && dataOfID0 != 0) {

								// file pointer 
								fstream fout;

								string fileName = "hua_1120_gyakuchikaku_28";
								// opens an existing csv file or creates a new file.  
								fout.open(fileName + ".csv", ios::out | ios::app);

								//settings when first time==0
								if (flag) {
									timeStart = clock();

									offset = dataOfID0 - pSD->ori[2];
									offsetHead = dataOfID0;

									fout << "time" << ", "
										//<< "head" << ", "
										//<< "back" << ", "
										<< "normalized_head" << ", "
										//<< "normalized_delta" << ", "
										<< "current" << ", "
										<< "\n";

									flag = false;
								}

								timeNow = clock() - timeStart;

								// Insert the data to file 
								fout << timeNow << ", "
									//<< dataOfID0 << ", "
									//<< pSD->ori[2] << ", "
									<< dataOfID0 - offsetHead << ", "
									//<< dataOfID0 - pSD->ori[2] - offset<< ", "
									<< (dat1 / currentUnit) / 10 << ", "
									<< "\n";
							}
							else if(pSD->nSnsID == 0){
								dataOfID0 = pSD->ori[2];
							}
						}

						//GVS SETTING----------------------------------------------------
						if (_kbhit()) {
							int ch = _getch();
							//std::cout << ch<<"\n";
							if (ch == 'Q') {
								break;
							}
							else if (ch == 8) {
								dat1 += currentUnit;
								//dat2 += currentUnit;
								dat3 += currentUnit;
							}
							else if (ch == 42) {
								dat1 -= currentUnit;
								//dat2 -= currentUnit;
								dat3 -= currentUnit;
							}
							else if (ch == 49) {
								dat1 = currentUnit * -6.66f;
								dat3 = currentUnit * -6.66f;
							}
							else if (ch == 50) {
								dat1 = currentUnit * -13.33f;
								dat3 = currentUnit * -13.33f;
							}
							else if (ch == 51) {
								dat1 = currentUnit * -20.0f;
								dat3 = currentUnit * -20.0f;
							}
							else if (ch == 52) {
								dat1 = currentUnit * 6.66f;
								dat3 = currentUnit * 6.66f;
							}
							else if (ch == 53) {
								dat1 = currentUnit * 13.33f;
								dat3 = currentUnit * 13.33f;
							}
							else if (ch == 54) {
								dat1 = currentUnit * 20.0f;
								dat3 = currentUnit * 20.0f;
							}
							else if (ch == 48) {
								dat1 = currentUnit * 0.0f;
								dat3 = currentUnit * 0.0f;
							}
							else if (ch == 55) {
								dat1 = currentUnit * 0.000001f;
								dat3 = currentUnit * 0.000001f;
							}
						}

						if (dat1 > Max) dat1 = Max;
						if (dat2 > Max) dat2 = Max;
						if (dat3 > Max) dat3 = Max;
						if (-dat1 > Max) dat1 = -Max;
						if (-dat2 > Max) dat2 = -Max;
						if (-dat3 > Max) dat3 = -Max;
						if (dat1 < 0) datf1 = 4096 - dat1; else datf1 = dat1;
						if (dat2 < 0) datf2 = 4096 - dat2; else datf2 = dat2;
						if (dat3 < 0) datf3 = 4096 - dat3; else datf3 = dat3;

						buffer[0] = 'G';
						buffer[1] = (ID1 << 5) + (POLE1 << 4) + (abs((int)datf1) >> 8);
						buffer[2] = (char)datf1;

						buffer[0 + 1 * 3] = 'G';
						buffer[1 + 1 * 3] = (ID2 << 5) + (POLE2 << 4) + (abs((int)datf2) >> 8);
						buffer[2 + 1 * 3] = (char)datf2;


						buffer[0 + 2 * 3] = 'G';
						buffer[1 + 2 * 3] = (ID3 << 5) + (POLE3 << 4) + (abs((int)datf3) >> 8);
						buffer[2 + 2 * 3] = (char)datf3;

						std::cout << '(' << dat1 << ", " << dat2 << ", " << dat3 << ')' ;

						hCom1->transmit((BYTE*)buffer, 3 * 3);
					}
					else
					{
						_stprintf_s( szFrame, _countof(szFrame), _T("%3d %3d|  %05d|  0x%04x|  % 7.3f % 7.3f % 7.3f| % 8.3f % 8.3f % 8.3f\r\n"),
								pHubFrame->nHubID, pSD->nSnsID,
								pHubFrame->nFrameCount, pHubFrame->dwDigIO,
								pSD->pos[0], pSD->pos[1], pSD->pos[2],
								pSD->ori[0], pSD->ori[1], pSD->ori[2] );
					}
					AddMsg( tstring(szFrame) );
				}
			}

		} // end while dwsize
	}
}

VOID DisplayFrame( PBYTE pBuf, DWORD dwSize )
{
	TCHAR	szFrame[200];
	DWORD	i = 0;

    while ( i<dwSize)
    {
		FT_BINHDR *pHdr = (FT_BINHDR*)(&pBuf[i]);
	    CHAR cSensor = pHdr->station;
		SHORT shSize = 6*(sizeof(FLOAT));

        // skip rest of header
        i += sizeof(FT_BINHDR);

		PFLOAT pPno = (PFLOAT)(&pBuf[i]);

		_stprintf_s( szFrame, _countof(szFrame), _T("%2c   %+011.6f %+011.6f %+011.6f   %+011.6f %+011.6f %+011.6f\r\n"), 
			     cSensor, pPno[0], pPno[1], pPno[2], pPno[3], pPno[4], pPno[5] );

		AddMsg( tstring( szFrame ) );

		i += shSize;
	}
}

