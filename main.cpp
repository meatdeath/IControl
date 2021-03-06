﻿//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "main.h"
#include "options.h"
#include "string.h"
#include "stdio.h"
#include <systdate.h>
#include <DateUtils.Hpp>
#include <Registry.hpp>
#include <dir.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;

UnicodeString APP_NAME = L"RTEC - Регистратор тока";

UnicodeString Com = "";
int Baudrate = 0;
bool AxisLog;
double AxisLogBase;
bool dataSaved = true;

//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ExitActionExecute(TObject *Sender)
{
	DataClearAllBlocks();
	Close();
}
//---------------------------------------------------------------------------
int data_count = 0;

//---------------------------------------------------------------------------
#define DATA_SIZE	750
#define SEC_IN_DAY	(24.0*60*60)

#define MSEC_IN_DAY	(24.0*60*60*1000)
#define MEASURE_PERIOD_MS	150
enum {
	STATE_IDLE = 0,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_DISCONNECTING,
	STATE_DISCONNECTED
};

//---------------------------------------------------------------------------
DATA_BLOCK *data_head = NULL;
DATA_BLOCK *data_tail = NULL;
int state = STATE_IDLE;
//---------------------------------------------------------------------------

void DataBlockAdd( TDateTime time, const void *data_ptr, WORD len )
{
	int offset = 0;
	BYTE *data = (char*)data_ptr;

//	if( data_head == NULL ) {
//		int err0 = 0, err1 = 0;
//		for( int i = 0; i < len; i++ ) {
//			if( (i&1) == 0 )
//				err0 += (data[i]>0x07)?1:0;
//			else
//				err1 += (data[i]>0x07)?1:0;
//		}
//
//		if( err0 > (len/2) && err1 > (len/2) )
//			return;
//
//		if( err0 > err1 ) {
//			offset = 1;
//			len--;
//		}
//	}
	DATA_BLOCK *new_data_block;
	try {
		new_data_block = new DATA_BLOCK;
		new_data_block->time = time;
		new_data_block->next = NULL;
		new_data_block->current_arr = new BYTE(len);

		memcpy( new_data_block->current_arr, &(data[offset]), len );
		new_data_block->current_arr_len = len;

		if( data_tail == NULL )
		{
			data_head = new_data_block;
			data_tail = new_data_block;
		}
		else
		{
			data_tail->next = new_data_block;
			data_tail = data_tail->next;
		}
	} catch (...) {
	}
}

//---------------------------------------------------------------------------

DATA_BLOCK* GetLastDataBlockPtr() {
	return data_tail;
}

//---------------------------------------------------------------------------

void DataClearAllBlocks()
{
	while( data_head != NULL )
	{
		DATA_BLOCK *next = data_head->next;
		delete data_head->current_arr;
		delete data_head;
		data_head = next;
	}
	data_tail = NULL;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::GenerateData()
{
	WORD data[DATA_SIZE] = {0};
	WORD data2[DATA_SIZE] = {0};
	TDateTime time1, time2;
	TDateTime tick = (1.0*MEASURE_PERIOD_MS) / MSEC_IN_DAY;
	TDateTime delay = (750.0*MEASURE_PERIOD_MS) / MSEC_IN_DAY;
	time2 = Now();
	time1 = time2 - delay;
	WORD gen_value = 1000;
	WORD gen_value2 = 130;

	randomize();
	for( int i = 0; i < DATA_SIZE; i++ )
	{
		int step = rand()%100 - 50;
		if( (gen_value < 800 && step < 0) || (gen_value > 1300 && step > 0) )
		{
			step = -step;
		}
		gen_value = gen_value + step;
		data[i] = gen_value;

		step = rand()%10 - 5;
		if( (gen_value2 < 100 && step < 0) || (gen_value2 > 180 && step > 0) )
		{
			step = -step;
		}
		gen_value2 = gen_value2 + step;
		data2[i] = gen_value2;
	}

	int i;
	TColor color;
	TDateTime time;
	UnicodeString time_str;
	double value, value2;

	for( i = 0; i < DATA_SIZE; i++, data_count++ )
	{
		color = clGreen;
		//value = data_tail->current_arr[i];
		value = data[i];
		//value2 = data2[i];
		if( value >= 1000 ) color = clRed;
		tick = ((double)i*MEASURE_PERIOD_MS) / (MSEC_IN_DAY);
		time = time1 + tick;
		time_str = time.FormatString("hh:nn:ss.zzz");
		// time_str = time.TimeString();
		CurrentSeries->AddXY( data_count, value, time_str, color );
	}

	ScrollBar->Max = data_count;
	UpdateScroll();

	DataBlockAdd( time1, data, data_count );
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::UpdateActionState(int new_state)
{
	state = new_state;
	if( data_head != NULL )
	{
		MagnifyPlusAction->Enabled = true;
		MagnifyMinusAction->Enabled = true;
	}
	else
	{
		MagnifyPlusAction->Enabled = false;
		MagnifyMinusAction->Enabled = false;
    }
	switch( state )
	{
		case STATE_IDLE:
			if( data_head == NULL )
			{
				SaveAction->Enabled = false;
			}
			else
			{
				SaveAction->Enabled = true;
			}
			NewAction->Enabled = true;
			OpenAction->Enabled = true;
			OptionsAction->Enabled = true;
			ConnectAction->Enabled = true;
			DisconnectAction->Enabled = false;
			break;

		case STATE_CONNECTING:
			DataClearAllBlocks();

		case STATE_DISCONNECTING:
			SaveAction->Enabled = false;
			NewAction->Enabled = false;
			OpenAction->Enabled = false;
			OptionsAction->Enabled = false;
			ConnectAction->Enabled = false;
			DisconnectAction->Enabled = false;
			break;

		case STATE_CONNECTED:
			DisconnectAction->Enabled = true;
			ConnectAction->Enabled = false;
			SaveAction->Enabled = false;
			OpenAction->Enabled = false;
			break;
    }
}
//---------------------------------------------------------------------------
int scroll_bar_page_size = 200;
void __fastcall TMainForm::UpdateScroll()
{
	ScrollBar->Min = 0;
	//ScrollBar->Position = 0;
	if(!data_count) return;
	ScrollBar->Max = data_count;
	ScrollBar->PageSize = scroll_bar_page_size;
	//ScrollBar->Position = ScrollBar->Max - ScrollBar->PageSize;
	ScrollBar->LargeChange = scroll_bar_page_size;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ConnectActionExecute(TObject *Sender)
{
	if( !dataSaved ) {
		if( Application->MessageBox( L"Данные не сохранены. Все равно продолжить и потерять раннее сохраненные данные?", L"Внимание", MB_ICONWARNING|MB_YESNO ) == IDNO )
			return;
	}
	CurrentSeries->Clear();
	data_count = 0;
	UpdateActionState( STATE_CONNECTING );
	Timer->Interval = 10;
	//GenerateData();
	Timer->Enabled = true;

	TComThread *thrd = new TComThread();
	thrd->OnTerminate = &ThreadTerminated;
	thrd->Start();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::ChartZoom(TObject *Sender)
{
	Application->ProcessMessages();
	scroll_bar_page_size = Chart->BottomAxis->Maximum - Chart->BottomAxis->Minimum;
	UpdateScroll();
	ScrollBar->Position = Chart->BottomAxis->Minimum;
//	Chart->BottomAxis->Minimum = ScrollBar->Position;
//	Chart->BottomAxis->Maximum = ScrollBar->Position + ScrollBar->PageSize;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::DisconnectActionExecute(TObject *Sender)
{
	Timer->Interval = 10;
	UpdateActionState( STATE_DISCONNECTING );
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::ChartUndoZoom(TObject *Sender)
{
	Application->ProcessMessages();
	scroll_bar_page_size = 200;
	UpdateScroll();
//	int pos = ScrollBar->Position - (200 - (Chart->BottomAxis->Maximum - Chart->BottomAxis->Minimum)) / 2;
//	if ( (pos+scroll_bar_page_size) > data_count) {
//		pos = data_count - scroll_bar_page_size;
//	}
//	if (pos < 0) pos = 0;
//	ScrollBar->Position = pos;
	ScrollBar->Position = Chart->BottomAxis->Minimum;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormResize(TObject *Sender)
{
	Chart->BottomAxis->Maximum = Chart->BottomAxis->Minimum + (Chart->Width-108)/10;
	UpdateScroll();
}
//---------------------------------------------------------------------------

#define BUF_SIZE 10000
HANDLE hPort = NULL;
BYTE buf[BUF_SIZE] = {0};
BYTE buf_index = 0;

bool show_value_assembled = true;
WORD show_value;
int progress_color = 0;

#define READ_BUF_SIZE  BUF_SIZE


void __fastcall TMainForm::TimerTimer(TObject *Sender)
{
	bool fSuccess;
	Timer->Interval = 100;

	Timer->Enabled = false;
	switch( state )
	{
	case STATE_IDLE:
		break;

	case STATE_CONNECTING:
		UpdateActionState( STATE_CONNECTED );
		progress_color = 0;
		StatusBar1->Panels->Items[0]->Text = L"Запись данных...";
		show_value_assembled = true;
		break;

	case STATE_CONNECTED:
		for( int x = 306; x <= 426; x += 24 ) {
			StatusBar1->Canvas->Brush->Color = clGreen + progress_color;
			StatusBar1->Canvas->Rectangle(x,5,x+20,16);
			if( x <= 402 ) {
				progress_color += 0x000800;
				if( progress_color >= 0x003000 ) progress_color = 0;
			}
		}
		break;

	case STATE_DISCONNECTING:
		break;

	case STATE_DISCONNECTED:
		StatusBar1->Repaint();
		StatusBar1->Panels->Items[0]->Text = L"";
		UpdateActionState( STATE_IDLE );

		data_count = 0;

		if( data_head != NULL ) {

			DATA_BLOCK *data_ptr = data_head;
			TColor color;
			UnicodeString time_str;
			double value;
			//double tick = 0.020;

			while(data_ptr) {
				int err0 = 0, err1 = 0;
				for( int i = 0; i < data_ptr->current_arr_len; i++ ) {
					if( (i&1) == 0 )
						err0 += (data_ptr->current_arr[i]>0x07)?1:0;
					else
						err1 += (data_ptr->current_arr[i]>0x07)?1:0;
				}

				if( err0 < (data_ptr->current_arr_len/2) || err1 < (data_ptr->current_arr_len/2) ) {
					if( err0 > err1 ) {
						show_value_assembled = false;
					} else {
						show_value_assembled = true;
					}
					break;
				}
				break;
			}

			const double data_byte_time = 1.0/(24.0*60.0*60.0*1000.0);
			const double data_word_time = 2.0/(24.0*60.0*60.0*1000.0);

			const double data_big_diff_time = 200.0/(24.0*60.0*60.0*1000.0);

			TDateTime  time1 = 0;

			while( data_ptr ) {
				TDateTime time2 = data_ptr->time;
				TDateTime time0 = time2 - (double) ( data_word_time * data_ptr->current_arr_len/2 );

				if( time1.Val < (time0.Val-data_big_diff_time) ||
					time1.Val > (time0.Val+data_big_diff_time) )
				{
                    time1 = time0;
                }

				if( !show_value_assembled && (data_ptr->current_arr_len&1) )
					time1 -= data_word_time;
				else
				if(  show_value_assembled && (data_ptr->current_arr_len&1) )
					time1 -= data_byte_time;
				else
				if( !show_value_assembled && !(data_ptr->current_arr_len&1) )
					time1 += data_byte_time;

				for( int i = 0; i < data_ptr->current_arr_len; i++ ) {
					show_value_assembled = !show_value_assembled;

					if( show_value_assembled )
					{
						show_value |= data_ptr->current_arr[i];
						color = clGreen;
						value = (double)show_value;
						if( value >= 1000 ) color = clRed;
						time_str = time1.FormatString("HH:nn:ss.zzz");
						CurrentSeries->AddXY( data_count++, value, time_str, color );
						time1 += 2 * (1.0/(24.0*60.0*60.0*1000.0));
						dataSaved = false;
//						str.sprintf(L"%02X %02X : ",buf[i], buf[i+1]);
//						line+=str;
					}  else {
						show_value = data_ptr->current_arr[i];
						show_value <<= 8;
                    }
				}
				data_ptr = data_ptr->next;
			}
			UpdateScroll();
			ScrollBar->Position = ScrollBar->Max - ScrollBar->PageSize;
		}

		break;
	}

	Timer->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ScrollBarChange(TObject *Sender)
{
	if( ScrollBar->Position > (ScrollBar->Max-ScrollBar->PageSize) )
	{
		ScrollBar->Position = ScrollBar->Max - ScrollBar->PageSize;
	}

	if( ScrollBar->Position > Chart->BottomAxis->Minimum )
	{
		Chart->BottomAxis->Maximum = ScrollBar->Position + ScrollBar->PageSize;
		Chart->BottomAxis->Minimum = ScrollBar->Position;
	}
	else
	{
		Chart->BottomAxis->Minimum = ScrollBar->Position;
		Chart->BottomAxis->Maximum = ScrollBar->Position + ScrollBar->PageSize;
	}
}
//---------------------------------------------------------------------------

// Example got from
// https://stackoverflow.com/questions/11639859/threads-in-c-builder
//
// For methods and properties go to page
// http://docs.embarcadero.com/products/rad_studio/delphiAndcpp2009/HelpUpdate2/EN/html/delphivclwin32/!!MEMBEROVERVIEW_Classes_TThread.html

__fastcall TComThread::TComThread()
    : TThread(true)
{
    FreeOnTerminate = true;
    // setup other thread parameters as needed...
}

void ReportStatusEvent( DWORD event ) {

}

void DoBackgroundWork(void) {
	Sleep(10);
}

#define READ_TIMEOUT_MSEC 100

BOOL ReadComPort(HANDLE hComm, char * lpBuf, DWORD &dwRead) {

    BOOL fRes = FALSE;
    OVERLAPPED osReader = { 0 };

    // Create the overlapped event. Must be closed before exiting
    // to avoid a handle leak.
    osReader.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    if (osReader.hEvent == NULL) {

        // error creating overlapped event handle
        return FALSE;
    }

    // Issue read operation.
    DWORD fSuccess = ::ReadFile(hComm, lpBuf, READ_BUF_SIZE, &dwRead, &osReader);

    DWORD dwErr = GetLastError();

    if (!fSuccess) {

        //if (::GetLastError() != ERROR_IO_PENDING) {
		if (false) {

            // WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
        }
        else {

			DWORD dwRes = ::WaitForSingleObject(osReader.hEvent, READ_TIMEOUT_MSEC);

            switch (dwRes) {

                // Read completed.
                case WAIT_OBJECT_0:

                    if (!::GetOverlappedResult(hComm, &osReader, &dwRead, FALSE)) {

                        // Error in communications; report it.
						//throw new exception("Error in communications");
						;
                    }
                    else {

                        // Read completed successfully.
                        fRes = TRUE;
                    }

                    break;

                case WAIT_TIMEOUT:

                    // Operation isn't complete yet. fWaitingOnRead flag isn't
                    // changed since I'll loop back around, and I don't want
                    // to issue another read until the first one finishes.
                    //
                    // This is a good time to do some background work.
                    fRes = FALSE;
                    break;

                default:
                    // Error in the WaitForSingleObject; abort.
                    // This indicates a problem with the OVERLAPPED structure's
                    // event handle.
                    fRes = FALSE;
                    break;
            }

        }

    }

	if (osReader.hEvent != NULL) ::CloseHandle(osReader.hEvent);
	return fRes;
}

void __fastcall TComThread::Execute()
{

// COM read example from
// https://docs.microsoft.com/en-us/previous-versions/ff802693(v=msdn.10)

	LPCTSTR gszPort = ("\\\\.\\"+Com).c_str();

	HANDLE hComm = NULL;

	try {

		hComm = ::CreateFile( gszPort,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL);

        if ( hComm == INVALID_HANDLE_VALUE ) {

			//throw new exception("INVALID_HANDLE_VALUE");
        }

        DCB dcb;

        ::FillMemory(&dcb, sizeof(dcb), 0);

        DWORD fSuccess = ::GetCommState(hComm, &dcb);

        if (!fSuccess){

			//throw new exception("GetCommState()");
        }

        dcb.DCBlength = sizeof(DCB);
        dcb.BaudRate = Baudrate;
        dcb.Parity = NOPARITY;
        dcb.fBinary = TRUE;
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;

        fSuccess = ::SetCommState(hComm, &dcb);

        if (!fSuccess){

			//throw new exception("SetCommState()");
        }

        // Contains various COM timeouts
        COMMTIMEOUTS CommTimeouts;

        // Timeouts in msec.
        CommTimeouts.ReadIntervalTimeout = MAXDWORD;
        CommTimeouts.ReadTotalTimeoutMultiplier = 0;
        CommTimeouts.ReadTotalTimeoutConstant = 0;
        CommTimeouts.WriteTotalTimeoutMultiplier = 0;
        CommTimeouts.WriteTotalTimeoutConstant = 0;

        // Set COM timeouts.
        fSuccess = ::SetCommTimeouts(hComm, &CommTimeouts);

        if (!fSuccess){

			//throw new exception("SetCommTimeouts()");
        }


        // Принимаем ответ.
        char * buff;
		DWORD dwRead;
		while( state == STATE_CONNECTING || state == STATE_CONNECTED ) {
			ReadComPort(hComm, buf, dwRead);
			if( dwRead )
				DataBlockAdd( Now(), buf, dwRead );
				//HandleASuccessfulRead(buf, dwRead);
		}

    }
	//catch ( exception &ex ) {
	catch ( ... ) {

		//cout << "[ERROR] " << ex.what() << endl;
	}

    if (hComm != NULL )    ::CloseHandle( hComm );

//
//	// if you need to access the UI controls,
//	// use the TThread::Synchornize() method for that
}

void __fastcall TMainForm::ThreadTerminated(TObject *Sender)
{
	// thread is finished with its work ...
	UpdateActionState(STATE_DISCONNECTED);
}



//---------------------------------------------------------------------------

void __fastcall TMainForm::FormShow(TObject *Sender)
{

	OpenDialog->InitialDir = GetCurrentDir();
	SaveDialog->InitialDir = GetCurrentDir();
	OpenDialog->FileName   = GetCurrentDir();
	SaveDialog->FileName   = GetCurrentDir();
	UpdateComBox();

	AnsiString com;
	long baudrate;
    int i;
	TRegistry *Reg = new TRegistry;
	Reg->RootKey = HKEY_CURRENT_USER;
	Reg->Access = KEY_ALL_ACCESS;
	if( Reg->OpenKey("\\Software\\RTEC\\CurrentControl\\", true) )
	{
		try
		{
			MainForm->Left   = Reg->ReadInteger( "WindowLeft" );
			MainForm->Top    = Reg->ReadInteger( "WindowTop" );
			MainForm->Width  = Reg->ReadInteger( "WindowWidth" );
			MainForm->Height = Reg->ReadInteger( "WindowHeight" );
			AxisLog = Reg->ReadBool( "AxisLog" );
			AxisLogBase = Reg->ReadFloat( "AxisLogBase" );
		}
		catch(...)
		{
			AxisLog = false;
			AxisLogBase = 10;
			//MessageBox( NULL, (wchar_t*)"Error occured while reading window position from registry!", (wchar_t*)"Error", MB_OK|MB_ICONERROR );
		}

		Chart->LeftAxis->Logarithmic = AxisLog;
		Chart->LeftAxis->LogarithmicBase = AxisLogBase;
		OptionsForm->LogCheckBox->Checked = AxisLog;
		OptionsForm->LogBaseEdit->Text = AxisLogBase;

        try
		{
            com = Reg->ReadString( "Port" );
        }
		catch(...)
        {
//MessageBox(NULL,"Step 13","Debug start", MB_ICONINFORMATION|MB_OK );
            com = "COM1";
        }
        for( i = 0; i < OptionsForm->PortBox->Items->Count; i++ )
        {
			if( com == OptionsForm->PortBox->Items->Strings[i] )
                break;
        }
        if( i == OptionsForm->PortBox->Items->Count )
        {
            Com = OptionsForm->PortBox->Items->Strings[0];
            OptionsForm->PortBox->ItemIndex = 0;
        }
        else
        {
            OptionsForm->PortBox->ItemIndex = i;
            Com = com;
        }


        try
        {
//MessageBox(NULL,"Step 14","Debug start", MB_ICONINFORMATION|MB_OK );
            baudrate = Reg->ReadInteger( "Baudrate" );
        }
		catch(...)
        {
//MessageBox(NULL,"Step 15","Debug start", MB_ICONINFORMATION|MB_OK );
			baudrate = 115200;
        }
        for( i = 0; i < OptionsForm->BaudrateBox->Items->Count; i++ )
        {
			if( baudrate == StrToInt((__int64)(OptionsForm->BaudrateBox->Items->Strings[i].ToIntDef(115200))) )
                break;
        }
        if( i == OptionsForm->BaudrateBox->Items->Count )
        {
            OptionsForm->BaudrateBox->ItemIndex = 6;
        }
        else
        {
            OptionsForm->BaudrateBox->ItemIndex = i;
        }
        Baudrate = StrToInt( OptionsForm->BaudrateBox->Items->Strings[OptionsForm->BaudrateBox->ItemIndex] );

        Reg->CloseKey();
//MessageBox(NULL,"Step 16","Debug start", MB_ICONINFORMATION|MB_OK );
    }
    else
    {
//MessageBox(NULL,"Step 17","Debug start", MB_ICONINFORMATION|MB_OK );
        OptionsForm->PortBox->ItemIndex = 0;
        //Com = OptionsForm->PortBox->Items->Strings[OptionsForm->PortBox->ItemIndex];
        Com = OptionsForm->PortBox->Text;
        OptionsForm->BaudrateBox->ItemIndex = 6;
        Baudrate = StrToInt( OptionsForm->BaudrateBox->Items->Strings[OptionsForm->BaudrateBox->ItemIndex] );
	}

	delete Reg;

	MainForm->Caption = APP_NAME + " - " + Com;
	Application->Title = APP_NAME + " - " + Com;

	UpdateActionState(STATE_IDLE);
}
//--------------------------------
void __fastcall TMainForm::RegistryWriteSettings()
{
	TRegistry *Reg = new TRegistry;
	Reg->RootKey = HKEY_CURRENT_USER;
	Reg->Access = KEY_ALL_ACCESS;
	if( Reg->OpenKey("\\Software\\RTEC\\CurrentControl\\", true) )
	{
		Reg->WriteString( "Port", Com );
		Reg->WriteInteger( "Baudrate", Baudrate );
		Reg->WriteBool( "AxisLog", 		AxisLog );
		Reg->WriteFloat( "AxisLogBase", 	AxisLogBase );
		Reg->WriteInteger( "WindowLeft",    MainForm->Left );
		Reg->WriteInteger( "WindowTop",     MainForm->Top );
		Reg->WriteInteger( "WindowWidth",   MainForm->Width );
		Reg->WriteInteger( "WindowHeight",  MainForm->Height );
		Reg->CloseKey();
	}
	delete Reg;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormClose(TObject *Sender, TCloseAction &Action)
{
	DataClearAllBlocks();
	RegistryWriteSettings();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::OpenActionExecute(TObject *Sender)
{
	int iFileHandle;
	int iFileLength;
	int iBytesRead;

	int i, ii;
	TColor color;
	TDateTime time;
	double value;
	TDateTime tick;

	CurrentSeries->Clear();
	data_count = 0;

	if (OpenDialog->Execute())
	{
		try
		{
			char ch = 0;

			iFileHandle = FileOpen(OpenDialog->FileName, fmOpenRead);
			iFileLength = FileSeek(iFileHandle,0,2);
			FileSeek(iFileHandle,0,0);

			// Skip first line

			for( i = 0; i < iFileLength && ch != '\n'; i++ ) {
				iBytesRead = FileRead( iFileHandle, &ch, 1 );
			}

			if( i < iFileLength ) {
				DATA_BLOCK *tmp = new DATA_BLOCK;
				memset( (char*)tmp, 0, sizeof(DATA_BLOCK) );
				data_head = tmp;
				data_tail = tmp;


				int buf_len = 0;
				WORD buf[256];
				double prev_time_val = 0;

				for( ; i < iFileLength; i += iBytesRead )
				{
					__int64 time = 0;
					double time_val;
					for( ii = 0; ii < 20; ii++ ) {
						iBytesRead = FileRead( iFileHandle, &ch, 1 );
						i += iBytesRead;
						if( iBytesRead != 1 || ch < '0' || ch > '9' ) {
							break;
						} else {
							time *= 10;
							time += ch-'0';
						}
					}
					if( ch != ',' ) break;

					time_val = time / (24.0*60.0*60.0*1000.0);
					if( buf_len != 0 )
					{
						if( buf_len == 256 || (time_val - prev_time_val) >= (3.0 / (24.0*60.0*60.0*1000.0)) ){
							BYTE *data_buf = new BYTE(buf_len*sizeof(WORD));
							for( ii = 0; ii < buf_len; ii++ ) {
								((WORD*)data_buf)[ii] = buf[ii]<<8 | buf[ii]>>8;
							}
							tmp->current_arr = data_buf;
							tmp->current_arr_len = buf_len;
							tmp = new DATA_BLOCK;
							memset( (char*)tmp, 0, sizeof(DATA_BLOCK) );
							tmp->time.Val = time_val;
							buf_len = 0;

							data_tail->next = tmp;
							data_tail = tmp;
						}
					} else {
                        tmp->time.Val = time_val;
                    }

					prev_time_val = time_val;


					for( ii = 0; ii < 24; ii++ ) {
						iBytesRead = FileRead( iFileHandle, &ch, 1 );
						if( !iBytesRead ) break;
					}
					i+=ii;
					if( ch != ',' ) break;

					WORD value = 0;
					for( ii = 0; ii < 5; ii++ ) {
						iBytesRead = FileRead( iFileHandle, &ch, 1 );
						i += iBytesRead;
						if( iBytesRead != 1 || ch < '0' || ch > '9' ) {
							break;
						} else {
							value *= 10;
							value += ch-'0';
						}
					}
					if( ch != '\n' ) break;

					buf[buf_len] = value;
					buf_len++;

					TColor color = clGreen;
					if( value >= 1000 ) color = clRed;
					double fValue = value;
					TDateTime date_time;
					date_time.Val = time_val;
					UnicodeString time_str = date_time.FormatString("HH:nn:ss.zzz");
					CurrentSeries->AddXY( data_count++, value, time_str, color );
				}

				if( buf_len > 0 ){
					BYTE *data_buf = new BYTE(buf_len*sizeof(WORD));
					for( ii = 0; ii < buf_len; ii++ ) {
						((WORD*)data_buf)[ii] = buf[ii]<<8 | buf[ii]>>8;
					}
					tmp->current_arr = data_buf;
					tmp->current_arr_len = buf_len;
				}

				if( data_count ) {
					ScrollBar->Max = data_count;
					UpdateScroll();
					UpdateActionState( STATE_IDLE );
				}
            }
			FileClose(iFileHandle);
		}
		catch(...)
		{
			Application->MessageBox(((UnicodeString)"Can't perform one of the following file operations: Open, Seek, Read, Close.").c_str(), ((UnicodeString)"File Error").c_str(), IDOK);
		}
	}
}
//---------------------------------------------------------------------------

#define MAX_SIZE 256

void __fastcall TMainForm::SaveActionExecute(TObject *Sender)
{
	char szFileName[MAXFILE+4];
	int iFileHandle;
	int iLength;
	if (SaveDialog->Execute())
	{
		if (FileExists(SaveDialog->FileName))
		{
			fnsplit(((AnsiString)(SaveDialog->FileName)).c_str(), 0, 0, szFileName, 0);
			strcat(szFileName, ".BAK");
			RenameFile(SaveDialog->FileName, szFileName);
		}
		iFileHandle = FileCreate(SaveDialog->FileName);
//		DATA_BLOCK *ptr = data_head;
//		while( ptr != NULL )
//		{
//			FileWrite(iFileHandle, (char*)ptr, sizeof(DATA_BLOCK));
//			ptr = ptr->next;
//		}
		if( data_head != NULL ) {

			DATA_BLOCK *data_ptr = data_head;
			//TColor color;
			UnicodeString time_str;
			UnicodeString date_str;
			double value;
			char mbstring[MAX_SIZE];
			//double tick = 0.020;

			while(data_ptr) {
				int err0 = 0, err1 = 0;
				for( int i = 0; i < data_ptr->current_arr_len; i++ ) {
					if( (i&1) == 0 )
						err0 += (data_ptr->current_arr[i]>0x07)?1:0;
					else
						err1 += (data_ptr->current_arr[i]>0x07)?1:0;
				}

				if( err0 < (data_ptr->current_arr_len/2) || err1 < (data_ptr->current_arr_len/2) ) {
					if( err0 > err1 ) {
						show_value_assembled = false;
					} else {
						show_value_assembled = true;
					}
					break;
				}
				break;
			}

			const double data_byte_time = 1.0/(24.0*60.0*60.0*1000.0);
			const double data_word_time = 2.0/(24.0*60.0*60.0*1000.0);

			const double data_big_diff_time = 200.0 * data_byte_time;

			TDateTime  time1 = 0;

			sprintf(mbstring,"Unix time (ms),Date,Time,Current (A)\n");
			FileWrite(iFileHandle, mbstring, strlen(mbstring));

			while( data_ptr ) {
				TDateTime time2 = data_ptr->time;
				TDateTime time0 = time2 - (double) ( data_word_time * data_ptr->current_arr_len/2 );

				if( time1.Val < (time0.Val-data_big_diff_time) ||
					time1.Val > (time0.Val+data_big_diff_time) )
				{
                    time1 = time0;
                }

				if( !show_value_assembled && (data_ptr->current_arr_len&1) )
					time1 -= data_word_time;
				else
				if(  show_value_assembled && (data_ptr->current_arr_len&1) )
					time1 -= data_byte_time;
				else
				if( !show_value_assembled && !(data_ptr->current_arr_len&1) )
					time1 += data_byte_time;

				for( int i = 0; i < data_ptr->current_arr_len; i++ ) {
					show_value_assembled = !show_value_assembled;

					if( show_value_assembled )
					{
						show_value |= data_ptr->current_arr[i];

						date_str = time1.FormatString("YYYY-MM-DD");
						time_str = time1.FormatString("HH:nn:ss.zzz");

						sprintf( mbstring, "%llu", DateTimeToUnix(time1) );
						FileWrite(iFileHandle, mbstring, strlen(mbstring));
						wcstombs( mbstring, time1.FormatString("zzz,").c_str(), MAX_SIZE);
						FileWrite(iFileHandle, mbstring, strlen(mbstring));

						wcstombs(mbstring,date_str.c_str(),MAX_SIZE);
						FileWrite(iFileHandle, mbstring, strlen(mbstring));

						FileWrite(iFileHandle, ",", 1);

						wcstombs(mbstring,time_str.c_str(),MAX_SIZE);
						FileWrite(iFileHandle, mbstring, strlen(mbstring));

						FileWrite(iFileHandle, ",", 1);

						sprintf(mbstring,"%d\n",show_value);
						FileWrite(iFileHandle, mbstring, strlen(mbstring));

						time1 += data_word_time;
					}  else {
						show_value = data_ptr->current_arr[i];
						show_value <<= 8;
					}
				}
				//dataSaved = true;
				data_ptr = data_ptr->next;
			}
			UpdateScroll();
			ScrollBar->Position = ScrollBar->Max - ScrollBar->PageSize;
		}
		FileClose(iFileHandle);
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::OptionsActionExecute(TObject *Sender)
{
	UpdateComBox();

	if( OptionsForm->ShowModal() == mrOk )
	{
        OptionsForm->PortBox->Text = OptionsForm->PortBox->Text.UpperCase();
        Com =  OptionsForm->PortBox->Text;
		Baudrate = StrToInt( OptionsForm->BaudrateBox->Items->Strings[OptionsForm->BaudrateBox->ItemIndex] );

		//RegistryWriteSettings();

		MainForm->Caption = APP_NAME + " - " + Com;
		Application->Title = APP_NAME + " - " + Com;
		AxisLog = OptionsForm->LogCheckBox->Checked;
		Chart->LeftAxis->Logarithmic = AxisLog;
		AxisLogBase = StrToFloatDef(OptionsForm->LogBaseEdit->Text, 10);
		Chart->LeftAxis->LogarithmicBase = AxisLogBase;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::NewActionExecute(TObject *Sender)
{
	if( !dataSaved ) {
		if( Application->MessageBox(
			L"Данные не сохранены. Все равно очистить и потерять раннее сохраненные данные?",
			L"Внимание",
			MB_ICONWARNING|MB_YESNO ) == IDNO )
		{
			return;
		}
	}
	DataClearAllBlocks();
	CurrentSeries->Clear();
	data_count = 0;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::MagnifyPlusActionExecute(TObject *Sender)
{
	double delta = (Chart->BottomAxis->Maximum - Chart->BottomAxis->Minimum) / 4;
	Chart->BottomAxis->Maximum -= delta;
	Chart->BottomAxis->Minimum += delta;
	UpdateScroll();
	ScrollBar->Position = Chart->BottomAxis->Minimum;
	UpdateActionState( state );
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::MagnifyMinusActionExecute(TObject *Sender)
{
	double max_overflow = 0, min_overflow = 0;
	double delta = (Chart->BottomAxis->Maximum - Chart->BottomAxis->Minimum) / 2;
	Chart->BottomAxis->Maximum += delta;
	Chart->BottomAxis->Minimum -= delta;
	if( Chart->BottomAxis->Maximum > data_count ) {
		max_overflow = Chart->BottomAxis->Maximum - data_count;
		Chart->BottomAxis->Maximum = data_count;
	}

	if( Chart->BottomAxis->Minimum < 0 ) {
		min_overflow = Chart->BottomAxis->Minimum;
		Chart->BottomAxis->Minimum = 0;
	}

	Chart->BottomAxis->Maximum += min_overflow;
	Chart->BottomAxis->Minimum -= max_overflow;

	if( Chart->BottomAxis->Maximum > data_count ) {
		Chart->BottomAxis->Maximum = data_count;
	}

	if( Chart->BottomAxis->Minimum < 0 ) {
		Chart->BottomAxis->Minimum = 0;
	}
	UpdateScroll();
	ScrollBar->Position = Chart->BottomAxis->Minimum;
	UpdateActionState( state );
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::UpdateComBox()
{
	TRegistry *ComReg = new TRegistry;
	ComReg->RootKey = HKEY_LOCAL_MACHINE;
	ComReg->Access = KEY_READ;

	OptionsForm->PortBox->ItemIndex = -1;
	OptionsForm->PortBox->Items->Clear();

	// Com Ports
	if( ComReg->OpenKey("\\HARDWARE\\DEVICEMAP\\SERIALCOMM\\", false) )
	{
		DWORD count = 0;
		DWORD result = RegQueryInfoKey(ComReg->CurrentKey, NULL, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL);
		if ((result == ERROR_SUCCESS)&&(count > 0))
		{
			DWORD name_len;
			DWORD value_len;
			wchar_t name[128];
			wchar_t value[128];
			BYTE comport_numbers_list[256];
			int comport_nums_count = 0;

			for (DWORD i=0; i<count; i++)
			{
				value_len=128;
				name_len=128;

				result = RegEnumValue( ComReg->CurrentKey, i, name, &name_len, 0,0, (BYTE *) value, &value_len);

				if (result!=ERROR_SUCCESS) continue;

				char *stopstring;
				int iComNum;

				//
				iComNum = _wtoi( &value[3]);


				//if( stopstring[0] == '\0' )
				{
				  for( int j = 0; j < comport_nums_count; j++ )
					  if( iComNum == comport_numbers_list[j] )
						  continue;

					comport_numbers_list[comport_nums_count] = iComNum;
					comport_nums_count++;
				}
			}
			while( comport_nums_count )
			{
				int min = 0;

				for( int j = 1; j < comport_nums_count; j++ )
				{
					if( comport_numbers_list[j] < comport_numbers_list[min] )
					{
						min = j;
					}
				}

				UnicodeString usNewPort = "COM"+IntToStr(comport_numbers_list[min]);

				OptionsForm->PortBox->Items->Add( usNewPort );

				if( OptionsForm->PortBox->ItemIndex == -1 ||
					OptionsForm->PortBox->Items->Strings[OptionsForm->PortBox->Items->Count-1] == Com )
				{
					OptionsForm->PortBox->ItemIndex = OptionsForm->PortBox->Items->Count-1;
				}

				comport_numbers_list[min] = comport_numbers_list[comport_nums_count-1];

				comport_nums_count--;
			}

			if( OptionsForm->PortBox->ItemIndex == -1 &&
				OptionsForm->PortBox->Items->Count > 0 )
			{
				OptionsForm->PortBox->ItemIndex = 0;
			}
		}
	}
	ComReg->CloseKey();
	delete ComReg;
}

int margin_left = 56;
int margin_right = 10;
int margin_top = 15;
int margin_bottom = 40;
bool value_hinted = false;
void __fastcall TMainForm::ChartMouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y)
{
	if( data_count == 0 ) return;
	double XVal, YVal, valy, valy2;
	int SXVal, SYVal;
	UnicodeString str;
	if( X < 0 || Y < 0 ) return;
	CurrentSeries->GetCursorValues( XVal, YVal );
	SXVal = Chart->Walls->Back->Width;
	SXVal = (int)((XVal>=0)?(XVal+0.5):-1);
	SYVal = (int)((YVal>=0)?(YVal+0.5):-1);

	if( SXVal >= Chart->BottomAxis->Minimum && SXVal <= Chart->BottomAxis->Maximum &&
		 SYVal >= Chart->LeftAxis->Minimum && SYVal <= Chart->LeftAxis->Maximum )
	{
		value_hinted = true;
		valy = CurrentSeries->YValues->Value[SXVal];
		str = CurrentSeries->Labels->Labels[SXVal];
		Chart->Repaint();

		int dx, dy;
		if( SXVal > ((Chart->BottomAxis->Maximum+Chart->BottomAxis->Minimum)/2) )
			dx = -160;
		else
			dx = 10;
		if( SYVal < ((Chart->LeftAxis->Maximum+Chart->LeftAxis->Minimum)/2) )
			dy = -60;
		else
			dy = 10;

		int x_size = Chart->Width;
		int y_size = Chart->Height;

		Chart->Canvas->Pen->Color = clBlack;
		Chart->Canvas->MoveTo( margin_left, Y );
		Chart->Canvas->LineTo( x_size-margin_right, Y );
		Chart->Canvas->MoveTo( X, margin_top );
		Chart->Canvas->LineTo( X, y_size-margin_bottom );

		// Hint rectangle
		Chart->Canvas->Brush->Color = clWhite;
		Chart->Canvas->Rectangle(X+dx,Y+dy,X+dx+120,Y+dy+35);

		// Hint text
		Chart->Canvas->TextOutW(X+dx+10,Y+dy+5,L"Время: "+str);
		Chart->Canvas->TextOutW(X+dx+10,Y+dy+17,L"Ток: "+FormatFloat("#", valy)+L" A");
		//Chart->Canvas->TextOutW(X+dx+10,Y+dy+29,L"Сопротивление: "+FormatFloat("#", valy2)+L" Ом");
	} else {
		value_hinted = false;
		Chart->Repaint();
	}
}
//---------------------------------------------------------------------------


void __fastcall TMainForm::ChartMouseWheelDown(TObject *Sender, TShiftState Shift,
		  TPoint &MousePos, bool &Handled)
{
//	double delta = 10;
//
//	int x_size = Chart->Width;
//	int y_size = Chart->Height;
//
//	TRect rect;
//	rect.Left = margin_left-delta;
//	rect.Right = x_size-margin_right+delta;
//	rect.Top = margin_top;
//	rect.Bottom = y_size-margin_bottom;
//
//	if( (rect.Right) <= (rect.Left) ) {
//		return;
//	}
//	Chart->ZoomRect(rect);
//
//	if( Chart->BottomAxis->Minimum < 0 )
//		Chart->BottomAxis->Minimum = 0;
//
//
//	if( Chart->BottomAxis->Maximum > data_count )
//		Chart->BottomAxis->Maximum = data_count;
//
//	if( (Chart->BottomAxis->Maximum-Chart->BottomAxis->Minimum) > ((Chart->Width-margin_left-margin_right)/3) )
//	{
//		Chart->BottomAxis->Maximum = Chart->BottomAxis->Minimum + (Chart->Width-margin_left-margin_right)/3;
//	}
//
//	UpdateScroll();
//	ScrollBar->Position = Chart->BottomAxis->Minimum;
//	UpdateActionState( state );
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::ChartMouseWheelUp(TObject *Sender, TShiftState Shift, TPoint &MousePos,
		  bool &Handled)
{
//	double delta = 10;
//
//	int x_size = Chart->Width;
//	int y_size = Chart->Height;
//
//	TRect rect;
//	rect.Left = margin_left+delta;
//	rect.Right = x_size-margin_right-delta;
//	rect.Top = margin_top;
//	rect.Bottom = y_size-margin_bottom;
//
//	if( (rect.Right) <= (rect.Left) ) {
//		return;
//	}
//	Chart->ZoomRect(rect);
//
//	UpdateScroll();
//	ScrollBar->Position = Chart->BottomAxis->Minimum;
//	UpdateActionState( state );
}
//---------------------------------------------------------------------------

