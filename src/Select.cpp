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
#include "vector2d.h"

#include "epl_pi.h"

#include "georef.h"


wxPoint *m_hl_pt_ary;
int n_hl_points;


Select::Select()
{
    pSelectList = new SelectableItemList;
	SetSelectLLRadius(8);
}


Select::~Select()
{
    pSelectList->DeleteContents( true );
    pSelectList->Clear();
    delete pSelectList;

}


/** Determines the ( minimum ) distance from a point po to a line defined by
 * p1 & p2 in cartesian co-ordinates.  Typical use is to find, in pixels, the
 * distance to a line defined in pixel co-ords, which returns the distance in
 * pixels.
 *
 * @param p1x               point 1 x-co-ordinate
 * @param p1y               point 1 y-co-ordinate
 * @param p2x               point 2 x-co-ordinate
 * @param p2y               point 2 y-co-ordinate
 * @param pox               test point x-co-ordinate
 * @param poy               test point y-co-ordinate
 *
 * @return                  distance from the point to the line in the given
 *                          units
 *
 * @see DistanceToLineLL    Lat / Long version
 */
float Select::DistanceToLine(float p1x, float p1y, float p2x, float p2y,
        float pox, float poy) {

    // taken from https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
    // which explains this nicely: the denominator is the distance between P1 &
    // P2 and the numerator is the area of the triangle p1, p2, po
    // that uses the area of a triangle formula = base * height perpendicular to
    // the base

    // but first, check we have a line and not a point!
    if (p1x == p2x && p1y == p2y) {
        // it's a point!  return this distance between this and po
        float dy = poy - p1y;
        float dx = pox - p1x;
        return sqrt(dy * dy + dx * dx);
    }

    // The article above assumes an inifinitely-long line which passes through
    // 2 points.  We therefore need to deal with non-inifinite lines - look for
    // the angle p1,p2,po or p2,p1,po being > 90.
    float ldy = p2y - p1y;
    float ldx = p2x - p1x;
    float ldyo1 = poy - p1y;
    float ldxo1 = pox - p1x;
    float ldyo2 = poy - p2y;
    float ldxo2 = pox - p2x;

    // b is base line, angA is angle p2,p1,po, angC is p1,p2,po: cosine rule
    float a = sqrt(ldyo2 * ldyo2 + ldxo2 * ldxo2);
    float b = sqrt(ldy * ldy + ldx * ldx);
    float c = sqrt(ldyo1 * ldyo1 + ldxo1 * ldxo1);
    float angA = acos((b * b + c * c - a * a) / (2 * b * c));
    if (angA > PI / 2.0) {
        // nearer to p1 than the rest of the line
        return c;
    }

    float angC = acos((a * a + b * b - c * c) / (2 * a * b));
    if (angC > PI / 2.0) {
        // nearer to p2 than the rest of the line
        return a;
    }

    return abs(ldy * pox - ldx * poy + p2x * p1y - p2y * p1x) / b;
}


/** Determines the ( minimum ) distance from a point po to a line defined by
 * p1 & p2 in lat & long co-ordinates.  Typical use is to find the distance to
 * a line defined in lat & long co-ords in NM.  This method assumes the line is
 * short enough to ignore any great-circle computation or other spherical-based
 * factors.
 *
 * @param p1Lat             point 1 latitude
 * @param p1Lon             point 1 longitude
 * @param p2Lat             point 2 latitude
 * @param p2Lon             point 2 longitude
 * @param poLat             test point latitude
 * @param poLat             test point longitude
 *
 * @return                  distance from the point to the line in the NM
 *
 * @see DistanceToLine      x / y version
 */
float Select::DistanceToLineLL(float p1Lat, float p1Lon, float p2Lat, float p2Lon,
        float poLat, float poLon) {

    // this uses DistanceToLine to compute the distance in NM by first
    // converting lat & long to cartesian co-ordinates, in NM, with the
    // origin at p1
    float xFactor = 60 * cos(p1Lat * PI / 180.0);  // Simon Cowell?
    float yFactor = 60;                // by definitiion

    float p2x = (p2Lon - p1Lon) * xFactor;
    float p2y = (p2Lat - p1Lat) * yFactor;
    float pox = (poLon - p1Lon) * xFactor;
    float poy = (poLat - p1Lat) * yFactor;

    return DistanceToLine(0, 0, p2x, p2y, pox, poy);
}


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


// PPW remove for the moment and using older IsSegmentSelected so we can use the DistanceBearingMercator function
/** Returns true if the given pointer position, po, is within a pre-defined
 * distance ( set by selectRadius ) of the segment defined by the line p1,p2.
 *
 * @param p1Lat             end 1 latitude
 * @param p1Lon             end 1 longitude
 * @param p2Lat             end 2 latitude
 * @param p2Lon             end 2 longitude
 * @param poLat             pointer latitude
 * @param poLat             pointer longitude
 *
 * @returns                 true if close
 */
//bool Select::IsSegmentSelected(float p1Lat, float p1Lon,
//            float p2Lat, float p2Lon,
//            float poLat, float poLon) {
//
//	//if (selectRadius_NM < 0.00001){
//	//	wxMessageBox("We have a select radius problem");
//	//}
//
//    // the return from DistanceToLineLL is in NM
//	return DistanceToLineLL(p1Lat, p1Lon, p2Lat, p2Lon, poLat, poLon)
//		< selectRadius_NM;
//}

bool Select::IsSegmentSelected( float a, float b, float c, float d, float slat, float slon )
{
    double adder = 0.;

    if( ( c * d ) < 0. ) {
        //    Arrange for points to be increasing longitude, c to d
        double dist, brg;
        DistanceBearingMercator( a, c, b, d, &brg, &dist );
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
	if ((slat >= (fmin(a, b) - selectRadius_NM)) && (slat <= (fmax(a, b) + selectRadius_NM))
		&& ((slon + adder) >= (fmin(c, d) - selectRadius_NM))
		&& ((slon + adder) <= (fmax(c, d) + selectRadius_NM))) {
        //    Use vectors to do hit test....
        vector2D va, vb, vn;

        //    Assuming a Mercator projection
        double ap, cp;
        toSM( a, c, 0., 0., &cp, &ap );
        double bp, dp;
        toSM( b, d, 0., 0., &dp, &bp );
        double slatp, slonp;
        toSM( slat, slon + adder, 0., 0., &slonp, &slatp );

        va.x = slonp - cp;
        va.y = slatp - ap;
        vb.x = dp - cp;
        vb.y = bp - ap;

        double delta = vGetLengthOfNormal( &va, &vb, &vn );
		if (fabs(delta) < (selectRadius_NM * 1852 * 60)) return true;
    }
    return false;
}


SelectItem *Select::FindSelection(float slat, float slon, int fseltype)
{
    float a, b, c, d;
    SelectItem *pFindSel, *returnItem;
	returnItem = NULL;


//    Iterate on the list
    wxSelectableItemListNode *node = pSelectList->GetFirst();

	bool exitLoop = true;
	if (node){
		exitLoop = false;
	}

	while (!exitLoop) {
        pFindSel = node->GetData();
        if( pFindSel->m_seltype == fseltype ) {
            switch( fseltype ){
				case SELTYPE_POINT_GENERIC:
				{
					// PPW changed back to previous layout for variables due to using mercator from original IsSegmentSelected
					a = fabs(slat - pFindSel->m_slat);
					b = fabs(slon - pFindSel->m_slon);

					if ((fabs(slat - pFindSel->m_slat) < selectRadius_NM)
						&& (fabs(slon - pFindSel->m_slon) < selectRadius_NM)) {
						returnItem = pFindSel;
						//returnItem = NULL; //PPW test for dead lines
						exitLoop = true;
					}
					break;
				}
                case SELTYPE_SEG_GENERIC: 
				{
					// PPW changed back to previous layout for variables due to using mercator from original IsSegmentSelected
                    a = pFindSel->m_slat;
                    c = pFindSel->m_slon;
                    b = pFindSel->m_slat2;
                    d = pFindSel->m_slon2;

					if (IsSegmentSelected(a, b, c, d, slat, slon)) 
					{
						returnItem = pFindSel;
						exitLoop = true;
					}
                    break;
                }
                default:
                    break;
            }
        }


        node = node->GetNext();
		if (!node){
			exitLoop = true;
		}
    }

	return returnItem;
}


bool Select::IsSelectableSegmentSelected( float slat, float slon, SelectItem *pFindSel )
{
    if(!pFindSel)
        return false;
    
    float a = pFindSel->m_slat;
    float b = pFindSel->m_slon;
    float c = pFindSel->m_slat2;
    float d = pFindSel->m_slon2;

    return IsSegmentSelected( a, b, c, d, slat, slon );
}

