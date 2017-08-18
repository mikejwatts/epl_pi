/***************************************************************************
*
* Project:  OpenCPN
* Purpose:  NMEA0183 Support Classes
* Author:   Samuel R. Blackburn, David S. Register
*
***************************************************************************
*   Copyright (C) 2010 by Samuel R. Blackburn, David S Register           *
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
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
***************************************************************************
*
* A01.0 13 Mar 17 M.J.Watts Added 2 extra mode options for an incoming message
*       and one for an outgoing / command.
*
*   S Blackburn's original source license:                                *
*         "You can use it any way you like."                              *
*   More recent (2010) license statement:                                 *
*         "It is BSD license, do with it what you will"                   *
*/


#if ! defined( EBL_CLASS_HEADER )
#define EBL_CLASS_HEADER

/*
** Author: Samuel R. Blackburn
** CI$: 76300,326
** Internet: sammy@sed.csc.com
**
** You can use it any way you like.
*/

#define  EPLBrgTrue      0
#define  EPLBrgMag       1
#define  EPLBrgUnknown   -1

#define EPLModeNormal       0
#define EPLModeLive         1
#define EPLModeImHere       2
#define EPLModeAck          3
#define EPLModeQuery        4       // outgoing only
#define EPLModeUnknown     -1

class EBL : public RESPONSE
{

public:

	EBL();
	~EBL();

	/*
	** Data
	*/

	double    BrgAbs;
	int       BrgAbsTM;
	double    BrgRelative;
	double    BrgSubtended;
	wxString  BrgIdent;
	wxString  BrgTargetName;
	int       BrgMode;

	wxString          UTCTime;

	/*
	** Methods
	*/

	virtual void Empty(void);
	virtual bool Parse(const SENTENCE& sentence);
	virtual bool Write(SENTENCE& sentence);

	/*
	** Operators
	*/

	virtual const EBL& operator = (const EBL& source);
};

#endif // EBL_CLASS_HEADER
