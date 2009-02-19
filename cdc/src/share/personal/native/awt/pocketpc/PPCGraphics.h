/*
 * @(#)PPCGraphics.h	1.11 06/10/10
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
#ifndef _WINCEGRAPHICS_H_
#define _WINCEGRAPHICS_H_

#include "awt.h"

#include "PPCComponentPeer.h"
#include "PPCImage.h"
#include "PPCBrush.h"
#include "PPCPen.h"
#include "sun_awt_pocketpc_PPCGraphics.h"

extern int LogicalSelectClipRgn(HDC hDC, HRGN hClipRgn);

class AwtSharedDC
{
public:
    INLINE AwtSharedDC(AwtComponent* pComponent, HDC hDC) {
        m_useCount = 1;
        m_pOwner = NULL;
        m_hDC = hDC;
        m_hOldBmp = NULL;
        m_pComponent = pComponent;
        ::InitializeCriticalSection(&m_lock);
    }

    // Get the Window being drawn to
    // (NULL if this is an offscreen graphics)
    INLINE AwtComponent* GetComponent() {
        return m_pComponent;
    }
    INLINE HWND GetHWnd() {
        return (m_pComponent == NULL) ? NULL : m_pComponent->GetHWnd();
    }

    // Get the Device Context without "grabbing it"
    // (only used as a source DC for off-screen blits)
    INLINE HDC GetDC() {
	return m_hDC;
    }

    // Attach this Shared DC to some object (increment reference count)
    // Returns TRUE if successfully attached, and FALSE if the object has
    // been asynchronously destroyed.
    INLINE BOOL Attach() {
        // If the object is already invalid (countReferences is zero)
        // we can't attach to the object
        if (!m_useCount)
            return FALSE;
        // Increase the number of references
        ::InterlockedIncrement(&m_useCount);
        return TRUE;
    }

    // Detach this Shared DC from some object
    // actual detach is performed in Detach()
    void Detach(class AwtGraphics *pOldOwner);

    INLINE void SetBitmap(HBITMAP hBM) {
	m_hOldBmp = (HBITMAP) ::SelectObject(m_hDC, hBM);
    }

    INLINE void UnsetBitmap() {
	if (m_hOldBmp != NULL) {
	    ::SelectObject(m_hDC, m_hOldBmp);
	}
    }

    // Lock this shared DC for exclusive writing.  This is necessary so that one 
    // thread isn't executing GDI calls when another thread has the DC.
    // Locking always implies attaching
    INLINE void Lock() {
        // keep this object referenced while a lock is outstanding
	Attach();
        AWT_CS_TRACE((stderr, "> %2X locking  (cnt:%d,rec:%d,own:%X) [%s hwnd:%X]\n", ::GetCurrentThreadId(),
                      m_lock.LockCount, m_lock.RecursionCount, m_lock.OwningThread,
                      "AwtSharedDC", GetHWnd()));
        ::EnterCriticalSection(&m_lock); 
        AWT_CS_TRACE((stderr, "> %2X  locked  (cnt:%d,rec:%d,own:%X) [%s hwnd:%X]\n", ::GetCurrentThreadId(),
                      m_lock.LockCount, m_lock.RecursionCount, m_lock.OwningThread,
                      "AwtSharedDC", GetHWnd()));
    } 

    // Grab this Device Context and assign a new "owner"
    // must only be called after lock is obtained
    HDC GrabDC(class AwtGraphics *pNewOwner);

    // The current (grabbed) DC has become invalid -- release and re-get it (workaround for NT 4.0 bug).
    // must only be called after lock is obtained, *and* dc is grabbed
    HDC ResetDC();

    // Unlock this shared DC
    // Unlocking implies detaching, including free/delete if last reference
    INLINE void Unlock() {
        ::LeaveCriticalSection(&m_lock);
        AWT_CS_TRACE((stderr, "> %2X unlocked (cnt:%d,rec:%d,own:%X) [%s hwnd:%X]\n", ::GetCurrentThreadId(),
                      m_lock.LockCount, m_lock.RecursionCount, m_lock.OwningThread,
                      "AwtSharedDC", GetHWnd()));
        // decrement reference count and free if necessary        
        Detach();
    } 

private:
    // private to make sure we are only deleted from our own Detach() below, and
    // always from a single thread
    INLINE ~AwtSharedDC() {
	/* FIX
        FreeDC();  // cleanup & release dc as needed
	*/
        ::DeleteCriticalSection(&m_lock);
    }

    // perform actual Detach of this Shared DC (decrement reference count and
    // free if necessary)
    INLINE void Detach() {
        if (m_useCount > 0 && ::InterlockedDecrement(&m_useCount) == 0)
            delete this;
    }

    // Get rid of the Windows DC object
    void FreeDC();

    long		 m_useCount;
    class AwtGraphics	*m_pOwner;
    HDC			 m_hDC;
    HBITMAP		 m_hOldBmp;
    AwtComponent	*m_pComponent;
    CRITICAL_SECTION     m_lock;
};

class AwtGraphics : public AwtObject
{
public:
    AwtGraphics();
    virtual ~AwtGraphics();

// Access methods
    INLINE int GetRop() { return m_rop; }

// Operations
    // Creates a graphics context on 'pWindow'.
    static AwtGraphics *Create(JNIEnv *env,
			       jobject pJavaObject, 
                               AwtComponent* pWindow);
    static AwtGraphics *Create(JNIEnv *env,
			       jobject pJavaObject, 
			       AwtImage *pImage);
    static AwtGraphics *CreateCopy(JNIEnv *env,
				   jobject pJavaObject, 
                                   AwtGraphics *pSource);
    static AwtGraphics *Create(JNIEnv *env,
			       jobject pJavaObject, 
			       HDC hdc);

    virtual void Dispose(JNIEnv *env);

    INLINE AwtComponent* GetComponent() {
        if (m_pSharedDC == NULL) {return NULL;}
        return m_pSharedDC->GetComponent();
    }

    INLINE HWND GetHWnd() {
        if (m_pSharedDC == NULL) {return NULL;}
        return m_pSharedDC->GetHWnd();
    }

protected:
    INLINE HDC GetDC() {
        if (m_pSharedDC != NULL) {
            m_pSharedDC->Lock();
            if (m_hDC == NULL) {
                ValidateDC();
                if (m_hDC == NULL) {
                    m_pSharedDC->Unlock();
                }
            } else if (m_bClipping) {
		VERIFY(LogicalSelectClipRgn(m_hDC, m_hClipRgn) != ERROR);
            }
        }
        return m_hDC;
    }
    INLINE void ReleaseDC() {
        if (m_pSharedDC != NULL) {
            if (m_pSharedDC->GetComponent() != NULL) {
                // Bug #4055623 - Performance enhancements
                if (m_bClipping) {
                    LogicalSelectClipRgn(m_hDC, NULL);
                }
            } 
            m_pSharedDC->Unlock();
        }
    }

public:
    void ValidateDC();
    void InvalidateDC();

    // Sets the m_color instance variable to the absolute COLORREF
    void ChangeColor(COLORREF color);

    // Calculates the appropriate new COLORREF according to the
    // current ROP and calls ChangeColor on the new COLORREF
    void SetColor(COLORREF color);

    void SetFont(AwtFont *pFont);

    INLINE AwtFont* GetFont() { return m_pFont; }

    void SetRop(int rop, COLORREF fgcolor, COLORREF color);

    void ChangeClip(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, BOOL set);
    void RemoveClip();

    // Returns the clipping rectangle.  If there is none, return NULL.
    RECT* GetClipRect(JNIEnv *env);

    // Draws a line using the foreground color between points (nX1, nY1)
    // and (nX2, nY2).  The end-point is painted.
    void DrawLine(JNIEnv *env, jint nX1, jint nY1, jint nX2, jint nY2);

    POINT* TransformPoly(JNIEnv *env, jint* xpoints, jint* ypoints, POINT *points,
			 int *pNpoints, BOOL close);

    BOOL Polyline(JNIEnv *env, jint* xpoints, jint* ypoints, int nPoints, BOOL close);

    BOOL FillPolygon(JNIEnv *env, jint* xpoints, jint* ypoints, int nPoints);

    void ClearRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH);

    void FillRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH);

    void DrawRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH);

    void RoundRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, jint nArcW, jint nArcH);
	
    void FillRoundRect(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, jint nArcW, 
                       jint nArcH);
	
    void Ellipse(JNIEnv *env, jint nX, jint nY, jint nW, jint nH);
	
    void FillEllipse(JNIEnv *env, jint nX, jint nY, jint nW, jint nH);
	
    // Converts an angle given in 1/64ths of a degree to a pair of coordinates
    // that intersects the line produced by the angle.
    void AngleToCoords(jint nAngle, jint *nX, jint *nY);
#ifdef WINCE
    void AwtGraphics::ArcAngleToIntercept(LONG angle,
					   LONG x, LONG y,
					   LONG width, LONG height,
					   PLONG pxIntercept,
					   PLONG pyIntercept);
    HRGN AwtGraphics::AddRectToRegion(HRGN hRgn, RECT *rect);
    void AwtGraphics::ComputeClipRect(LONG angle1, LONG angle2,
				      LONG x, LONG y, LONG w, LONG h,
				      RECT *rect);
    void AwtGraphics::ceDrawArc(jint x, jint y, jint width, jint height,
				jint startAngle, jint arcAngle, BOOL filled);

#endif /* WINCE */
    void Arc(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, jint nStartAngle, 
             jint nEndAngle);
	
    void FillArc(JNIEnv *env, jint nX, jint nY, jint nW, jint nH, jint nStartAngle, 
                 jint nEndAngle);
	
private:
    INLINE void SetBrushColor(BOOL bSetBrush) {
        if (bSetBrush != m_bBrushSet) {
            ChangeBrush(bSetBrush);
        }
    };
    void ChangeBrush(BOOL bSetBrush);
    INLINE void SetPenColor(BOOL bSetPen) {
        if (bSetPen != m_bPenSet) {
            ChangePen(bSetPen);
        }
    };
    void ChangePen(BOOL bSetPen);

public:
    // Draws the string at the specified point.  (nX, nY) designates
    // the reference point of the string - i.e. the top of the string
    // plus the ascent.  The width of the string is returned.
    long DrawStringWidth(JNIEnv *env, wchar_t *pzString, jint nX, jint nY);

    // Draws a single piece of a multi-font string at the specified 
    // point.  (nX, nY) designates the reference point of the string - 
    // i.e. the top of the string plus the ascent.  The width of the 
    // string is returned.
    long DrawSubstringWidth(JNIEnv *env, BYTE* bytes, jint length, jobject font, 
                            jobject des,
                            jint nX, jint nY, BOOL unicode);
	
    // Draws the image at ('nX', 'nY') with optional background color c.
    void DrawImage(JNIEnv *env, AwtImage *pImage, jint nX, jint nY,
                   jobject color);

    // Draws the subrectangle ('nSX1', 'nSY1') => ('nSX2', 'nSY2') of the image
    // inside the rectangle ('nDX1', 'nDY1') => ('nDX2', 'nDY2') of the DC
    // with optional background color c.
    void DrawImage(JNIEnv *env, AwtImage *pImage,
		   jint nDX1, jint nDY1, jint nDX2, jint nDY2,
		   jint nSX1, jint nSY1, jint nSX2, jint nSY2,
                   jobject color);

    void CopyArea(JNIEnv *env, jint nDstX, jint nDstY, jint nX, jint nY, jint nW, jint nH);

    // Delete any AwtGraphics objects which were not disposed.
    static void Cleanup();

    void PrintComponent(class AwtComponent* c);
    int GetOX() { 
	JNIEnv *env;
	
	if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	    return 0;
	}
	
	return env->GetIntField(m_pJavaObject, WCachedIDs.PPCGraphics_originXFID); 
    }
    int GetOY() { 
	JNIEnv *env;
	
	if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	    return 0;
	}
	
	return env->GetIntField(m_pJavaObject, WCachedIDs.PPCGraphics_originYFID); 
    }

// Variables
protected:
    // TRUE iff the underlying DC was obtained from ::PrintDlg, FALSE
    // otherwise
    BOOL m_isPrinter;

    class AwtSharedDC *m_pSharedDC;
    HDC m_hDC;
    jobject m_pJavaObject;

private:
    BOOL m_bPaletteSelected;
    RECT m_rectClip;
    HRGN m_hClipRgn;
    BOOL m_bClipping;

    AwtFont *m_pFont;

    COLORREF m_color;
    COLORREF m_xorcolor;
    BOOL m_bBrushSet;
    AwtBrush* m_hBrush;
    BOOL m_bPenSet;
    AwtPen* m_hPen;
    int m_rop;
};

class AwtMasterPrinterGraphics : public AwtGraphics
{
public:
    AwtMasterPrinterGraphics();
    virtual ~AwtMasterPrinterGraphics() {}

    BOOL StartPrintJob(jobject pj);
    void StartPage();
// PPC3.0 doesn't support printing
#ifndef WINCE
    void EndPage();
    void EndPrintJob();
#endif

    virtual void Dispose(JNIEnv *env);

    static AwtMasterPrinterGraphics *
        Create(JNIEnv *env, jobject pJavaObject, jobject pj);

private:
    BOOL m_printingPage;
    BOOL m_printingJob;
};

extern int LogicalSelectClipRgn(HDC hDC, HRGN hClipRgn);

#endif
