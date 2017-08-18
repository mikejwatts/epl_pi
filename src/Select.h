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

    //void SetSelectPixelRadius(int radius){ pixelRadius = radius; }
    void SetSelectLLRadius(float radius){ selectRadius_NM = radius; }
    
	SelectItem *FindSelection(float slat, float slon, int fseltype, PlugIn_ViewPort *vp = NULL);
	SelectableItemList FindSelectionList( float slat, float slon, int fseltype );

    bool IsSegmentSelected(float p1Lat, float p1Lon,
            float p2Lat, float p2Lon,
            float poLat, float poLon);
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

    // distance of point po to line p1,p2
    static float DistanceToLine(float p1x, float p1y,
            float p2x, float p2y,
            float pox, float poy);
    // ditto, in Lat & Long co-ords
    static float DistanceToLineLL(float p1Lat, float p1Lon,
            float p2Lat, float p2Lon,
            float poLat, float poLon);
    
    //  Accessors
    SelectableItemList *GetSelectList()
    {
        return pSelectList;
    }

private:
    SelectableItemList *pSelectList;
    //int pixelRadius;
    float selectRadius_NM;          // defines select distance in NM
};

#endif
