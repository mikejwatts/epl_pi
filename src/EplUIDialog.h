///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __EPLUIDIALOG_H__
#define __EPLUIDIALOG_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class EPLPrefsDlg
///////////////////////////////////////////////////////////////////////////////
class EPLPrefsDlg : public wxDialog
{
private:

protected:
	wxCheckBox* m_cbConWPToLast;
	wxStdDialogButtonSizer* m_sdbSizer1;
	wxButton* m_sdbSizer1OK;
	wxButton* m_sdbSizer1Cancel;

public:

	EPLPrefsDlg(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("EPL Preferences"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(260, 100), long style = wxDEFAULT_DIALOG_STYLE);
	~EPLPrefsDlg();
	void GetPreferences(bool &connectWaypointToLast);
	bool SetPreferences();
};

#endif //__EPLUIDIALOG_H__
