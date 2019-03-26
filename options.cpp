//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "options.h"
#include "main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TOptionsForm *OptionsForm;
//---------------------------------------------------------------------------
__fastcall TOptionsForm::TOptionsForm(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------



void __fastcall TOptionsForm::UpdatePortButtonClick(TObject *Sender)
{
	//MainForm->UpdateComBox();
}
//---------------------------------------------------------------------------





void __fastcall TOptionsForm::Button1Click(TObject *Sender)
{
	OptionsForm->LogBaseEdit->Text = M_E;
}
//---------------------------------------------------------------------------

