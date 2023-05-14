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
	TCheckBox *SwapCheckBox;
	TCheckBox *ConvertCheckBox;
	TEdit *GapFillEdit;
	TLabel *Label1;
	TRadioButton *AddRadioButton;
	TRadioButton *RemoveRadioButton;
	TRadioButton *CreateRadioButton;
	TListBox *LogListBox;
	TRadioButton *CheckRadioButton;
	TRadioButton *ImportRadioButton;
	TRadioButton *ExportRadioButton;
	TGroupBox *GroupBox4;
	TRadioButton *NormalRadioButton;
	TRadioButton *IgnoreRadioButton;
	TRadioButton *ForceRadioButton;
	void __fastcall ConvertButtonClick(TObject *Sender);
	void __fastcall PatchButtonClick(TObject *Sender);
	void __fastcall CALButtonClick(TObject *Sender);
	void __fastcall addToLog(AnsiString line);
	void __fastcall fileResult(int8_t result, AnsiString fileName);
	void __fastcall LogListBoxDblClick(TObject *Sender);
private:	// User declarations
	int __fastcall PatchFunction(AnsiString inFileName, AnsiString patchFileName, AnsiString outFileName);
public:		// User declarations
	__fastcall TMainForm(TComponent* Owner);
	__fastcall ~TMainForm();
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
