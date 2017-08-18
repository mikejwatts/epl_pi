///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "EplUIDialog.h"

///////////////////////////////////////////////////////////////////////////

EPLPrefsDlg::EPLPrefsDlg(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer(0, 1, 0, 0);
	fgSizer1->AddGrowableCol(0);
	fgSizer1->SetFlexibleDirection(wxBOTH);
	fgSizer1->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	m_cbConWPToLast = new wxCheckBox(this, wxID_ANY, wxT("Connect dropped bearing waypoint to last"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer1->Add(m_cbConWPToLast, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton(this, wxID_OK);
	m_sdbSizer1->AddButton(m_sdbSizer1OK);
	m_sdbSizer1Cancel = new wxButton(this, wxID_CANCEL);
	m_sdbSizer1->AddButton(m_sdbSizer1Cancel);
	m_sdbSizer1->Realize();

	fgSizer1->Add(m_sdbSizer1, 0, wxALIGN_CENTER_HORIZONTAL, 0);


	this->SetSizer(fgSizer1);
	this->Layout();

	this->Centre(wxBOTH);
}

EPLPrefsDlg::~EPLPrefsDlg()
{
}

// PPW Takes in bool and sets the dialog checkbox control
void EPLPrefsDlg::GetPreferences(bool &connectWaypointToLast)
{
	m_cbConWPToLast->SetValue(connectWaypointToLast);
}
// PPW returns the current dialog checkbox value
bool EPLPrefsDlg::SetPreferences()
{
	return m_cbConWPToLast->GetValue();
}
