/***************************************************************************
 *
 * Project:  OpenCPN
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

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers
#include <typeinfo>
#include "epl_pi.h"
#include "icons.h"
#include "Select.h"
#include "vector2d.h"

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif


//      Global Static variable
PlugIn_ViewPort         *g_vp;
PlugIn_ViewPort         g_ovp;

double                   g_Var;         // assummed or calculated variation


#if wxUSE_GRAPHICS_CONTEXT
wxGraphicsContext       *g_gdc;
#endif
wxDC                    *g_pdc;


#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(ArrayOfBrgLines);
WX_DEFINE_OBJARRAY(ArrayOf2DPoints);

#include "default_pi.xpm"

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi( void *ppimgr )
{
    return (opencpn_plugin *) new epl_pi( ppimgr );
}

extern "C" DECL_EXP void destroy_pi( opencpn_plugin* p )
{
    delete p;
}



/*  These two function were taken from gpxdocument.cpp */
int GetRandomNumber(int range_min, int range_max)
{
      long u = (long)wxRound(((double)rand() / ((double)(RAND_MAX) + 1) * (range_max - range_min)) + range_min);
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
    dest->clat =                   src->clat;                   // center point
    dest->clon =                   src->clon;
    dest->view_scale_ppm =         src->view_scale_ppm;
    dest->skew =                   src->skew;
    dest->rotation =               src->rotation;
    dest->chart_scale =            src->chart_scale;
    dest->pix_width =              src->pix_width;
    dest->pix_height =             src->pix_height;
    dest->rv_rect =                src->rv_rect;
    dest->b_quilt =                src->b_quilt;
    dest->m_projection_type =      src->m_projection_type;
    
    dest->lat_min =                src->lat_min;
    dest->lat_max =                src->lat_max;
    dest->lon_min =                src->lon_min;
    dest->lon_max =                src->lon_max;
    
    dest->bValid =                 src->bValid;                 // This VP is valid
}
//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

epl_pi::epl_pi( void *ppimgr ) :
        wxTimer( this ), opencpn_plugin_112( ppimgr )
{
    // Create the PlugIn icons
//    initialize_images();

    m_pplugin_icon = new wxBitmap(default_pi);

}

epl_pi::~epl_pi( void )
{
}

int epl_pi::Init( void )
{
    AddLocaleCatalog( _T("opencpn-epl_pi") );
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
    m_nfix = 0;
    m_bshow_fix_hat = false;
    
    m_FixHatColor = wxColour(0, 128, 128);
    

    //    Get a pointer to the opencpn configuration object
//    m_pconfig = GetOCPNConfigObject();

    //    And load the configuration items
//    LoadConfig();

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
    m_RolloverPopupTimer.SetOwner( m_event_handler, ROLLOVER_TIMER );
    m_rollover_popup_timer_msec = 20;
    m_pBrgRolloverWin = NULL;
    m_pRolloverBrg = NULL;
    
    m_select = new Select();

    m_head_dog_timer.SetOwner( m_event_handler, HEAD_DOG_TIMER );
    m_head_active = false;
    
    ///  Testing
#if 0
    brg_line *brg = new brg_line(350, TRUE_BRG, 42.033, -70.183, 2.);
    m_brg_array.Add(brg);
    m_select->AddSelectablePoint( brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
    m_select->AddSelectablePoint( brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
    m_select->AddSelectableSegment( brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG );

    brg = new brg_line(120, TRUE_BRG, 42.033, -70.183, 2.);
    m_brg_array.Add(brg);
    m_select->AddSelectablePoint( brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
    m_select->AddSelectablePoint( brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
    m_select->AddSelectableSegment( brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG );
    
    brg = new brg_line(200, TRUE_BRG, 42.033, -70.183, 2.);
    m_brg_array.Add(brg);
    m_select->AddSelectablePoint( brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
    m_select->AddSelectablePoint( brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
    m_select->AddSelectableSegment( brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG );
#endif    
    
    
    return (WANTS_OVERLAY_CALLBACK |
            WANTS_OPENGL_OVERLAY_CALLBACK |
            WANTS_CURSOR_LATLON       |
            WANTS_TOOLBAR_CALLBACK    |
            INSTALLS_TOOLBAR_TOOL     |
            WANTS_CONFIG              |
            WANTS_PREFERENCES         |
            WANTS_PLUGIN_MESSAGING    |
            WANTS_NMEA_SENTENCES      |
            WANTS_NMEA_EVENTS         |
            WANTS_MOUSE_EVENTS
            );
    
}

bool epl_pi::DeInit( void )
{
    if( IsRunning() ) // Timer started?
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


/*----------------------------------------------------------------------
 * Implements the menu items shown on the context menu.
 *--------------------------------------------------------------------*/
void epl_pi::PopupMenuHandler( wxCommandEvent& event )
{
	bool handled = false;
	switch( event.GetId() ) {
	case ID_EPL_DELETE_SGL:
		if(m_sel_brg){

			// delete the selectable
			if(m_pFind){
				m_select->DeleteSelectablePoint( m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
				m_select->DeleteSelectablePoint( m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
				m_select->DeleteSelectableSegment( m_sel_brg, SELTYPE_SEG_GENERIC, SEL_SEG );
			}

			//  Remove the bearing from the array
			for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
				brg_line *pb = m_brg_array.Item(i);
				if(pb == m_sel_brg){
					m_brg_array.Detach(i);
				}
			}

			//  Finally, delete the bearing
			delete m_sel_brg;
		}

		handled = true;
		break;

	case ID_EPL_DELETE_ALL:
		//  simply empty the array of bearing lines
		m_brg_array.Clear();

		handled = true;
		break;

	case ID_EPL_XMIT:
		{
			m_NMEA0183.TalkerID = _T("EC");

			SENTENCE snt;

			if( m_fix_lat < 0. )
				m_NMEA0183.Gll.Position.Latitude.Set( -m_fix_lat, _T("S") );
			else
				m_NMEA0183.Gll.Position.Latitude.Set( m_fix_lat, _T("N") );

			if( m_fix_lon < 0. )
				m_NMEA0183.Gll.Position.Longitude.Set( -m_fix_lon, _T("W") );
			else
				m_NMEA0183.Gll.Position.Longitude.Set( m_fix_lon, _T("E") );

			wxDateTime now = wxDateTime::Now();
			wxDateTime utc = now.ToUTC();
			wxString time = utc.Format( _T("%H%M%S") );
			m_NMEA0183.Gll.UTCTime = time;

			m_NMEA0183.Gll.Mode = _T("M");              // Use GLL 2.3 specification
			// and send "M" for "Manual fix"
			m_NMEA0183.Gll.IsDataValid = NFalse;        // Spec requires "Invalid" for manual fix

			m_NMEA0183.Gll.Write( snt );

			wxLogMessage(snt.Sentence);
			PushNMEABuffer( snt.Sentence );

			handled = true;
		}
		break;

	default:
		;		// do nothing
	}

	if(!handled)
		event.Skip();
}
    

/*----------------------------------------------------------------------
 * Deals with the low-level actions on the mouse including roll-over,
 * mouse clicks etc.  Dispatches to various handlers depending on
 * whether a context menu or pop-up item is required.
 * 
 * @return					true if this method handled the event
 *--------------------------------------------------------------------*/
bool epl_pi::MouseEventHook( wxMouseEvent &event )
{
	bool bret = false;

	m_mouse_x = event.m_x;
	m_mouse_y = event.m_y;

	//  Retrigger the rollover timer
	if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() )
		m_RolloverPopupTimer.Start( 10, wxTIMER_ONE_SHOT );               // faster response while the rollover is turned on
	else
		m_RolloverPopupTimer.Start( m_rollover_popup_timer_msec, wxTIMER_ONE_SHOT );

	wxPoint mp(event.m_x, event.m_y);
	GetCanvasLLPix( &g_ovp, mp, &m_cursor_lat, &m_cursor_lon);

	//  On button push, find any bearing line selecteable, and other useful data
	if( event.RightDown() || event.LeftDown()) {
		m_sel_brg = NULL;

		m_pFind = m_select->FindSelection( m_cursor_lat, m_cursor_lon, SELTYPE_POINT_GENERIC );

		if(m_pFind){
			for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
				brg_line *pb = m_brg_array.Item(i);

				if(m_pFind->m_pData1 == pb){
					m_sel_brg = pb;
					m_sel_part = m_pFind->GetUserData();
					if(SEL_POINT_A == m_sel_part){
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
			m_pFind = m_select->FindSelection( m_cursor_lat, m_cursor_lon, SELTYPE_SEG_GENERIC );

			if(m_pFind){
				for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
					brg_line *pb = m_brg_array.Item(i);

					if(m_pFind->m_pData1 == pb){
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
	}


	if( event.RightDown() ) {

		if( m_sel_brg || m_bshow_fix_hat){

			wxMenu* contextMenu = new wxMenu;

			wxMenuItem *brg_item_sgl = 0;
			wxMenuItem *brg_item_all = 0;
			wxMenuItem *fix_item = 0;

			if(!m_bshow_fix_hat && m_sel_brg){
				// add items to the menu only relevant to a bearing line
				brg_item_sgl = new wxMenuItem(contextMenu, ID_EPL_DELETE_SGL, _("Delete Bearing Line") );
				contextMenu->Append(brg_item_sgl);
				GetOCPNCanvasWindow()->Connect( ID_EPL_DELETE_SGL, wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler( epl_pi::PopupMenuHandler ), NULL, this );

				brg_item_all = new wxMenuItem(contextMenu, ID_EPL_DELETE_ALL, _("Delete All Bearing Lines") );
				contextMenu->Append(brg_item_all);
				GetOCPNCanvasWindow()->Connect( ID_EPL_DELETE_ALL, wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler( epl_pi::PopupMenuHandler ), NULL, this );
			}

			if(m_bshow_fix_hat){
				// add items to the menu only relevant to a potential fix area
				wxMenuItem *fix_item = new wxMenuItem(contextMenu, ID_EPL_XMIT, _("Send sighted fix to device") );
				contextMenu->Append(fix_item);
				GetOCPNCanvasWindow()->Connect( ID_EPL_XMIT, wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler( epl_pi::PopupMenuHandler ), NULL, this );
			}

			//   Invoke the drop-down menu
			GetOCPNCanvasWindow()->PopupMenu( contextMenu, m_mouse_x, m_mouse_y );

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
			if(brg_item_sgl){
				GetOCPNCanvasWindow()->Disconnect( ID_EPL_DELETE_SGL, wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler( epl_pi::PopupMenuHandler ), NULL, this );
			}

			if(brg_item_all){
				GetOCPNCanvasWindow()->Disconnect( ID_EPL_DELETE_ALL, wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler( epl_pi::PopupMenuHandler ), NULL, this );
			}

			if(fix_item){
				GetOCPNCanvasWindow()->Connect( ID_EPL_XMIT, wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler( epl_pi::PopupMenuHandler ), NULL, this );
			}

			bret = true;                // I have eaten this event
		}

	}

	else if( event.LeftDown() ) {
	}

	else if(event.Dragging()){
		if(m_sel_brg){
			if( (SEL_POINT_A == m_sel_part) || (SEL_POINT_B == m_sel_part)){
				double dx, dy;
				toSM_Plugin(m_cursor_lat, m_cursor_lon, m_sel_pt_lat, m_sel_pt_lon, &dx, &dy);
				double distance = sqrt( (dx * dx) + (dy * dy));

				double new_lat = m_sel_pt_lat;
				double new_lon = m_sel_pt_lon;

				double alpha = atan2(dx, dy);
				if(alpha < 0)
					alpha += 2 * PI;

				double brg_perp = (m_sel_brg->GetBearingTrue() - 90) * PI / 180.;
				if(brg_perp < 0)
					brg_perp += 2 * PI;

				double delta_alpha = alpha - brg_perp;
				if(delta_alpha < 0)
					delta_alpha += 2 * PI;

				double move_dist = distance * sin(delta_alpha);

				double ndy = move_dist * cos(m_sel_brg->GetBearingTrue() * PI / 180.);
				double ndx = move_dist * sin(m_sel_brg->GetBearingTrue() * PI / 180.);

				fromSM_Plugin(ndx, ndy, m_sel_pt_lat, m_sel_pt_lon, &new_lat, &new_lon);

				//  Update the brg line parameters
				if(SEL_POINT_A == m_sel_part){
					m_sel_brg->m_latA = new_lat;
					m_sel_brg->m_lonA = new_lon;
				}
				else if(SEL_POINT_B == m_sel_part){
					m_sel_brg->m_latB = new_lat;
					m_sel_brg->m_lonB = new_lon;
				}

				m_sel_brg->CalcLength();

				// Update the selectable items
				if(m_pFind){
					m_select->DeleteSelectableSegment( m_sel_brg, SELTYPE_SEG_GENERIC, SEL_SEG );
					m_select->AddSelectableSegment( m_sel_brg->GetLatA(), m_sel_brg->GetLonA(),
						m_sel_brg->GetLatB(), m_sel_brg->GetLonB(),
						m_sel_brg, SEL_SEG );

					m_pFind->m_slat = new_lat;
					m_pFind->m_slon = new_lon;
				}

				GetOCPNCanvasWindow()->Refresh();
				bret = true;
			}
			else if(SEL_SEG == m_sel_part){

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
				m_select->DeleteSelectablePoint( m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
				m_select->DeleteSelectablePoint( m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
				m_select->DeleteSelectableSegment( m_sel_brg, SELTYPE_SEG_GENERIC, SEL_SEG );

				m_select->AddSelectablePoint( m_sel_brg->GetLatA(), m_sel_brg->GetLonA(), m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
				m_select->AddSelectablePoint( m_sel_brg->GetLatB(), m_sel_brg->GetLonB(), m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
				m_select->AddSelectableSegment( m_sel_brg->GetLatA(), m_sel_brg->GetLonA(),
					m_sel_brg->GetLatB(), m_sel_brg->GetLonB(),
					m_sel_brg, SEL_SEG );


				GetOCPNCanvasWindow()->Refresh();
				bret = true;
			}
		}
	}
	if( event.LeftUp() ) {
		if(m_sel_brg)
			bret = true;

		m_pFind = NULL;
		m_sel_brg = NULL;

		m_nfix = CalculateFix();

	}

	return bret;
}


void epl_pi::ProcessTimerEvent( wxTimerEvent& event )
{
    if(event.GetId() == ROLLOVER_TIMER)
        OnRolloverPopupTimerEvent( event );
    else if(event.GetId() == HEAD_DOG_TIMER)    // no hdt source available
        m_head_active = false;
}


void epl_pi::OnRolloverPopupTimerEvent( wxTimerEvent& event )
{
    bool b_need_refresh = false;
    
    bool showRollover = false;

    //  Check to see if we are in the cocked hat fix region...
    
    if(m_nfix >= 2){
        MyFlPoint hat_array[100];            // enough			** TODO - deal with any number of bearing lines
        
        for(unsigned int i=0 ; i < m_hat_array.GetCount() ; i++){
            vector2D *pt = m_hat_array.Item(i);
            
            hat_array[i].y = pt->lat;
            hat_array[i].x = pt->lon;
        }
        
        if( G_FloatPtInPolygon(hat_array, m_hat_array.GetCount(), m_cursor_lon, m_cursor_lat) ){
            if(!m_bshow_fix_hat)
                b_need_refresh = true;
            
            m_bshow_fix_hat = true;
        }
        else{
            if(m_bshow_fix_hat)
                b_need_refresh = true;
            
            m_bshow_fix_hat = false;
        }
        
    }

    if(m_nfix == 1){
 
        vector2D *pt = m_hat_array.Item(0);
        
        wxPoint fpoint;
        GetCanvasPixLL(&g_ovp, &fpoint, pt->lat,  pt->lon);
        
        wxPoint cpoint;
        GetCanvasPixLL(&g_ovp, &cpoint, m_cursor_lat, m_cursor_lon);
        
        double dx2 = ( ((cpoint.x - fpoint.x) * (cpoint.x - fpoint.x)) +
                            ((cpoint.y - fpoint.y) * (cpoint.y - fpoint.y)) );
        
        if(dx2 < 20 * 20){
            if(!m_bshow_fix_hat)
                b_need_refresh = true;
            
            m_bshow_fix_hat = true;
        }
        else{
            if(m_bshow_fix_hat)
                b_need_refresh = true;
            
            m_bshow_fix_hat = false;
        }
    }
        
        
    if( !m_bshow_fix_hat && (NULL == m_pRolloverBrg) ) {
       
        m_pFind = m_select->FindSelection( m_cursor_lat, m_cursor_lon, SELTYPE_SEG_GENERIC );
        
        if(m_pFind){
            for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
                brg_line *pb = m_brg_array.Item(i);
                
                if(m_pFind->m_pData1 == pb){
                    m_pRolloverBrg = pb;
                    break;
                }
            }
        }
    
        if( m_pRolloverBrg )
        {
                showRollover = true;
                
                if( NULL == m_pBrgRolloverWin ) {
                    m_pBrgRolloverWin = new RolloverWin( GetOCPNCanvasWindow() );
                    m_pBrgRolloverWin->IsActive( false );
                }
                
                if( !m_pBrgRolloverWin->IsActive() ) {
                    wxString s;
                    
                    s = _T("Test Rollover");
#if 0                    
                    RoutePoint *segShow_point_a = (RoutePoint *) m_pRolloverRouteSeg->m_pData1;
                    RoutePoint *segShow_point_b = (RoutePoint *) m_pRolloverRouteSeg->m_pData2;
                    
                    double brg, dist;
                    DistanceBearingMercator( segShow_point_b->m_lat, segShow_point_b->m_lon,
                                             segShow_point_a->m_lat, segShow_point_a->m_lon, &brg, &dist );
                    
                    if( !pr->m_bIsInLayer )
                        s.Append( _("Route: ") );
                    else
                        s.Append( _("Layer Route: ") );
                    
                    if( pr->m_RouteNameString.IsEmpty() ) s.Append( _("(unnamed)") );
                    else
                        s.Append( pr->m_RouteNameString );
                    
                    s << _T("\n") << _("Total Length: ") << FormatDistanceAdaptive( pr->m_route_length)
                    << _T("\n") << _("Leg: from ") << segShow_point_a->GetName()
                    << _(" to ") << segShow_point_b->GetName()
                    << _T("\n");
                    
                    if( g_bShowMag )
                        s << wxString::Format( wxString("%03d°(M)  ", wxConvUTF8 ), (int)gFrame->GetTrueOrMag( brg ) );
                    else
                        s << wxString::Format( wxString("%03d°  ", wxConvUTF8 ), (int)gFrame->GetTrueOrMag( brg ) );
                    
                    s << FormatDistanceAdaptive( dist );
                    
                    // Compute and display cumulative distance from route start point to current
                    // leg end point.
                    
                    if( segShow_point_a != pr->pRoutePointList->GetFirst()->GetData() ) {
                        wxRoutePointListNode *node = (pr->pRoutePointList)->GetFirst()->GetNext();
                        RoutePoint *prp;
                        float dist_to_endleg = 0;
                        wxString t;
                        
                        while( node ) {
                            prp = node->GetData();
                            dist_to_endleg += prp->m_seg_len;
                            if( prp->IsSame( segShow_point_a ) ) break;
                            node = node->GetNext();
                        }
                        s << _T(" (+") << FormatDistanceAdaptive( dist_to_endleg ) << _T(")");
                    }
#endif                    
                    m_pBrgRolloverWin->SetString( s );
                    
//                    wxSize win_size = GetSize();
                    m_pBrgRolloverWin->SetBestPosition( m_mouse_x, m_mouse_y, 16, 16, 0, wxSize(100, 100));
                    m_pBrgRolloverWin->SetBitmap( 0 );
                    m_pBrgRolloverWin->IsActive( true );
                    b_need_refresh = true;
                    showRollover = true;
                }
            }
        
    } else {
        //    Is the cursor still in select radius?
        if( !m_select->IsSelectableSegmentSelected( m_cursor_lat, m_cursor_lon, m_pFind ) )
            showRollover = false;
        else
            showRollover = true;
    }
    
    
        
    if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() && !showRollover ) {
        m_pBrgRolloverWin->IsActive( false );
        m_pRolloverBrg = NULL;
        m_pBrgRolloverWin->Destroy();
        m_pBrgRolloverWin = NULL;
        b_need_refresh = true;
    } else if( m_pBrgRolloverWin && showRollover ) {
        m_pBrgRolloverWin->IsActive( true );
        b_need_refresh = true;
    }

    if( b_need_refresh )
        GetOCPNCanvasWindow()->Refresh(true);
        
}




bool epl_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
    g_vp = vp;
    Clone_VP(&g_ovp, vp);                // deep copy
    
    double selec_radius = (10 / vp->view_scale_ppm) / (1852. * 60.);
    m_select->SetSelectLLRadius(selec_radius);
    
    
#if wxUSE_GRAPHICS_CONTEXT
    wxMemoryDC *pmdc;
    pmdc = wxDynamicCast(&dc, wxMemoryDC);
    wxGraphicsContext *pgc = wxGraphicsContext::Create( *pmdc );
    g_gdc = pgc;
    g_pdc = &dc;
#else
    g_pdc = &dc;
#endif
    
    //Render

    for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
        brg_line *pb = m_brg_array.Item(i);
        pb->Draw();
//        pb->DrawInfoBox();
        pb->DrawInfoAligned();
    }
    
    if(m_bshow_fix_hat)
        RenderFixHat(  );
    
        
#if wxUSE_GRAPHICS_CONTEXT
    delete g_gdc;
#endif

    if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() ) {
        dc.DrawBitmap( *(m_pBrgRolloverWin->GetBitmap()),
                       m_pBrgRolloverWin->GetPosition().x,
                       m_pBrgRolloverWin->GetPosition().y, false );
    }

    
    return true;
}

bool epl_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
    g_gdc = NULL;
    g_pdc = NULL;
    
    g_vp = vp;
    Clone_VP(&g_ovp, vp);                // deep copy
    
    double selec_radius = (10 / vp->view_scale_ppm) / (1852. * 60.);
    m_select->SetSelectLLRadius(selec_radius);
    
    //Render
    
    for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
        brg_line *pb = m_brg_array.Item(i);
        pb->Draw();
        pb->DrawInfoAligned();
    }
    
    if(m_bshow_fix_hat)
        RenderFixHat();
    
    if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() ) {
        wxImage image = m_pBrgRolloverWin->GetBitmap()->ConvertToImage();
        unsigned char *imgdata = image.GetData();
        if(imgdata){
            glRasterPos2i(m_pBrgRolloverWin->GetPosition().x, m_pBrgRolloverWin->GetPosition().y);
            
            glPixelZoom(1.0, -1.0);
            glDrawPixels(image.GetWidth(), image.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE,imgdata);
            glPixelZoom(1.0, 1.0);
        }
        
    }
    
    return true;
}


void epl_pi::SetNMEASentence( wxString &sentence )
{
    m_NMEA0183 << sentence;
    
    m_NMEA0183 << sentence;

    if( m_NMEA0183.PreParse() ) {
        if( m_NMEA0183.LastSentenceIDReceived == _T("EBL") ) {
            if( m_NMEA0183.Parse() ) {
                ProcessBrgCapture(m_NMEA0183.Ebl.BrgRelative, m_NMEA0183.Ebl.BrgSubtended,
                                  m_NMEA0183.Ebl.BrgAbs, m_NMEA0183.Ebl.BrgAbsTM,
                                  m_NMEA0183.Ebl.BrgIdent, m_NMEA0183.Ebl.BrgTargetName);
                
            }
        }


        else if( m_NMEA0183.LastSentenceIDReceived == _T("HDG") ) {
            if( m_NMEA0183.Parse() ) {
                
                    if( !wxIsNaN( m_NMEA0183.Hdg.MagneticVariationDegrees ) ){
                        if( m_NMEA0183.Hdg.MagneticVariationDirection == East )
                            mVar =  m_NMEA0183.Hdg.MagneticVariationDegrees;
                        else if( m_NMEA0183.Hdg.MagneticVariationDirection == West )
                            mVar = -m_NMEA0183.Hdg.MagneticVariationDegrees;
                    }

                    if( !wxIsNaN(m_NMEA0183.Hdg.MagneticSensorHeadingDegrees) ) {
                        if( !wxIsNaN( mVar ) ){
                            m_hdt = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees + mVar;
                            m_head_active = true;
                            m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
                        }
                    }
                    
            }
        }

        else if( m_NMEA0183.LastSentenceIDReceived == _T("HDM") ) {
            if( m_NMEA0183.Parse() ) {

                if( !wxIsNaN(m_NMEA0183.Hdm.DegreesMagnetic) ) {
                    if( !wxIsNaN( mVar ) ){
                        m_hdt = m_NMEA0183.Hdm.DegreesMagnetic + mVar;
                        m_head_active = true;
                        m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
                    }
                }

            }
        }

        else if( m_NMEA0183.LastSentenceIDReceived == _T("HDT") ) {
            if( m_NMEA0183.Parse() ) {
                if( mPriHeadingT >= 1 ) {
                    mPriHeadingT = 1;
                    if( m_NMEA0183.Hdt.DegreesTrue < 999. ) {
                        m_hdt = m_NMEA0183.Hdt.DegreesTrue;
                        m_head_active = true;
                        m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
                    }
                }
            }
        }

         

    }
}

void epl_pi::SetPositionFix( PlugIn_Position_Fix &pfix )
{
    if(1){
        m_ownship_lat = pfix.Lat;
        m_ownship_lon = pfix.Lon;
        if(!wxIsNaN( pfix.Cog ))
            m_ownship_cog = pfix.Cog;
        if(!wxIsNaN( pfix.Var ))
            mVar = pfix.Var;
    }
}

void epl_pi::SetCursorLatLon( double lat, double lon )
{
}

void epl_pi::SetPluginMessage(wxString &message_id, wxString &message_body)
{
}

int epl_pi::GetToolbarToolCount( void )
{
    return 1;
}


void epl_pi::SetColorScheme( PI_ColorScheme cs )
{
}


void epl_pi::OnToolbarToolCallback( int id )
{
}


bool epl_pi::LoadConfig( void )
{
    
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if( pConf ) {

        return true;
    } else
        return false;
}

bool epl_pi::SaveConfig( void )
{
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if( pConf ) {

        return true;
    } else
        return false;
}

void epl_pi::ApplyConfig( void )
{

}


void epl_pi::ProcessBrgCapture(double brg_rel, double brg_subtended, double brg_TM, int brg_TM_flag,
                           wxString Ident, wxString target)
{
    double brg_true = 0;
    BearingTypeEnum type = TRUE_BRG;
    
    double lat = m_ownship_lat;          // initial declaration causes bearling line to pass thru ownship
    double lon = m_ownship_lon;
    
    //  What initial length?
    //  say 20% of canvas horizontal size, and max of 10 NMi
    
    double l1 = ((g_ovp.pix_width / g_ovp.view_scale_ppm) /1852.) * 0.2;
    double length = wxMin(l1, 10.0);
    
    // normal case is for absolute bearing but if the bearing is relative,
	// the ship's heading needs to be applied
	// ** TODO should check the TM_flag is valid too ( what to do if not... )
	if ( !wxIsNaN(brg_TM) ) {
		// ** TODO change the type [enum] var(?) and apply variation if brg_TM_flag indicates magnetic
		brg_true = brg_TM;
	
	} else {
		//  If we don't have a true heading available, use ownship cog
		if(m_head_active)
			brg_true = brg_rel + m_hdt;
		else
			// ** TODO TBD - if no reliable heading then (a) pop up an error message? and (b) don't plot anything
			brg_true = brg_rel + m_ownship_cog;
	}

    type = TRUE_BRG;
    
    //  Do not add duplicates
    bool b_add = true;
    for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
        brg_line *pb = m_brg_array.Item(i);
        if(pb->GetIdent() == Ident){
            b_add = false;
            break;
        }
    }

    if(b_add){
        brg_line *brg = new brg_line(brg_true, type, lat, lon, length);
        brg->SetIdent(Ident);
        brg->SetTargetName(target);
        
        m_brg_array.Add(brg);
        m_select->AddSelectablePoint( brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
        m_select->AddSelectablePoint( brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
        m_select->AddSelectableSegment( brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG );
        
        GetOCPNCanvasWindow()->Refresh();
        
    }
    
    m_nfix = CalculateFix();
}


int epl_pi::CalculateFix( void )
{
    //  If there are two or more bearing lines stored, calculate the resulting fix
    //  Also, keep an array of points defining the fix for later rendering
 
    m_hat_array.Clear();
    
    if(m_brg_array.GetCount() < 2)
        return 0;
    
    double lat_acc = 0.;
    double lon_acc = 0.;
    int n_intersect = 0;
    for(size_t i=0 ; i < m_brg_array.GetCount()-1 ; i++){
        brg_line *a = m_brg_array.Item(i);
        brg_line *b = m_brg_array.Item(i+1);
        
        double lat, lon;
        if(a->getIntersect(b, &lat, &lon)){
            lat_acc += lat;
            lon_acc += lon;
            n_intersect ++;
            
            vector2D *pt = new vector2D(lon, lat);
            m_hat_array.Add(pt);
        }
    }

    if(m_brg_array.GetCount() > 2){
        brg_line *a = m_brg_array.Item(0);
        brg_line *b = m_brg_array.Item(m_brg_array.GetCount()-1);
        
        double lat, lon;
        if(a->getIntersect(b, &lat, &lon)){
            lat_acc += lat;
            lon_acc += lon;
            n_intersect ++;
            
            vector2D *pt = new vector2D(lon, lat);
            m_hat_array.Add(pt);
        }
        
    }
    
    //  Have at least one intersection,
    //  so calculate the fix as the average of all intersections
    if(n_intersect){
        m_fix_lat = lat_acc / n_intersect;
        m_fix_lon = lon_acc / n_intersect;
    }
    
    return (n_intersect);    
    
}


void epl_pi::RenderFixHat( void )
{
    if(!m_nfix)
        return;                 // no fix
   
    if(m_nfix == 1){            // two line fix

        if(m_hat_array.GetCount() == 1){
            vector2D *pt = m_hat_array.Item(0);
            wxPoint ab;
            GetCanvasPixLL(g_vp, &ab, pt->lat, pt->lon);
 
            int crad = 20;
            AlphaBlending( g_pdc, ab.x - crad, ab.y - crad, crad*2, crad *2, 3.0, m_FixHatColor, 250 );
        }
    }
    else if(m_nfix > 2){
        
        //      Get an array of wxPoints
//        printf("\n");
        wxPoint *pta = new wxPoint[m_nfix];
        for(unsigned int i=0 ; i < m_hat_array.GetCount() ; i++){
            vector2D *pt = m_hat_array.Item(i);
            wxPoint ab;
            GetCanvasPixLL(g_vp, &ab, pt->lat, pt->lon);
//            printf("%g %g %d %d\n", pt->lat, pt->lon, ab.x, ab.y);
            pta[i] = ab;
        }

        AlphaBlendingPoly( g_pdc, m_hat_array.GetCount(), pta, m_FixHatColor, 250 );
        
    }
 
}


//-----------------------------------------------------------------------------------------------------------------
//
//      Bearing Line Class implementation
//
//-----------------------------------------------------------------------------------------------------------------

brg_line::brg_line(double bearing, BearingTypeEnum type)
{
    Init();
    
    m_bearing = bearing;
    m_type = type;
}

brg_line::brg_line(double bearing, BearingTypeEnum type, double lat_point, double lon_point, double length)
{ 
    Init();

    m_bearing = bearing;
    m_type = type;
    if(TRUE_BRG == type)
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
    fromSM_Plugin(dx, dy, m_latA, m_lonA, &adj_lata, &adj_lona);

    m_latA = adj_lata;
    m_lonA = adj_lona;
    
    CalcPointB();
}

brg_line::~brg_line()
{
}


void brg_line::Init(void)
{
    //  Set some innocent initial values
    m_bearing = 0;
    m_bearing_true = 0;
    m_type = TRUE_BRG;
    m_latA = m_latB = m_lonA = m_lonB = 0;
    m_length = 100;
    
    m_color = wxColour(0,0,0);
    m_width = 4;
    
    m_color_text = wxColour(0,0,0);
    
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
    
  
    bool ret = LineIntersect( p1, p2, p3, p4, &ilon, &ilat);
    
    if(lat)
        *lat = ilat;
    if(lon)
        *lon = ilon;
    
    return ret;

}



void brg_line::CalcPointB(void)
{
    //  We calculate the other endpoint using simple mercator projection
    
    double dy = m_length * 1852.0 * cos(m_bearing_true * PI / 180.);        // east/north in metres
    double dx = m_length * 1852.0 * sin(m_bearing_true * PI / 180.);
    
    fromSM_Plugin(dx, dy, m_latA, m_lonA, &m_latB, &m_lonB);
    
}

void brg_line::CalcLength(void)
{
    // We re-calculate the length from the two endpoints
    double dx, dy;
    toSM_Plugin(m_latA, m_lonA, m_latB, m_lonB, &dx, &dy);
    double distance = sqrt( (dx * dx) + (dy * dy));

    m_length = distance / 1852.;
}

void brg_line::DrawInfoBox( void )
{
    //  Calculate the points
    wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, m_latA, m_lonA);
    
    wxPoint cd;
    GetCanvasPixLL(g_vp, &cd, m_latB, m_lonB);
    
    //  Draw a blank, semi-transparent box
    // Where to draw the info box, by default?
    //  Centered on the brg_line
    int box_width = 200;
    int box_height = 50;
    
    
   
    //  Create the text info string
    g_pdc->SetFont(*m_Font);
    
    //  Use a sample string to gather some text font parameters
    wxString info;
    int sx, sy;
    int ypitch;
    int max_info_text_length = 0;
    int xmargin = 5;    // margin for text in the box
 
    wxString sample = _T("W");
    g_pdc->GetTextExtent(sample, &sx, &sy);
    ypitch = sy;    // line pitch
    
    info.Printf(_T("Bearing: %g\n"), m_bearing_true);
    g_pdc->GetTextExtent(info, &sx, &sy);
    max_info_text_length = wxMax(max_info_text_length, sx);
    int nlines = 1;
    
    //  Now add some more lines
    wxString info2 = _T("Capture time: ");
    info2 << m_create_time.FormatDate() <<_T("  ") << m_create_time.FormatTime();
    g_pdc->GetTextExtent(info2, &sx, &sy);
    max_info_text_length = wxMax(max_info_text_length, sx);
    info  << info2;
    nlines++;
    
    // Draw the blank box
    
    // Size...
    box_width = max_info_text_length + (xmargin * 2);
    box_height = nlines * (ypitch + 4);
    
    //Position...
    int xp = ((ab.x + cd.x)/2) ;
    int yp = ((ab.y + cd.y)/2) ;
    
    double angle = 0.;
    double distance = box_width * .75;
    
    double dx = 0;
    double dy = 0;
//    double lx = 0;
//    double ly = 0;
    
    //  Position of info box depends on orientation of the bearing line.
    if((m_bearing_true >0 ) && (m_bearing_true < 90)){
        angle = m_bearing_true - 90.;
        angle = angle * PI / 180.;
        dx = distance * cos(angle);
        dy = -distance * sin(angle);
//        lx = 0;
//        ly = 0;
    }
  
    else if((m_bearing_true > 90 ) && (m_bearing_true < 180)){
      angle = m_bearing_true - 90.;
      angle = angle * PI / 180.;
      dx = (distance * sin(angle));
      dy = -distance * cos(angle);
//      lx = 0;
//      ly = 0;
    }
  
  else if((m_bearing_true > 180 ) && (m_bearing_true < 270)){
      angle = m_bearing_true - 90.;
      angle = angle * PI / 180.;
      dx = -(distance * sin(angle)) - box_width;
      dy = distance * cos(angle);
//      lx = box_width;
//      ly = 0;
  }

  else if((m_bearing_true > 270 ) && (m_bearing_true < 360)){
      angle = m_bearing_true - 90.;
      angle = angle * PI / 180.;
      dx = (distance * sin(angle)) - box_width;
      dy = (-distance * cos(angle));
//      lx = box_width;
//      ly = 0;
  }
  
    int box_xp = xp + dx;
    int box_yp = yp + dy;
    
    
    AlphaBlending( g_pdc, box_xp, box_yp, box_width, box_height, 6.0, m_InfoBoxColor, 127 );

    //  Leader line
//    RenderLine(xp, yp, box_xp + lx, box_yp + ly, m_InfoBoxColor, 4);
    
    //  Now draw the text
    
    g_pdc->DrawLabel( info, wxRect( box_xp + xmargin, box_yp, box_width, box_height ),
                      wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
    
    
    if(g_pdc) {
        wxPen ppbrg ( m_color, m_width );
        
#if wxUSE_GRAPHICS_CONTEXT
        g_gdc->SetPen(ppbrg);
#endif
        
        g_pdc->SetPen(ppbrg);
    } else { /* opengl */
        
#ifdef ocpnUSE_GL          
glColor4ub(m_color.Red(), m_color.Green(), m_color.Blue(),
           255/*m_color.Alpha()*/);
#endif          
    }
    
    
            
     if(g_pdc) {
#if wxUSE_GRAPHICS_CONTEXT
//        g_gdc->StrokeLine(ab.x, ab.y, cd.x, cd.y);
#else
//        g_dc->DrawLine(ab.x, ab.y, cd.x, cd.y);
#endif        
     } else { /* opengl */
#ifdef ocpnUSE_GL                
//        pof->DrawGLLine(ab.x, ab.y, cd.x, cd.y, 2);
#endif                
     }
}



void brg_line::DrawInfoAligned( void )
{

    //  Calculate the points
    wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, m_latA, m_lonA);
    
    wxPoint cd;
    GetCanvasPixLL(g_vp, &cd, m_latB, m_lonB);

    //  Create the text info string
    
    wxString info;
    info.Printf(_T("Bearing: %g"), m_bearing_true);
    wxString degree = wxString::Format(_T("%cT"), 0x00B0); //_T("°");
    info += degree;
    wxString info2 = _T("  Ident: ");
    info2 += m_Ident;
    
    info += info2;

    //  Use a screen DC to calulate the drawing location/size
    wxScreenDC sdc;
    sdc.SetFont(*m_Font);
    
    int sx, sy;
    sdc.GetTextExtent(info, &sx, &sy);
    
    int offset = 5;
    double angle = m_bearing_true * PI / 180.;
    
    int off_x = offset * cos(angle);
    int off_y = offset * sin(angle);
    
    int ctr_x = -(sx/2) * sin(angle);
    int ctr_y = (sx/2) * cos(angle);
    
    if(m_bearing_true < 180.){
        int xp, yp;
        xp = ctr_x + off_x + (ab.x + cd.x)/2;
        yp = ctr_y + off_y + (ab.y + cd.y)/2;

        RenderText( g_pdc, g_gdc, info, m_Font, m_color_text, xp, yp, 90. - m_bearing_true);
    }
    else{
        int xp, yp;
        xp = -ctr_x - off_x + (ab.x + cd.x)/2;
        yp = -ctr_y - off_y + (ab.y + cd.y)/2;
        
        RenderText( g_pdc, g_gdc, info, m_Font, m_color_text, xp, yp, 90. - m_bearing_true + 180.);
    }
}

void brg_line::Draw( void )
{
    //  Calculate the points
    wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, m_latA, m_lonA);
    
    wxPoint cd;
    GetCanvasPixLL(g_vp, &cd, m_latB, m_lonB);
    
    //  Adjust the rendering width, to make it 100 metres wide
    double dwidth = 100 * g_ovp.view_scale_ppm;
    
    dwidth = wxMin(4, dwidth);
    dwidth = wxMax(2, dwidth);
    
//    printf("%g\n", dwidth);
    RenderLine(g_pdc, g_gdc, ab.x, ab.y, cd.x, cd.y, m_color, dwidth);

}




//      Event Handler implementation
BEGIN_EVENT_TABLE ( PI_EventHandler, wxEvtHandler )
EVT_TIMER ( ROLLOVER_TIMER, PI_EventHandler::OnTimerEvent )
EVT_TIMER ( HEAD_DOG_TIMER, PI_EventHandler::OnTimerEvent )

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
    m_parent->ProcessTimerEvent( event );
}

void PI_EventHandler::PopupMenuHandler(wxCommandEvent& event )
{
    m_parent->PopupMenuHandler( event );
}
