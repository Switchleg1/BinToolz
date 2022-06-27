//---------------------------------------------------------------------------

#ifndef mainH
#define mainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ComCtrls.hpp>
//---------------------------------------------------------------------------
class TMainForm : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TGroupBox *GroupBox1;
	TGroupBox *GroupBox2;
	TGroupBox *GroupBox3;
	TButton *SwapButton;
	TButton *PatchButton;
	TRadioButton *FullRadioButton;
	TButton *ConvertButton;
	TCheckBox *CALCheckBox;
	TRadioButton *BlockRadioButton;
	TSaveDialog *SaveDialog;
	TOpenDialog *OpenDialog;
	TStatusBar *StatusBar;
	TCheckBox *SwapCheckBox;
	TCheckBox *ConvertCheckBox;
	TEdit *GapFillEdit;
	TLabel *Label1;
	TRadioButton *AddRadioButton;
	TRadioButton *RemoveRadioButton;
	TRadioButton *CreateRadioButton;
	void __fastcall ConvertButtonClick(TObject *Sender);
	void __fastcall PatchButtonClick(TObject *Sender);
	void __fastcall SwapButtonClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TMainForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
