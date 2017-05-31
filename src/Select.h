/***************************************************************************
 *
 * Project:  OpenCPN
 *
 ***************************************************************************
 *   Copyright (C) 2013 by David S. Register                               *
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
 **************************************************************************/

#ifndef __SELECT_H__
#define __SELECT_H__

#include "SelectItem.h"
#include "ocpn_plugin.h"

// PPW
#include "graphics.h" 

#define SELTYPE_UNKNOWN              0x0001
#define SELTYPE_ROUTEPOINT           0x0002
#define SELTYPE_ROUTESEGMENT         0x0004
#define SELTYPE_TIDEPOINT            0x0008
#define SELTYPE_CURRENTPOINT         0x0010
#define SELTYPE_ROUTECREATE          0x0020
#define SELTYPE_AISTARGET            0x0040
#define SELTYPE_MARKPOINT            0x0080
#define SELTYPE_TRACKSEGMENT         0x0100

#define SELTYPE_POINT_GENERIC        0x0200
#define SELTYPE_SEG_GENERIC          0x0400




class Select
{
public:
    Select();
    ~Select();

    void SetSelectPixelRadius(int radius){ pixelRadius = radius; }
    void SetSelectLLRadius(double radius){ selectRadius = radius; }
    
//    bool AddSelectableRoutePoint( float slat, float slon, RoutePoint *pRoutePointAdd );
//    bool AddSelectableRouteSegment( float slat1, float slon1, float slat2, float slon2,
//            RoutePoint *pRoutePointAdd1, RoutePoint *pRoutePointAdd2, Route *pRoute );

//    bool AddSelectableTrackSegment( float slat1, float slon1, float slat2, float slon2,
//            RoutePoint *pRoutePointAdd1, RoutePoint *pRoutePointAdd2, Route *pRoute );

	SelectItem *FindSelection(float slat, float slon, int fseltype, PlugIn_ViewPort *vp = NULL);
	SelectableItemList FindSelectionList( float slat, float slon, int fseltype );

//    bool DeleteAllSelectableRouteSegments( Route * );
//    bool DeleteAllSelectableTrackSegments( Route * );
//    bool DeleteAllSelectableRoutePoints( Route * );
//    bool AddAllSelectableRouteSegments( Route *pr );
//    bool AddAllSelectableTrackSegments( Route *pr );
//    bool AddAllSelectableRoutePoints( Route *pr );
//    bool UpdateSelectableRouteSegments( RoutePoint *prp );
//    bool DeletePointSelectableTrackSegments( RoutePoint *pr );
    bool IsSegmentSelected( float a, float b, float c, float d, float slat, float slon );
    bool IsSelectableSegmentSelected( float slat, float slon, SelectItem *pFindSel );

    //    Generic Point/Segment Support
    SelectItem *AddSelectablePoint(float slat, float slon, const void *data, int fseltype, int UserData);
    bool AddSelectableSegment( float slat1, float slon1, float slat2, float slon2,
                                       const void *pdata, int UserData );
    
    
    
    bool DeleteAllPoints( void );
    bool DeleteSelectablePoint( void *data, int SeltypeToDelete, int UserData );
    bool ModifySelectablePoint( float slat, float slon, void *data, int fseltype );

    bool DeleteSelectableSegment( void *data, int SeltypeToDelete, int UserData );
    
    //    Delete all selectable points in list by type
    bool DeleteAllSelectableTypePoints( int SeltypeToDelete );

//    bool DeleteSelectableRoutePoint( RoutePoint *prp );
    
    //  Accessors

    SelectableItemList *GetSelectList()
    {
        return pSelectList;
    }

private:
    void CalcSelectRadius();

    SelectableItemList *pSelectList;
    int pixelRadius;
    float selectRadius;
};

#endif
