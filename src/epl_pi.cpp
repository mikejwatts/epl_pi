/***************************************************************************
*
* Project:  OpenCPN
* 20 Apr 17 PPW Replaced cocked hat drop for bearing line instersects with 
*				waypoint object using API call.
*				Added in preferences menu item to automatically connect 
*				waypoints to last.
* 20 Mar 17 PPW Upgraded to latest plugin api and rebuilt usin version 4.6.1
				of opencpn lib
* 13 Mar 17 M.J.Watts Added clause to check for incoming message 'mode': if
*        'H' ( regular ping ) or 'A' ( acknowledgement ) then ignore it.
*
* 03 Nov 16 MJW - Updated to have 'radar' or 'live' bearing lines which are
*                 transient; fix of relative-only bearings being available.
*                 Changed the length of the lines to extend to the edge of the
*                 displayed chart.
*
* 24 Nov 15 MJW - Added option to drop a waypoint and delete all
*                 associated bearing lines.
* 16 Nov 15 MJW - Dealt with multiple fix groups and polygon shapes.
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

#include "wx/wxprec.h"
//PPW added to stop undefined type error with wxGraphicsContext
#include "wx/graphics.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers
#include <typeinfo>
#include "epl_pi.h"
#include "icons.h"
#include "Select.h"
#include "vector2d.h"
#include <algorithm>

//PPW
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include "EplUIDialog.h"

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif

// minimum and maxmum lengths for bearing lines, in NM
#define MIN_BRG_LINE_LEN        0.05
#define MAX_BRG_LINE_LEN        10.0

// proportion of distance-to-edge for the bearing lines to be added: 0.9 works
// with a typical border
#define BRG_LEN_FACTOR          0.9

// bearing colour sequence restarts if no sighting in this time period
static const wxTimeSpan BRG_CLR_SEQ_TIMEOUT = wxTimeSpan(0, 5, 0); // 5 mins

// 'live' or transient bearing line expiry
static const wxTimeSpan BRG_LIVE_TIMEOUT = wxTimeSpan(0, 0, 0, 300);     // 300 ms

//      Global Static variable
PlugIn_ViewPort         *g_vp;
PlugIn_ViewPort         g_ovp;

double                  g_Var;         // assumed or calculated variation


#if wxUSE_GRAPHICS_CONTEXT
wxGraphicsContext       *g_gdc;
#endif
wxDC                    *g_pdc;


#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(ArrayOfBrgLines);
WX_DEFINE_OBJARRAY(ArrayOf2DPoints);
WX_DEFINE_OBJARRAY(ListOfFixes);

#include "default_pi.xpm"

#include <wx/glcanvas.h> // PPW added due to use of deprecated functions Raster2i etc

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
	return (opencpn_plugin *) new epl_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
	delete p;
}


// PPW added for waypoint trial
int iNameCnt = 0;
bool bTrackAdded = false;
//#define GPX_EMPTY_STRING _T("")

/*  These two function were taken from gpxdocument.cpp */
int GetRandomNumber(int range_min, int range_max)
{
	long u = (long)wxRound(((double)rand() / ((double)(RAND_MAX)+1) * (range_max - range_min)) + range_min);
	return (int)u;
}

// RFC4122 version 4 compliant random UUIDs generator.
wxString GetUUID(void)
{
	wxString str;
	struct {
		int time_low;
		int time_mid;
		int time_hi_and_version;
		int clock_seq_hi_and_rsv;
		int clock_seq_low;
		int node_hi;
		int node_low;
	} uuid;

	uuid.time_low = GetRandomNumber(0, 2147483647);//FIXME: the max should be set to something like MAXINT32, but it doesn't compile un gcc...
	uuid.time_mid = GetRandomNumber(0, 65535);
	uuid.time_hi_and_version = GetRandomNumber(0, 65535);
	uuid.clock_seq_hi_and_rsv = GetRandomNumber(0, 255);
	uuid.clock_seq_low = GetRandomNumber(0, 255);
	uuid.node_hi = GetRandomNumber(0, 65535);
	uuid.node_low = GetRandomNumber(0, 2147483647);

	/* Set the two most significant bits (bits 6 and 7) of the
	* clock_seq_hi_and_rsv to zero and one, respectively. */
	uuid.clock_seq_hi_and_rsv = (uuid.clock_seq_hi_and_rsv & 0x3F) | 0x80;

	/* Set the four most significant bits (bits 12 through 15) of the
	* time_hi_and_version field to 4 */
	uuid.time_hi_and_version = (uuid.time_hi_and_version & 0x0fff) | 0x4000;

	str.Printf(_T("%08x-%04x-%04x-%02x%02x-%04x%08x"),
		uuid.time_low,
		uuid.time_mid,
		uuid.time_hi_and_version,
		uuid.clock_seq_hi_and_rsv,
		uuid.clock_seq_low,
		uuid.node_hi,
		uuid.node_low);

	return str;
}

void Clone_VP(PlugIn_ViewPort *dest, PlugIn_ViewPort *src)
{
	dest->clat = src->clat;                   // center point
	dest->clon = src->clon;
	dest->view_scale_ppm = src->view_scale_ppm;
	dest->skew = src->skew;
	dest->rotation = src->rotation;
	dest->chart_scale = src->chart_scale;
	dest->pix_width = src->pix_width;
	dest->pix_height = src->pix_height;
	dest->rv_rect = src->rv_rect;
	dest->b_quilt = src->b_quilt;
	dest->m_projection_type = src->m_projection_type;

	dest->lat_min = src->lat_min;
	dest->lat_max = src->lat_max;
	dest->lon_min = src->lon_min;
	dest->lon_max = src->lon_max;

	dest->bValid = src->bValid;                 // This VP is valid
}


/** Returns the destination point ( lat & long ) given the start point plus a
* distance and bearing from the start point.
*/
void DestFrom(const double lata, const double lona,
	const double dist_m, const double brg_deg,
	double *latb, double *lonb);

/** Returns the distance in NM from the given start point and bearing to the
* nearest edge assuming simple Mercator geometry.  Deals with out-of-bounds or
* very large scales by returning minimum lengths; very small scales with
* maximum lengths.
*/
double DistanceToEdge(const double lat_p, const double lon_p,
	const double brg_deg,
	const double lat_top, const double lat_bot,
	const double lon_left, const double lon_right);


//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

epl_pi::epl_pi(void *ppimgr) :
wxTimer(this), opencpn_plugin_113(ppimgr)
{
	// Create the PlugIn icons
	//    initialize_images();

	m_pplugin_icon = new wxBitmap(default_pi);

}

epl_pi::~epl_pi(void)
{
}

int epl_pi::Init(void)
{
	AddLocaleCatalog(_T("opencpn-epl_pi"));
	m_config_version = -1;

	//  Configure the NMEA processor
	mHDx_Watchdog = 2;
	mHDT_Watchdog = 2;
	mGPS_Watchdog = 2;
	mVar_Watchdog = 2;
	mPriPosition = 9;
	mPriDateTime = 9;
	mPriVar = 9;
	mPriHeadingM = 9;
	mPriHeadingT = 9;

	mVar = 0;
	m_hdt = 0;
	m_ownship_cog = 0;
	m_bshow_fix_hat = false;
	m_fix_lat = 0;
	m_fix_lon = 0;

	m_FixHatColor = wxColour(0, 128, 128);
	//PPW added different colour if over two points intersecting
	m_FixHatColorOver2Points = wxColour(255, 0, 0);


	// PPW get pointer to the opencpn configuration object, needed for config file location
	m_pconfig = GetOCPNConfigObject();
	// PPW load the preferences into accessable variables from the config file
	LoadConfig();

	// PPW set up Plugin Track for connecting bearing waypoints
	pBearingFixesTrack->m_NameString = "Bearing Fixes Track";

	//    This PlugIn needs a toolbar icon
	//    m_toolbar_item_id = InsertPlugInTool( _T(""), _img_dashboard, _img_dashboard, wxITEM_CHECK,
	//            _("Dashboard"), _T(""), NULL, EPL_TOOL_POSITION, 0, this );

	//    ApplyConfig();

	//  If we loaded a version 1 config setup, convert now to version 2
	//    if(m_config_version == 1) {
	//       SaveConfig();
	//    }

	//    Start( 1000, wxTIMER_CONTINUOUS );

	m_event_handler = new PI_EventHandler(this);
	m_RolloverPopupTimer.SetOwner(m_event_handler, ROLLOVER_TIMER);
	m_rollover_popup_timer_msec = 20;
	m_pBrgRolloverWin = NULL;
	m_pRolloverBrg = NULL;

	m_select = new Select();
	m_select->SetSelectLLRadius(1.0);                               // added MJW 13 Oct 16 as an experiment, but changing this doesn't seem to make any difference

	m_head_dog_timer.SetOwner(m_event_handler, HEAD_DOG_TIMER);
	m_head_active = false;

	// preset the bearing colour sequencer
	m_brg_clr_seq = 0;
	m_last_brg_time = wxDateTime::Now();

	// prepare the live bearing timer
	m_live_brg_timer.SetOwner(m_event_handler, LIVE_BRG_TIMER);

	// used for debugging, just to ensure the DLL being used has actually been
	// updated!
	//int version = 6;
	//wxString debug_info;
	//debug_info.Printf(L"version: %i\r\n", version);
	//OutputDebugString(debug_info);

	return (WANTS_OVERLAY_CALLBACK |
		WANTS_OPENGL_OVERLAY_CALLBACK |
		WANTS_CURSOR_LATLON |
		WANTS_TOOLBAR_CALLBACK |
		INSTALLS_TOOLBAR_TOOL |
		WANTS_CONFIG |
		WANTS_PREFERENCES |
		WANTS_PLUGIN_MESSAGING |
		WANTS_NMEA_SENTENCES |
		WANTS_NMEA_EVENTS |
		WANTS_MOUSE_EVENTS
		);
}

bool epl_pi::DeInit(void)
{
	if (IsRunning()) // Timer started?
		Stop(); // Stop timer

	return true;
}


int epl_pi::GetAPIVersionMajor()
{
	return MY_API_VERSION_MAJOR;
}

int epl_pi::GetAPIVersionMinor()
{
	return MY_API_VERSION_MINOR;
}

int epl_pi::GetPlugInVersionMajor()
{
	return PLUGIN_VERSION_MAJOR;
}

int epl_pi::GetPlugInVersionMinor()
{
	return PLUGIN_VERSION_MINOR;
}

wxBitmap *epl_pi::GetPlugInBitmap()
{
	return m_pplugin_icon;
}

wxString epl_pi::GetCommonName()
{
	return _("ePelorus");
}

wxString epl_pi::GetShortDescription()
{
	return _("ePelorus PlugIn for OpenCPN");
}

wxString epl_pi::GetLongDescription()
{
	return _("ePelorus PlugIn for OpenCPN");

}
// PPW we need to add function to AddCustomWaypointIcon API call for cocked hat or newer icon

/*
* PPW 
* Preferences Menu
*/
void epl_pi::ShowPreferencesDialog(wxWindow* parent)
{
	// Uses some dialog defaults from wxwidgets form builder
	EPLPrefsDlg *dialog = new EPLPrefsDlg(parent, wxID_ANY, _("EPL Preferences"), wxPoint(0, 0), wxSize(800,800), wxDEFAULT_DIALOG_STYLE);

	// PPW change name of functions is confusing also seems convoluted with the global variables to 
	// member variables mish-mash from his other plugins check thats as good as it can be
	// GetPreferences passes in bool that has been assigned value from config file during LoadConfig() line 257
	dialog->GetPreferences(m_bConnectWaypointToLast);
	// On OK press
	if (dialog->ShowModal() == wxID_OK)
	{
		// Get the current value of the preferences dialog checkbox control into temp bool
		bool cbConnect = dialog->SetPreferences();
		// Set the global accessable bool to value
		m_bConnectWaypointToLast = cbConnect;
		// Pass to save to be written back to config file
		SaveConfig(cbConnect);
	}
	delete dialog;

}


/*----------------------------------------------------------------------
* Implements the menu items shown on the context menu.
*--------------------------------------------------------------------*/

// PPW Seems to be incrementaly increasing amount of times this is called each time it's selected from drop down??
void epl_pi::PopupMenuHandler(wxCommandEvent& event)
{
	iNameCnt++;

	// these 2 used in one of the case blocks
	int n_brgs = m_brg_array.Count();
	FixPoint *fix = NULL;

	bool handled = false;
	switch (event.GetId()) {
		case ID_EPL_DELETE_SGL:
		{
			if (m_sel_brg){

				// delete the selectable
				if (m_pFind){
					m_select->DeleteSelectablePoint(m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_A);
					m_select->DeleteSelectablePoint(m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_B);
					m_select->DeleteSelectableSegment(m_sel_brg, SELTYPE_SEG_GENERIC, SEL_SEG);
				}

				//  Remove the bearing from the array
				for (unsigned int i = 0; i < m_brg_array.GetCount(); i++){
					brg_line *pb = m_brg_array.Item(i);
					if (pb == m_sel_brg){
						m_brg_array.Detach(i);
					}
				}

				//  Finally, delete the bearing
				delete m_sel_brg;
			}

			handled = true;
			break;
		}
		case ID_EPL_DELETE_ALL:
		{
			//  simply empty the array of bearing lines
			m_brg_array.Clear();
			m_brg_clr_seq = 0;

			handled = true;
			break;
		}
		case ID_EPL_DROP_DELETE:
		{
			//int iNameCnt;
			wxString piWName = "", piWIcon = _T("Diamond"), piWGuid = "";
			bool addedWaypoint, addedTrack;
			PlugIn_Waypoint *piW;
			std::ostringstream oss;
			// we're in the fix region so compute the mid-point
			ComputeFixPoint();
			// PPW removed more manual point fix 
			//fix = new FixPoint(m_fix_lat, m_fix_lon);
			//// add the stored point as a fix
			//m_fix_list.Add(fix);

			// PPW trial dropping way point at end of bearing line using plugin API
			// Cnt for naming and GUID
			//++iNameCnt;
			//piWName = _T("Brg Waypoint " + std::to_string(++iNameCnt));
			// Create time object from system and string it
			auto t = std::time(nullptr);
			auto tm = *std::localtime(&t);
			oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
			auto str = oss.str();

			// Create waypoint and using current calculation for centre of intersecting bearing lines drop it there
			// using the time of droppionmg as the unique name need to use the GUID generating function instead of my
			// incrementing int
			piWName = _T("Brg Fix WP: " + str);
			//piWName = _T("Brg Waypoint " + std::to_string(iNameCnt));
			//piWGuid = _T("9990" + std::to_string(iNameCnt));
			// Guid left blank so it will auto generate one
			piW = new PlugIn_Waypoint(m_fix_lat, m_fix_lon, piWIcon, piWName, "");
			addedWaypoint = AddSingleWaypoint(piW, false);  // PPW currently set to non perminant waypoints so not saved upon exit Check this ?????
			
			// If the preference to connect WPs with track
			if (m_bConnectWaypointToLast == true){
				// Add waypoint to plugin track waypoint list
				pBearingFixesTrack->pWaypointList->Append(piW);
				//Need to check if first connection and to add the track else we update the track with the latest addition
				if (bTrackAdded == false){
					addedTrack = AddPlugInTrack(pBearingFixesTrack, false);
					bTrackAdded = true;
				}
				else{
					addedTrack = UpdatePlugInTrack(pBearingFixesTrack);
					// If update has failed we presume track has been deleted by user and needs to be added again
					if (addedTrack == false){
						addedTrack = AddPlugInTrack(pBearingFixesTrack, false);
					}
				}
				piW = NULL;
			}


			// remove all bearings with the stored ID
			for (int i = n_brgs - 1; i >= 0; i--) {
				brg_line *pb = m_brg_array.Item(i);
				if (pb->GetSightGroup() == m_brg_grp) {
					m_brg_array.Detach(i);
					delete pb;
				}
			}

			// assume that as this posn is complete the colour sequence can be reset
			m_brg_clr_seq = 0;
			break;
		}
		default:
		{
			break;		// do nothing
		}
	}

	if (!handled)
		event.Skip();
}


/*----------------------------------------------------------------------
* Deals with the low-level actions on the mouse including roll-over,
* mouse clicks etc.  Dispatches to various handlers depending on
* whether a context menu or pop-up item is required.
*
* @return					true if this method handled the event
*--------------------------------------------------------------------*/
bool epl_pi::MouseEventHook(wxMouseEvent &event)
{
	// PPW constantly pushed data from main appliication virtual func from plugin and plugin manager
	bool bret = false;

	m_mouse_x = event.m_x;
	m_mouse_y = event.m_y;

	//  Retrigger the rollover timer
	if (m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive())
		m_RolloverPopupTimer.Start(10, wxTIMER_ONE_SHOT);               // faster response while the rollover is turned on
	else
		m_RolloverPopupTimer.Start(m_rollover_popup_timer_msec, wxTIMER_ONE_SHOT);

	wxPoint mp(event.m_x, event.m_y);
	GetCanvasLLPix(&g_ovp, mp, &m_cursor_lat, &m_cursor_lon);

	//  On button push, find any bearing line selecteable, and other useful data
	if (event.RightDown() || event.LeftDown()) {
		m_sel_brg = NULL;
		m_pFind = m_select->FindSelection(m_cursor_lat, m_cursor_lon, SELTYPE_POINT_GENERIC);

		if (m_pFind){
			for (unsigned int i = 0; i < m_brg_array.GetCount(); i++){
				brg_line *pb = m_brg_array.Item(i);

				if (m_pFind->m_pData1 == pb){
					m_sel_brg = pb;
					m_sel_part = m_pFind->GetUserData();
					if (SEL_POINT_A == m_sel_part){
						m_sel_pt_lat = pb->GetLatA();
						m_sel_pt_lon = pb->GetLonA();
					}
					else {
						m_sel_pt_lat = pb->GetLatB();
						m_sel_pt_lon = pb->GetLonB();
					}
					break;
				}
			}
		}
		else{
			m_pFind = m_select->FindSelection(m_cursor_lat, m_cursor_lon, SELTYPE_SEG_GENERIC);

			if (m_pFind){
				for (unsigned int i = 0; i < m_brg_array.GetCount(); i++){
					brg_line *pb = m_brg_array.Item(i);

					if (m_pFind->m_pData1 == pb){
						m_sel_brg = pb;
						m_sel_part = m_pFind->GetUserData();

						//  Get the mercator offsets from the cursor point to the brg "A" point
						toSM_Plugin(m_cursor_lat, m_cursor_lon, pb->GetLatA(), pb->GetLonA(),
							&m_segdrag_ref_x, &m_segdrag_ref_y);
						m_sel_pt_lat = m_cursor_lat;
						m_sel_pt_lon = m_cursor_lon;
					}
				}
			}
		}
		//} // PPW  closing if (event.RightDown() || event.LeftDown()) 
		//  moved to lower point stop unnecessary checking of events seems to be better
		if (event.RightDown()) {
			if (m_sel_brg || m_bshow_fix_hat){
				wxMenu* contextMenu = new wxMenu;
				wxMenuItem *brg_item_sgl = 0;
				wxMenuItem *brg_item_all = 0;
				wxMenuItem *fix_item = 0;

				// PPW replaced All old connect methods with Bind
				if (!m_bshow_fix_hat && m_sel_brg){
					// add items to the menu only relevant to a bearing line
					brg_item_sgl = new wxMenuItem(contextMenu, ID_EPL_DELETE_SGL, _("Delete Bearing Line"));
					contextMenu->Append(brg_item_sgl);
					//GetOCPNCanvasWindow()->Connect(ID_EPL_DELETE_SGL, wxEVT_COMMAND_MENU_SELECTED,
					//	wxCommandEventHandler(epl_pi::PopupMenuHandler), NULL, this);
					GetOCPNCanvasWindow()->Bind(wxEVT_COMMAND_MENU_SELECTED, &epl_pi::PopupMenuHandler, this,
						ID_EPL_DELETE_SGL);

					brg_item_all = new wxMenuItem(contextMenu, ID_EPL_DELETE_ALL, _("Delete All Bearing Lines"));
					contextMenu->Append(brg_item_all);
					//GetOCPNCanvasWindow()->Connect(ID_EPL_DELETE_ALL, wxEVT_COMMAND_MENU_SELECTED,
					//	wxCommandEventHandler(epl_pi::PopupMenuHandler), NULL, this);
					GetOCPNCanvasWindow()->Bind(wxEVT_COMMAND_MENU_SELECTED, &epl_pi::PopupMenuHandler, this,
						ID_EPL_DELETE_ALL);
				}
				if (m_bshow_fix_hat){
					// add items to the menu only relevant to a potential fix area
					// PPW remove bastard of a bug wxMenuitem pointer used incorrectly incrementing connects being created 
					//wxMenuItem *fix_item = new wxMenuItem(contextMenu, ID_EPL_DROP_DELETE, _("Drop Marker & Delete Lines"));
					fix_item = new wxMenuItem(contextMenu, ID_EPL_DROP_DELETE, _("Drop Marker & Delete Lines"));
					contextMenu->Append(fix_item);
					//GetOCPNCanvasWindow()->Connect(ID_EPL_DROP_DELETE, wxEVT_COMMAND_MENU_SELECTED,
					//	wxCommandEventHandler(epl_pi::PopupMenuHandler), NULL, this);
					GetOCPNCanvasWindow()->Bind(wxEVT_COMMAND_MENU_SELECTED, &epl_pi::PopupMenuHandler, this,
						ID_EPL_DROP_DELETE);
				}
				//   Invoke the drop-down menu
				GetOCPNCanvasWindow()->PopupMenu(contextMenu, m_mouse_x, m_mouse_y);

				/* TODO                       MJW - what do these actions do, and why is one a connect and the other a disconnect? ********************************
				if(brg_item){
				GetOCPNCanvasWindow()->Disconnect( ID_EPL_DELETE_SGL, wxEVT_COMMAND_MENU_SELECTED,
				wxCommandEventHandler( epl_pi::PopupMenuHandler ), NULL, this );
				}

				if(fix_item){
				GetOCPNCanvasWindow()->Connect( ID_EPL_XMIT, wxEVT_COMMAND_MENU_SELECTED,
				wxCommandEventHandler( epl_pi::PopupMenuHandler ), NULL, this );
				}


				************** I think they should all be disconnects after opening the menu: test and implement
				*/

				// disconnect / release the items again after showing the menu
				// PPW replaced All ther old disconnect methods with Unbind
				if (brg_item_sgl){
					//GetOCPNCanvasWindow()->Disconnect(ID_EPL_DELETE_SGL, wxEVT_COMMAND_MENU_SELECTED,
					//	wxCommandEventHandler(epl_pi::PopupMenuHandler), NULL, this);
					GetOCPNCanvasWindow()->Unbind(wxEVT_COMMAND_MENU_SELECTED, &epl_pi::PopupMenuHandler, this,
						ID_EPL_DELETE_SGL);
				}

				if (brg_item_all){
					//GetOCPNCanvasWindow()->Disconnect(ID_EPL_DELETE_ALL, wxEVT_COMMAND_MENU_SELECTED,
					//	wxCommandEventHandler(epl_pi::PopupMenuHandler), NULL, this);
					GetOCPNCanvasWindow()->Unbind(wxEVT_COMMAND_MENU_SELECTED, &epl_pi::PopupMenuHandler, this,
						ID_EPL_DELETE_ALL);
				}

				if (fix_item){
					// MJW changed to Disconnect - I think that's what it should be
					//GetOCPNCanvasWindow()->Disconnect(ID_EPL_DROP_DELETE, wxEVT_COMMAND_MENU_SELECTED,
					//	wxCommandEventHandler(epl_pi::PopupMenuHandler), NULL, this);
					GetOCPNCanvasWindow()->Unbind(wxEVT_COMMAND_MENU_SELECTED, &epl_pi::PopupMenuHandler, this,
						ID_EPL_DROP_DELETE);
				}

				bret = true;                // I have eaten this event
			}

		}
		else if (event.LeftDown()) {
			// PPW currently do nothing
		}
	} // PPW closing if (event.RightDown() || event.LeftDown())  
	// moved from top stop unnecessary checking of events seems to be better

		else if (event.Dragging()){
			if (m_sel_brg){
				if ((SEL_POINT_A == m_sel_part) || (SEL_POINT_B == m_sel_part)){
					double dx, dy;
					toSM_Plugin(m_cursor_lat, m_cursor_lon, m_sel_pt_lat, m_sel_pt_lon, &dx, &dy);
					double distance = sqrt((dx * dx) + (dy * dy));

					double new_lat = m_sel_pt_lat;
					double new_lon = m_sel_pt_lon;

					double alpha = atan2(dx, dy);
					if (alpha < 0)
						alpha += 2 * PI;

					double brg_perp = (m_sel_brg->GetBearingTrue() - 90) * PI / 180.;
					if (brg_perp < 0)
						brg_perp += 2 * PI;

					double delta_alpha = alpha - brg_perp;
					if (delta_alpha < 0)
						delta_alpha += 2 * PI;

					double move_dist = distance * sin(delta_alpha);

					double ndy = move_dist * cos(m_sel_brg->GetBearingTrue() * PI / 180.);
					double ndx = move_dist * sin(m_sel_brg->GetBearingTrue() * PI / 180.);

					fromSM_Plugin(ndx, ndy, m_sel_pt_lat, m_sel_pt_lon, &new_lat, &new_lon);

					//  Update the brg line parameters
					if (SEL_POINT_A == m_sel_part){
						m_sel_brg->m_latA = new_lat;
						m_sel_brg->m_lonA = new_lon;
					}
					else if (SEL_POINT_B == m_sel_part){
						m_sel_brg->m_latB = new_lat;
						m_sel_brg->m_lonB = new_lon;
					}

					m_sel_brg->CalcLength();

					// Update the selectable items
					if (m_pFind){
						m_select->DeleteSelectableSegment(m_sel_brg, SELTYPE_SEG_GENERIC, SEL_SEG);
						m_select->AddSelectableSegment(m_sel_brg->GetLatA(), m_sel_brg->GetLonA(),
							m_sel_brg->GetLatB(), m_sel_brg->GetLonB(),
							m_sel_brg, SEL_SEG);

						m_pFind->m_slat = new_lat;
						m_pFind->m_slon = new_lon;
					}

					GetOCPNCanvasWindow()->Refresh();
					bret = true;
				}
				else if (SEL_SEG == m_sel_part){

					//  Get the Mercator offsets from original select point to this drag point 
					double dx, dy;
					toSM_Plugin(m_cursor_lat, m_cursor_lon, m_sel_pt_lat, m_sel_pt_lon, &dx, &dy);

					//  Add in the offsets to item point "A"
					dx -= m_segdrag_ref_x;
					dy -= m_segdrag_ref_y;

					//   And calculate new position of item point "A"
					double nlatA, nlonA;
					fromSM_Plugin(dx, dy, m_sel_pt_lat, m_sel_pt_lon, &nlatA, &nlonA);

					//  Set point "A"
					m_sel_brg->m_latA = nlatA;
					m_sel_brg->m_lonA = nlonA;

					// Recalculate point "B"
					m_sel_brg->CalcPointB();

					// Update the selectable items
					m_select->DeleteSelectablePoint(m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_A);
					m_select->DeleteSelectablePoint(m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_B);
					m_select->DeleteSelectableSegment(m_sel_brg, SELTYPE_SEG_GENERIC, SEL_SEG);

					m_select->AddSelectablePoint(m_sel_brg->GetLatA(), m_sel_brg->GetLonA(), m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_A);
					m_select->AddSelectablePoint(m_sel_brg->GetLatB(), m_sel_brg->GetLonB(), m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_B);
					m_select->AddSelectableSegment(m_sel_brg->GetLatA(), m_sel_brg->GetLonA(),
						m_sel_brg->GetLatB(), m_sel_brg->GetLonB(),
						m_sel_brg, SEL_SEG);


					GetOCPNCanvasWindow()->Refresh();
					bret = true;
				}
			}
		}
	if (event.LeftUp()) {
		if (m_sel_brg)
			bret = true;

		m_pFind = NULL;
		m_sel_brg = NULL;

		CalculateFix();

	}

	return bret;
}


void epl_pi::ProcessTimerEvent(wxTimerEvent& event)
{
	int evID = event.GetId();
	switch (evID) {
	case ROLLOVER_TIMER:
		OnRolloverPopupTimerEvent(event);
		break;

	case HEAD_DOG_TIMER:        // no hdt source available
		m_head_active = false;
		break;

	case LIVE_BRG_TIMER:
		// go through list of bearings, any that are (a) live and (b) timed-
		// out are removed
		bool no_live_brg_found = true;
		for (int i = m_brg_array.Count() - 1; i >= 0; i--) {
			brg_line *pb = m_brg_array.Item(i);

			if (pb->IsLive()) {
				no_live_brg_found = false;
				if (pb->IsExpired()) {
					m_brg_array.Detach(i);
					delete pb;
				}
			}
		}

		if (no_live_brg_found) {
			// no more live bearings to test for
			m_live_brg_timer.Stop();
		}
		break;
	}
}


/*----------------------------------------------------------------------
* Called when the mouse has been stationary for a period of time.
* Checks for items being rolled-over: a cocked-hat region or at least
* close to a 2-sighting fix point; a selectable point; a bearing line.
*--------------------------------------------------------------------*/
void epl_pi::OnRolloverPopupTimerEvent(wxTimerEvent& event)
{
	bool b_need_refresh = false;

	bool showRollover = false;

	//----------------------------------------------------------------------
	//                cocked-hat or 2-line fix highlighting
	//----------------------------------------------------------------------
	// check to see if we are near the centre of the cocked hat fix region
	// or close to a 2-sighting fix
	int n_brgs = m_brg_array.Count();
	m_brg_grp = -1;					// invalid

	// set if this routine finds a match
	bool show_now = false;

	// find the number of groups
	int max_grp_index = -1;
	for (int i = 0; i < n_brgs; i++) {
		int grp_idx = m_brg_array.Item(i)->GetSightGroup();
		if (grp_idx > max_grp_index) {
			max_grp_index = grp_idx;
		}
	}

	for (int grp_idx = 0; grp_idx <= max_grp_index; grp_idx++) {
		m_hat_array.Clear();

		// link each bearing with all of its partners
		for (int i = 0; i < n_brgs - 1; i++) {
			brg_line *a = m_brg_array.Item(i);
			if (a->GetSightGroup() == grp_idx) {
				for (int j = i + 1; j < n_brgs; j++) {
					brg_line *b = m_brg_array.Item(j);
					double lat, lon;
					if (b->GetSightGroup() == grp_idx
						&& a->getIntersect(b, &lat, &lon)) {

						vector2D *pt = new vector2D(lon, lat);
						m_hat_array.Add(pt);
					}
				}
			}
		}

		if (m_hat_array.Count() > 0) {
			// find the extents of the vertices and the mid-points of those;
			// use that to determine where to highlight - also works if there is
			// only one intersection
			double lat_min = 90, lat_max = -90, lon_min = 180, lon_max = -180;
			for (unsigned int i = 0; i < m_hat_array.Count(); i++) {
				vector2D *pt = m_hat_array.Item(i);
				lat_min = std::min(lat_min, pt->lat);
				lat_max = std::max(lat_max, pt->lat);
				lon_min = std::min(lon_min, pt->lon);
				lon_max = std::max(lon_max, pt->lon);
			}
			m_fix_lat = (lat_min + lat_max) / 2;
			m_fix_lon = (lon_min + lon_max) / 2;

			wxPoint fpoint;             // fix point
			GetCanvasPixLL(&g_ovp, &fpoint, m_fix_lat, m_fix_lon);

			wxPoint cpoint;             // cursor point
			GetCanvasPixLL(&g_ovp, &cpoint, m_cursor_lat, m_cursor_lon);

			double dx2 = (((cpoint.x - fpoint.x) * (cpoint.x - fpoint.x)) +
				((cpoint.y - fpoint.y) * (cpoint.y - fpoint.y)));

			if (dx2 < 20 * 20) {
				m_brg_grp = grp_idx;
				show_now = true;
				break;
			}
		}
	}

	// refresh if the 'show' status has changed
	if (show_now != m_bshow_fix_hat) {
		b_need_refresh = true;
	}
	m_bshow_fix_hat = show_now;

	//----------------------------------------------------------------------
	//           any other roll-over actions
	//----------------------------------------------------------------------
	// DEBUGGING ONLY - create a set of points outlining an area
	// as an example, take a outline of bearing line first
	//if (m_brg_array.Count() > 0) {
	//	wxPoint point;
	//	GetCanvasPixLL(&g_ovp, &point,
	//		m_brg_array.Item(0)->GetLatA(),
	//		m_brg_array.Item(0)->GetLonA());

	//	wxPoint point2;
	//	GetCanvasPixLL(&g_ovp, &point2,
	//		m_brg_array.Item(0)->GetLatB(),
	//		m_brg_array.Item(0)->GetLonB());

	//	m_hl_pt_ary = new wxPoint[4];
	//	m_hl_pt_ary[0] = wxPoint(point2.x - 40, point2.y - 40);       // top-left
	//	m_hl_pt_ary[1] = wxPoint(point.x - 40, point.y + 40);
	//	m_hl_pt_ary[2] = wxPoint(point.x + 40, point.y + 40);
	//	m_hl_pt_ary[3] = wxPoint(point2.x + 40, point2.y - 40);

	//	n_hl_points = 4;

	//}

	if (!m_bshow_fix_hat && (NULL == m_pRolloverBrg)) {

		m_pFind = m_select->FindSelection(m_cursor_lat, m_cursor_lon, SELTYPE_SEG_GENERIC, &g_ovp);

		if (m_pFind){
			for (unsigned int i = 0; i < m_brg_array.Count(); i++){
				brg_line *pb = m_brg_array.Item(i);

				if (m_pFind->m_pData1 == pb){
					m_pRolloverBrg = pb;
					break;
				}
			}
		}

		if (m_pRolloverBrg)
		{
			showRollover = true;

			if (NULL == m_pBrgRolloverWin) {
				m_pBrgRolloverWin = new RolloverWin(GetOCPNCanvasWindow());
				m_pBrgRolloverWin->IsActive(false);
			}

			if (!m_pBrgRolloverWin->IsActive()) {
				wxString info;
				info.Printf(_T("Bearing: %g"), m_pRolloverBrg->GetBearingTrue());
				wxString degree = wxString::Format(_T("%cT"), 0x00B0); //_T("°");
				info += degree + _T("\nID: ") + m_pRolloverBrg->GetIdent();

				m_pBrgRolloverWin->SetString(info);

				//                    wxSize win_size = GetSize();
				m_pBrgRolloverWin->SetBestPosition(m_mouse_x, m_mouse_y, 16, 16, 0, wxSize(100, 100));
				m_pBrgRolloverWin->SetBitmap(0);
				m_pBrgRolloverWin->IsActive(true);
				b_need_refresh = true;
				showRollover = true;
			}
		}

	}
	else {
		//    Is the cursor still in select radius?
		if (!m_select->IsSelectableSegmentSelected(m_cursor_lat, m_cursor_lon, m_pFind))
			showRollover = false;
		else
			showRollover = true;
	}



	if (m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() && !showRollover) {
		m_pBrgRolloverWin->IsActive(false);
		m_pRolloverBrg = NULL;
		m_pBrgRolloverWin->Destroy();
		m_pBrgRolloverWin = NULL;
		b_need_refresh = true;
	}
	else if (m_pBrgRolloverWin && showRollover) {
		m_pBrgRolloverWin->IsActive(true);
		b_need_refresh = true;
	}

	if (b_need_refresh)
		GetOCPNCanvasWindow()->Refresh(true);

}


bool epl_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
	g_vp = vp;
	Clone_VP(&g_ovp, vp);                // deep copy



	double select_radius = (10 / vp->view_scale_ppm) / 1852.0;
	m_select->SetSelectLLRadius(select_radius);


#if wxUSE_GRAPHICS_CONTEXT
	wxMemoryDC *pmdc;
	pmdc = wxDynamicCast(&dc, wxMemoryDC);
	wxGraphicsContext *pgc = wxGraphicsContext::Create(*pmdc);
	g_gdc = pgc;
	g_pdc = &dc;
#else
	g_pdc = &dc;
#endif

	// common rendering for both graphic engines
	RenderOverlay();

#if wxUSE_GRAPHICS_CONTEXT
	delete g_gdc;
#endif

	if (m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive()) {
		dc.DrawBitmap(*(m_pBrgRolloverWin->GetBitmap()),
			m_pBrgRolloverWin->GetPosition().x,
			m_pBrgRolloverWin->GetPosition().y, false);
	}


	return true;
}

bool epl_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
	g_gdc = NULL;
	g_pdc = NULL;

	g_vp = vp;
	Clone_VP(&g_ovp, vp);                // deep copy

	//int testh = vp->pix_height; // PPW debug only
	//int testhw= vp->pix_width; //

	double selec_radius = (10 / vp->view_scale_ppm) / (1852. * 60.);
	m_select->SetSelectLLRadius(selec_radius);

	// common rendering for both graphic engines
	RenderOverlay();

	if (m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive()) {
		wxImage image = m_pBrgRolloverWin->GetBitmap()->ConvertToImage();
		unsigned char *imgdata = image.GetData();
		if (imgdata){
			glRasterPos2i(m_pBrgRolloverWin->GetPosition().x, m_pBrgRolloverWin->GetPosition().y);

			glPixelZoom(1.0, -1.0);
			glDrawPixels(image.GetWidth(), image.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, imgdata);
			glPixelZoom(1.0, 1.0);
		}

	}

	return true;
}


/*----------------------------------------------------------------------
* Helper method has common parts of RenderOverlay and RenderGLOverlay.
*--------------------------------------------------------------------*/
void epl_pi::RenderOverlay() {

	// DEBUGGING ONLY
	// render the highlight areas
	//if (n_hl_points > 0) {
	//	RenderPolygon(g_pdc, 4, m_hl_pt_ary, wxColour(255, 0, 255), 150);
	//}

	//if (n_hl_points2 > 0) {
	//	RenderPolygon(g_pdc, 4, m_hl_pt_ary2, wxColour(255, 255, 0), 150);
	//}

	for (unsigned int i = 0; i < m_brg_array.GetCount(); i++){
		brg_line *pb = m_brg_array.Item(i);
		pb->Draw();
	}

	for (unsigned int i = 0; i < m_fix_list.Count(); i++) {
		FixPoint *fix = m_fix_list.Item(i);
		fix->Draw();
	}

	if (m_bshow_fix_hat)
		RenderFixHat();
}


void epl_pi::SetNMEASentence(wxString &sentence)
{
	m_NMEA0183 << sentence;

	m_NMEA0183 << sentence;

	if (m_NMEA0183.PreParse()) {
		if (m_NMEA0183.LastSentenceIDReceived == _T("EBL")) {
			if (m_NMEA0183.Parse()) {
				ProcessBrgCapture(m_NMEA0183.Ebl.BrgRelative, m_NMEA0183.Ebl.BrgSubtended,
					m_NMEA0183.Ebl.BrgAbs, m_NMEA0183.Ebl.BrgAbsTM,
					m_NMEA0183.Ebl.BrgIdent, m_NMEA0183.Ebl.BrgTargetName,
					m_NMEA0183.Ebl.BrgMode);
			}
		}


		else if (m_NMEA0183.LastSentenceIDReceived == _T("HDG")) {
			if (m_NMEA0183.Parse()) {

				if (!wxIsNaN(m_NMEA0183.Hdg.MagneticVariationDegrees)){
					if (m_NMEA0183.Hdg.MagneticVariationDirection == East)
						mVar = m_NMEA0183.Hdg.MagneticVariationDegrees;
					else if (m_NMEA0183.Hdg.MagneticVariationDirection == West)
						mVar = -m_NMEA0183.Hdg.MagneticVariationDegrees;
				}

				if (!wxIsNaN(m_NMEA0183.Hdg.MagneticSensorHeadingDegrees)) {
					if (!wxIsNaN(mVar)){
						m_hdt = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees + mVar;
						m_head_active = true;
						m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
					}
				}

			}
		}

		else if (m_NMEA0183.LastSentenceIDReceived == _T("HDM")) {
			if (m_NMEA0183.Parse()) {

				if (!wxIsNaN(m_NMEA0183.Hdm.DegreesMagnetic)) {
					if (!wxIsNaN(mVar)){
						m_hdt = m_NMEA0183.Hdm.DegreesMagnetic + mVar;
						m_head_active = true;
						m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
					}
				}

			}
		}

		else if (m_NMEA0183.LastSentenceIDReceived == _T("HDT")) {
			if (m_NMEA0183.Parse()) {
				if (mPriHeadingT >= 1) {
					mPriHeadingT = 1;
					if (m_NMEA0183.Hdt.DegreesTrue < 999.) {
						m_hdt = m_NMEA0183.Hdt.DegreesTrue;
						m_head_active = true;
						m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
					}
				}
			}
		}



	}
}


void epl_pi::SetPositionFix(PlugIn_Position_Fix &pfix)
{
	if (1){
		m_ownship_lat = pfix.Lat;
		m_ownship_lon = pfix.Lon;
		if (!wxIsNaN(pfix.Cog))
			m_ownship_cog = pfix.Cog;
		if (!wxIsNaN(pfix.Var))
			mVar = pfix.Var;
	}
}

void epl_pi::SetCursorLatLon(double lat, double lon)
{
}

void epl_pi::SetPluginMessage(wxString &message_id, wxString &message_body)
{
}

int epl_pi::GetToolbarToolCount(void)
{
	return 1;
}


void epl_pi::SetColorScheme(PI_ColorScheme cs)
{
}


void epl_pi::OnToolbarToolCallback(int id)
{
}


bool epl_pi::LoadConfig(void)
{
	// PPW Uses global config file
	wxFileConfig *pConf = (wxFileConfig *)m_pconfig;
	// If the config file is valid
	if (pConf) {
		// This sets the path within the opencpn.ini config file for the settings to be saved to
		pConf->SetPath(_T("/PlugIns/EPL"));
		// reads the data from the specified entity from the config file
		pConf->Read(_T("ConnectWaypointToLast"), &m_bConnectWaypointToLast, 0);
		return true;
	}
	else
		return false;
}

bool epl_pi::SaveConfig(bool &connectWaypointToLast)
{
	// PPW Uses global config file
	wxFileConfig *pConf = (wxFileConfig *)m_pconfig;
	// If the config file is valid
	if (pConf) {
		// This sets the path within the opencpn.ini config file for the settings to be saved to 
		pConf->SetPath(_T("/PlugIns/EPL"));
		// writes the data from the specified entity from the config file
		pConf->Write(_T("ConnectWaypointToLast"), connectWaypointToLast);
		return true;
	}
	else
		return false;
}

//void epl_pi::ApplyConfig(void)
//{
//
//}

/*------------------------------------------------------------------------------
* Called when a new sighting is taken.  Accepts the values from the
* NMEA sent to the ECDIS, computes the bearing from relative if
* required and creates & sets the new bearing line.
*----------------------------------------------------------------------------*/
void epl_pi::ProcessBrgCapture(double brg_rel, double brg_subtended,
	double brg_TM, int brg_TM_flag, wxString Ident, wxString target,
	int brgMode) {

	// at some point we might want to check for an acknowledgement to a query
	// message sent out, or have a watchdog timer on incoming messages but for
	// the moment 'H' and 'A' mode messages are simply ignored
	if (brgMode == EPLModeImHere || brgMode == EPLModeAck) {
		return;
	}

	double brg_true = 0;
	BearingTypeEnum type = TRUE_BRG;

	double lat = m_ownship_lat;          // initial declaration causes bearing line to pass thru ownship
	double lon = m_ownship_lon;

	// normal case is for absolute bearing but if the bearing is relative, the
	//  ship's heading needs to be applied - note 999. is OpenCPN empty field
	// ** TODO should check the TM_flag is valid too ( what to do if not... )
	if (!wxIsNaN(brg_TM) && brg_TM != 999.) {
		// ** TODO change the type [enum] var(?) and apply variation if brg_TM_flag indicates magnetic
		brg_true = brg_TM;

	}
	else {
		//  If we don't have a true heading available, use ownship cog
		if (m_head_active)
			brg_true = brg_rel + m_hdt;
		else
			// ** TODO TBD - if no reliable heading then (a) pop up an error message? and (b) don't plot anything
			brg_true = brg_rel + m_ownship_cog;
	}

	type = TRUE_BRG;

	// ensure bearing is between 0 and 359.9999 deg
	while (brg_true < 0) {
		brg_true += 360.0;
	}
	while (brg_true >= 360) {
		brg_true -= 360.0;
	}

	if (brgMode == EPLModeLive) {
		// compute a suitable length based on position, bearing and the displayed
		// chart
		double length = DistanceToEdge(lat, lon, brg_true,
			g_ovp.lat_max, g_ovp.lat_min, g_ovp.lon_min, g_ovp.lon_max);

		// start the timer if it isn't already going
		if (!m_live_brg_timer.IsRunning()) {
			m_live_brg_timer.Start(100, wxTIMER_CONTINUOUS);
		}

		brg_line *brg = new brg_line(brg_true, type, lat, lon, length);
		brg->SetIdent(Ident);
		m_brg_array.Add(brg);
		GetOCPNCanvasWindow()->Refresh();

	}
	else {
		//  Do not add duplicates
		bool b_add = true;
		for (unsigned int i = 0; i < m_brg_array.GetCount(); i++){
			brg_line *pb = m_brg_array.Item(i);
			if (pb->GetIdent() == Ident){
				b_add = false;
				break;
			}
		}

		// work out a colour scheme
		if (wxDateTime::Now().IsLaterThan(m_last_brg_time.Add(BRG_CLR_SEQ_TIMEOUT))) {
			m_brg_clr_seq = 0;
		}

		wxColor brg_colour;
		switch (m_brg_clr_seq) {
		case 0: brg_colour = wxColor(255, 0, 0);    break;  // red
		case 1: brg_colour = wxColor(0, 0, 255);    break;  // blue
		case 2: brg_colour = wxColour(0, 206, 209); break;  // dark turquoise
		case 3: brg_colour = wxColour(255, 140, 0); break;  // dark orange

			// shouldn't happen
		default: brg_colour = wxColor(0, 0, 0);
		}
		// next colour in the sequence
		m_brg_clr_seq = (m_brg_clr_seq + 1) % 4;

		// compute a suitable length based on position, bearing and the displayed
		// chart
		double length = BRG_LEN_FACTOR * DistanceToEdge(lat, lon, brg_true,
			g_ovp.lat_max, g_ovp.lat_min, g_ovp.lon_min, g_ovp.lon_max);

		if (b_add){
			brg_line *brg = new brg_line(brg_true, type, lat, lon, length, brg_colour);
			brg->SetIdent(Ident);
			brg->SetTargetName(target);

			m_brg_array.Add(brg);
			m_select->AddSelectablePoint(brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A);
			m_select->AddSelectablePoint(brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B);
			m_select->AddSelectableSegment(brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG);

			GetOCPNCanvasWindow()->Refresh();

			m_last_brg_time = wxDateTime::Now();
		}

		CalculateFix();
	}
}


/*----------------------------------------------------------------------
* Called when a new sighting / bearing line is added or one is altered
* by dragging or changing the length.  Assigns each bearing line to a
* group where members of the group intersect.
*--------------------------------------------------------------------*/
void epl_pi::CalculateFix(void)
{
	//  If there are two or more bearing lines stored, calculate the resulting fix
	//  Also, keep an array of points defining the fix for later rendering

	if (m_brg_array.Count() < 2) {
		// nothing to work on
		return;
	}

	// this could be achieved in one loop but split to make it more
	// obvious
	// first set all of the sight group to -1
	int n_brgs = m_brg_array.Count();
	for (int i = 0; i < n_brgs; i++) {
		m_brg_array.Item(i)->SetSightGroup(-1);
	}

	int group_index = 0;
	for (int i = 0; i < n_brgs - 1; i++) {
		// compare each bearing with each other and check for an
		// intersection
		brg_line *a = m_brg_array.Item(i);
		if (a->GetSightGroup() == -1) {
			a->SetSightGroup(group_index);
			group_index++;
		}

		for (int j = i + 1; j < n_brgs; j++) {
			brg_line *b = m_brg_array.Item(j);
			double lat, lon;
			if (a->getIntersect(b, &lat, &lon)) {
				b->SetSightGroup(a->GetSightGroup());
			}
		}
	}
}


/*----------------------------------------------------------------------
* Implements the rendering of the cocked-hat region or, for a 2-bearing
* fix, an area near the intersection.
*--------------------------------------------------------------------*/
void epl_pi::RenderFixHat(void)
{
	int n_curr_fix = m_hat_array.Count();
	if (n_curr_fix < 1)
		return;						// no fix


	// centre of fix has been set to m_fix_lat/lon in OnRolloverPopupTimerEvent
	wxPoint ab;
	GetCanvasPixLL(g_vp, &ab, m_fix_lat, m_fix_lon);
	int crad = 20;

	// indicate a 2-bearing fix with a square, otherwise show a triangle
	if (m_hat_array.Count() > 1) {
		// create 3 points of a triangle
		wxPoint *triVerts = new wxPoint[3];
		triVerts[0] = wxPoint(ab.x, ab.y - 2 * crad);
		triVerts[1] = wxPoint(ab.x - 1.732 * crad, ab.y + crad);
		triVerts[2] = wxPoint(ab.x + 1.732 * crad, ab.y + crad);

		RenderPolygon(g_pdc, 3, triVerts, m_FixHatColor, 250);

	}
	else {
		RenderRoundedRect(g_pdc, ab.x - crad, ab.y - crad,
			crad * 2, crad * 2, 3.0, m_FixHatColor, 250);
	}
}


/*----------------------------------------------------------------------
* Helper method sets m_lat and m_lon from the currently displayed
* cocked-hat region - uses the m_hat_array points.
*--------------------------------------------------------------------*/
void epl_pi::ComputeFixPoint() {
	if (!m_bshow_fix_hat) {
		// not valid unless the cocked-hat region is being shown
		return;
	}

	int n_hat_vectors = m_hat_array.Count();

	// find the centre point
	double lat_sum = 0, lon_sum = 0;
	for (int i = 0; i < n_hat_vectors; i++) {
		lat_sum += m_hat_array.Item(i)->lat;
		lon_sum += m_hat_array.Item(i)->lon;
	}
	m_fix_lat = lat_sum / n_hat_vectors;
	m_fix_lon = lon_sum / n_hat_vectors;
}


//-----------------------------------------------------------------------------------------------------------------
//
//      Bearing Line Class implementation
//
//-----------------------------------------------------------------------------------------------------------------

brg_line::brg_line(double bearing, BearingTypeEnum type)
{
	Init();
	m_color = wxColour(0, 0, 0);

	m_type = type;
}


/** Creates a new bearing line given the bearing and its type, nominal position,
*  length and colour.
*
* @param bearing [in]      bearing angle in degrees N = 0 or 360
* @param type [in]         bearing type: true or magnetic
* @param lat_point [in]    latitude of the nominal position
* @param lon_point [in]    longitude of the nominal position
* @param length [in]       length of the line to add in NM
* @parma colour [in]       colour for the sighting line
*/
brg_line::brg_line(double bearing, BearingTypeEnum type,
	double lat_point, double lon_point,
	double length, wxColor colour) {

	Init();
	m_color = colour;

	m_type = type;
	if (TRUE_BRG == type)
		m_bearing_true = bearing;
	else
		m_bearing_true = bearing - g_Var;

	m_latA = lat_point;
	m_lonA = lon_point;
	m_length = length;

	// shift point a just a bit (20% of length), to help make a cocked hat
	double adj_lata, adj_lona;
	double dy = -0.2 * m_length * 1852.0 * cos(m_bearing_true * PI / 180.);        // east/north in metres
	double dx = -0.2 * m_length * 1852.0 * sin(m_bearing_true * PI / 180.);

	// reset the length to the extended value
	m_length *= 1.2;
	fromSM_Plugin(dx, dy, m_latA, m_lonA, &adj_lata, &adj_lona);

	m_latA = adj_lata;
	m_lonA = adj_lona;

	CalcPointB();

	// no expiry
	m_expiry_time = wxDateTime(0LL);
}


/** Creates a new 'live' or transient bearing line given the bearing and its
* type, nominal position and length.  Always black.
*
* @param bearing [in]      bearing angle in degrees N = 0 or 360
* @param type [in]         bearing type: true or magnetic
* @param lat_point [in]    latitude of the nominal position
* @param lon_point [in]    longitude of the nominal position
* @param length [in]       length of the line to add in NM
*/
brg_line::brg_line(double bearing, BearingTypeEnum type,
	double lat_point, double lon_point,
	double length) {

	Init();
	m_color = wxColor(0, 0, 0);

	m_type = type;
	if (TRUE_BRG == type)
		m_bearing_true = bearing;
	else
		m_bearing_true = bearing - g_Var;

	m_latA = lat_point;
	m_lonA = lon_point;
	m_length = length;

	CalcPointB();

	// expires after a few moments
	m_expiry_time = wxDateTime::UNow().Add(BRG_LIVE_TIMEOUT);
}

brg_line::~brg_line()
{
}


void brg_line::Init(void)
{
	//  Set some innocent initial values
	m_bearing_true = 0;
	m_type = TRUE_BRG;
	m_latA = m_latB = m_lonA = m_lonB = 0;
	m_length = 100;

	m_width = 4;

	m_color_text = wxColour(0, 0, 0);

	m_Font = wxTheFontList->FindOrCreateFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

	m_create_time = wxDateTime::Now();
	m_InfoBoxColor = wxTheColourDatabase->Find(_T("LIME GREEN"));

}

bool brg_line::getIntersect(brg_line* b, double* lat, double* lon)
{
	double ilat, ilon;

	MyFlPoint p1; p1.x = this->m_lonA; p1.y = this->m_latA;
	MyFlPoint p2; p2.x = this->m_lonB; p2.y = this->m_latB;
	MyFlPoint p3; p3.x = b->m_lonA; p3.y = b->m_latA;
	MyFlPoint p4; p4.x = b->m_lonB; p4.y = b->m_latB;


	bool ret = LineIntersect(p1, p2, p3, p4, &ilon, &ilat);

	if (lat)
		*lat = ilat;
	if (lon)
		*lon = ilon;

	return ret;

}


/** Computes the far end of the bearing line using the length, bearing and
* start position assuming simple Mercator projection.  Sets m_latB and m_lonB.
*/
void brg_line::CalcPointB(void) {
	// MJW 30 Oct 16 - this did use 
	// fromSM_Plugin(dx, dy, m_latA, m_lonA, &m_latB, &m_lonB);
	// where
	// double dy = m_length * 1852.0 * cos(m_bearing_true * PI / 180.);        // east/north in metres
	// double dx = m_length * 1852.0 * sin(m_bearing_true * PI / 180.);
	// but that was found to be faulty - use the simple approach instead

	double lat_fact = cos(m_latA * PI / 180.0);
	double dx_NM = m_length * sin(m_bearing_true * PI / 180.0);
	double dy_NM = m_length * cos(m_bearing_true * PI / 180.0);

	m_latB = m_latA + dy_NM / 60.0;
	m_lonB = m_lonA + dx_NM / (60.0 * lat_fact);
}


void brg_line::CalcLength(void)
{
	// We re-calculate the length from the two endpoints
	double dx, dy;
	toSM_Plugin(m_latA, m_lonA, m_latB, m_lonB, &dx, &dy);
	double distance = sqrt((dx * dx) + (dy * dy));

	m_length = distance / 1852.;
}
// Draws the bearing line to the screen
void brg_line::Draw(void)
{
	//  Calculate the points
	wxPoint ab;
	GetCanvasPixLL(g_vp, &ab, m_latA, m_lonA);

	wxPoint cd;
	GetCanvasPixLL(g_vp, &cd, m_latB, m_lonB);

	//  Adjust the rendering width, to make it 100 metres wide for a normal
	// line, half that for live mode
	double dwidth = (IsLive() ? 50 : 100) * g_ovp.view_scale_ppm; //PPW useful use of scaling for device view_scale_ppm

	dwidth = wxMin(4, dwidth);
	dwidth = wxMax(2, dwidth);
	//dwidth = 10; // Debugging PPW line width only for non opengl method

	RenderLine(g_pdc, g_gdc, ab.x, ab.y, cd.x, cd.y, m_color, dwidth);
}

/** Returns true if the bearing line is transient i.e. will be removed after it
* has expired.
*
* @return                  true if this is a 'live' aka radar mode line
*/
bool brg_line::IsLive() {
	return m_expiry_time.GetValue() != 0;
}


/** Returns true if the bearing line expiry time has passed.  Always returns
* false if the line is not a 'live' mode bearing.
*
* @return                  true if line period has expired
*/
bool brg_line::IsExpired(void) {
	return IsLive() && wxDateTime::UNow().IsLaterThan(m_expiry_time);
}


//----------------------------------------------------------------------
//
//      Fix Point Class implementation
//
//----------------------------------------------------------------------

/*----------------------------------------------------------------------
* Creates the FixPoint with the given position.
*
* @param lat [in]			latitude in decimal format
* @param lon [in]			longitude in decimal format
*--------------------------------------------------------------------*/
FixPoint::FixPoint(double lat, double lon) {
	Init();

	m_lat = lat;
	m_lon = lon;
}


/*----------------------------------------------------------------------
* Deletes the FixPoint instance.
*--------------------------------------------------------------------*/
FixPoint::~FixPoint()
{
}


/*----------------------------------------------------------------------
* Initialises the FixPoint object with typical values.
*--------------------------------------------------------------------*/
void FixPoint::Init() {
	//  typical values
	m_color = wxColour(0, 0, 0);
	m_size = 4;

	m_color_text = wxColour(0, 0, 0);

	m_Font = wxTheFontList->FindOrCreateFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

	m_create_time = wxDateTime::Now();
}


/*----------------------------------------------------------------------
* Draws the fix point on the canvas.
*--------------------------------------------------------------------*/
void FixPoint::Draw() {
	// fix point as screen pixels
	wxPoint screenPoint;
	GetCanvasPixLL(g_vp, &screenPoint, m_lat, m_lon);

	// 3-point 'star' 30 pixels across, not N-aligned
	RenderNpoint(g_pdc, screenPoint.x, screenPoint.y, 3,
		30, (float)(PI / 5), m_color, 250);
}


//----------------------------------------------------------------------
//
//      Event Handler implementation
//
//----------------------------------------------------------------------
// PPW whats this? its using event table and bind????
BEGIN_EVENT_TABLE(PI_EventHandler, wxEvtHandler)
EVT_TIMER(ROLLOVER_TIMER, PI_EventHandler::OnTimerEvent)
EVT_TIMER(HEAD_DOG_TIMER, PI_EventHandler::OnTimerEvent)
EVT_TIMER(LIVE_BRG_TIMER, PI_EventHandler::OnTimerEvent)

END_EVENT_TABLE()



PI_EventHandler::PI_EventHandler(epl_pi * parent)
{
	m_parent = parent;
}

PI_EventHandler::~PI_EventHandler()
{
}


void PI_EventHandler::OnTimerEvent(wxTimerEvent& event)
{
	m_parent->ProcessTimerEvent(event);
}

//PPW removed  need to find what this does????
//void PI_EventHandler::PopupMenuHandler(wxCommandEvent& event)
//{
//	
//	//m_parent->PopupMenuHandler(event);
//}


/** Returns the destination point ( lat & long ) given the start point plus a
* distance and bearing from the start point.
*
* @param lata [in]         start point latitude in degrees
* @param lona [in]         start point longitude in degrees
* @param dist_m [in]       distance from the start point in NM
* @param brg_deg [in]      bearing from the start point in degrees, 0 = N
* @parma latb [out]        computed end point latitude in degrees
* @param lonb [out]        computed end point longitude in degrees
*/
void DestFrom(const double lata, const double lona,
	const double dist_m, const double brg_deg,
	double *latb, double *lonb) {

	// from http://www.movable-type.co.uk/scripts/latlong.html
	// Formula: 	φ2 = asin( sin φ1 ⋅ cos δ + cos φ1 ⋅ sin δ ⋅ cos θ )
	// λ2 = λ1 + atan2( sin θ ⋅ sin δ ⋅ cos φ1, cos δ − sin φ1 ⋅ sin φ2 )
	// where 	φ is latitude, λ is longitude, θ is the bearing (clockwise from
	// north), δ is the angular distance d/R; d being the distance travelled,
	//  R the earth’s radius  (mean radius = 6,371km)
	double phi1 = lata * PI / 180;
	double lambda1 = lona * PI / 180;
	double delta = dist_m * 1852.0 / 6371000;
	double theta = brg_deg * PI / 180;
	double phi2 = asin(sin(phi1) * cos(delta) + cos(phi1) * sin(delta) * cos(theta));
	double lambda2 = lambda1 + atan2(sin(theta) * sin(delta) * cos(phi1), cos(delta) - sin(phi1) * sin(phi2));

	*latb = phi2 / PI * 180;
	*lonb = lambda2 / PI * 180;
}

/** Returns the distance in NM from the given start point and bearing to the
* nearest edge assuming simple Mercator geometry.  Deals with out-of-bounds or
* very large scales by returning minimum lengths; very small scales with
* maximum lengths.
*
* @param lat_p [in]        start point latitude in degrees
* @param lon_p [in]        start point longitude in degrees
* @param brg_deg [in]      bearing from the start point in degrees, 0 = N
* @parma lat_top [in]      latitude at the top of the chart [area] in degrees
* @parma lat_bot [in]      latitude at the bottom of the chart [area] in degrees
* @parma lon_left [in]     longitude at the left of the chart [area] in degrees
* @parma lon_right [in]    longitude at the right of the chart [area] in degrees
*/
double DistanceToEdge(const double lat_p, const double lon_p,
	const double brg_deg,
	const double lat_top, const double lat_bot,
	const double lon_left, const double lon_right) {

	// take lat_p as the defining latitude
	// a = dist to top of chart area
	// b = dist to left
	// c = dist to bottom
	// d = dist to right
	double lat_fact = cos(lat_p * PI / 180.0);
	double a = (lat_top - lat_p) * 60;
	double b = (lon_p - lon_left) * 60 * lat_fact;
	double c = (lat_p - lat_bot) * 60;
	double d = (lon_right - lon_p) * 60 * lat_fact;

	// deal with very large scales
	if (a + c < 0.1 || b + d < 0.1) {
		OutputDebugString(L"large scale, setting to minimum length\r\n");
		return MIN_BRG_LINE_LEN;
	}

	// ditto very small scales
	if (a + c > 30.0 && b + d > 30.0) {
		return MAX_BRG_LINE_LEN;
		OutputDebugString(L"small scale, setting to maximum length\r\n");
	}

	// not inside area?
	if (a <= 0 || b <= 0 || c <= 0 || d <= 0) {
		return MIN_BRG_LINE_LEN;
	}

	// simple, but unlikely cases
	if (brg_deg == 0.0) {
		return a;

	}
	else if (brg_deg == 90.0) {
		return d;

	}
	else if (brg_deg == 180.0) {
		return c;

	}
	else if (brg_deg == 270.0) {
		return b;
	}

	// using simple Mercator projection assumption, compute the bearings to the
	// four corners
	double brg = brg_deg * PI / 180.0;
	double ang_a = atan(d / a);
	if (brg <= ang_a) {
		return a / cos(brg);
	}

	double ang_b = PI / 2 + atan(c / d);
	if (brg <= ang_b) {
		return d / sin(brg);
	}

	double ang_c = PI + atan(b / c);
	if (brg <= ang_c) {
		return c / -cos(brg);
	}

	double ang_d = 3 * PI / 2 + atan(a / b);
	if (brg <= ang_d) {
		return b / -sin(brg);
	}

	return a / cos(brg);
}
