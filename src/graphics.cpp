/***************************************************************************
 * $Id: graphics.cpp, v1.0 2010/08/05 Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   David S Register
 *
 * 25 Nov 15 MJW Fixed programmer-forced crash for OpenGL engine on rounded
 *               rectangle, also changed a couple of function names to
 *				 better match what they do.
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

#ifdef ocpnUSE_GL
#include <GL/gl.h>
#include <GL/glu.h>
#endif


#include "graphics.h"

void GLDrawLine( wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2 );

TexFont                 g_TexFontMessage;

#ifndef PI
#define PI 3.14159
#endif

void SetPen(wxDC *dc, void *pgc, wxColour color, int width)
{
    if(dc) {
        wxPen ppbrg ( color, width );
        
        #if wxUSE_GRAPHICS_CONTEXT
        wxGraphicsContext *gc = (wxGraphicsContext *)pgc;
        gc->SetPen(ppbrg);
        #endif
        
        dc->SetPen(ppbrg);
    } else { /* opengl */
        
        #ifdef ocpnUSE_GL          
        glColor4ub(color.Red(), color.Green(), color.Blue(),
                   255/*color.Alpha()*/);
        glLineWidth( width );
        
        #endif          
    }
    
}


void DrawLine(wxDC *dc, void *pgc, int x1, int y1, int x2, int y2)
{
    if(dc) {
        #if wxUSE_GRAPHICS_CONTEXT
        wxGraphicsContext *gc = (wxGraphicsContext *)pgc;
        gc->StrokeLine(x1, y1, x2, y2);
        #else
        dc->DrawLine(x1, y1, x2, y2);
        #endif        
    } else { /* opengl */
        GLDrawLine(x1, y1, x2, y2);
    }
}

void GLDrawLine( wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2 )
{
#ifdef ocpnUSE_GL
    {
        glPushAttrib( GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT ); //Save state

        //      Assume color and line width are preset
        
        {

 //           glDisable( GL_MULTISAMPLE );
            glDisable( GL_LINE_STIPPLE );
            
            //      Enable anti-aliased lines, at best quality

                glEnable( GL_LINE_SMOOTH );
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

                glBegin( GL_LINES );
                glVertex2i( x1, y1 );
                glVertex2i( x2, y2 );
                glEnd();
        }

        glPopAttrib();
    }
#endif    
}

void RenderLine(wxDC *dc, void *pgc, int x1, int y1, int x2, int y2, wxColour color, int width)
{
    SetPen(dc, pgc, color, width);
    
    DrawLine(dc, pgc, x1, y1, x2, y2);
    
}

void RenderText( wxDC *dc, void *pgc, wxString &msg, wxFont *font, wxColour &color, int xp, int yp, double angle)
{
    if(dc)
        dc->SetFont(*font);
    
    if(dc) {
        wxPen ppbrg_text ( color, 1 );
        
        #if wxUSE_GRAPHICS_CONTEXT
        wxGraphicsContext *gc = (wxGraphicsContext *)pgc;
        gc->SetPen(ppbrg_text);
        #endif
        
        dc->SetPen(ppbrg_text);
        dc->SetTextForeground(color);
    } else { /* opengl */
        
        #ifdef ocpnUSE_GL          
        glColor4ub(color.Red(), color.Green(), color.Blue(),
                   255/*color.Alpha()*/);
        #endif          
    }

    if(dc){
        int w, h;
        dc->GetTextExtent( msg, &w, &h );
        w += 2;
        h += 2;
  
        float sin_rot = sin( -angle * PI / 180. );
        float cos_rot = cos( -angle * PI / 180. );
        
        wxPoint points[4], pointsr[4];
        points[0].x = 0;
        points[0].y = 0;

        points[1].x = w;
        points[1].y = 0;
        
        points[2].x = w;
        points[2].y = h;
        
        points[3].x = 0;
        points[3].y = h;
 
        int xt, yt;
        for( int ip = 0; ip < 4; ip++ ) {
            xt = points[ip].x;
            yt = points[ip].y;
            
            float xr = ( xt * cos_rot ) - ( yt * sin_rot );
            float yr = ( xt * sin_rot ) + ( yt * cos_rot );
            pointsr[ip].x = (int)xr;
            pointsr[ip].y = (int)yr;
        }
        
        wxBrush fill_brush( wxTheColourDatabase->Find(_T("GREEN")), 50);
        dc->SetBrush(fill_brush);
        dc->DrawPolygon(4, pointsr, xp, yp);
        
    }
    
    if(dc){
        dc->DrawRotatedText( msg, xp, yp, angle  );
    }
    else{                       // OpenGL
        RenderGLText( &g_TexFontMessage, msg, font, xp, yp, angle);
    }
    
}


/*----------------------------------------------------------------------
 * Render a rounded rectangle at the position provided with the given
*  size, corner radius, colour and transparency.
 *
 * @param pdc [in]			pointer to the graphics display controller -
 *							NULL for OpenGL engine
 * @param x [in]			x position in pixels
 * @param y [in]			y position in pixels
 * @param size_x [in]		width in pixels
 * @param size_y [in]		height in pixels
 * @param radius [in]       corner radius in pixels
 * @param color [in]		fill colour
 * @param transparency [in] transparency, 255 = solid
 *--------------------------------------------------------------------*/
void RenderRoundedRect( wxDC *pdc, int x, int y, int size_x, int size_y,
		float radius, wxColour color, unsigned char transparency )
{
    if( pdc ) {
        //    Get wxImage of area of interest
        wxBitmap obm( size_x, size_y );
        wxMemoryDC mdc1;
        mdc1.SelectObject( obm );
        mdc1.Blit( 0, 0, size_x, size_y, pdc, x, y );
        mdc1.SelectObject( wxNullBitmap );
        wxImage oim = obm.ConvertToImage();
        
        //    Create destination image
        wxBitmap olbm( size_x, size_y );
        wxMemoryDC oldc( olbm );
        if(!oldc.IsOk())
            return;
        
        oldc.SetBackground( *wxBLACK_BRUSH );
        oldc.SetBrush( *wxWHITE_BRUSH );
        oldc.Clear();
        
        if( radius > 0.0 )
            oldc.DrawRoundedRectangle( 0, 0, size_x, size_y, radius );
        
        wxImage dest = olbm.ConvertToImage();
        unsigned char *dest_data = (unsigned char *) malloc(
            size_x * size_y * 3 * sizeof(unsigned char) );
        unsigned char *bg = oim.GetData();
        unsigned char *box = dest.GetData();
        unsigned char *d = dest_data;
        
        //  Sometimes, on Windows, the destination image is corrupt...
        if(NULL == box)
            return;
        
        float alpha = 1.0 - (float)transparency / 255.0;
        int sb = size_x * size_y;
        for( int i = 0; i < sb; i++ ) {
            float a = alpha;
            if( *box == 0 && radius > 0.0 ) a = 1.0;
            int r = ( ( *bg++ ) * a ) + (1.0-a) * color.Red();
            *d++ = r; box++;
            int g = ( ( *bg++ ) * a ) + (1.0-a) * color.Green();
            *d++ = g; box++;
            int b = ( ( *bg++ ) * a ) + (1.0-a) * color.Blue();
            *d++ = b; box++;
        }
        
        dest.SetData( dest_data );
        
        //    Convert destination to bitmap and draw it
        wxBitmap dbm( dest );
        pdc->DrawBitmap( dbm, x, y, false );
        
        // on MSW, the dc Bounding box is not updated on DrawBitmap() method.
        // Do it explicitely here for all platforms.
        pdc->CalcBoundingBox( x, y );
        pdc->CalcBoundingBox( x + size_x, y + size_y );

    } else {
#ifdef ocpnUSE_GL

        /* opengl version */
/* Developer note - only used with small radii at the moment, for larger
 * values may need to mess with anti-aliasing hints or the like	to make
 * it aesthetically pleasing										  */

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glColor4ub( color.Red(), color.Green(), color.Blue(), transparency );
        
        if(radius > 1.0f){
			glBegin(GL_POLYGON);

			// top-left corner
			DrawGLRoundedCorner(x, y + radius, 3 * PI / 2, PI / 2, radius);

			// top-right
			DrawGLRoundedCorner(x + size_x - radius, y, 0.0, PI / 2, radius);

			// bottom-right
			DrawGLRoundedCorner(x + size_x, y + size_y - radius, PI / 2, PI / 2, radius);

			// bottom-left
			DrawGLRoundedCorner(x + radius, y + size_y, PI, PI / 2, radius);

			glEnd();

        } else {
            glBegin( GL_QUADS );
            glVertex2i( x, y );
            glVertex2i( x + size_x, y );
            glVertex2i( x + size_x, y + size_y );
            glVertex2i( x, y + size_y );
            glEnd();
        }
        glDisable( GL_BLEND );

#endif
    }
}


/*----------------------------------------------------------------------
 * Helper method for GL only draws a rounded corner, from the start
 * position and angle given, through the arc at the radius provided in a
 * clockwise direction.  Assumes that glBegin and glEnd are called
 * from the calling context.  Both the start and the end of the 'arc'
 * are included, so a typical convex polygon only needs to make calls
 * to this method and the intervening lines will be joined automatically.
 *
 * @param x [in]			x position in pixels
 * @param y [in]			y position in pixels
 * @param sa [in]			start angle in radians 0 means going East,
 *							PI / 2 is going South!
 * @param arc [in]			length of arc in radians
 * @param r [in]			length of radius in pixels
 *--------------------------------------------------------------------*/
void DrawGLRoundedCorner(int x, int y, double sa, double arc, float r) {
// only valid for OpenGL graphics engine
#ifdef ocpnUSE_GL
	// centre of the arc, for clockwise sense
	float cent_x = x + r * cos(sa + PI / 2);
	float cent_y = y + r * sin(sa + PI / 2);

	// build up piecemeal including end of the arc
	int n = ceil(N_ROUNDING_PIECES * arc / PI * 2);
	for (int i = 0; i <= n; i++) {
		double ang = sa + arc * (double)i  / (double)n;
		
		// compute the next point
		float next_x = cent_x + r * sin(ang);
		float next_y = cent_y - r * cos(ang);
		glVertex2f(next_x, next_y);
	}

#endif
}


/*----------------------------------------------------------------------
 * Render a convex polygon using the points provided along with the
 * colour and transparency given.  A more complex polygon might work
 * with pdc but not with OpenGL using this implementation.
 *
 * @param pdc [in]			pointer to the graphics display controller -
 *							NULL for OpenGL engine
 * @param n [in]			number of vertices
 * @param points [in]		array of coordinates defining the polygon
 * @param color [in]		fill colour
 * @param transparency [in] transparency, 255 = solid
 *--------------------------------------------------------------------*/
void RenderPolygon( wxDC *pdc, int n, wxPoint points[], wxColour color,
	unsigned char transparency )
{
    if( pdc ) {
        wxColour c(color.Red(), color.Green(), color.Blue(), transparency);
        pdc->SetBrush(wxBrush(c));
        pdc->SetPen(wxPen(c));
        
        pdc->DrawPolygon(n, points);

	} else {
#ifdef ocpnUSE_GL

        /* opengl version */
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glColor4ub( color.Red(), color.Green(), color.Blue(), transparency );

		glBegin(GL_POLYGON);
		for (int i = 0; i < n; i++) {
			glVertex2i(points[i].x, points[i].y);
		}
		glEnd();
	
        glDisable( GL_BLEND );

#endif
	}
}




void RenderGLText( TexFont *ptf, wxString &msg, wxFont *font, int xp, int yp, double angle)
{
    if(ptf){
        ptf->Build(*font);
        int w, h;
        ptf->GetTextExtent( msg, &w, &h);
        h += 2;
        
        glColor3ub( 243, 229, 47 );
        
        glPushMatrix();
        glTranslatef(xp, yp, 0.);
        glRotatef(-angle, 0., 0., 1.);
        
        glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(0 + w, 0);
        glVertex2i(0 + w, 0+h);
        glVertex2i(0, 0+h);
        glEnd();
        
        glEnable(GL_BLEND);
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        
        glColor3ub( 0, 0, 0 );
        glEnable(GL_TEXTURE_2D);
        ptf->RenderString( msg, 0, 0);
        glDisable(GL_TEXTURE_2D);
        
        glPopMatrix();
    }
}
