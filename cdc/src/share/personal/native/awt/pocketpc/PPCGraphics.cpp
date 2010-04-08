/*
 * @(#)PPCGraphics.cpp	1.30 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 *
 */
#include "awt.h"
#include "PPCComponentPeer.h"
#include "PPCWindowPeer.h"
#include "PPCUnicode.h"
#include "PPCGraphics.h"

#include <java_awt_Color.h>
#include <java_awt_Dimension.h>
#include <java_awt_Rectangle.h>
//#include "sun_awt_pocketpc_PPCPrintJob.h"
#include "sun_awt_pocketpc_PPCComponentPeer.h"
#include "sun_awt_pocketpc_PPCGraphics.h"
#include <commdlg.h>

#define OX      (env->GetIntField(m_pJavaObject, WCachedIDs.PPCGraphics_originXFID))
#define OY      (env->GetIntField(m_pJavaObject, WCachedIDs.PPCGraphics_originYFID))
#define TX(x)       ((x) + OX)
#define TY(y)       ((y) + OY)

#define POLYTEMPSIZE	(512 / sizeof(POINT))

// Hardcode the logical printing resolution at 72dpi. We will set up a
// mapping between this value and the actual printer dpi.
#define PRINT_RESOLUTION 72

# include "PPCFramePeer.h"           // we reference frames for inset calculation
# include <math.h>                // for cos(), sin(), etc
# include <limits.h>              // for LONG_MIN, LONG_MAX, etc

AwtGraphics::AwtGraphics()
{
    m_hDC = NULL;
    m_pSharedDC = NULL;
    m_bPenSet = FALSE;
    m_hPen = NULL;
    m_color = RGB(0, 0, 0);
    m_xorcolor = RGB(255, 255, 255);
    m_bBrushSet = FALSE;
    m_hBrush = NULL;
    m_pFont = NULL;
    m_bPaletteSelected = FALSE;
    ::SetRectEmpty(&m_rectClip);
    m_bClipping = FALSE;
    m_rop = R2_COPYPEN;
    // if rect has no area, rgn will be null, but that's OK
    m_hClipRgn = ::CreateRectRgnIndirect(&m_rectClip);
    m_pJavaObject = NULL;
    m_isPrinter = FALSE;
}

AwtGraphics::~AwtGraphics()
{
    AwtSharedDC* sdc = (AwtSharedDC*)::InterlockedExchange((LPLONG)&m_pSharedDC, NULL);
    if (sdc) {
		sdc->Lock();
        sdc->Detach(this);
		sdc->Unlock();
	}

    AwtPen* pen = (AwtPen*)::InterlockedExchange((LPLONG)&m_hPen, NULL);
    if (pen)
        pen->Release();

    AwtBrush* brush = (AwtBrush*)::InterlockedExchange((LPLONG)&m_hBrush, NULL);
    if (brush)
        brush->Release();

    HRGN rgn = (HRGN)::InterlockedExchange((LPLONG)&m_hClipRgn, NULL);
    if (rgn)
        VERIFY(::DeleteObject(rgn));
}

AwtGraphics *AwtGraphics::Create(JNIEnv *env, 
				 jobject pJavaObject, 
				 AwtComponent* pWindow)
{
#ifdef DEBUG
// Enable this call to track down GDIFlush() failures on NT 3.51.
//    VERIFY(::GdiSetBatchLimit(1));
#endif

    AwtGraphics *pGraphics = new AwtGraphics();

    pGraphics->m_pJavaObject = env->NewGlobalRef(pJavaObject);
    AwtSharedDC* sharedDC = pWindow->GetSharedDC();
    if (sharedDC != NULL && sharedDC->Attach())
        pGraphics->m_pSharedDC = sharedDC;
    pGraphics->SetColor(pWindow->GetBackgroundColor());
    
    AwtWindow* cont = pWindow->GetContainer();
    if (cont != NULL && cont == pWindow) {
#ifndef WINCE
        cont->SubtractInsetPoint(env->GetIntField(pJavaObject, WCachedIDs.PPCGraphics_originXFID),
                                 env->GetIntField(pJavaObject, WCachedIDs.PPCGraphics_originYFID));
#else
	int originX = (int) env->GetIntField(pJavaObject,
					     WCachedIDs.PPCGraphics_originXFID);
	int originY = (int) env->GetIntField(pJavaObject,
					     WCachedIDs.PPCGraphics_originYFID);

	cont->TranslateToClient(originX, originY);

	env->SetIntField(pJavaObject,
			 WCachedIDs.PPCGraphics_originXFID, (jint) originX);
	env->SetIntField(pJavaObject,
			 WCachedIDs.PPCGraphics_originYFID, (jint) originY);
#endif /* WINCE */
    }

    return pGraphics;
}

AwtGraphics *AwtGraphics::CreateCopy(JNIEnv *env,
				     jobject pJavaObject, 
				     AwtGraphics *pSource)
{
    AwtGraphics *pGraphics;
    AwtComponent* pWnd = pSource->GetComponent();

    if (pWnd != NULL) {
        pGraphics = AwtGraphics::Create(env, pJavaObject, pWnd);
        if (pGraphics == NULL) {
            return NULL;
        }
    } else {
        pGraphics = new AwtGraphics();

        pGraphics->m_pJavaObject = env->NewGlobalRef(pJavaObject);
        AwtSharedDC* sharedDC = pSource->m_pSharedDC;
        if (sharedDC != NULL && sharedDC->Attach())
            pGraphics->m_pSharedDC = sharedDC;
    }

    if (pSource->m_bClipping) {
        int x = pSource->m_rectClip.left;
        int y = pSource->m_rectClip.top;
        int w = pSource->m_rectClip.right - x;
        int h = pSource->m_rectClip.bottom - y;
        pGraphics->ChangeClip(env, x, y, w, h, TRUE);
    }
    pGraphics->SetRop(pSource->m_rop, pSource->m_color, RGB(0, 0, 0));
    pGraphics->SetColor(pSource->m_color);
    pGraphics->SetFont(pSource->m_pFont);
    pGraphics->m_isPrinter = pSource->m_isPrinter;
    return pGraphics;
}

AwtGraphics *AwtGraphics::Create(JNIEnv *env,
				 jobject pJavaObject, 
				 AwtImage *pImage)
{
    AwtGraphics *pGraphics = new AwtGraphics();

    pGraphics->m_pJavaObject = env->NewGlobalRef(pJavaObject);
    AwtSharedDC* sharedDC = pImage->m_pSharedDC;
    if (sharedDC != NULL && sharedDC->Attach())
      pGraphics->m_pSharedDC = sharedDC;

    return pGraphics;
}


AwtGraphics *AwtGraphics::Create(JNIEnv *env,
				 jobject pJavaObject, 
				 HDC hdc)
{
    AwtGraphics *pGraphics = new AwtGraphics();

    pGraphics->m_pSharedDC = new AwtSharedDC(NULL, hdc);
    pGraphics->m_pSharedDC->Attach();  // one extra attach to prevent us from ever deleting hdc

    // Initialize pen and brush to match AwtGraphics settings.
    ::SelectObject(hdc, ::GetStockObject(NULL_PEN));
    ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));

    return pGraphics;
}

void AwtGraphics::Dispose(JNIEnv *env) {
    if (m_pJavaObject != NULL) {
        env->SetIntField(m_pJavaObject, WCachedIDs.PPCGraphics_pDataFID, 0);
	env->DeleteGlobalRef(m_pJavaObject);
    }
    delete this;
}

void AwtGraphics::ValidateDC() {
    m_hDC = m_pSharedDC->GrabDC(this);
    if (m_hDC == NULL) {
        return;
    }
    if (::SetTextColor(m_hDC, m_color) == CLR_INVALID) {
        m_hDC = m_pSharedDC->ResetDC();
        VERIFY(::SetTextColor(m_hDC, m_color) != CLR_INVALID);
    }
    if (m_pFont != NULL) {
        VERIFY(::SelectObject(m_hDC, m_pFont->GetHFont()) != NULL);
    }
    VERIFY(::SetROP2(m_hDC, m_rop));
#ifndef WINCE
    VERIFY(::SetStretchBltMode(m_hDC, COLORONCOLOR));
#endif
    /* FIX
    if (AwtImage::m_hPal != NULL) {
	VERIFY(::SelectPalette(m_hDC, AwtImage::m_hPal, FALSE));
	::RealizePalette(m_hDC);
	m_bPaletteSelected = TRUE;
	} else { */
	m_bPaletteSelected = FALSE;
	/* FIX
    }
	*/
    if (m_bClipping) {
        VERIFY(LogicalSelectClipRgn(m_hDC, m_hClipRgn) != ERROR);
    }
}

void AwtGraphics::InvalidateDC() {
#ifndef WINCE
    ::GdiFlush();
#endif
    SetPenColor(FALSE);
    SetBrushColor(FALSE);
    ::SelectObject(m_hDC, ::GetStockObject(SYSTEM_FONT));
    if (m_bPaletteSelected) {
        ::SelectObject(m_hDC, ::GetStockObject(DEFAULT_PALETTE));
        m_bPaletteSelected = FALSE;
    }
    if (m_bClipping) {
        LogicalSelectClipRgn(m_hDC, NULL);
    }
    m_hDC = NULL;
}

static COLORREF XorResult(COLORREF fgcolor, COLORREF xorcolor) {
    COLORREF c;
    if (AwtImage::GetBitsPerPixel() > 8) {
        c = (fgcolor & 0xff000000) | ((fgcolor ^ xorcolor) & 0x00ffffff);
    } else {
#ifndef WINCE
        c = PALETTEINDEX(GetNearestPaletteIndex(AwtImage::m_hPal, fgcolor) ^
                         GetNearestPaletteIndex(AwtImage::m_hPal, xorcolor));
#else /\* WINCE *\/
/* FIX: - m_hPal is not used at all currently.
	PALETTEENTRY xorPalEntry[1]; // Xor palette entry, array of one

	WORD paletteIndex = GetNearestPaletteIndex(AwtImage::m_hPal, fgcolor) ^
	                    GetNearestPaletteIndex(AwtImage::m_hPal, xorcolor);
	GetPaletteEntries(AwtImage::m_hPal, paletteIndex, 1, xorPalEntry);
	c = PALETTERGB(xorPalEntry[0].peRed, xorPalEntry[0].peGreen, xorPalEntry[0].peBlue);
*/
#endif /\* WINCE *\/
    }
    return c;
}

void AwtGraphics::SetColor(COLORREF color)
{
    if (m_rop == R2_XORPEN) {
        ChangeColor(XorResult(color, m_xorcolor));
    } else {
        ChangeColor(color);
    }
}

void AwtGraphics::ChangeColor(COLORREF color)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    if (m_color != color) {
        m_color = color;
        VERIFY(::SetTextColor(hDC, color) != CLR_INVALID);
        if (m_hPen != NULL) {
            SetPenColor(FALSE);
            m_hPen->Release();
            m_hPen = NULL;
        }
        if (m_hBrush != NULL) {
            SetBrushColor(FALSE);
            m_hBrush->Release();
            m_hBrush = NULL;
        }
    }
    ReleaseDC();
}

void AwtGraphics::SetFont(AwtFont *pFont)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    if (m_pFont != pFont) {
        VERIFY(::SelectObject(hDC, pFont->GetHFont()) != NULL);
        m_pFont = pFont;
    }
    ReleaseDC();
}

void AwtGraphics::SetRop(int rop, COLORREF fgcolor, COLORREF color)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    if (m_rop != rop) {
        VERIFY(::SetROP2(hDC, rop));
        m_rop = rop;
    }
    if (m_rop == R2_XORPEN) {
        ChangeColor(XorResult(fgcolor, color));
        m_xorcolor = color;
    } else {
        ChangeColor(fgcolor);
    }
    ReleaseDC();
}

void AwtGraphics::DrawLine(JNIEnv *env, jint nX1, jint nY1, jint nX2, jint nY2)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    // Bug #4055623 (Performance enhancements)
    if (nX1 == nX2) { 
        // vertical line; use PatBlt to draw it
        SetBrushColor(TRUE);
        long y, h;
        if (nY1 <= nY2) {
            y = nY1; 
            h = nY2 - nY1 + 1;
        } else {
            y = nY2;
            h = nY1 - nY2 + 1;
        }
        if (h == 0) {
            h = 1;
        }
        VERIFY(::PatBlt(hDC, TX(nX1), TY(y), 1, h, 
                        ((m_rop == R2_XORPEN) ? PATINVERT : PATCOPY)));
        ReleaseDC();
        return;
    }
    if (nY1 == nY2) { 
        // horizontal line; use PatBlt to draw it
        SetBrushColor(TRUE);
        long x, w;
        if (nX1 <= nX2) {
            x = nX1; 
            w = nX2 - nX1 + 1;
        } else {
            x = nX2;
            w = nX1 - nX2 + 1;
        }
        if (w == 0) {
            w = 1;
        }
        VERIFY(::PatBlt(hDC, TX(x), TY(nY1), w, 1, 
                        ((m_rop == R2_XORPEN) ? PATINVERT : PATCOPY)));
        ReleaseDC();
        return;
    }
    // you come here when the line is not horizontal or vertical
    SetPenColor(TRUE);
#ifndef WINCE
    VERIFY(::MoveToEx(hDC, TX(nX1), TY(nY1), NULL));
    VERIFY(::LineTo(hDC, TX(nX2), TY(nY2)));
    // Paint the end-point.
    SetBrushColor(TRUE);
    VERIFY(::PatBlt(hDC, TX(nX2), TY(nY2), 1, 1, 
                    ((m_rop == R2_XORPEN) ? PATINVERT : PATCOPY)));
#else
    {
	POINT   apt[2];
	apt[0].x = TX(nX1);
	apt[0].y = TY(nY1);
	apt[1].x = TX(nX2) + 1;
	apt[1].y = TY(nY2) + 1;
	::Polyline(hDC, apt, 2);   
    }
#endif
    ReleaseDC();
}

void AwtGraphics::ClearRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH)
{
    if (nW <= 0 || nH <= 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    HBRUSH hBrush;
    AwtComponent* pComponent = GetComponent();
    if (pComponent != NULL) {
        hBrush = pComponent->GetBackgroundBrush();
    } else {
	jobject bgh;
	jobject image = env->GetObjectField(m_pJavaObject,
					    WCachedIDs.PPCGraphics_imageFID);

	if (image == NULL) {
	    // no image - for now force bg to white
	    bgh = 0;
	} else {
	    bgh = env->CallObjectMethod(image,
					WCachedIDs.sun_awt_image_Image_getBackgroundMID);
	    if (env->ExceptionCheck()) {
                ReleaseDC();
		return;
	    }
	}
	COLORREF color;
	if (bgh == 0) {
	    color = PALETTERGB(0xff, 0xff, 0xff);
	} else {
	    jint value = env->CallIntMethod(bgh,
					    WCachedIDs.java_awt_Color_getRGBMID);
	    color = PALETTERGB((value>>16)&0xff,
			       (value>>8)&0xff,
			       (value)&0xff);
	}
        hBrush = (HBRUSH)AwtBrush::Get(color)->GetHandle();
    }
    RECT rect;
    ::SetRect(&rect, TX(nX), TY(nY), TX(nX+nW), TY(nY+nH));
    ::FillRect(hDC, &rect, hBrush);
    ReleaseDC();
}

void AwtGraphics::FillRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH)
{       
    if (nW <= 0 || nH <= 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    SetBrushColor(TRUE);
    // Bug #4047723 (FillRect apears wrong (was using the wrong DC handle))
    // Bug #4055623 (Performance enhancements)
    VERIFY(::PatBlt(hDC, TX(nX), TY(nY), nW, nH, 
                    ((m_rop == R2_XORPEN) ? PATINVERT : PATCOPY)));  
    ReleaseDC();
}

void AwtGraphics::RoundRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, jint nArcW, jint nArcH)
{       
    if (nW < 0 || nH < 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    SetPenColor(TRUE);
    SetBrushColor(FALSE);
    ::RoundRect(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1), nArcW, nArcH);
    ReleaseDC();
}

void AwtGraphics::FillRoundRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, jint nArcW, jint nArcH)
{       
    if (nW <= 0 || nH <= 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    SetPenColor(FALSE);
    SetBrushColor(TRUE);
    ::RoundRect(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1), nArcW, nArcH);
    ReleaseDC();
}

void AwtGraphics::Ellipse(JNIEnv *env, jint nX, jint nY, jint nW, jint nH)
{       
    if (nW < 0 || nH < 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    SetPenColor(TRUE);
    SetBrushColor(FALSE);
    VERIFY(::Ellipse(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));
    ReleaseDC();
}

void AwtGraphics::FillEllipse(JNIEnv *env, jint nX, jint nY, jint nW, jint nH)
{       
    if (nW <= 0 || nH <= 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    SetPenColor(FALSE);
    SetBrushColor(TRUE);
    VERIFY(::Ellipse(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1)));
    ReleaseDC();
}

void AngleToCoord(jint nAngle, jint nW, jint nH, long *nX, long *nY)
{
    const double pi = 3.1415926535;
    const double toRadians = 2 * pi / 360;

    *nX = (long)(cos((double)nAngle * toRadians) * nW);
    *nY = -(long)(sin((double)nAngle * toRadians) * nH);
}

#ifdef WINCE

HRGN AwtGraphics::AddRectToRegion(HRGN hRgn, RECT *rect)
{
	HRGN newRgn = CreateRectRgn(rect->left, rect->top, rect->right, rect->bottom);

	if (!hRgn) {
		return (newRgn);
	} else {
		CombineRgn(hRgn, hRgn, newRgn, RGN_OR); 
		DeleteObject(newRgn);
		return (hRgn);
	}

}

void AwtGraphics::ArcAngleToIntercept( LONG    angle
                                      , LONG    x
                                      , LONG    y
                                      , LONG    width
                                      , LONG    height
                                      , PLONG   pxIntercept
                                      , PLONG   pyIntercept
                                      )
{
   const double   pi        = 3.1415926535;
   const double   toRadians = 2 * pi / 360;

   *pxIntercept = x + ((width + 1) / 2)  + (LONG)(cos( (double)angle * toRadians ) * ((width+1)/2));
   *pyIntercept = y + ((height + 1) / 2) - (LONG)(sin( (double)angle * toRadians ) * ((height+1)/2));
}

void AwtGraphics::ComputeClipRect(LONG angle1, LONG angle2, LONG x, LONG y, LONG w, LONG h, RECT *rect)
{
	ArcAngleToIntercept(angle1, x, y, w, h, &(rect->left), &(rect->top));
	ArcAngleToIntercept(angle2, x, y, w, h, &(rect->right), &(rect->bottom));
}

void AwtGraphics::ceDrawArc( jint x, jint y, jint width, jint height, jint startAngle, jint arcAngle, BOOL filled)
{

   HDC    hdc;
   LONG   endAngle;

   if ((width > 0) && (height > 0) && (arcAngle != 0))
   {
		POINT poly1[4];
		POINT poly2[3];
		int nPolys = 0;
		int nPtsPoly1 = 0;
		
		hdc = GetDC();	  
		HRGN prevClipRgn, newClipRgn=NULL;
		RECT rect;

      if ((arcAngle > -360) && (arcAngle < 360))
	  {
		  POINT center;
		  
		  if (filled) {
			center.x = x + ((width+1)/2);
			center.y = y + ((height+1)/2);
		  }

  		  if (arcAngle < 0) {
			  startAngle += arcAngle;
			  arcAngle *= -1;
		  }
		  if (startAngle < 0) {
			  startAngle = 360 + (startAngle%360);
		  }
		  endAngle = startAngle + arcAngle;

		  if (startAngle % 90 != 0) {
			  if (endAngle/90 == startAngle/90) { // in same Quadrant as end 
				  ComputeClipRect(startAngle, endAngle, x, y, width, height, &rect);
				  arcAngle = 0; /* done */
				  if (filled) {
					  if (abs(rect.top - center.y) < abs(rect.bottom -center.y)) {
						  poly1[2].y = rect.top ;
					  } else {
						  poly1[2].y = rect.bottom ;
					  }
					  if (abs(rect.left - center.x) < abs(rect.right -center.x)) {
						  poly1[2].x = rect.left;
					  } else {
						  poly1[2].x = rect.right;
					  }
					  poly1[0].x = center.x;
					  poly1[0].y = center.y;
					  poly1[1].x = rect.left;
					  poly1[1].y = rect.top;
					  poly1[3].x = rect.right;
					  poly1[3].y = rect.bottom;
					  nPtsPoly1 = 4;
					  nPolys++;
				  }
			  } else {
				  LONG nextQ = 90* ((startAngle+89)/90); /* angle of next quadrant */
				  ComputeClipRect(startAngle, nextQ, x, y, width, height,  &rect);
				  arcAngle -= nextQ - startAngle;
				  startAngle = nextQ;

				  poly1[0].x = center.x;
				  poly1[0].y = center.y;
				  poly1[1].x = rect.left;
				  poly1[1].y = rect.top;
				
				  if (rect.bottom == center.y) {
				      poly1[2].x = rect.left;
					  poly1[2].y = rect.bottom;
				  } else {
					  poly1[2].x = rect.right;
					  poly1[2].y = rect.top;
				  }
				  nPtsPoly1 = 3;
				  nPolys++;
			  }
			  newClipRgn = AddRectToRegion(newClipRgn, &rect);
		  }
		  while (arcAngle >= 90 ) {
			  ComputeClipRect(startAngle, startAngle+90, x, y, width, height, &rect);
			  newClipRgn = AddRectToRegion(newClipRgn, &rect);
			  startAngle += 90;
			  arcAngle -= 90;
		  }
		  if (arcAngle) { /* anythig left? it must be less than 90 (and in one Quadrant ) */
				ComputeClipRect(startAngle, endAngle,  x, y, width, height,&rect);
				newClipRgn = AddRectToRegion(newClipRgn, &rect);
				  poly2[0].x = center.x;
				  poly2[0].y = center.y;
				  poly2[1].x = rect.right;
				  poly2[1].y = rect.bottom;
				
				  if (rect.top == center.y) {
				      poly2[2].x = rect.right;
					  poly2[2].y = rect.top;
				  } else {
					  poly2[2].x = rect.left;
					  poly2[2].y = rect.bottom;
				  }
				  nPolys++;
		  }
			  
		  prevClipRgn = CreateRectRgn(0,0,0,0); /* need an existing region */

		  /* get existing clip region */
		  switch (GetClipRgn(hdc, prevClipRgn)) {
		  case -1:
			  fprintf(stderr, "GetClipRgn Failed\n");
			  /* drop through */
		  case 0:
			  // * No region */
			  DeleteObject(prevClipRgn);
			  prevClipRgn = NULL;
			  break;
		  case 1: /* success, we have a region, combine it */
			  if (CombineRgn(newClipRgn, prevClipRgn, newClipRgn, RGN_AND) == ERROR) {
					fprintf(stderr, "Combine Region Failed\n");	
			  }
			  break;
		  }

		  // select clip region and draw ellipse
		  SelectClipRgn(hdc, newClipRgn);
	  }


	  if (!filled) {
		  SetPenColor(TRUE);
		  SetBrushColor(FALSE);
		  ::Ellipse(hdc, x, y, x+width+1, y+height+1);
	  } else {
		  SetPenColor(FALSE);
		  SetBrushColor(TRUE);
		  ::Ellipse(hdc, x, y, x+width+1, y+height+1);
	  }


	  if (newClipRgn) {
		SelectClipRgn(hdc, prevClipRgn); /* restore clip region (even if null) */
		if (prevClipRgn) {
			  DeleteObject(prevClipRgn);
		}
		// restore original clip region - destroy created clip region
		DeleteObject(newClipRgn);
	  }
	
	  if (filled && nPolys) {
		  // Check which Polygon(s) to draw
		  if (nPtsPoly1 != 0) {
			  ::Polygon(hdc, poly1, nPtsPoly1);
		  }
		  if ((nPtsPoly1 == 0 && nPolys == 1) || (nPolys == 2)) {
			  ::Polygon(hdc, poly2, 3);
		  }
	  }
      ReleaseDC();
   }
}

#endif /* WINCE */

void AwtGraphics::Arc(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, jint nStartAngle, jint nEndAngle)
{
#ifdef WINCE
    ceDrawArc(TX(nX), TY(nY), nW, nH, nStartAngle, nEndAngle, FALSE);
#else       
    if (nW <= 0 || nH <= 0 || nEndAngle == 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    jint nSX, nSY, nDX, nDY;
    if (nEndAngle >= 360 || nEndAngle <= -360) {
        nSX = nDX = nX + nW;
        nSY = nDY = nY + nH / 2;
    } else {
        BOOL bEndAngleNeg = nEndAngle < 0;

        nEndAngle += nStartAngle;
        if (bEndAngleNeg) {
            // swap angles
            long tmp = nStartAngle;
            nStartAngle = nEndAngle;
            nEndAngle = tmp;
        }
        AngleToCoord(nStartAngle, nW, nH, &nSX, &nSY);
        nSX += nX + nW/2;
        nSY += nY + nH/2;
        AngleToCoord(nEndAngle, nW, nH, &nDX, &nDY);
        nDX += nX + nW/2;
        nDY += nY + nH/2;
    }
    SetPenColor(TRUE);
    SetBrushColor(FALSE);

    //Netscape : VERIFY check was dropped for arc as this function returns
	//false in win95 if bounding rectangle is 2x2 or smaller.
	//It does work OK for NT.
	
	::Arc(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1),
                      TX(nSX), TY(nSY), TX(nDX), TY(nDY));
    ReleaseDC();
#endif /* !WINCE */
}

void AwtGraphics::FillArc(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, jint nStartAngle, jint nEndAngle)
{
#ifdef WINCE
    ceDrawArc(TX(nX), TY(nY), nW, nH, nStartAngle, nEndAngle, TRUE);
#else /* !WINCE */
    if (nW <= 0 || nH <= 0 || nEndAngle == 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    jint nSX, nSY, nDX, nDY;
    if (nEndAngle >= 360 || nEndAngle <= -360) {
        nSX = nDX = nX + nW;
        nSY = nDY = nY + nH / 2;
    } else {
        BOOL bEndAngleNeg = nEndAngle < 0;

        nEndAngle += nStartAngle;
        if (bEndAngleNeg) {
            // swap angles
            long tmp = nStartAngle;
            nStartAngle = nEndAngle;
            nEndAngle = tmp;
        }
        AngleToCoord(nStartAngle, nW, nH, &nSX, &nSY);
        nSX += nX + nW/2;
        nSY += nY + nH/2;
        AngleToCoord(nEndAngle, nW, nH, &nDX, &nDY);
        nDX += nX + nW/2;
        nDY += nY + nH/2;
    }
    SetPenColor(FALSE);
    SetBrushColor(TRUE);
    //Netscape ; Verify was removed for Pie because this function
	//returns false in win95 if bounding rect is 2x2 or smaller.
	//It works Ok on NT.
	::Pie(hDC, TX(nX), TY(nY), TX(nX+nW+1), TY(nY+nH+1),
                      TX(nSX), TY(nSY), TX(nDX), TY(nDY));
    ReleaseDC();
#endif
}

void AwtGraphics::ChangePen(BOOL bSetPen)
{
    if (bSetPen) {
        if (m_hPen == NULL) {
            m_hPen = AwtPen::Get(m_color);
            m_bPenSet = FALSE;
        }
        if (!m_bPenSet) {
            ::SelectObject(m_hDC, m_hPen->GetHandle());
            m_bPenSet = TRUE;
        }
    } else if (m_bPenSet) {
        ::SelectObject(m_hDC, ::GetStockObject(NULL_PEN));
        m_bPenSet = FALSE;
    }
}

void AwtGraphics::ChangeBrush(BOOL bSetBrush)
{
    if (bSetBrush) {
        if (m_hBrush == NULL) {
            m_hBrush = AwtBrush::Get(m_color);
            m_bBrushSet = FALSE;
        }
        if (!m_bBrushSet) {
            ::SelectObject(m_hDC, m_hBrush->GetHandle());
            m_bBrushSet = TRUE;
        }
    } else if (m_bBrushSet) {
        ::SelectObject(m_hDC, ::GetStockObject(NULL_BRUSH));
        m_bBrushSet = FALSE;
    }
}

POINT* AwtGraphics::TransformPoly(JNIEnv *env, jint* xpoints, jint* ypoints,
				  POINT* points, int *pNpoints, BOOL close)
{
    int		i;
    int		npoints = *pNpoints;

    if (close) {
	close = (npoints > 2
		 && (xpoints[npoints - 1] != xpoints[0]
		     || ypoints[npoints - 1] != ypoints[0]));
	if (close) {
	    npoints++;
	    *pNpoints = npoints;
	}
    }
    if (npoints > POLYTEMPSIZE) {
	points = new POINT[npoints];
	if (points == 0) {
	    return 0;
	}
    }
    i = npoints;
    if (close) {
	--i;
	points[i].x = TX(xpoints[0]);
	points[i].y = TY(ypoints[0]);
    }
    while (--i >= 0) {
	points[i].x = TX(xpoints[i]);
	points[i].y = TY(ypoints[i]);
    }

    return points;
}

BOOL AwtGraphics::Polyline(JNIEnv *env, jint* xPoints, jint* yPoints,
			   int nPoints, BOOL close)
{
    POINT tmpPts[POLYTEMPSIZE], *pPoints;

    pPoints = TransformPoly(env, xPoints, yPoints, tmpPts, &nPoints, close);
    if (pPoints == NULL) {
	return FALSE;
    }

    HDC hDC = GetDC();
    if (hDC != NULL) {
	SetPenColor(TRUE);
	SetBrushColor(FALSE);
	VERIFY(::Polyline(hDC, pPoints, nPoints));
	ReleaseDC();
    }
    if (pPoints != tmpPts) {
	delete pPoints;
    }
    return TRUE;
}

BOOL AwtGraphics::FillPolygon(JNIEnv *env, jint* xPoints, jint* yPoints, int nPoints)
{
    POINT tmpPts[POLYTEMPSIZE], *pPoints;

    pPoints = TransformPoly(env, xPoints, yPoints, tmpPts, &nPoints, FALSE);
    if (pPoints == NULL) {
	return FALSE;
    }

    HDC hDC = GetDC();
    if (hDC != NULL) {
	SetPenColor(FALSE);
	SetBrushColor(TRUE);
	VERIFY(::Polygon(hDC, pPoints, nPoints));
	ReleaseDC();
    }
    if (pPoints != tmpPts) {
	delete pPoints;
    }
    return TRUE;
}

void AwtGraphics::DrawRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH)
{
    if (nW < 0 || nH < 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    // Bug #4055623 (Performance enhancements)
    SetBrushColor(TRUE);
    DWORD rop = (m_rop == R2_XORPEN) ? PATINVERT : PATCOPY;
    VERIFY(::PatBlt(hDC, TX(nX), TY(nY), nW+1, 1, rop));
    VERIFY(::PatBlt(hDC, TX(nX+nW), TY(nY), 1, nH+1, rop));
    VERIFY(::PatBlt(hDC, TX(nX), TY(nY), 1, nH+1, rop));
    VERIFY(::PatBlt(hDC, TX(nX), TY(nY+nH), nW+1, 1, rop));
    ReleaseDC();
}

long AwtGraphics::DrawStringWidth(JNIEnv *env, wchar_t *pzString, jint nX, jint nY)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return 0;
    }

    if (m_pFont != NULL && m_pFont->GetAscent() < 0) {
        AwtFont::SetupAscent(m_pFont);
    }
#ifndef WINCE
    if (m_rop == R2_XORPEN) {
        BeginPath(hDC);
    }
#endif
    if (m_isPrinter) {
        // Microsoft recommended solution: use ExtTextOutW to force each printed character to
        // start on an exact (screen unit) pixel boundary to avoid clipping the edges of
        // printed text.
        int length = wcslen(pzString);
        int* charwidths = new int[length];
        SIZE size;
        for (int i=0; i<length; i++) {
            GetTextExtentPoint32W(hDC, &pzString[i], 1, &size);
            charwidths[i] = size.cx;
        }
        VERIFY(::ExtTextOutW(hDC, 
                             TX(nX), TY(nY - (m_pFont ? m_pFont->GetAscent() : 0)), 
                             0, NULL,
                             pzString, length,
                             charwidths));
        delete[] charwidths;
    } else {
#ifndef WINCE
        VERIFY(::TextOutW(hDC, 
                          TX(nX), TY(nY - (m_pFont ? m_pFont->GetAscent() : 0)), 
                          pzString, wcslen(pzString)));
#else
	VERIFY(::ExtTextOutW(hDC, 
                          TX(nX), TY(nY - (m_pFont ? m_pFont->GetAscent() : 0)),  0, NULL,
                          pzString, wcslen(pzString), NULL));
#endif
    }
#ifndef WINCE
    if (m_rop == R2_XORPEN) {
        EndPath(hDC);
        SetBrushColor(TRUE);
        SetPenColor(FALSE);
        FillPath(hDC);  
    }
#endif
    SIZE size;
    VERIFY(::GetTextExtentPoint32W(hDC, pzString, wcslen(pzString), &size));
    ReleaseDC();
    return size.cx;
}

long AwtGraphics::DrawSubstringWidth(JNIEnv *env, BYTE* bytes, jint length,
                                     jobject font, 
                                     jobject des,
                                     jint nX, jint nY,
                                     BOOL unicode)
{
    HDC hDC = GetDC();
    if (hDC == NULL || length == 0) {
        return 0;
    }

    if (m_pFont != NULL && m_pFont->GetAscent() < 0) {
        AwtFont::SetupAscent(m_pFont);
    }
#ifndef WINCE
    if (m_rop == R2_XORPEN) {
        BeginPath(hDC);
    }
#endif
    AwtFont* af = AwtFont::GetFont(env, font);
    int fdIndex = AwtFont::getFontDescriptorNumber(env, font, des);
    HGDIOBJ hFont = af->GetHFont(fdIndex);

    SIZE size;
    HGDIOBJ oldFont = ::SelectObject(hDC, hFont);
    if (unicode) {
		//Netscape : Verify check was dropped for TextOutW because this 
		//function always returns FALSE in win95 if it is called 
		//after SetROP2(hDC, r2_XORPEN).  It does work for NT.
#ifndef WINCE
        ::TextOutW(hDC, TX(nX), 
                          TY(nY - (m_pFont ? m_pFont->GetAscent() : 0)), 
                          (WCHAR*)bytes, length);
#else
	::ExtTextOutW(hDC, TX(nX),
		      TY(nY - (m_pFont ? m_pFont->GetAscent() : 0)),
		      0, NULL,
		      (WCHAR*)bytes, length, NULL);
#endif /* WINCE */
        VERIFY(::GetTextExtentPoint32W(hDC, (WCHAR*)bytes, length, &size));
    } else {
#ifndef WINCE
        VERIFY(::TextOutA(hDC, TX(nX), 
                          TY(nY - (m_pFont ? m_pFont->GetAscent() : 0)), 
                          (CHAR*)bytes, length));
        VERIFY(::GetTextExtentPoint32A(hDC, (CHAR*)bytes, length, &size));
#else
	/* TODO */
#endif /* WINCE */
    }
    ::SelectObject(hDC, oldFont);
#ifndef WINCE
    if (m_rop == R2_XORPEN) {
        EndPath(hDC);
        SetBrushColor(TRUE);
        SetPenColor(FALSE);
        FillPath(hDC);  
    }
#endif
    ReleaseDC();
    return size.cx;
}

void AwtGraphics::DrawImage(JNIEnv *env, AwtImage *pImage, jint nX,
			    jint nY, jobject c)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    int xormode = (m_rop == R2_XORPEN);
    HBRUSH hOldBrush;
    AwtBrush* hBrush = NULL;
    if (xormode) {
        SetPenColor(FALSE);
        hBrush = AwtBrush::Get(m_xorcolor);
        VERIFY(hBrush != NULL);
        hOldBrush = (HBRUSH)::SelectObject(hDC, hBrush->GetHandle());
    }
    if (pImage->m_pSharedDC != NULL) {
        pImage->m_pSharedDC->Lock();
        HDC hMemDC = pImage->m_pSharedDC->GetDC();
        if (hMemDC != NULL) {
            // See comment at top of AwtImage::Draw about BitBlt rop codes...
            VERIFY(::BitBlt(hDC, TX(nX), TY(nY),
                            pImage->dstWidth, pImage->dstHeight,
                            hMemDC, 0, 0,
                            xormode ? 0x00960000 : SRCCOPY));
        }
        pImage->m_pSharedDC->Unlock();
    } else {	
    
           pImage->Draw(env, hDC, TX(nX), TY(nY), xormode, m_xorcolor,
           hBrush ? (HBRUSH)hBrush->GetHandle() : NULL, c);	
    }
    if (xormode) {
        VERIFY(::SelectObject(hDC, hOldBrush));
        hBrush->Release();
    }
    m_bPaletteSelected = TRUE;
    ReleaseDC();
}

void AwtGraphics::DrawImage(JNIEnv *env, AwtImage *pImage,
			    jint nDX1, jint nDY1, jint nDX2, jint nDY2,
			    jint nSX1, jint nSY1, jint nSX2, jint nSY2,
			    jobject c)
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    int xormode = (m_rop == R2_XORPEN);
    HBRUSH hOldBrush;
    AwtBrush* hBrush = NULL;
    if (xormode) {
        SetPenColor(FALSE);
        hBrush = AwtBrush::Get(m_xorcolor);
        VERIFY(hBrush != NULL);
        hOldBrush = (HBRUSH)::SelectObject(hDC, hBrush->GetHandle());
    }
    if (pImage->m_pSharedDC != NULL) {
        HDC hMemDC = pImage->m_pSharedDC->GetDC();
        if (hMemDC != NULL) {
            // See comment at top of AwtImage::Draw about BitBlt rop codes...
            VERIFY(::StretchBlt(hDC, TX(nDX1), TY(nDY1),
				nDX2 - nDX1, nDY2 - nDY1,
				hMemDC, nSX1, nSY1,
				nSX2 - nSX1, nSY2 - nSY1,
				xormode ? 0x00960000 : SRCCOPY));
        }
    } else {	
      pImage->Draw(env, hDC, TX(nDX1), TY(nDY1), nDX2 - nDX1, nDY2 - nDY1,
                   nSX1, nSY1, nSX2 - nSX1, nSY2 - nSY1,
                   xormode, m_xorcolor,
                   hBrush ? (HBRUSH)hBrush->GetHandle() : NULL, c);
      
    }
    if (xormode) {
        VERIFY(::SelectObject(hDC, hOldBrush));
        hBrush->Release();
    }
    m_bPaletteSelected = TRUE;
    ReleaseDC();
}

void AwtGraphics::CopyArea(JNIEnv *env, jint nDstX, jint nDstY, jint nX, jint nY, jint nW, jint nH)
{
    if (nW <= 0 || nH <= 0) {
        return;
    }
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }

    long dX = nDstX - nX;
    long dY = nDstY - nY;
    nDstX = TX(nDstX);
    nDstY = TY(nDstY);
    RECT rectSrc, rectDst;
    ::SetRect(&rectDst, nDstX, nDstY, nDstX+nW, nDstY+nH);
    if (m_bClipping) {
	// ScrollDC uses the clip to limit both reading of source pixels
	// and writing of destination pixels.  Our CopyArea operation
	// wants to use the clip only for writing pixels, not reading
	// them.  So, we need to manually clip the destination of the
	// operation and perform the ScrollDC operation with no clipping
	// installed in the DC.  Note that this design only works for
	// simple rectangular clips and copy areas, but could be
	// extended to iterate over more complicated regions.
	if (!::IntersectRect(&rectDst, &rectDst, &m_rectClip)) {
	    ReleaseDC();
	    return;
	}
        LogicalSelectClipRgn(hDC, NULL);
    }
    ::CopyRect(&rectSrc, &rectDst);
    ::OffsetRect(&rectSrc, -dX, -dY);

    HRGN rgnUpdate;

    if (m_rop == R2_XORPEN) {
        // This is fairly gratuitious in that it forces the entire
        // region to be updated twice.
        rgnUpdate = ::CreateRectRgnIndirect(&rectDst);

        VERIFY(::BitBlt(hDC, rectDst.left, rectDst.top,
                        rectDst.right - rectDst.left,
                        rectDst.bottom - rectDst.top,
                        hDC, rectSrc.left, rectSrc.top,
                        SRCINVERT));
    } else {
        rgnUpdate = ::CreateRectRgn(0, 0, 0, 0);
        VERIFY(::ScrollDC(hDC, dX, dY, &rectSrc, NULL, rgnUpdate, NULL));
    }

    if (m_bClipping) {
        VERIFY(LogicalSelectClipRgn(hDC, m_hClipRgn) != ERROR);
    }

    // ScrollDC invalidates the part of the source rectangle that
    // is outside of the destination rectangle on the assumption
    // that you wanted to "move" the pixels from source to dest,
    // and so now you will want to paint new pixels in the source.
    // Since our copyarea operation involves no such semantics we
    // are only interested in the part of the update region that
    // corresponds to unavailable source pixels - i.e. the part
    // that falls within the destination rectangle.
    HRGN rgnDst = ::CreateRectRgnIndirect(&rectDst);
    int result = ::CombineRgn(rgnUpdate, rgnUpdate, rgnDst, RGN_AND);

    // Invalidate the exposed area.
    HWND hWnd;
    if (result != NULLREGION && (hWnd = GetHWnd()) != NULL) {
	// NOTE: must somehow send exposures for bitmaps...
#ifndef WINCE
	::InvalidateRgn(hWnd, rgnUpdate, TRUE);
#else
	/* TODO */
#endif
    }
    ::DeleteObject(rgnUpdate);
    ::DeleteObject(rgnDst);
    ReleaseDC();
}

// Translate logical units to device units (needed by SelectClipRgn)
int LogicalSelectClipRgn(HDC hDC, HRGN hClipRgn)
{
#ifdef WINCE
    return ::SelectClipRgn(hDC, hClipRgn);
#else
    SIZE wExt, vExt;
    POINT vOrg;
    HRGN hDevRgn;
    RECT rectClip;

    if (hClipRgn == NULL) {
        hDevRgn = hClipRgn;
    } else {
        GetWindowExtEx(hDC, &wExt);
        GetViewportExtEx(hDC, &vExt);
	GetViewportOrgEx(hDC, &vOrg);
        VERIFY(::GetRgnBox(hClipRgn, &rectClip) != ERROR);
        hDevRgn = CreateRectRgn((rectClip.left   * vExt.cx) / wExt.cx + vOrg.x,
                                (rectClip.top    * vExt.cy) / wExt.cy + vOrg.y,
                                (rectClip.right  * vExt.cx) / wExt.cx + vOrg.x,
                                (rectClip.bottom * vExt.cy) / wExt.cy + vOrg.y
				);
    }
    int ret = ::SelectClipRgn(hDC, hDevRgn);
    if (hDevRgn != NULL) {
        ::DeleteObject(hDevRgn);
    }

    return ret;
#endif
}

void AwtGraphics::ChangeClip(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, BOOL set)
{
      int x1, y1, x2, y2;
      /* Origin is already adjusted at the Java layer - do not add twice */
      //x1 = TX(nX);
      //y1 = TY(nY);
      x1 = nX;
      y1 = nY;
      if (nW <= 0 || nH <= 0) {
          x2 = x1;
          y2 = y1;
      } else {
          x2 = x1 + nW;
          y2 = y1 + nH;
      }
      if (!m_bClipping ||
          m_rectClip.left != x1 || m_rectClip.top != y1 ||
          m_rectClip.right != x2 || m_rectClip.bottom != y2)
      {
          RECT rectTmp;
          ::SetRect(&rectTmp, x1, y1, x2, y2);
          if (!set && m_bClipping) {
              ::IntersectRect(&m_rectClip, &m_rectClip, &rectTmp);
          } else {
              VERIFY(::CopyRect(&m_rectClip, &rectTmp));
          }
          ASSERT(m_rectClip.right != 0 && m_rectClip.bottom !=0) ;
          ::SetRectRgn(m_hClipRgn,
                       m_rectClip.left,
                       m_rectClip.top,
                       m_rectClip.right,
                       m_rectClip.bottom);
      }
      HDC hDC = GetDC();
      if (hDC == NULL) {
          return;
      }
      m_bClipping = TRUE;
  #if 0
      VERIFY(LogicalSelectClipRgn(hDC, m_hClipRgn) != ERROR);
  #else
      if (LogicalSelectClipRgn(hDC, m_hClipRgn) == ERROR) {
          hDC = m_pSharedDC->ResetDC();
          VERIFY(LogicalSelectClipRgn(hDC, m_hClipRgn) != ERROR);
      }
       
  #endif
      ReleaseDC();
}

void AwtGraphics::RemoveClip()
{
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    m_bClipping = FALSE;
    LogicalSelectClipRgn(hDC, NULL);
    ReleaseDC();
}

void AwtGraphics::Cleanup()
{
    // TODO: do this
}

void AwtGraphics::PrintComponent(AwtComponent* c)
{
#ifndef WINCE
    HDC hdc = GetDC();
    if (hdc == NULL) {
        return;
    }
    ::OffsetWindowOrgEx(hdc, -OX, -OY, NULL);
    c->SendMessage(WM_PRINT, (WPARAM)hdc, 
		   PRF_CHECKVISIBLE | PRF_CLIENT | PRF_NONCLIENT);
    ::OffsetWindowOrgEx(hdc, OX, OY, NULL);
    ReleaseDC();
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

// Grab this Device Context and assign a new "owner"
// must only be called after lock is obtained
HDC AwtSharedDC::GrabDC(AwtGraphics *pNewOwner)
{
    if (m_pOwner != NULL) {
		if ((m_pOwner != pNewOwner) || (m_hDC == NULL))
            m_pOwner->InvalidateDC();
    }
    if (m_hDC != NULL) {
        m_pOwner = pNewOwner;
    }
    return m_hDC;
}

// Workaround for NT 4.0 bug, where the DC in a window from a CS_OWNDC class becomes
// invalidated inexplicibly.
// must only be called after lock is obtained, *and* dc is grabbed
HDC AwtSharedDC::ResetDC()
{
    ASSERT(m_pComponent);
    VERIFY(::IsWindow(GetHWnd()));
    ::ReleaseDC(GetHWnd(), m_hDC);
    m_hDC = ::GetDC(GetHWnd());
    return m_hDC;
}

void AwtSharedDC::Detach(AwtGraphics *pOldOwner) {
    // rely on lockers bumping the reference count when they grab the lock,
    // to avoid the need for another lock here
    if (pOldOwner == NULL) {
        // Must be the destination drawable that is detaching,
        // make sure we cleanup if we are not about to in Detach() anyway,
        // athough calling FreeDC twice is OK
        FreeDC();
    } else if (m_pOwner == pOldOwner) {
        // Current owner is detaching, release this Device Context and
        // leave it "ownerless"
        m_pOwner = NULL;
        pOldOwner->InvalidateDC();
    }
    Detach();  // perform actual decrement & free operation
}

void AwtSharedDC::FreeDC() {
    // grab the m_hDC, & if it is not null then clean up, otherwise
    // do nothing--cleanup is already done
	Lock(); // Wait to be sure nothing's using the DC
    HDC hdc = (HDC)::InterlockedExchange((LPLONG)&m_hDC, (LONG)NULL);
	Unlock();
    if (!hdc)
        return;
    if (m_pOwner != NULL) {
        AwtGraphics* owner = m_pOwner;
        m_pOwner = NULL;
        owner->InvalidateDC();
    }
    if (m_pComponent == NULL) {
        VERIFY(::DeleteDC(hdc));
    } else {
        AwtComponent* comp = m_pComponent;
        m_pComponent = NULL;
        //jio_fprintf(stderr, "> %X AwtSharedDC::FreeDC   [%s hwnd:%X]\n", ::GetCurrentThreadId(), "AwtSharedDC", GetHWnd());
        ::ReleaseDC(comp->GetHWnd(), hdc);
    }
}


/////////////////////////////////////////////////////////////////////////////
// AwtMasterPrinterGraphcis

AwtMasterPrinterGraphics::AwtMasterPrinterGraphics()
    : AwtGraphics()
{
    m_isPrinter = TRUE;
    m_printingPage = FALSE;
    m_printingJob = FALSE;
}

BOOL CALLBACK PrintDlgHook(HWND hDlg, UINT iMsg, UINT wParam, LONG lParam)
{
    if (iMsg == WM_INITDIALOG) {
	SetForegroundWindow(hDlg);
	return FALSE;
    }
    return FALSE;
}

#define ROUNDTOINT(x) ((int)((x)+0.5))

BOOL AwtMasterPrinterGraphics::StartPrintJob(jobject pj)
{
#ifdef WINCE
    return FALSE;
#else
    PRINTDLG pd;
    DOCINFO di;

    memset(&pd, 0, sizeof(PRINTDLG));
    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner = NULL;  //for now disembodied&modeless - need parent win
    pd.lpfnPrintHook = (LPPRINTHOOKPROC) PrintDlgHook;
    pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION
        | PD_ENABLEPRINTHOOK | PD_NOWARNING;

    if (::PrintDlg(&pd)) {
        Hjava_lang_String* jname = (Hjava_lang_String *)unhand(pj)->title;
        JavaStringBuffer sb(jname);

        // Set pageResolution -- hardcode here and set up mapping between
	// this value and the actual printer dpi in StartPage
	unhand(pj)->pageResolution = PRINT_RESOLUTION;

		// NETIVA FIX (4049865) - begin - setup PrintJob.getPageDimension info
        // Set page dimensions.  Microsoft page dimensions are in tenths of
        // of a millimeter (254 per inch).  PrintJob dimensions are in points
        // (72 per inch).

//      LPDEVMODE pDevMode = (LPDEVMODE)::GlobalLock(pd.hDevMode);
//
//      Hjava_awt_Dimension* hPageDimension = unhand(pj)->pageDimension;
//      CHECK_OBJ(hPageDimension);
//      if (pDevMode->dmPaperWidth > 0) {
//          unhand(hPageDimension)->width = pDevMode->dmPaperWidth * 72 / 254;
//      }
//      if (pDevMode->dmPaperLength > 0) {
//          unhand(hPageDimension)->height = pDevMode->dmPaperLength * 
//	                                         72 / 254;
//      }
//
//      ::GlobalUnlock(pd.hDevMode);

	// the original 1.1.1 code does not work on all printer drivers.  it is erroneous because
	// the dmPaperWidth and dmPaperLength fields are only supposed to be interrogated when
	// dmPaperSize is set to DMPAPER_USER in which case DM_PAPERLENGTH and DM_PAPERWIDTH are
	// set in the dmFields member of the DEVMODE structure.  The page dimensions should be
	// interpolated from the dmPaperSize member or DeviceCaps should be consulted (this is what
	// the code below does, it avoids a large switch statement).  In addition the 1.1.1 code
	// does not take into account DM_ORIENTATION (portrait vs. landscape).
	// Finally, not all printer driver writers give the paper width and length in
	// MM_LOMETRIC (1/10th of a millimeter) even though the DDK spec says this should be the
	// measurement unit.  Some printer drivers use MM_TEXT (device units) making use of
	// dmPaperWidth/Length an unreliable mechanism which makes DeviceCaps much more robust
	// than consulting DEVMODE.

	// As an added bonus, using GetDeviceCaps() automatically rotates the values in case
	// the user selects landscape orientation.  DEVMODE is insensitive to the setting of the
	// orientation in the print dialog box.

	int iResolutionX    =   ::GetDeviceCaps(pd.hDC, LOGPIXELSX);    // printer DPI
	int iResolutionY    =   ::GetDeviceCaps(pd.hDC, LOGPIXELSY);

	int iPhysicalX      =   ::GetDeviceCaps(pd.hDC, PHYSICALWIDTH); // physical size in device units
	int iPhysicalY      =   ::GetDeviceCaps(pd.hDC, PHYSICALHEIGHT);

	Hjava_awt_Dimension* hPageDimension = unhand(pj)->pageDimension;
	CHECK_OBJ(hPageDimension);

	// convert device units to pixels and then multiply by 72dpi (points per inch)
	// note that this Dimension includes the unprintable area of the page.
	unhand(hPageDimension)->width  = ROUNDTOINT((iPhysicalX / (double)iResolutionX) * PRINT_RESOLUTION);
	unhand(hPageDimension)->height = ROUNDTOINT((iPhysicalY / (double)iResolutionY) * PRINT_RESOLUTION);

     // NETIVA FIX - end


        // Initialize pen and brush to match AwtGraphics settings.
        ::SelectObject(m_hDC, ::GetStockObject(NULL_PEN));
        ::SelectObject(m_hDC, ::GetStockObject(NULL_BRUSH));

        m_pSharedDC = new AwtSharedDC(NULL, pd.hDC);

        memset(&di, 0, sizeof(DOCINFO));
        di.cbSize      = sizeof(DOCINFO);
        di.lpszDocName = (char*)sb;
        if (::StartDoc(pd.hDC, &di) <= 0) {
            //If ::GetLastError() returns ERROR_CANCELLED,
			//ERROR_PRINT_CANCELLED, the user cancelled the operation.
			DWORD errorCode = ::GetLastError();
			if (errorCode != ERROR_CANCELLED &&
					errorCode != ERROR_PRINT_CANCELLED) {
				SignalError(0, JAVAPKG "InternalError",
						"printing StartDoc failed");
       		} 
		}
	else {
	    m_printingJob = TRUE;
	}
    }
    return m_printingJob;
#endif /* !WINCE */
}

void AwtMasterPrinterGraphics::StartPage()
{
#ifndef WINCE
    HDC hDC = GetDC();
    if (hDC == NULL) {
        return;
    }
    if (::StartPage(hDC) <= 0) {
	SignalError(0, JAVAPKG "InternalError", "printing StartPage failed");
    }
    else {
        VERIFY(::SetMapMode(hDC, MM_ISOTROPIC) > 0);
	HWND hWnd = ::GetDesktopWindow();
	HDC hDesktopDC = ::GetDC(hWnd);
	ASSERT(hDesktopDC != NULL);

	// FIX joe.warzecha@Eng 3Nov97 -- Printer hardcoded at 72dpi above
	VERIFY(::SetWindowExtEx(hDC, PRINT_RESOLUTION, PRINT_RESOLUTION,
				NULL));

	VERIFY(::SetViewportExtEx(hDC,
				  ::GetDeviceCaps(hDC, LOGPIXELSX),
				  ::GetDeviceCaps(hDC, LOGPIXELSY),
				  NULL));
	VERIFY(::SetViewportOrgEx(hDC,
				  -::GetDeviceCaps(hDC, PHYSICALOFFSETX),
				  -::GetDeviceCaps(hDC, PHYSICALOFFSETY),
				  NULL));
	VERIFY(::ReleaseDC(hWnd, hDesktopDC));
	// NETIVA FIX  (4078973) - printer DC's default to transparent just like screen DC's
	::SetBkMode(hDC, TRANSPARENT);
	// NETIVA FIX - end
	m_printingPage = TRUE;
    }
    ReleaseDC();
#endif /* WINCE */
}

#ifndef WINCE
// WinCE3.0 doesn't support printing
void AwtMasterPrinterGraphics::EndPage()
{
    if (m_printingPage) {
        HDC hDC = GetDC();
	if (hDC == NULL) {
	    return;
	}

        if (::EndPage(hDC) <= 0) {
	    SignalError(0, JAVAPKG "InternalError", "printing EndPage failed");
        }
	else {
            m_printingPage = FALSE;
	}
	ReleaseDC();
    }
}

void AwtMasterPrinterGraphics::EndPrintJob()
{
    if (m_printingJob) {
        HDC hDC = GetDC();
	if (hDC == NULL) {
            return;
	}
	if (::EndDoc(hDC) <= 0) {
	    SignalError(0, JAVAPKG "InternalError", "printing EndDoc failed");
	}
	else {
	    m_printingJob = FALSE;
	}
	ReleaseDC();
    }
}
#endif

void AwtMasterPrinterGraphics::Dispose(JNIEnv *env)
{
    // End the final page and the document -- if they were already
    // ended, these commands will be ignored

    // WinCE3.0 doesn't support printing
#ifndef WINCE
    EndPage();
    EndPrintJob();
#endif

    AwtGraphics::Dispose(env);
}

AwtMasterPrinterGraphics *
AwtMasterPrinterGraphics::Create(JNIEnv *env, 
				 jobject pJavaObject,
				 jobject pj)
{
#ifndef WINCE
    // This method should only be invoked when a WPrintGraphics has not
    // yet been bound to a WPrintJob (i.e., during WPrintJob.initJob())
    ASSERT(unhand(pj)->graphics == NULL);

    AwtMasterPrinterGraphics *pGraphics = new AwtMasterPrinterGraphics();
    pGraphics->m_pJavaObject = pJavaObject;

    return pGraphics;
#else
    return NULL;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Graphics native methods

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_initIDs(JNIEnv *env, jclass cls) 
{
    GET_FIELD_ID(PPCGraphics_foregroundFID, "foreground",
		 "Ljava/awt/Color;");
    GET_FIELD_ID(PPCGraphics_imageFID, "image", "Lsun/awt/image/Image;");
    GET_FIELD_ID(PPCGraphics_originXFID, "originX", "I");
    GET_FIELD_ID(PPCGraphics_originYFID, "originY", "I");
    GET_FIELD_ID(PPCGraphics_pDataFID, "pData", "I");

    cls = env->FindClass( "sun/awt/image/Image" );
    if ( cls == NULL ) {
        return;
    }
    GET_METHOD_ID(sun_awt_image_Image_getBackgroundMID,
		   "getBackground", "()Ljava/awt/Color;");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_createFromComponent(JNIEnv *env,
						      jobject self,
						      jobject hComp)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.createFromComponent(%x)\n"), self, hComp));
    if (hComp == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "null component");
        return;
    }

    AwtComponent* comp = PDATA(AwtComponent, hComp);;
    if (::IsWindow(comp->GetHWnd()) == FALSE) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "disposed component");
        return;
    }

    AwtGraphics* graphics = AwtGraphics::Create(env, self, comp);
    if (graphics == NULL) {
	env->ThrowNew(WCachedIDs.InternalErrorClass, 
		      "getGraphics not implemented for this component");
        return;
    }

    env->SetIntField(self, WCachedIDs.PPCGraphics_pDataFID, (jint)
		     graphics);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_createFromImage (JNIEnv *env, jobject self, jobject imageRep)
{

    AWT_TRACE((TEXT("%x: PPCGraphics.createFromImage(%x)\n"), self, imageRep));
    if (imageRep == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "null ImageRepresentation");
        return;
    }
    AwtImage* pimage = AwtImage::getAwtImageFromJNI(env, imageRep, NULL);
    if (pimage == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "null AwtImage");
        return;
    }

    AwtGraphics* graphics = AwtGraphics::Create(env, self, pimage);
    if (graphics == NULL) {
	env->ThrowNew(WCachedIDs.InternalErrorClass, 
		      "getGraphics not implemented for this Image");
        return;
    }

    env->SetIntField(self, WCachedIDs.PPCGraphics_pDataFID, (jint)
		     graphics);

}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_createFromGraphics(JNIEnv *env,
						     jobject self,
						     jobject hG)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.createFromGraphics(%x)\n"), self, hG));
    if (hG == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, 
		      "null graphics");
	return;
    }
    AwtGraphics* g = (AwtGraphics *) env->GetIntField(hG,
						      WCachedIDs.PPCGraphics_pDataFID);

    if (g == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass,
		      "null graphics");
	return;
    }

    AwtGraphics* graphics = AwtGraphics::CreateCopy(env, self, g);
    if (graphics == NULL) {
	env->ThrowNew(WCachedIDs.InternalErrorClass,
		      "getGraphics implemented only for Canvas Components");
        return;
    }

    env->SetIntField(self, WCachedIDs.PPCGraphics_pDataFID, (jint) graphics);
}

/*
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_createFromPrintJob(Hsun_awt_windows_WGraphics* self,
                                             Hsun_awt_windows_WPrintJob* pj)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.createFromPrintJob(%x)\n"), self, pj));
    if (pj == NULL) {
        SignalError(0, JAVAPKG "NullPointerException", "null printjob");
        return;
    }

    // This method should only be invoked when a WPrintGraphics has not
    // yet been bound to a WPrintJob (i.e., during WPrintJob.initJob())
    ASSERT(unhand(pj)->graphics == NULL);

    AwtMasterPrinterGraphics *pGraphics = 
        AwtMasterPrinterGraphics::Create(self, pj);
    unhand(self)->pData = (long) pGraphics;

    if (!pGraphics->StartPrintJob(pj)) {
        pGraphics->Dispose();
    }
}
*/

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_createFromHDC(JNIEnv *env,
						jobject self,
						jint hdc)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.createFromHDC(%x)\n"), self, hdc));
    if (hdc == 0) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "null HDC");
	return;
    }

    AwtGraphics *pGraphics = AwtGraphics::Create(env, self, (HDC)hdc);
    env->SetIntField(self, WCachedIDs.PPCGraphics_pDataFID, (jint) pGraphics);
}

/*
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_imageCreate(Hsun_awt_windows_WGraphics* self,
                                      Hsun_awt_image_ImageRepresentation* ir)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.imageCreate(%x)\n"), self, ir));
    if (self == 0 || ir == 0) {
        SignalError(0, JAVAPKG "NullPointerException", "null image");
        return;
    }

    AwtImage *pImage = AwtImage::GetpImage(ir);
    AwtGraphics *pGraphics = AwtGraphics::Create(self, pImage);
    unhand(self)->pData = (long)pGraphics;
}
*/

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_pSetFont(JNIEnv *env,
					   jobject self,
					   jobject hFont)
{
    AWT_TRACE((TEXT("%x: WGraphics.pSetFont(%x)\n"), self, hFont));
    if (hFont == 0) {
        env->ThrowNew(WCachedIDs.NullPointerExceptionClass, 
		      "null font");
        return;
    }

    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        AwtFont* font = AwtFont::GetFont(env, hFont);
        graphics->SetFont(font);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_pSetForeground(JNIEnv *env,
						 jobject self,
						 jint value)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.pSetForeground(%x)\n"), self, value));
    AwtGraphics* graphics = (AwtGraphics*) env->GetIntField(self,
							    WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        COLORREF color = PALETTERGB((value>>16)&0xff, 
                                    (value>>8)&0xff, 
                                    (value)&0xff);
        graphics->SetColor(color);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_disposeImpl(JNIEnv *env,
					      jobject self)
{
    AWT_TRACE((TEXT("%x: PPCGraphics._dispose()\n"), self));
    AwtGraphics* pGraphics = (AwtGraphics*)env->GetIntField(self, 
							    WCachedIDs.PPCGraphics_pDataFID);
    if (pGraphics != NULL) {
        pGraphics->Dispose(env);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawSFChars(JNIEnv *env,
					      jobject self,
					      jcharArray text,
					      jint offset,
					      jint length,
					      jint x,
					      jint y)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawSFChars(%x, %d, %d, %d, %d)\n"), 
               self, text, offset, length, x, y));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self, 
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics == NULL || length == 0) {
        return;
    }
    if (text == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass,
		      "null chars");
        return;
    }
    if (offset < 0 || length < 0 || 
	offset + length > env->GetArrayLength(text)) {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass,
		      0);
        return;
    }

    jboolean isCopy;
    jchar* pSrcBody = env->GetCharArrayElements(text, &isCopy);
    jchar* pSrc= pSrcBody + offset;
    jchar *buffer = new jchar[length + 1];

    for (int i=0; i<length; i++)
        buffer[i] = pSrc[i];
    buffer[length] = 0;
 
    (void)graphics->DrawStringWidth(env, (WCHAR *) buffer, x, y);
    delete[] buffer;
    
    env->ReleaseCharArrayElements(text, pSrcBody, JNI_ABORT);
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawMFCharsSegment(JNIEnv *env,
						     jobject self,
						     jobject font, 
						     jobject des,
						     jcharArray chars,
						     jint offset, 
						     jint length,
						     jint x,
						     jint y)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawMFCharSegment(%x, %x, %x, %d, %d, %d, %d)\n"), 
               self, font, des, chars, offset, length, x, y));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self, 
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics == NULL || length == 0) {
        return 0;
    }
    CHECK_NULL_RETURN(font, "null font");
    CHECK_NULL_RETURN(des, "null FontDescriptor");
    CHECK_NULL_RETURN(chars, "null chars");
    if (length < 0 || 
	length+offset > env->GetArrayLength(chars)) {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass, 
		      0);
        return 0;
    }

    jboolean isCopy;
    jchar *charsElems = env->GetCharArrayElements(chars, &isCopy);
    BYTE *string = (BYTE *) charsElems + offset;
    ASSERT(string != NULL);
    
    jint retVal = graphics->DrawSubstringWidth(env, string, length, font,
					       des, x, y, TRUE);
    
    env->ReleaseCharArrayElements(chars, charsElems, JNI_ABORT);
    
    return retVal;
}

JNIEXPORT jint JNICALL 
Java_sun_awt_pocketpc_PPCGraphics_drawMFCharsConvertedSegment(JNIEnv *env,
							      jobject self, 
							      jobject font,
							      jobject des, 
							      jbyteArray bytes,
							      jint length, 
							      jint x, 
							      jint y)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawMFCharSegment(%x, %x, %x, %d, %d, %d)\n"), 
               self, font, des, bytes, length, x, y));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics == NULL) {
        return 0;
    }
    CHECK_NULL_RETURN(font, "null font");
    CHECK_NULL_RETURN(des, "null FontDescriptor");
    CHECK_NULL_RETURN(bytes, "null bytes");
    if (length < 0 || 
	length > env->GetArrayLength(bytes)) {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass,
		      0);
        return 0;
    }

    jboolean isCopy;
    jbyte *bytesElems = env->GetByteArrayElements(bytes, &isCopy);

    jint retVal = graphics->DrawSubstringWidth(env, (BYTE*)bytesElems, length,
					       font, des, x, y,
					       FALSE);
    
    env->ReleaseByteArrayElements(bytes, bytesElems, JNI_ABORT);
    
    return retVal;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawBytesNative(JNIEnv *env,
						  jobject self,
						  jbyteArray text,
						  jint offset,
						  jint length,
						  jint x,
						  jint y)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawBytes(%x, %d, %d, %d, %d)\n"), 
               self, text, offset, length, x, y));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics == NULL || text == NULL) {
        if (text == NULL) {
	    env->ThrowNew(WCachedIDs.NullPointerExceptionClass, 
			  "null bytes");
        }
        return;
    }
    if (offset < 0 || length < 0 || 
	offset + length > env->GetArrayLength(text)) {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass,
		      0);
        return;
    }

    jboolean isCopy;
    jbyte* pSrcBody = env->GetByteArrayElements(text, &isCopy);
    jbyte* pSrc= pSrcBody + offset;
    jbyte* buffer = new jbyte[length + 1];
    memcpy(buffer, pSrc, length);
    buffer[length] = 0;

    graphics->DrawStringWidth(env, A2W(((char *) buffer)), x, y);
    delete[] buffer;

    env->ReleaseByteArrayElements(text, pSrcBody, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawLineNative(JNIEnv *env,
						 jobject self,
						 jint x1, jint y1,
						 jint x2, jint y2)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawLine(%d, %d, %d, %d)\n"), self, x1, y1, x2, y2));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->DrawLine(env, x1, y1, x2, y2);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_clearRect(JNIEnv *env,
					    jobject self,
					    jint x,
					    jint y,
					    jint w,
					    jint h)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.clearRect(%d, %d, %d, %d)\n"), self, x, y, w, h));
    AwtGraphics* graphics = (AwtGraphics*) env->GetIntField(self,
							    WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->ClearRect(env, x, y, w, h);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_fillRectNative(JNIEnv *env,
						 jobject self,
						 jint x,
						 jint y,
						 jint w,
						 jint h)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.fillRect(%d, %d, %d, %d)\n"), self, x, y, w, h));
    AwtGraphics* graphics = (AwtGraphics*) env->GetIntField(self,
							    WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->FillRect(env, x, y, w, h);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawRectNative(JNIEnv *env,
						 jobject self,
						 jint x,
						 jint y,
						 jint w,
						 jint h)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawRect(%d, %d, %d, %d)\n"), self, x, y, w, h));
    AwtGraphics* graphics = (AwtGraphics*) env->GetIntField(self,
							    WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->DrawRect(env, x, y, w, h);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawRoundRectNative(JNIEnv *env,
						      jobject self,
						      jint x, jint y, jint w, jint h,
						      jint arcWidth, jint arcHeight)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawRoundRect(%d, %d, %d, %d, %d, %d)\n"), 
               self, x, y, w, h, arcWidth, arcHeight));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->RoundRect(env, x, y, w, h, arcWidth, arcHeight);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_fillRoundRectNative(JNIEnv *env,
						      jobject self,
						      jint x, jint y, jint w, jint h,
						      jint arcWidth, jint arcHeight)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.fillRoundRect(%d, %d, %d, %d, %d, %d)\n"), 
               self, x, y, w, h, arcWidth, arcHeight));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->FillRoundRect(env, x, y, w, h, arcWidth, arcHeight);
    }
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_setPaintMode(JNIEnv *env,
					       jobject self)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.setPaintMode()\n"), self));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    jobject fg = env->GetObjectField(self, WCachedIDs.PPCGraphics_foregroundFID);
    if (fg == 0) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, 
		      "null color");
        return;
    }

    jint value = env->CallIntMethod(fg,
				    WCachedIDs.java_awt_Color_getRGBMID);
    COLORREF fgcolor = PALETTERGB((value>>16)&0xff,
                                  (value>>8)&0xff,
                                  (value)&0xff);

    if (graphics != NULL) {
        graphics->SetRop(R2_COPYPEN, fgcolor, RGB(0, 0, 0));
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_setXORMode(JNIEnv *env,
					     jobject self,
					     jobject c)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.setXORMode(%x)\n"), self, c));
    jobject fg = env->GetObjectField(self,
				     WCachedIDs.PPCGraphics_foregroundFID);
    if (c == 0 || fg == 0) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, 
		      "null color");
        return;
    }
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    jint cValue = env->CallIntMethod(c,
				     WCachedIDs.java_awt_Color_getRGBMID);
    COLORREF color = PALETTERGB((cValue>>16)&0xff,
                                (cValue>>8)&0xff,
                                (cValue)&0xff);
    jint fgValue = env->CallIntMethod(fg,
				      WCachedIDs.java_awt_Color_getRGBMID);
    COLORREF fgcolor = PALETTERGB((fgValue>>16)&0xff,
                                  (fgValue>>8)&0xff,
                                  (fgValue)&0xff);

    if (graphics != NULL) {
        graphics->SetRop(R2_XORPEN, fgcolor, color);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_setClipNative(JNIEnv *env,
						jobject self,
						jint x, jint y,
						jint w, jint h)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.setClipNative(%d, %d, %d, %d)\n"), self, x, y, w, h));
    AwtGraphics* graphics = (AwtGraphics*) env->GetIntField(self,
							    WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->ChangeClip(env, x, y, w, h, TRUE);  // Overwrite previous clip
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_removeClip(JNIEnv *env,
					     jobject self)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.removeClip()\n"), self));
    AwtGraphics* graphics = (AwtGraphics*) env->GetIntField(self,
							    WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->RemoveClip();
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_copyArea(JNIEnv *env,
					   jobject self,
					   jint x, jint y,
					   jint width, jint height,
					   jint dx, jint dy)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.copyArea(%d, %d, %d, %d, %d, %d)\n"),
               self, x, y, width, height, dx, dy));
    AwtGraphics* graphics = (AwtGraphics*) env->GetIntField(self,
							    WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->CopyArea(env, dx+x, dy+y, x, y, width, height);
    }
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawPolygonNative(JNIEnv *env,
						    jobject self,
						    jintArray xpoints,
						    jintArray ypoints,
						    jint npoints)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawPolygon(%x, %x, %d)\n"), self, xpoints, ypoints, npoints));
    AwtGraphics* graphics = (AwtGraphics*) env->GetIntField(self,
							    WCachedIDs.PPCGraphics_pDataFID);
    if (graphics == NULL || xpoints == NULL || ypoints == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, 0);
	return;
    }

    if (env->GetArrayLength(ypoints) < npoints
	|| env->GetArrayLength(xpoints) < npoints)
    {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass,
		      0);
	return;
    }

    jboolean xpointsIsCopy;
    jint *xpointsElems = env->GetIntArrayElements(xpoints,
						  &xpointsIsCopy);
    
    jboolean ypointsIsCopy;
    jint *ypointsElems = env->GetIntArrayElements(ypoints,
						  &ypointsIsCopy);

    if (!graphics->Polyline(env, xpointsElems, ypointsElems,
			    npoints, TRUE)) {
	env->ThrowNew(WCachedIDs.OutOfMemoryErrorClass, 0);
    }

    env->ReleaseIntArrayElements(xpoints, xpointsElems, JNI_ABORT);
    env->ReleaseIntArrayElements(ypoints, ypointsElems, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawPolylineNative(JNIEnv *env,
						     jobject self,
						     jintArray xpoints,
						     jintArray ypoints,
						     jint npoints)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawPolyline(%x, %x, %d)\n"), self, xpoints, ypoints, npoints));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self, WCachedIDs.PPCGraphics_pDataFID);
    if (graphics == NULL || xpoints == NULL || ypoints == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, 0);
	return;
    }
    if (env->GetArrayLength(ypoints) < npoints
	|| env->GetArrayLength(xpoints) < npoints)
    {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass,
		      0);
	return;
    }

    jboolean xpointsIsCopy;
    jint *xpointsElems = env->GetIntArrayElements(xpoints,
						  &xpointsIsCopy);
    
    jboolean ypointsIsCopy;
    jint *ypointsElems = env->GetIntArrayElements(ypoints,
						  &ypointsIsCopy);

    if (!graphics->Polyline(env, xpointsElems, ypointsElems,
			    npoints, FALSE)) {
	env->ThrowNew(WCachedIDs.OutOfMemoryErrorClass, 0);
    }

    env->ReleaseIntArrayElements(xpoints, xpointsElems, JNI_ABORT);
    env->ReleaseIntArrayElements(ypoints, ypointsElems, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_fillPolygonNative(JNIEnv *env,
						    jobject self,
						    jintArray xpoints,
						    jintArray ypoints,
						    jint npoints)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.fillPolygon(%x, %x, %d)\n"), self, xpoints, ypoints, npoints));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self, WCachedIDs.PPCGraphics_pDataFID);
    if (graphics == NULL || xpoints == NULL || ypoints == NULL) {
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, 0);
	return;
    }
    if (env->GetArrayLength(ypoints) < npoints
	|| env->GetArrayLength(xpoints) < npoints)
    {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass,
		      0);
	return;
    }

    jboolean xpointsIsCopy;
    jint *xpointsElems = env->GetIntArrayElements(xpoints,
						  &xpointsIsCopy);
    
    jboolean ypointsIsCopy;
    jint *ypointsElems = env->GetIntArrayElements(ypoints,
						  &ypointsIsCopy);

    if (!graphics->FillPolygon(env, xpointsElems, ypointsElems,
			       npoints)) {
	env->ThrowNew(WCachedIDs.OutOfMemoryErrorClass, 0);
    }

    env->ReleaseIntArrayElements(xpoints, xpointsElems, JNI_ABORT);
    env->ReleaseIntArrayElements(ypoints, ypointsElems, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawOvalNative(JNIEnv *env,
						 jobject self,
						 jint x, jint y, 
						 jint w, jint h)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawOval(%d, %d, %d, %d)\n"), self, x, y, w, h));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->Ellipse(env, x, y, w, h);
#ifdef DEBUG
    } else {
        DebugBreak();
#endif
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_fillOvalNative(JNIEnv *env,
						 jobject self,
						 jint x, jint y, 
						 jint w, jint h)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.fillOval(%d, %d, %d, %d)\n"), self, x, y, w, h));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->FillEllipse(env, x, y, w, h);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_drawArcNative(JNIEnv *env,
						jobject self,
						jint x, jint y, jint w, jint h,
						jint startAngle, jint endAngle)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.drawArc(%d, %d, %d, %d %d %d)\n"), 
               self, x, y, w, h, startAngle, endAngle));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self,
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->Arc(env, x, y, w, h, startAngle, endAngle);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_fillArcNative(JNIEnv *env,
						jobject self,
						jint x, jint y, jint w, jint h,
						jint startAngle, jint endAngle)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.fillArc(%d, %d, %d, %d %d %d)\n"), 
               self, x, y, w, h, startAngle, endAngle));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self, 
							   WCachedIDs.PPCGraphics_pDataFID);
    if (graphics != NULL) {
        graphics->FillArc(env, x, y, w, h, startAngle, endAngle);
    }
}

/*
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCGraphics_print(Hsun_awt_windows_WGraphics* self,
                                Hsun_awt_pocketpc_PPCComponentPeer* peer)
{
    AWT_TRACE((TEXT("%x: PPCGraphics.print(%x)\n"), self, peer));
    AwtGraphics* graphics = (AwtGraphics*)env->GetIntField(self, PPCGraphics_pDataFID);
    if (graphics != NULL) {
	CHECK_PEER(peer);
        AwtComponent* c = (AwtComponent*)unhand(peer)->pData;
	c->SendMessage(WM_AWT_PRINT_COMPONENT, (WPARAM)graphics,
		       MAKELPARAM(graphics->GetOX(), graphics->GetOY()));
    }
}
*/
