#if !defined( MT_SERIAL_H )
#define MT_SERIAL_H
//多重定義防止

#include <windows.h>

class COMPORT
{
	HANDLE			hCom;		//Handle of This Communication Port.
	COMMPROP		Properties;	//Performance Properties of This Communication Port
	COMMTIMEOUTS	ctmo;		//Setting of Timeout Duration
	DCB				dcb;		//Setting of Serial Connection

public:
	COMPORT( LPCWSTR PORT );
	COMPORT(LPCWSTR PORT, DWORD Speed);
	operator BOOL( void )
	{
		if( hCom == INVALID_HANDLE_VALUE ) return FALSE;
		return TRUE;
	}
	void loadDefalutState();
	~COMPORT() { CloseHandle( hCom ); }
	void setBaudRate( DWORD setValue )
	{
		dcb.BaudRate = setValue;
		setState();
	}
	void setStopBits( BYTE setValue )
	{
		dcb.StopBits = setValue;
		setState();
	}
	void setParity( BYTE setValue )
	{
		dcb.Parity = setValue;
		dcb.Parity == 0 ? dcb.fParity = FALSE: dcb.fParity = TRUE;
		setState();
	}
	void setState()
	{
		SetCommState(hCom, &dcb);
		SetCommTimeouts(hCom, &ctmo);
	}
	void  purgeNow( void )	{ PurgeComm( hCom, PURGE_TXABORT | PURGE_RXABORT );	}
	void  purgeBuff( void )	{ PurgeComm( hCom, PURGE_TXCLEAR | PURGE_RXCLEAR );	}
	DWORD receive( BYTE *lpBuff, DWORD BuffLength )
	{
		DWORD	ret;
		ReadFile( hCom, lpBuff, BuffLength, &ret, NULL ); //読み込み
		return ret;
	}
	DWORD transmit( BYTE *lpBuff, DWORD BuffLength )
	{
		DWORD	ret;
		WriteFile( hCom, lpBuff, BuffLength, &ret, NULL ); //書き出し
		return ret;
	}
};

COMPORT::COMPORT(LPCWSTR PORT)
{
	hCom	= CreateFile(PORT, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, NULL, NULL);
	loadDefalutState();
	setState();
}

COMPORT::COMPORT(LPCWSTR PORT, DWORD Speed)
{
	hCom	= CreateFile(PORT, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, NULL, NULL);
	loadDefalutState();
	setBaudRate( Speed );
	setState();
}

void COMPORT::loadDefalutState()
{
	GetCommProperties(hCom, &Properties);
	GetCommState(hCom, &dcb);

	//Default Value of This Class.
	dcb.DCBlength			= sizeof(DCB);
	dcb.BaudRate			= 9600;
	dcb.fBinary				= TRUE;		//In the NT Kernel, allways keep true;
	dcb.fParity				= FALSE;
	dcb.fOutxCtsFlow		= FALSE;
	dcb.fOutxDsrFlow		= FALSE;
	dcb.fDtrControl			= DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity		= FALSE;

	dcb.fTXContinueOnXoff	= FALSE;	/* XOFF後も送信を継続	*/
	dcb.fOutX				= FALSE;	/* XON/XOFF出力フロー制御	*/
	dcb.fInX				= FALSE;	/* XON/XOFF入力フロー制御	*/
	dcb.fErrorChar			= FALSE;	/* エラーによる置換を有効	*/
	dcb.fNull				= FALSE;	/* ヌル ストリッピングを有効	*/
	dcb.fRtsControl			= FALSE;	/* RTSフロー制御	*/
	dcb.fAbortOnError		= FALSE;	/* エラー時に読み取りまたは書き込みを中止	*/
//		dcb.fDummy2				;			/* 予約済み	*/
//		dcb.wReserved			;			/* 現バージョンでは使わない	*/
//		dcb.XonLim				= ;			/* 送信におけるXONスレッショルド	*/
//		dcb.XoffLim				= ;			/* 送信におけるXOFFスレッショルド	*/

	dcb.ByteSize			= 8;
	dcb.Parity				= 0;
	dcb.StopBits			= ONESTOPBIT;	//This Memory Take ONESTOPBIT or ONE5STOPBITS or TWOSTOPBITS

	dcb.XonChar				= 0;			/* 送信および受信に対するXON文字	*/
	dcb.XoffChar			= (char)0xff;	/* 送信および受信に対するXON文字	*/
	dcb.ErrorChar			= 0;			/* エラー置換文字	*/
	dcb.EofChar				= 0;			/* 入力文字の終わり	*/
	dcb.EvtChar				= 0;			/* 受け取ったイベント文字	*/

	ctmo.ReadIntervalTimeout = 0;
	ctmo.ReadTotalTimeoutMultiplier = 0;
	ctmo.ReadTotalTimeoutConstant = 500;//50-1
	ctmo.WriteTotalTimeoutMultiplier = 0;
	ctmo.WriteTotalTimeoutConstant = 500;
}

#endif // !defined( MT_SERIAL_H )