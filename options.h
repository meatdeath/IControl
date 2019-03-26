//---------------------------------------------------------------------------

#ifndef optionsH
#define optionsH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
//---------------------------------------------------------------------------
class TOptionsForm : public TForm
{
__published:	// IDE-managed Components
    TLabel *Label1;
    TLabel *Label2;
    TComboBox *PortBox;
    TComboBox *BaudrateBox;
    TBitBtn *OkButton;
    TBitBtn *CancelButton;
	TSpeedButton *UpdatePortButton;
	TGroupBox *SerialGroupBox;
	TGroupBox *AxisGroupBox;
	TCheckBox *LogCheckBox;
	TEdit *LogBaseEdit;
	TLabel *Label3;
	TButton *Button1;
	void __fastcall UpdatePortButtonClick(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TOptionsForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TOptionsForm *OptionsForm;
//---------------------------------------------------------------------------
#endif
