/******************************************************************************
 *
 * Project:  OpenCPN PlugIn support
 *
 ***************************************************************************
 *   Copyright (C) 2014 4by David S. Register                               *
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

#include "Select.h"
//#include "georef.h"
//#include "vector2D.h"
//#include "navutil.h"
//#include "chcanv.h"
#include "ocpn_plugin.h"
#include "vector2d.h"


Select::Select()
{
    pSelectList = new SelectableItemList;
    pixelRadius = 8;
//    int w,h;
//    wxDisplaySize( &w, &h );
//    if( h > 800 ) pixelRadius = 10;
//    if( h > 1024 ) pixelRadius = 12;
    
}

Select::~Select()
{
    pSelectList->DeleteContents( true );
    pSelectList->Clear();
    delete pSelectList;

}

#if 0
bool Select::AddSelectableRoutePoint( float slat, float slon, RoutePoint *pRoutePointAdd )
{
    SelectItem *pSelItem = new SelectItem;
    pSelItem->m_slat = slat;
    pSelItem->m_slon = slon;
    pSelItem->m_seltype = SELTYPE_ROUTEPOINT;
    pSelItem->m_bIsSelected = false;
    pSelItem->m_pData1 = pRoutePointAdd;

    wxSelectableItemListNode *node;
    
    if( pRoutePointAdd->m_bIsInLayer )
        node = pSelectList->Append( pSelItem );
    else
        node = pSelectList->Insert( pSelItem );

    pRoutePointAdd->SetSelectNode(node);
    
    return true;
}

bool Select::AddSelectableRouteSegment( float slat1, float slon1, float slat2, float slon2,
        RoutePoint *pRoutePointAdd1, RoutePoint *pRoutePointAdd2, Route *pRoute )
{
    SelectItem *pSelItem = new SelectItem;
    pSelItem->m_slat = slat1;
    pSelItem->m_slon = slon1;
    pSelItem->m_slat2 = slat2;
    pSelItem->m_slon2 = slon2;
    pSelItem->m_seltype = SELTYPE_ROUTESEGMENT;
    pSelItem->m_bIsSelected = false;
    pSelItem->m_pData1 = pRoutePointAdd1;
    pSelItem->m_pData2 = pRoutePointAdd2;
    pSelItem->m_pData3 = pRoute;

    if( pRoute->m_bIsInLayer ) pSelectList->Append( pSelItem );
    else
        pSelectList->Insert( pSelItem );

    return true;
}

bool Select::DeleteAllSelectableRouteSegments( Route *pr )
{
    SelectItem *pFindSel;

//    Iterate on the select list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == SELTYPE_ROUTESEGMENT ) {

//                  RoutePoint *ps1 = (RoutePoint *)pFindSel->m_pData1;
//                  RoutePoint *ps2 = (RoutePoint *)pFindSel->m_pData2;

            if( (Route *) pFindSel->m_pData3 == pr ) {
                delete pFindSel;
                pSelectList->DeleteNode( node );   //delete node;

                node = pSelectList->GetFirst();     // reset the top node

                goto got_next_outer_node;
            }
        }

        node = node->GetNext();
        got_next_outer_node: continue;
    }

    return true;
}

bool Select::DeleteAllSelectableRoutePoints( Route *pr )
{
    SelectItem *pFindSel;

//    Iterate on the select list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == SELTYPE_ROUTEPOINT ) {
            RoutePoint *ps = (RoutePoint *) pFindSel->m_pData1;

            //    inner loop iterates on the route's point list
            wxRoutePointListNode *pnode = ( pr->pRoutePointList )->GetFirst();
            while( pnode ) {
                RoutePoint *prp = pnode->GetData();

                if( prp == ps ) {
                    delete pFindSel;
                    pSelectList->DeleteNode( node );   //delete node;
                    prp->SetSelectNode( NULL );
                    
                    node = pSelectList->GetFirst();

                    goto got_next_outer_node;
                }
                pnode = pnode->GetNext();
            }
        }

        node = node->GetNext();
got_next_outer_node: continue;
    }
    return true;
}

bool Select::AddAllSelectableRoutePoints( Route *pr )
{
    if( pr->pRoutePointList->GetCount() ) {
        wxRoutePointListNode *node = ( pr->pRoutePointList )->GetFirst();

        while( node ) {
            RoutePoint *prp = node->GetData();
            AddSelectableRoutePoint( prp->m_lat, prp->m_lon, prp );
            node = node->GetNext();
        }
        return true;
    } else
        return false;
}

bool Select::AddAllSelectableRouteSegments( Route *pr )
{
    wxPoint rpt, rptn;
    float slat1, slon1, slat2, slon2;

    if( pr->pRoutePointList->GetCount() ) {
        wxRoutePointListNode *node = ( pr->pRoutePointList )->GetFirst();

        RoutePoint *prp0 = node->GetData();
        slat1 = prp0->m_lat;
        slon1 = prp0->m_lon;

        node = node->GetNext();

        while( node ) {
            RoutePoint *prp = node->GetData();
            slat2 = prp->m_lat;
            slon2 = prp->m_lon;

            AddSelectableRouteSegment( slat1, slon1, slat2, slon2, prp0, prp, pr );

            slat1 = slat2;
            slon1 = slon2;
            prp0 = prp;

            node = node->GetNext();
        }
        return true;
    } else
        return false;
}

bool Select::AddAllSelectableTrackSegments( Route *pr )
{
    wxPoint rpt, rptn;
    float slat1, slon1, slat2, slon2;

    if( pr->pRoutePointList->GetCount() ) {
        wxRoutePointListNode *node = ( pr->pRoutePointList )->GetFirst();

        RoutePoint *prp0 = node->GetData();
        slat1 = prp0->m_lat;
        slon1 = prp0->m_lon;

        node = node->GetNext();

        while( node ) {
            RoutePoint *prp = node->GetData();
            slat2 = prp->m_lat;
            slon2 = prp->m_lon;

            AddSelectableTrackSegment( slat1, slon1, slat2, slon2, prp0, prp, pr );

            slat1 = slat2;
            slon1 = slon2;
            prp0 = prp;

            node = node->GetNext();
        }
        return true;
    } else
        return false;
}

bool Select::UpdateSelectableRouteSegments( RoutePoint *prp )
{
    SelectItem *pFindSel;
    bool ret = false;

//    Iterate on the select list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == SELTYPE_ROUTESEGMENT ) {
            if( pFindSel->m_pData1 == prp ) {
                pFindSel->m_slat = prp->m_lat;
                pFindSel->m_slon = prp->m_lon;
                ret = true;
                ;
            }

            else
                if( pFindSel->m_pData2 == prp ) {
                    pFindSel->m_slat2 = prp->m_lat;
                    pFindSel->m_slon2 = prp->m_lon;
                    ret = true;
                }
        }
        node = node->GetNext();
    }

    return ret;
}
#endif
SelectItem *Select::AddSelectablePoint( float slat, float slon, const void *pdata, int fseltype, int UserData )
{
    SelectItem *pSelItem = new SelectItem;
    if( pSelItem ) {
        pSelItem->m_slat = slat;
        pSelItem->m_slon = slon;
        pSelItem->m_seltype = fseltype;
        pSelItem->m_bIsSelected = false;
        pSelItem->m_pData1 = pdata;
        pSelItem->UserData = UserData;

        pSelectList->Append( pSelItem );
    }

    return pSelItem;
}


bool Select::AddSelectableSegment( float slat1, float slon1, float slat2, float slon2,
                                   const void *pdata, int UserData )
{
    SelectItem *pSelItem = new SelectItem;
    pSelItem->m_slat = slat1;
    pSelItem->m_slon = slon1;
    pSelItem->m_slat2 = slat2;
    pSelItem->m_slon2 = slon2;
    pSelItem->m_seltype = SELTYPE_SEG_GENERIC;
    pSelItem->m_bIsSelected = false;
    pSelItem->m_pData1 = pdata;
    pSelItem->UserData = UserData;
    
    pSelectList->Append( pSelItem );
    
    return true;
}














/*
bool Select::DeleteAllPoints( void )
{
    pSelectList->DeleteContents( true );
    pSelectList->Clear();
    return true;
}
*/

bool Select::DeleteSelectablePoint( void *pdata, int SeltypeToDelete, int UserData )
{
    SelectItem *pFindSel;

    if( NULL != pdata ) {
//    Iterate on the list
        wxSelectableItemListNode *node = pSelectList->GetFirst();

        while( node ) {
            pFindSel = node->GetData();
            if( pFindSel->m_seltype == SeltypeToDelete ) {
                if( (pdata == pFindSel->m_pData1) && (pFindSel->GetUserData() == UserData) ) {
                    delete pFindSel;
                    delete node;
                    return true;
                }
            }
            node = node->GetNext();
        }
    }
    return false;
}

bool Select::DeleteSelectableSegment( void *pdata, int SeltypeToDelete, int UserData )
{
    SelectItem *pFindSel;
    
    if( NULL != pdata ) {
        //    Iterate on the list
        wxSelectableItemListNode *node = pSelectList->GetFirst();
        
        while( node ) {
            pFindSel = node->GetData();
            if( pFindSel->m_seltype == SeltypeToDelete ) {
                if( (pdata == pFindSel->m_pData1) && (pFindSel->GetUserData() == UserData) ) {
                    delete pFindSel;
                    delete node;
                    return true;
                }
            }
            node = node->GetNext();
        }
    }
    return false;
}


bool Select::DeleteAllSelectableTypePoints( int SeltypeToDelete )
{
    SelectItem *pFindSel;

//    Iterate on the list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == SeltypeToDelete ) {
            delete pFindSel;
            delete node;
            
            if( SELTYPE_ROUTEPOINT == SeltypeToDelete ){
//                RoutePoint *prp = (RoutePoint *)pFindSel->m_pData1;
//                prp->SetSelectNode( NULL );
            }
            
            node = pSelectList->GetFirst();
            goto got_next_node;
        }

        node = node->GetNext();
        got_next_node: continue;
    }
    return true;
}

#if 0
bool Select::DeleteSelectableRoutePoint( RoutePoint *prp )
{
    
    if( NULL != prp ) {
        wxSelectableItemListNode *node = (wxSelectableItemListNode *)prp->GetSelectNode();
        if(node){
            SelectItem *pFindSel = node->GetData();
            if(pFindSel){
                delete pFindSel;
                delete node;            // automatically removes from list
                prp->SetSelectNode( NULL );
                return true;
            }
        }
        else
            return DeleteSelectablePoint( prp, SELTYPE_ROUTEPOINT );
        
    }
    return false;
}

#endif
bool Select::ModifySelectablePoint( float lat, float lon, void *data, int SeltypeToModify )
{
    SelectItem *pFindSel;

//    Iterate on the list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == SeltypeToModify ) {
            if( data == pFindSel->m_pData1 ) {
                pFindSel->m_slat = lat;
                pFindSel->m_slon = lon;
                return true;
            }
        }

        node = node->GetNext();
    }
    return false;
}
#if 0
bool Select::AddSelectableTrackSegment( float slat1, float slon1, float slat2, float slon2,
        RoutePoint *pRoutePointAdd1, RoutePoint *pRoutePointAdd2, Route *pRoute )
{
    SelectItem *pSelItem = new SelectItem;
    pSelItem->m_slat = slat1;
    pSelItem->m_slon = slon1;
    pSelItem->m_slat2 = slat2;
    pSelItem->m_slon2 = slon2;
    pSelItem->m_seltype = SELTYPE_TRACKSEGMENT;
    pSelItem->m_bIsSelected = false;
    pSelItem->m_pData1 = pRoutePointAdd1;
    pSelItem->m_pData2 = pRoutePointAdd2;
    pSelItem->m_pData3 = pRoute;

    if( pRoute->m_bIsInLayer ) pSelectList->Append( pSelItem );
    else
        pSelectList->Insert( pSelItem );

    return true;
}

bool Select::DeleteAllSelectableTrackSegments( Route *pr )
{
    SelectItem *pFindSel;

//    Iterate on the select list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == SELTYPE_TRACKSEGMENT ) {

            if( (Route *) pFindSel->m_pData3 == pr ) {
                delete pFindSel;
                pSelectList->DeleteNode( node );   //delete node;

                node = pSelectList->GetFirst();     // reset the top node
                goto got_next_outer_node;
            }
        }
        node = node->GetNext();
        got_next_outer_node: continue;
    }
    return true;
}

bool Select::DeletePointSelectableTrackSegments( RoutePoint *pr )
{
    SelectItem *pFindSel;

//    Iterate on the select list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == SELTYPE_TRACKSEGMENT ) {

            if( (RoutePoint *) pFindSel->m_pData1 == pr || (RoutePoint *) pFindSel->m_pData2 == pr ) {
                delete pFindSel;
                pSelectList->DeleteNode( node );   //delete node;

                node = pSelectList->GetFirst();     // reset the top node
                goto got_next_outer_node;
            }
        }
        node = node->GetNext();
        got_next_outer_node: continue;
    }
    return true;
}
#endif

bool Select::IsSegmentSelected( float a, float b, float c, float d, float slat, float slon )
{
    double adder = 0.;

    if( ( c * d ) < 0. ) {
        //    Arrange for points to be increasing longitude, c to d
        double dist, brg;
        DistanceBearingMercator_Plugin( a, c, b, d, &brg, &dist );
        
        if( brg < 180. )             // swap points?
                {
            double tmp;
            tmp = c;
            c = d;
            d = tmp;
            tmp = a;
            a = b;
            b = tmp;
        }
        if( d < 0. )     // idl?
                {
            d += 360.;
            if( slon < 0. ) adder = 360.;
        }
    }

//    As a course test, use segment bounding box test
    if( ( slat >= ( wxMin ( a,b ) - selectRadius ) ) && ( slat <= ( wxMax ( a,b ) + selectRadius ) )
        && ( ( slon + adder ) >= ( wxMin ( c,d ) - selectRadius ) )
        && ( ( slon + adder ) <= ( wxMax ( c,d ) + selectRadius ) ) ) {
        //    Use vectors to do hit test....
        vector2D va, vb, vn;

        //    Assuming a Mercator projection
        double ap, cp;
        toSM_Plugin( a, c, 0., 0., &cp, &ap );
    
        double bp, dp;
        toSM_Plugin( b, d, 0., 0., &dp, &bp );
        double slatp, slonp;
        toSM_Plugin( slat, slon + adder, 0., 0., &slonp, &slatp );

        va.x = slonp - cp;
        va.y = slatp - ap;
        vb.x = dp - cp;
        vb.y = bp - ap;

        double delta = vGetLengthOfNormal( &va, &vb, &vn );
        if( fabs( delta ) < ( selectRadius * 1852 * 60 ) ) return true;
    }
    return false;
}

void Select::CalcSelectRadius()
{
//    selectRadius = pixelRadius / ( cc1->GetCanvasTrueScale() * 1852 * 60 );
}

SelectItem *Select::FindSelection( float slat, float slon, int fseltype )
{
    float a, b, c, d;
    SelectItem *pFindSel;

    CalcSelectRadius();

//    Iterate on the list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == fseltype ) {
            switch( fseltype ){
                case SELTYPE_POINT_GENERIC:
                    a = fabs( slat - pFindSel->m_slat );
                    b = fabs( slon - pFindSel->m_slon );

                    if( ( fabs( slat - pFindSel->m_slat ) < selectRadius )
                            && ( fabs( slon - pFindSel->m_slon ) < selectRadius ) ) goto find_ok;
                    break;
                case SELTYPE_SEG_GENERIC: {
                    a = pFindSel->m_slat;
                    b = pFindSel->m_slat2;
                    c = pFindSel->m_slon;
                    d = pFindSel->m_slon2;

                    if( IsSegmentSelected( a, b, c, d, slat, slon ) ) goto find_ok;
                    break;
                }
                default:
                    break;
            }
        }

        node = node->GetNext();
    }

    return NULL;
    find_ok: return pFindSel;
}

// PPW Experiment in bug findingtrying to draw bounding box around rollover need to stop lat and longs 
// for all brg lines for use later when rendering
SelectItem *Select::FindAndDrawSelection(float slat, float slon, int fseltype, wxDC *pdc, wxGraphicsContext *gdc)
{
	float a, b, c, d;
	SelectItem *pFindSel;

	//CalcSelectRadius();

	//    Iterate on the list
	wxSelectableItemListNode *node = pSelectList->GetFirst();


	wxColor bob = wxColour(0, 0, 255);

	while (node) {
		pFindSel = node->GetData();
		if (pFindSel->m_seltype == fseltype) {
			switch (fseltype){
			case SELTYPE_POINT_GENERIC:
			{
				a = fabs(slat - pFindSel->m_slat);
				b = fabs(slon - pFindSel->m_slon);

				if ((fabs(slat - pFindSel->m_slat) < selectRadius)
					&& (fabs(slon - pFindSel->m_slon) < selectRadius)) goto find_ok;
				break;
			}
			case SELTYPE_SEG_GENERIC: {
				a = pFindSel->m_slat;
				b = pFindSel->m_slat2;
				c = pFindSel->m_slon;
				d = pFindSel->m_slon2;


				//  Calculate the points
				wxPoint ab, cd;
			/*	GetCanvasPixLL(vp, &ab, pFindSel->m_slat, pFindSel->m_slon);
				GetCanvasPixLL(vp, &cd, pFindSel->m_slat2, pFindSel->m_slon2);*/

				RenderRoundedRect(pdc, 20, 20, 40, 40, 2, bob, 255);
				RenderLine(pdc, gdc, 1426, 942, 1324, 28, bob, 20);


				if (IsSegmentSelected(a, b, c, d, slat, slon)) goto find_ok;
				break;
			}
			default:
				break;
			}
		}

		node = node->GetNext();
	}

	return NULL;
find_ok: return pFindSel;
}

bool Select::IsSelectableSegmentSelected( float slat, float slon, SelectItem *pFindSel )
{
    if(!pFindSel)
        return false;
    
    CalcSelectRadius();

    float a = pFindSel->m_slat;
    float b = pFindSel->m_slat2;
    float c = pFindSel->m_slon;
    float d = pFindSel->m_slon2;

    return IsSegmentSelected( a, b, c, d, slat, slon );
}

SelectableItemList Select::FindSelectionList( float slat, float slon, int fseltype )
{
    float a, b, c, d;
    SelectItem *pFindSel;
    SelectableItemList ret_list;

    CalcSelectRadius();

//    Iterate on the list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

    while( node ) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == fseltype ) {
            switch( fseltype ){
                case SELTYPE_ROUTEPOINT:
                case SELTYPE_TIDEPOINT:
                case SELTYPE_CURRENTPOINT:
                case SELTYPE_AISTARGET:
                    if( ( fabs( slat - pFindSel->m_slat ) < selectRadius )
                            && ( fabs( slon - pFindSel->m_slon ) < selectRadius ) ) {
                        ret_list.Append( pFindSel );
                    }
                    break;
                case SELTYPE_ROUTESEGMENT:
                case SELTYPE_TRACKSEGMENT: {
                    a = pFindSel->m_slat;
                    b = pFindSel->m_slat2;
                    c = pFindSel->m_slon;
                    d = pFindSel->m_slon2;

                    if( IsSegmentSelected( a, b, c, d, slat, slon ) ) ret_list.Append( pFindSel );

                    break;
                }
                default:
                    break;
            }
        }

        node = node->GetNext();
    }

    return ret_list;
}

