/******************************************************************************
* $Id: dashboard_pi.h, v1.0 2010/08/05 SethDart Exp $
*
* Project:  OpenCPN
* Purpose:  Dashboard Plugin
* Author:   Jean-Eudes Onfray
*
* 24 Nov 15 MJW - New FixPoint class and a list to handle them.
*
* 16 Nov 15 MJW - Added a 'sighting group' identifier to the brg_line
*                 class.
*
***************************************************************************
*   Copyright (C) 2010 by David S. Register                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
***************************************************************************
*/

#ifndef _DASHBOARDPI_H_
#define _DASHBOARDPI_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers

#define     PLUGIN_VERSION_MAJOR    0
#define     PLUGIN_VERSION_MINOR    4

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    12

#include "graphics.h"

#include <wx/notebook.h>
#include <wx/fileconf.h>
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/spinctrl.h>
#include <wx/aui/aui.h>
#include <wx/fontpicker.h>
#include "ocpn_plugin.h"

#include "nmea0183/nmea0183.h"
#include "PI_RolloverWin.h"
#include "TexFont.h"
#include "vector2d.h"


#define EPL_TOOL_POSITION -1          // Request default positioning of toolbar tool

// more generic definitions are in PI_RolloverWin.h
#define LIVE_BRG_TIMER 5002         // ID for live bearing check

#define gps_watchdog_timeout_ticks  10

//    Constants
#ifndef PI
#define PI        3.1415926535897931160E0      /* pi */
#endif

#define SEL_POINT_A     0
#define SEL_POINT_B     1
#define SEL_SEG         2

//      Menu items
#define ID_EPL_DELETE_SGL       8867
#define ID_EPL_DELETE_ALL		8868
#define ID_EPL_DROP_DELETE      8869

//----------------------------------------------------------------------------------------------------------
//    Forward declarations
//----------------------------------------------------------------------------------------------------------
class brg_line;
class FixPoint;
class Select;
class SelectItem;
class PI_EventHandler;

WX_DECLARE_OBJARRAY(brg_line *, ArrayOfBrgLines);
WX_DECLARE_OBJARRAY(vector2D *, ArrayOf2DPoints);
WX_DECLARE_OBJARRAY(FixPoint *, ListOfFixes);

//void AlphaBlending( wxDC *pdc, int x, int y, int size_x, int size_y, float radius, wxColour color,
//                    unsigned char transparency );
//void RenderLine(int x1, int y1, int x2, int y2, wxColour color, int width);
//void GLDrawLine( wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2 );
//void RenderGLText( wxString &msg, wxFont *font, int xp, int yp, double angle);

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

// FOR DEBUGGING ONLY - array of points defining the select regions
//extern wxPoint *m_hl_pt_ary;
//extern int n_hl_points;

class epl_pi : public wxTimer, opencpn_plugin_113
{
public:
	epl_pi(void *ppimgr);
	~epl_pi(void);

	//    The required PlugIn Methods
	int Init(void);
	bool DeInit(void);


	int GetAPIVersionMajor();
	int GetAPIVersionMinor();
	int GetPlugInVersionMajor();
	int GetPlugInVersionMinor();
	wxBitmap *GetPlugInBitmap();
	wxString GetCommonName();
	wxString GetShortDescription();
	wxString GetLongDescription();

	//    The optional method overrides
	void SetNMEASentence(wxString &sentence);
	void SetPositionFix(PlugIn_Position_Fix &pfix);
	void SetCursorLatLon(double lat, double lon);
	int GetToolbarToolCount(void);
	void OnToolbarToolCallback(int id);
	// PPW Preferences
	void ShowPreferencesDialog( wxWindow* parent );
	//PPW awful added extra bool due confusion with preferences system refactor asap
	bool m_bConnectWaypointToLast;
	void SetColorScheme(PI_ColorScheme cs);

	bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
	bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
	bool MouseEventHook(wxMouseEvent &event);

	void OnRolloverPopupTimerEvent(wxTimerEvent& event);
	void PopupMenuHandler(wxCommandEvent& event);

	bool SaveConfig(bool &connectWaypointToLast);
	void PopulateContextMenu(wxMenu* menu);
	int GetToolbarItemId(){ return m_toolbar_item_id; }
	void SetPluginMessage(wxString &message_id, wxString &message_body);

	void ProcessTimerEvent(wxTimerEvent& event);
	void RenderFixHat(void);

	// PPW add track to waypoints currrently set as public variable we can go private and add member function instead if preferred 
	PlugIn_Track *pBearingFixesTrack = new PlugIn_Track();

private:
	bool LoadConfig(void);
	//void ApplyConfig(void);

	void ProcessBrgCapture(double brg_rel, double brg_subtended,
		double brg_TM, int brg_TM_flag, wxString Ident, wxString target,
		int brgMode);

	void RenderOverlay();
	void CalculateFix(void);
	void SortMHatArray();
	void ComputeFixPoint();

	wxBitmap         *m_pplugin_icon;
	wxFileConfig     *m_pconfig;
	int              m_toolbar_item_id;

	int               m_show_id;
	int               m_hide_id;

	NMEA0183             m_NMEA0183;                 // Used to parse NMEA Sentences
	// FFU
	int                  m_config_version;
	wxString             m_VDO_accumulator;

	Select               *m_select;

	ArrayOfBrgLines      m_brg_array;

	/* fix from the currently hightlighted cocked-hat region - only
	valid if m_bshow_fix_hat									  */
	double               m_fix_lat;
	double               m_fix_lon;

	/* vertices of the currently highlighted cocked-hat region - may
	* be a point if only 2 lines intersect                         */
	ArrayOf2DPoints		m_hat_array;

	/* dropped waypoints											  */
	ListOfFixes			m_fix_list;

	/* set if there is currently a highlighted area for cocked-hat  */
	bool					m_bshow_fix_hat;

	/* bearing line group index used to compute the intersect points
	currently being displayed - only valid if m_bshow_fix_hat	  */
	int					m_brg_grp;

	//        Selection variables
	brg_line             *m_sel_brg;
	int                  m_sel_part;
	SelectItem           *m_pFind;
	double               m_sel_pt_lat;
	double               m_sel_pt_lon;
	double               m_segdrag_ref_x;
	double               m_segdrag_ref_y;
	double               m_cursor_lat;
	double               m_cursor_lon;
	int                  m_mouse_x;
	int                  m_mouse_y;

	//        State variables captured from NMEA stream
	int                  mHDx_Watchdog;
	int                  mHDT_Watchdog;
	int                  mGPS_Watchdog;
	int                  mVar_Watchdog;
	double               mVar;
	//      double               mHdm;
	double               m_ownship_cog;
	bool                 m_head_active;
	wxTimer              m_head_dog_timer;


	int                  mPriPosition;
	int                  mPriDateTime;
	int                  mPriVar;
	int                  mPriHeadingM;
	int                  mPriHeadingT;

	double               m_ownship_lat;
	double               m_ownship_lon;
	double               m_hdt;
	wxDateTime           mUTCDateTime;


	//        Rollover Window support
	brg_line             *m_pRolloverBrg;
	RolloverWin          *m_pBrgRolloverWin;
	wxTimer               m_RolloverPopupTimer;
	int                   m_rollover_popup_timer_msec;

	wxColour              m_FixHatColor;
	// PPW added additional colour
	wxColour              m_FixHatColorOver2Points;


	PI_EventHandler       *m_event_handler;

	// coloured line sequence support
	unsigned int          m_brg_clr_seq;
	wxDateTime            m_last_brg_time;

	// 'live' bearing aka radar mode support
	wxTimer               m_live_brg_timer;
};



//      An event handler to manage timer ticks, and the like
class PI_EventHandler : public wxEvtHandler
{
public:
	PI_EventHandler(epl_pi *parent);
	~PI_EventHandler();

	void OnTimerEvent(wxTimerEvent& event);
	//void PopupMenuHandler(wxCommandEvent& event);

private:
	epl_pi              *m_parent;

	DECLARE_EVENT_TABLE();

};


typedef enum BearingTypeEnum
{
	MAG_BRG = 0,
	TRUE_BRG
}_BearingTypeEnum;


//    Generic Bearing line class definition
class brg_line
{
public:
	brg_line(double bearing, BearingTypeEnum type);
	brg_line(double bearing, BearingTypeEnum type,
		double lat_point, double lon_point, double length,
		wxColor colour);
	brg_line(double bearing, BearingTypeEnum type,
		double lat_point, double lon_point, double length);
	~brg_line();

	// Accessors
	void SetColor(wxColour &color);
	void SetWidth(int width);
	void SetStyle(int style);
	double GetLatA(){ return m_latA; }
	double GetLonA(){ return m_lonA; }
	double GetLatB(){ return m_latB; }
	double GetLonB(){ return m_lonB; }
	double GetBearingTrue(){ return m_bearing_true; }

	void SetIdent(wxString Ident){ m_Ident = Ident; }
	void SetTargetName(wxString target){ m_TargetName = target; };
	wxString GetIdent(){ return m_Ident; }
	wxString GetTargetName(){ return m_TargetName; }

	// accessors for the sighting group - related bearing lines
	void SetSightGroup(int group){ m_sight_group = group; }
	int GetSightGroup() { return m_sight_group; }

	void CalcPointB(void);
	void CalcLength(void);

	//  Public methods
	void Draw(void);
	bool getIntersect(brg_line *b, double *lat, double *lon);
	bool IsLive(void);
	bool IsExpired(void);


	double      m_latA;
	double      m_lonA;
	double      m_latB;
	double      m_lonB;

private:
	//  Methods
	void        Init(void);

	wxString    m_TargetName;
	wxString    m_Ident;

	// details which group of sightings to which this bearing belongs
	int			m_sight_group;

	BearingTypeEnum m_type;
	double      m_bearing_true;


	double      m_length;   // Nautical Miles

	wxColour    m_color;
	int         m_width;
	wxFont      *m_Font;
	wxColour    m_color_text;

	wxDateTime  m_create_time;
	wxColour    m_InfoBoxColor;

	// set to NULL if no expiry i.e. normal bearing line
	wxDateTime  m_expiry_time;
};


/* Fix point class declaration.  Deals with a point with lat * lon
* which can be rendered onto the canvass.
*/
class FixPoint
{
public:
	FixPoint(double lat, double lon);
	~FixPoint();

	// Accessors
	void SetColor(wxColour &color);
	void SetSize(int size);
	void SetStyle(int style);
	double GetLat(){ return m_lat; }
	double GetLon(){ return m_lon; }

	//  Public methods
	void Draw(void);

	double      m_lat;
	double      m_lon;

private:
	//  Methods
	void        Init(void);

	wxColour    m_color;
	int         m_size;
	wxFont      *m_Font;
	wxColour    m_color_text;

	wxDateTime   m_create_time;
};

// PPW Add standard wxDialog for preferences 
// skipped having another class which inherits wxdialog and then inheriting that check correct??
// Could split this out to own dialog class file if needed
//class EPLPrefsDlg : public wxDialog
//{
//private:
//protected:
//	wxButton* m_sdbSizer1OK;
//	wxButton* m_sdbSizer1Cancel;
//public:
//	wxCheckBox* m_cbConnectBrgWaypoints;
//	EPLPrefsDlg(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("EPL Preferences"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION | wxDEFAULT_DIALOG_STYLE);
//	~EPLPrefsDlg();
//};


#endif

