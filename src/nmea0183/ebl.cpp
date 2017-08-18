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


#include "nmea0183.h"
//#pragma hdrstop

/*
** Author: Samuel R. Blackburn
** CI$: 76300,326
** Internet: sammy@sed.csc.com
**
** You can use it any way you like.
*/

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif


EBL::EBL()
{
	Mnemonic = _T("EBL");
	Empty();
}

EBL::~EBL()
{
	Mnemonic.Empty();
	Empty();
}

void EBL::Empty(void)
{
	BrgAbs = NAN;
	BrgAbsTM = EPLBrgUnknown;
	BrgRelative = NAN;
	BrgSubtended = NAN;
	BrgIdent.Empty();
	BrgTargetName.Empty();
	BrgMode = EPLModeUnknown;

	UTCTime.Empty();
}

bool EBL::Parse(const SENTENCE& sentence) {

	/*
	$--EBL,hhmmss.ss,xxx.x,c,xxx.x,x.xxxx,c--c,c--c,c*hh<CR><LF>
	|        |   |   |      |     |    |  |
	|        |   |   |      |     |    |  |__ (6) mode, may be null
	|        |   |   |      |     |    |
	|        |   |   |      |     |    |__ (5) target, may be null
	|        |   |   |      |     |
	|        |   |   |      |     |__ (4) bearing ID
	|        |   |   |      |
	|        |   |   |      |__ (3) angle subtended, may be null
	|        |   |   |
	|        |   |   |__ (2) bearing relative to the ship
	|        |   |
	|        |___|__ (1,2) absolute bearing in degrees, T or M
	|
	|__ UTC of sighting, may be null


	(1) absolute bearing using the compass - true (T) or magnetic (M)

	(2) bearings are in degrees.  Either absolute or relative bearing can be
	omitted but at least one must be present.

	(3) angle subtended - possible future use to determine distance-off

	(4) bearing ID - max 4 chars, normally a number but any NMEA-valid
	characters are allowed

	(5) target name - max 30 NMEA-valid chars

	(6) mode - empty or 'N' for normal mode, 'L' for live mode i.e. show the
	bearing for a short period of time only, 'H' for an 'I'm here' message
	sent every so often ( once per second ) and typically has no
	information, 'A' for an acknowledgement to a 'Q' query message, again
	neither of these is expected to contain useful information

	*/

	// test the checksum - API note: although there are only 8 fields, for the
	// IsChecksumBad call * is taken as a delimiter so the CS is field 9
	int target_field_count = 8;
	if (sentence.IsChecksumBad(9) == NMEA0183_BOOLEAN::NTrue) {
		SetErrorMessage(_T("Invalid Checksum"));
		return(FALSE);
	}

	if (sentence.GetNumberOfDataFields() == target_field_count)
	{
		UTCTime = sentence.Field(1);

		BrgAbs = sentence.Double(2);
		wxString BATM = sentence.Field(3);
		if (BATM == _T("T"))
			BrgAbsTM = EPLBrgTrue;
		else
			BrgAbsTM = EPLBrgMag;

		BrgRelative = sentence.Double(4);
		BrgSubtended = sentence.Double(5);
		BrgIdent = sentence.Field(6);
		BrgTargetName = sentence.Field(7);
		wxString BAM = sentence.Field(8);
		if (BAM == _T("L")) {
			BrgMode = EPLModeLive;

		}
		else if (BAM == _T("H")) {
			BrgMode = EPLModeImHere;

		}
		else if (BAM == _T("A")) {
			BrgMode = EPLModeAck;

		}
		else {
			// should be empty or N
			BrgMode = EPLModeNormal;
		}

		return(TRUE);
	}


	//    A real error...
	SetErrorMessage(_T("Invalid FieldCount"));
	return(FALSE);
}




bool EBL::Write(SENTENCE& sentence)
{
	//   ASSERT_VALID( this );

	/*
	** Let the parent do its thing
	*/

	RESPONSE::Write(sentence);


	sentence.Finish();

	return(TRUE);
}

const EBL& EBL::operator = (const EBL& source)
{
	//   ASSERT_VALID( this );


	return(*this);
}
