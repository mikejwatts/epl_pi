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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 *
 *   S Blackburn's original source license:                                *
 *         "You can use it any way you like."                              *
 *   More recent (2010) license statement:                                 *
 *         "It is BSD license, do with it what you will"                   *
 */


#include "nmea0183.h"
#pragma hdrstop

/*
** Author: Samuel R. Blackburn
** CI$: 76300,326
** Internet: sammy@sed.csc.com
**
** You can use it any way you like.
*/

IMPLEMENT_DYNAMIC( DPT, RESPONSE )

DPT::DPT()
{
   Mnemonic = "DPT";
   Empty();
}

DPT::~DPT()
{
   Mnemonic.Empty();
   Empty();
}

void DPT::Empty( void )
{
   ASSERT_VALID( this );

   DepthMeters                = 0.0;
   OffsetFromTransducerMeters = 0.0;
}

BOOL DPT::Parse( const SENTENCE& sentence )
{
   ASSERT_VALID( this );

   /*
   ** DPT - Heading - Deviation & Variation
   **
   **        1   2   3
   **        |   |   |
   ** $--DPT,x.x,x.x*hh<CR><LF>
   **
   ** Field Number: 
   **  1) Depth, meters
   **  2) Offset from transducer, 
   **     positive means distance from tansducer to water line
   **     negative means distance from transducer to keel
   **  3) Checksum
   */

   /*
   ** First we check the checksum...
   */

   if ( sentence.IsChecksumBad( 3 ) == True )
   {
      SetErrorMessage( "Invalid Checksum" );
      return( FALSE );
   } 

   DepthMeters                = sentence.Double( 1 );
   OffsetFromTransducerMeters = sentence.Double( 2 );

   return( TRUE );
}

BOOL DPT::Write( SENTENCE& sentence )
{
   ASSERT_VALID( this );

   /*
   ** Let the parent do its thing
   */
   
   RESPONSE::Write( sentence );

   sentence += DepthMeters;
   sentence += OffsetFromTransducerMeters;

   sentence.Finish();

   return( TRUE );
}

const DPT& DPT::operator = ( const DPT& source )
{
   ASSERT_VALID( this );

   DepthMeters                = source.DepthMeters;
   OffsetFromTransducerMeters = source.OffsetFromTransducerMeters;

   return( *this );
}
