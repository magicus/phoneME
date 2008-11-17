/*
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
 */

#include "jni.h"
#include "awt.h"
#include "sun_awt_pocketpc_PPCImageRepresentation.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include "java_awt_image_BufferedImage.h"
#include "java_awt_image_ImageObserver.h"
#include "java_awt_image_ImageConsumer.h"
#include "PPCImage.h"
#include "PPCGraphics.h"
#include "PPCToolkit.h"
#include "PPCComponentPeer.h"

#define HINTS_DITHERFLAGS (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT)
#define HINTS_SCANLINEFLAGS (java_awt_image_ImageConsumer_COMPLETESCANLINES)

#define SCREEN_DRIVER_NAME NULL

#define IMAGE_BITS_PER_PIXEL 24
#define RED_MASK 0xFF0000     /* 8 Red Mask */
#define GREEN_MASK 0x00FF00   /* 8 Green Mask */
#define BLUE_MASK 0x0000FF    /* 8 Blue Mask */
#define BMP_CHANNELS  3      /*  24 bit     */

#define SetRGBQUAD(p, r, g, b)	\
    {				\
	(p)->rgbRed = r;	\
	(p)->rgbGreen = g;	\
	(p)->rgbBlue = b;	\
	(p)->rgbReserved = 0;	\
    }

// This is data structure holds the common colormap that is used to paint
// all images.  When an image is painted, the BITMAPINFOHEADER of 
// the image is copied into imageBitmapInfo and then used to
// paint the image.  This strategy eliminates an alloc/free when painting
// an image.

BITMAPINFO imageBitmapInfo = {0}; 
BITMAPINFO *imageTransMaskInfo = NULL;

/* Perform OR with a byte & following mask to turn any bit to 1 */
ushort bitmask1[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
/* Perform AND with a byte & following mask to turn any bit to 0 */
ushort bitmask0[] = {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};

CriticalSection imageLock("imageLock");

/**
 * Constructor of the class
 */

AwtImage::AwtImage(int width, int height, 
                   HBITMAP dibBitmap, 
                   HBITMAP maskBitmap,
                   BYTE **pBits,
                   BYTE **maskBits)
{
  this->pixBuffer = *pBits;
  this->currentPtr = this->pixBuffer;
  this->hbitmap = dibBitmap;

  this->maskBuffer = *maskBits;
  this->currmaskPtr = this->maskBuffer;
  this->maskBitmap = maskBitmap;
  this->hasTransparentPixels = FALSE;

  this->dstWidth = width;
  this->dstHeight = height;

  /* Initialize the bitmap stride and padding for DWORD alignment */
  this->bmp_stride = width * BMP_CHANNELS;
  this->bmp_padding = 0;
  while ((this->bmp_stride % 4 ) != 0) {
    this->bmp_stride++;
    this->bmp_padding++;
  }
  /* Initialize the mask bitmap stride and padding for DWORD alignment */
  this->mask_stride = int(width/8) + ((width % 8) ? 1 : 0);
  this->mask_padding = 0;
  while((this->mask_stride % 4) != 0) {
    this->mask_stride++;
    this->mask_padding++;
  }

  if ((this->mask_padding > 0) && (width % 8)) {
      this->mask_padding++;
  }

  m_bgcolor = 0;
  m_hIcon = NULL;
  m_pSharedDC = NULL;
  m_pSeen = NULL;
  m_hRgn = NULL;
}

/**
 * Destructor of the class
 */
AwtImage::~AwtImage()
{  
  if (m_pSharedDC != NULL) {
    m_pSharedDC->Detach(NULL);
    m_pSharedDC = NULL;
  }
  if (hbitmap != NULL) {
    VERIFY(::DeleteObject(hbitmap));
  }
  if (pixBuffer != NULL) {
    free(pixBuffer);
    pixBuffer = NULL;
  }
  if (currentPtr != NULL) {
    free(currentPtr);
    currentPtr = NULL;
  }
  free(imageTransMaskInfo);
}

void AwtImage::CleanUp()
{
  free(imageTransMaskInfo);
}

void AwtImage::SetDIBRGB(int blue, int green, int red)
{
  *(this->currentPtr) = (unsigned char)blue;    
  this->currentPtr++;                           
  *(this->currentPtr) = (unsigned char) green;  
  this->currentPtr++;                           
  *(this->currentPtr) = (unsigned char) red;    
  this->currentPtr++;                          
}

void AwtImage::SetMaskBuffer(int alpha, int index)             
{
  int bitpos = (index % 8);
    if (index != 0 && bitpos == 0) {                              
      this->currmaskPtr++;                        
      *this->currmaskPtr = 0;                     
    }                                 
    // Here we should actually check if (0 <= alpha < 127)
    // instead of just (alpha == 0). But ok for now.
    if (alpha == 0) {
        *(this->currmaskPtr) |= bitmask1[bitpos];     
         this->hasTransparentPixels = TRUE;
    }
    else {                                            
        *(this->currmaskPtr) &= bitmask0[bitpos];     
    }
}

BOOL AwtImage::OffScreenInit(JNIEnv *env,
                             jobject thisObj,
                             jobject ch,
                             jint nW, 
                             jint nH)
{
    dstWidth = srcWidth = nW;
    dstHeight = srcHeight = nH;
    HDC hDC = ::CreateDC(NULL, NULL, NULL, NULL);
    // We don't want to create a DDB here : we want to use
    // the DIB created in getAwtImageFromJNI call - so that
    // we have support for Buffered images.
    //hbitmap = ::CreateCompatibleBitmap(hDC, nW, nH);
    if (hbitmap) {
	HDC hMemDC = ::CreateCompatibleDC(hDC);
    
	// Clear the bitmap to the indicated background color.
	m_pSharedDC = new AwtSharedDC(NULL, hMemDC);
	m_pSharedDC->SetBitmap(hbitmap);

	COLORREF color;

	if (ch != NULL) {
	    jint value = env->CallIntMethod(ch,
					    WCachedIDs.java_awt_Color_getRGBMID);
	    color = PALETTERGB((value>>16)&0xff,
			       (value>>8)&0xff,
			       (value)&0xff);
	} else {
	    color = PALETTERGB(0, 0, 0);
	}
    
	HBRUSH hBrush = ::CreateSolidBrush(color);
	RECT rect;
	::SetRect(&rect, 0, 0, nW, nH);
	::FillRect(hMemDC, &rect, hBrush);
	VERIFY(::SetBkMode(hMemDC, TRANSPARENT) != NULL);
	::SelectObject(hMemDC, ::GetStockObject(NULL_BRUSH));
	::SelectObject(hMemDC, ::GetStockObject(NULL_PEN));
	VERIFY(::DeleteObject(hBrush));
    }
  
    ::DeleteDC(hDC);
  
    return (hbitmap != NULL);
}

HICON AwtImage::ToIcon()
{
  return NULL;
}

void AwtImage::Draw(JNIEnv *env, HDC hDC, jint nX, jint nY,
                    int xormode, COLORREF xorcolor, HBRUSH xorBrush,
                    jobject c)
{
  HDC hMemDC;
  HBITMAP hOldBmp = NULL;
  VERIFY(hMemDC = ::CreateCompatibleDC(hDC));
  
  HBITMAP hBitmap = GetDDB(hDC);
  if (this->hasTransparentPixels) {
    hOldBmp = (HBITMAP)::SelectObject(hMemDC, this->maskBitmap);
    COLORREF color;
    HBRUSH hBrush;
    int rop;
    if (xormode) {
      hBrush = xorBrush;
      color = xorcolor;
      rop = 0x00e20000;
    } else if (c == 0) {
      hBrush = NULL;
      color = PALETTERGB(0, 0, 0);
      rop = 0x00220000;
    } else {
      hBrush = NULL;
      jint value = env->CallIntMethod(c,
                                      WCachedIDs.java_awt_Color_getRGBMID);
      color = PALETTERGB((value>>16)&0xff,
                         (value>>8)&0xff,
                         (value)&0xff);
      rop = 0x00e20000;
    }
    if (color != m_bgcolor) {
      HDC hMemDC2;
      HBITMAP hOldBitmap2;
      HBRUSH hOldBrush;
      if (!xormode && c != 0) {
        hBrush = ::CreateSolidBrush(color);
      }
      VERIFY(hMemDC2 = ::CreateCompatibleDC(hDC));
      hOldBitmap2 = (HBITMAP) ::SelectObject(hMemDC2, hBitmap);
      if (hBrush != NULL) {
        hOldBrush = (HBRUSH) ::SelectObject(hMemDC2, hBrush);
      }
      /* This blt blacks out the transparent pixels on the */
      /* Source image */
      VERIFY(::BitBlt(hMemDC2, 0, 0, dstWidth, dstHeight,
                      hMemDC, 0, 0, rop));
      if (hBrush != NULL) {
        VERIFY(::SelectObject(hMemDC2, hOldBrush));
      }
      VERIFY(::SelectObject(hMemDC2, hOldBitmap2));
      VERIFY(::DeleteDC(hMemDC2));
      if (!xormode && c != 0) {
        VERIFY(::DeleteObject(hBrush));
      }
      m_bgcolor = color;
    }
    if (xormode || c != 0) {
      ::SelectObject(hMemDC, hBitmap);
      VERIFY(::BitBlt(hDC, nX, nY, dstWidth, dstHeight,
                      hMemDC, 0, 0,
                      xormode ? 0x00960000 : SRCCOPY));
    } else {      
      COLORREF prevBkColor, prevTxtColor;
      
      prevBkColor = SetBkColor(hDC, RGB(255,255,255)); /* white */
      prevTxtColor = SetTextColor(hDC, RGB(0,0,0)); /* black */
      
      /* Paints opaque pixels with 0 (black), so when we paint */
      /* the image (with OR), it will come out correct */
      VERIFY(::BitBlt(hDC, nX, nY, dstWidth,   dstHeight,
                      hMemDC, 0, 0, SRCAND));
      SetBkColor(hDC, prevBkColor);
      SetTextColor(hDC, prevTxtColor);
      
      ::SelectObject(hMemDC, hBitmap);
      /* blit actual image (transparent parts should have 0's) */
      BOOL mflag = ::BitBlt(hDC, nX, nY, dstWidth, dstHeight,
                            hMemDC, 0, 0, SRCPAINT);
#ifdef CVM_DEBUG
        if (!mflag) {
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0, // default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL);
        
        MessageBox(NULL, 
        (LPCTSTR) lpMsgBuf, 
        TEXT("Error"), 
        MB_OK | MB_ICONINFORMATION);
        
        LocalFree(lpMsgBuf);
        }      
#endif                
    }
  } else {
    //int hasRgn;
    //HRGN hR;
    //hR = ::CreateRectRgn(0, 0, 0, 0);
    //hasRgn = ::GetClipRgn(hDC, hR);  
    hOldBmp = (HBITMAP)::SelectObject(hMemDC, hBitmap);
    BOOL flag = ::BitBlt(hDC, nX, nY, dstWidth, dstHeight,
                         hMemDC, 0, 0,
                         SRCCOPY);
#ifdef CVM_DEBUG
    if (!flag) {
      LPVOID lpMsgBuf;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    GetLastError(),
                    0, /* default language */
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL);
      
      MessageBox(NULL, 
                 (LPCTSTR) lpMsgBuf, 
                 TEXT("Error"), 
                 MB_OK | MB_ICONINFORMATION);
      
      LocalFree(lpMsgBuf);
    }
#endif  
  }    
  ::SelectObject(hMemDC, hOldBmp);
  DeleteDC(hMemDC);
  
}

void AwtImage::Draw(JNIEnv *env, HDC hDC, jint nDX, jint nDY, jint nDW, jint nDH,
		    jint nSX, jint nSY, jint nSW, jint nSH,
		    int xormode, COLORREF xorcolor, HBRUSH xorBrush,
		    jobject c)
{
  HDC hMemDC;
  HBITMAP hOldBmp = NULL;
  VERIFY(hMemDC = ::CreateCompatibleDC(hDC));
  HBITMAP hBitmap = GetDDB(hDC);
  //int hasRgn;
  //HRGN hR;
  //hR = ::CreateRectRgn(0, 0, 0, 0);
  //hasRgn = ::GetClipRgn(hDC, hR);
  if (nSX == 0 && nSY == 0 &&
      nDW == nSW && nDW == dstWidth &&
      nDH == nSH && nDH == dstHeight) {
      /*
       * The StretchBlt code probably has special cases for source
       * and destination size being equal, but since this code forces
       * the creation of a mask if the image is only partially loaded
       * then it will be better if we can vector a common non-scaled
       * case to the non-scaled draw routine that deals with partially
       * loaded images without turning them into masks.
       */
      Draw(env, hDC, nDX, nDY, xormode, xorcolor, xorBrush, c);
      return;
  }
  if (this->hasTransparentPixels) {
      hOldBmp = (HBITMAP)::SelectObject(hMemDC, this->maskBitmap);
      COLORREF color;
      HBRUSH hBrush;
      int rop;
      if (xormode) {
          hBrush = xorBrush;
          color = xorcolor;
          rop = 0x00e20000;
      } else if (c == 0) {
          hBrush = NULL;
          color = PALETTERGB(0, 0, 0);
          rop = 0x00220000;
      } else {
          hBrush = NULL;
          jint value = env->CallIntMethod(c,
                                          WCachedIDs.java_awt_Color_getRGBMID);
          color = PALETTERGB((value>>16)&0xff,
                             (value>>8)&0xff,
                             (value)&0xff);
          rop = 0x00e20000;	               
          HDC hMemDC2;
          HBITMAP hOldBitmap2;
          HBRUSH hOldBrush;
          if (!xormode && c != 0) {
              hBrush = ::CreateSolidBrush(color);
          }
          VERIFY(hMemDC2 = ::CreateCompatibleDC(hDC));
          hOldBitmap2 = (HBITMAP) ::SelectObject(hMemDC2, hBitmap);
          if (hBrush != NULL) {
              hOldBrush = (HBRUSH) ::SelectObject(hMemDC2, hBrush);
          }
          /* This blt blacks out the transparent pixels on the */
          /* Source image */
          VERIFY(::BitBlt(hMemDC2, 0, 0, dstWidth, dstHeight,
                          hMemDC, 0, 0, rop));
          if (hBrush != NULL) {
              VERIFY(::SelectObject(hMemDC2, hOldBrush));
          }
          VERIFY(::SelectObject(hMemDC2, hOldBitmap2));
          VERIFY(::DeleteDC(hMemDC2));
          if (!xormode && c != 0) {
              VERIFY(::DeleteObject(hBrush));
          }
          m_bgcolor = color;
      }
      if (xormode || c != 0) {
          ::SelectObject(hMemDC, hBitmap);
          VERIFY(::StretchBlt(hDC, nDX, nDY, nDW, nDH,
                              hMemDC, nSX, nSY, nSW, nSH,
                              xormode ? 0x00960000 : SRCCOPY));
      } else {                
          /* SRCAND should turn masked pixels to 0 */
          /* Opaque bits should have 0's */
          COLORREF prevBkColor, prevTxtColor;
          
          prevBkColor = SetBkColor(hDC, RGB(255,255,255)); /* white */
          prevTxtColor = SetTextColor(hDC, RGB(0,0,0)); /* black */
          VERIFY(::StretchBlt(hDC, nDX, nDY, nDW, nDH,
                              hMemDC, nSX, nSY, nSW, nSH, SRCAND));
          SetBkColor(hDC, prevBkColor);
          SetTextColor(hDC, prevTxtColor);
          ::SelectObject(hMemDC, hBitmap);
          VERIFY(::StretchBlt(hDC, nDX, nDY, nDW, nDH,
                              hMemDC, nSX, nSY, nSW, nSH, SRCPAINT));
      } 
  }     
  else {
      hOldBmp = (HBITMAP)::SelectObject(hMemDC, hBitmap);
      //BOOL flag = ::BitBlt(hDC, nDX, nDY, dstWidth, dstHeight,
      //                   hMemDC, 0, 0,
      //                   SRCCOPY);
      BOOL flag = ::StretchBlt(hDC, nDX, nDY, nDW, nDH,
                               hMemDC, nSX, nSY, nSW, nSH,
                               xormode ? 0x00960000 : SRCCOPY);
#ifdef CVM_DEBUG
      if (!flag) {
          LPVOID lpMsgBuf;
          FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        GetLastError(),
                        0, /* default language */
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL);
          
          MessageBox(NULL, 
                     (LPCTSTR) lpMsgBuf, 
                     TEXT("Error"), 
                     MB_OK | MB_ICONINFORMATION);
          
          LocalFree(lpMsgBuf);
      }
#endif
      
  } // else 
  VERIFY(::SelectObject(hMemDC, hOldBmp) != NULL);
  VERIFY(::DeleteDC(hMemDC));
    
}

void AwtImage::sendPixels(JNIEnv *env, jobject osis)
{
    jobject ich = 
        env->GetObjectField (osis,
                             WCachedIDs.sun_awt_image_OffScreenImageSource_theConsumerFID); 
    
    if (ich == NULL)
        return;
    
    int i, j;
    HDC hDC;
    HBITMAP hDIBitmap;
    
    jobject toolkitObj = 
        env->CallStaticObjectMethod (WCachedIDs.java_awt_ToolkitClass, 
                                     WCachedIDs.java_awt_Toolkit_getDefaultToolkitMID);
    
    jobject cm = 
        env->CallObjectMethod (toolkitObj, 
                               WCachedIDs.java_awt_Toolkit_getColorModelMID);    
    
    env->CallVoidMethod (ich,
			 WCachedIDs.java_awt_image_ImageConsumer_setColorModelMID, cm); 
    
    if (env->ExceptionCheck())
        return;
    
    env->CallVoidMethod (ich, 
                         WCachedIDs.java_awt_image_ImageConsumer_setHintsMID,
                         HINTS_OFFSCREENSEND);
    
    if (env->ExceptionCheck ())
	return;
    
    VERIFY(hDC = ::CreateDC(NULL, NULL, NULL, NULL));
    BITMAPINFO *pBitmapInfo = (BITMAPINFO*) malloc(sizeof(BITMAPINFOHEADER) +
						   256*sizeof(RGBQUAD));
    pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pBitmapInfo->bmiHeader.biBitCount = 0;
    
    int depth;
    BYTE *pTemp;
    {
        m_pSharedDC->UnsetBitmap();
        
	BITMAP bitmap;
        
	GetObject(hbitmap, sizeof(BITMAP), &bitmap);
	pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBitmapInfo->bmiHeader.biPlanes = 1;
	depth = pBitmapInfo->bmiHeader.biBitCount = bitmap.bmBitsPixel; 
	pBitmapInfo->bmiHeader.biCompression = BI_RGB; 
	pBitmapInfo->bmiHeader.biSizeImage = 0; //will compute 
	pBitmapInfo->bmiHeader.biXPelsPerMeter= 1000; 
	pBitmapInfo->bmiHeader.biYPelsPerMeter= 1000; 
	pBitmapInfo->bmiHeader.biClrUsed =
	    pBitmapInfo->bmiHeader.biClrImportant = 256;
	pBitmapInfo->bmiHeader.biHeight = -bitmap.bmHeight; /* negative */
	pBitmapInfo->bmiHeader.biWidth = bitmap.bmWidth;
	hDIBitmap = CreateDIBSection(hDC, pBitmapInfo,
				     DIB_RGB_COLORS, /* iUsage */
				     (void **)&pTemp, /* will fill in pointer */
				     NULL, NULL);	        
        m_pSharedDC->SetBitmap(hbitmap);
    }
    
    VERIFY(::DeleteDC(hDC));
    free(pBitmapInfo);    

    jintArray pixh = env->NewIntArray(dstWidth);

    if (pixh == NULL || pTemp == 0) {
	// Note: delete of a NULL pTemp is OK, pixh is GC'd if necessary
	DeleteObject(hDIBitmap);
        env->ThrowNew (WCachedIDs.OutOfMemoryErrorClass, NULL);
	return;
    }
    
    jint *buf = env->GetIntArrayElements(pixh, 0);

    unsigned char *pTSrc = (unsigned char *) pTemp;
    unsigned int *pTDst;
    int adjust = (((dstWidth * 3) + 3) & ~3) - (dstWidth * 3);
    for (i = 0; i < dstHeight; i++) {
        if (ich == NULL) {
            break;
        }
        pTDst = (unsigned int *) buf;
        for (j = 0; j < dstWidth; j++) {
            *pTDst++ = ((pTSrc[0] << 16) | (pTSrc[1] << 8) | pTSrc[2]);
            pTSrc += 3;
        }
        pTSrc += adjust;
        
        env->CallVoidMethod (ich, 
                             WCachedIDs.java_awt_image_ImageConsumer_setPixelsMID,
                             0, i, dstWidth, 1, cm,
                             (jintArray)pixh, 0, dstWidth); 
        
        if (env->ExceptionCheck ())
            return;            
        
        DeleteObject(hDIBitmap);
        
    }
}

jobject 
AwtImage::getColorModel(JNIEnv *env)
{
  jobject awt_colormodel = NULL;

  awt_colormodel = 
    env->NewObject(WCachedIDs.java_awt_image_DirectColorModelClass,
                   WCachedIDs.java_awt_image_DirectColorModel_constructorMID,
                   IMAGE_BITS_PER_PIXEL,
                   RED_MASK,
                   GREEN_MASK,
                   BLUE_MASK,
                   0);
  
  if (env->ExceptionCheck())
    return NULL;

  return awt_colormodel;
}

AwtImage* AwtImage::getAwtImageFromJNI(JNIEnv *env, 
                                       jobject thisObj,
                                       jobject background) 
{
  AwtImage *pImage = 
    (AwtImage*)env->GetIntField(thisObj,
                                WCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);
  
  jint availInfo = 
    env->GetIntField(thisObj,
                     WCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);
  if (pImage == NULL &&
      (availInfo & IMAGE_SIZEINFO) == IMAGE_SIZEINFO) {
    int width, height;
    width = env->GetIntField(thisObj, 
                             WCachedIDs.sun_awt_image_ImageRepresentation_widthFID);    
    height = env->GetIntField(thisObj, 
                              WCachedIDs.sun_awt_image_ImageRepresentation_heightFID);
    if (width > 0 && height > 0) {

      HDC hDC = ::CreateDC(NULL, NULL, NULL, NULL);
      BYTE *pBits; // A pointer for holding the address of pixel bits
      // Prepare to create a 24 bit DIBSection
      imageBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      imageBitmapInfo.bmiHeader.biWidth = width;
      imageBitmapInfo.bmiHeader.biHeight = -height;// Top down approach
      imageBitmapInfo.bmiHeader.biPlanes = 1; // Must be set to one
      imageBitmapInfo.bmiHeader.biBitCount = 24; 
      imageBitmapInfo.bmiHeader.biCompression = BI_RGB; 
      
      HBITMAP bitmap = ::CreateDIBSection(NULL, &imageBitmapInfo, DIB_RGB_COLORS,
                                          (void**)&pBits, NULL, 0);    
#ifdef CVM_DEBUGXg
      if (bitmap == NULL) {
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      0, /* default language */
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL);
        
        MessageBox(NULL, 
                   (LPCTSTR) lpMsgBuf, 
                   TEXT("Error"), 
                   MB_OK | MB_ICONINFORMATION);
        
        LocalFree(lpMsgBuf);
      }
#endif      

      // Create a BitmapInfo object for transparency masks
      imageTransMaskInfo = (BITMAPINFO *)
	malloc(sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD));
      imageTransMaskInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      imageTransMaskInfo->bmiHeader.biWidth = width;
      imageTransMaskInfo->bmiHeader.biHeight = -height;
      imageTransMaskInfo->bmiHeader.biPlanes = 1;
      imageTransMaskInfo->bmiHeader.biBitCount = 1;
      imageTransMaskInfo->bmiHeader.biCompression = BI_RGB;
      imageTransMaskInfo->bmiHeader.biSizeImage = 0;
      imageTransMaskInfo->bmiHeader.biXPelsPerMeter = 0;
      imageTransMaskInfo->bmiHeader.biYPelsPerMeter = 0;
      imageTransMaskInfo->bmiHeader.biClrUsed = 2;
      imageTransMaskInfo->bmiHeader.biClrImportant = 2;
      SetRGBQUAD(&(imageTransMaskInfo->bmiColors[0]), 0, 0, 0);
      SetRGBQUAD(&(imageTransMaskInfo->bmiColors[1]), 255, 255, 255);

      BYTE *maskBits;
      HBITMAP mask_bitmap = ::CreateDIBSection(NULL, imageTransMaskInfo, DIB_RGB_COLORS,
                                               (void**)&maskBits, NULL, 0);    
#ifdef CVM_DEBUG
      if (mask_bitmap == NULL) {
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      0, /* default language */
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL);
        
        MessageBox(NULL, 
                   (LPCTSTR) lpMsgBuf, 
                   TEXT("Error"), 
                   MB_OK | MB_ICONINFORMATION);
        
        LocalFree(lpMsgBuf);
      }
#endif      

      pImage = new AwtImage (width, height, bitmap, mask_bitmap, &pBits, &maskBits);
      if (pImage == NULL) {
        env->ThrowNew (WCachedIDs.OutOfMemoryErrorClass, NULL);
        return NULL;
      }

      /* Attach Data to this object */ 
      env->SetIntField(thisObj,
                       WCachedIDs.sun_awt_image_ImageRepresentation_pDataFID,
                       (jint)pImage);

      /* Prepare transparency mask */
      pImage->initializeTransparencyMask(hDC);
    }
    /* Invalid width and height for image. */ 
    else {
      env->ThrowNew(WCachedIDs.InternalErrorClass, 
                    "Cannot create HBITMAP as width and height must be > 0");
      return NULL;
    }
    
  }
  return pImage;
}

void 
AwtImage::initializeTransparencyMask(HDC hDC)
{
  if (this->maskBuffer != NULL) {
    RECT rect;
    HDC hMemDC = ::CreateCompatibleDC(hDC);
    HBITMAP hOldBmp = (HBITMAP)
      ::SelectObject(hMemDC, this->maskBitmap);
    HBRUSH hBrush = (HBRUSH)::GetStockObject( WHITE_BRUSH );
    ::SetRect(&rect, 0, 0, dstWidth, dstHeight);
    ::FillRect(hMemDC, &rect, hBrush);
    hBrush = (HBRUSH)::GetStockObject( BLACK_BRUSH );
    ::SetRect(&rect, 0, 0, 0, 0);
    ::FillRect(hMemDC, &rect, hBrush);
    if ((m_nHints & HINTS_SCANLINEFLAGS) != 0) {
      ::SetRect(&rect, 0, 0, dstWidth, dstHeight);
      ::FillRect(hMemDC, &rect, hBrush);
    } else {
      if (m_hRgn != NULL) {
        ::SelectClipRgn(hMemDC, m_hRgn);
        ::SetRect(&rect, 0, 0, dstWidth, dstHeight);
        ::FillRect(hMemDC, &rect, hBrush);
        VERIFY(::SelectClipRgn(hMemDC, NULL) != ERROR);
        VERIFY(::DeleteObject(m_hRgn));
        //m_hRgn = NULL;
      }
    }
    // The HBRUSH's are stock objects, so don't delete them
    VERIFY(::SelectObject(hMemDC, hOldBmp) != NULL);
    VERIFY(::DeleteDC(hMemDC));
  }
  VERIFY(::DeleteDC(hDC));
  // Getting the mask data invalidates the values of the transparent pixels
  m_bgcolor = 0;
}

/* This convenience method performs argument checking for the
   setBytePixels and setIntPixels methods and creates a buffer, if
   necessary, to store the pixel values in before drawing them to the
   pixmap. ThisObj code is defined here so as to share as much code as
   possible between the two methods. */
   
AwtImage *
AwtImage::getAwtImageForSetPixels (JNIEnv *env,
                                   jobject thisObj,
                                   jint x,
                                   jint y,
                                   jint w,
                                   jint h,
                                   jobject colorModel,
                                   jobject pixels,
                                   jint offset,
                                   jint scansize)
{
    AwtImage *imgRep;
  
    /* Perform some argument checking. */
  
    if (thisObj == NULL || colorModel == NULL || pixels == NULL) {  
	env->ThrowNew(WCachedIDs.NullPointerExceptionClass, NULL);
	return NULL;
    }
  
    if (x < 0 || y < 0 || w < 0 || h < 0
	|| scansize < 0 || offset < 0) {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
	return NULL;
    }
	
    if (w == 0 || h == 0)
	return NULL;
  
    if ((env->GetArrayLength((jintArray)pixels) < (offset + w)) 
	|| ((scansize != 0) && 
	 (((env->GetArrayLength((jintArray)pixels) - w - offset) /
	   scansize) < (h - 1)))) {
	env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
	return NULL;
    }
	
    /* The rgb data has now been created for the supplied pixels. 
       First create a pixbuf representing these pixels and then 
       render them into the offscreen pixmap. */  
    imgRep = AwtImage::getAwtImageFromJNI(env,
                                          thisObj,
                                          NULL);		

    return imgRep;

}

// skip to the end of the row if x + w is smaller than the actual width
// this function modifies the value of currentPtr
void 
AwtImage::skipToEndOfRow(int x, int w) {
 if (x + w < this->GetWidth()) {
   for (int i = x + w; i < this->GetWidth(); i++) {
     this->currentPtr++; // for blue
     this->currentPtr++; // for green
     this->currentPtr++; // for red
     // Also increment the current pointer of Mask Buffer
     if ((i % 8) == 0) {
         this->currmaskPtr++;                       
         //this->currmaskPtr = 0;
     }                                              
   }			
 } 
}
// skip over imgRep->bmp_padding value
// this function modifies the value of currentPtr
void 
AwtImage::bmpPadding() {
  for (int pc = 0; pc < this->bmp_padding; pc++ ) {
    this->currentPtr++;
  } 
}

// skip over imgRep->mask_padding value for Mask buffer
void 
AwtImage::maskPadding() {
  for (int mc = 0; mc < this->mask_padding; mc++) {
    this->currmaskPtr++;
    //this->currmaskPtr = 0;
  }           
}

// skip parts of the beginning of the row if x > 0
// this function modifies the value of currentPtr
void 
AwtImage::skipBeginningOfRow(int x) {
  for (int i = 0; i < x; i++) {
    this->currentPtr++; // for blue
    this->currentPtr++; // for green
    this->currentPtr++; // for red
    if (i == 0) continue;
      if ((i % 8) == 0) {
          // Also increment the current pointer of Mask Buffer
          this->currmaskPtr++;                       
          //this->currmaskPtr = 0;
      }                    
  }
}

// return the number of bits per pixel
int 
AwtImage::GetBitsPerPixel() {
   return (int)IMAGE_BITS_PER_PIXEL; 
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_initIDs (JNIEnv *env, jclass cls)
{
    cls = env->FindClass("java/awt/image/ColorModel");

    if (cls == NULL) 
      return;
    WCachedIDs.java_awt_image_ColorModelClass = (jclass) env->NewGlobalRef (cls);
    GET_FIELD_ID(java_awt_image_ColorModel_pDataFID, "pData", "I");

    GET_METHOD_ID (java_awt_image_ColorModel_getRGBMID, "getRGB", "(I)I");
    
    cls = env->FindClass ("java/awt/image/DirectColorModel");

    if (cls == NULL)
        return;

    WCachedIDs.java_awt_image_DirectColorModelClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID (java_awt_image_DirectColorModel_constructorMID, "<init>", "(IIIII)V");
    GET_FIELD_ID (java_awt_image_DirectColorModel_red_maskFID, "red_mask", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_red_offsetFID, "red_offset", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_red_scaleFID, "red_scale", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_green_maskFID, "green_mask", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_green_offsetFID, "green_offset", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_green_scaleFID, "green_scale", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_blue_maskFID, "blue_mask", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_blue_offsetFID, "blue_offset", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_blue_scaleFID, "blue_scale", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_maskFID, "alpha_mask", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_offsetFID, "alpha_offset", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_scaleFID, "alpha_scale", "I");

    cls = env->FindClass ("java/awt/image/IndexColorModel");
    
    if (cls == NULL)
        return;

    WCachedIDs.java_awt_image_IndexColorModelClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID (java_awt_image_IndexColorModel_constructorMID, "<init>", "(II[B[B[B)V");
    GET_FIELD_ID (java_awt_image_IndexColorModel_rgbFID, "rgb", "[I");

    cls = env->FindClass ("java/awt/image/ImageConsumer");    
    if (cls == NULL)
        return;
    
    GET_METHOD_ID (java_awt_image_ImageConsumer_setColorModelMID,
		   "setColorModel", "(Ljava/awt/image/ColorModel;)V"); 
    GET_METHOD_ID (java_awt_image_ImageConsumer_setHintsMID,
		   "setHints", "(I)V");
    GET_METHOD_ID (java_awt_image_ImageConsumer_setPixelsMID,
		   "setPixels",
		   "(IIIILjava/awt/image/ColorModel;[III)V"); 

    cls = env->FindClass ("sun/awt/image/ImageRepresentation");
    
    if (cls == NULL)
	return;
    
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_pDataFID, "pData", "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_availinfoFID, "availinfo",
                  "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_widthFID, "width", "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_heightFID, "height", "I");
    //GET_FIELD_ID (sun_awt_image_ImageRepresentation_srcWFID, "srcW", "I");
    //GET_FIELD_ID (sun_awt_image_ImageRepresentation_srcHFID, "srcH", "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_hintsFID, "hints", "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_newbitsFID, "newbits", "Ljava/awt/Rectangle;"); 
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_offscreenInit(JNIEnv *env, 
                                                           jobject thisObj,
                                                           jobject background)
{
  CriticalSection::Lock l(imageLock);
  
  CHECK_NULL(thisObj, "null ImageRepresentation jobject");

  env->SetIntField(thisObj,
                   WCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID,
                   IMAGE_OFFSCRNINFO);     

  AwtImage *pImage = AwtImage::getAwtImageFromJNI(env,
                                                  thisObj,
                                                  background);		
  if (pImage == NULL) {
    env->ThrowNew (WCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }    
  if (!pImage->OffScreenInit(env, thisObj, background, 
                             pImage->GetWidth(), pImage->GetHeight())) {
    env->ThrowNew (WCachedIDs.OutOfMemoryErrorClass, NULL);      
    delete pImage;
  } 

}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_disposeImage(JNIEnv *env, 
                                                          jobject thisObj)
{
    
}


/* As the image is read in and passed through the java image filters this method
   gets called to store the pixels in the native image representation. The PPC
   pixbuf library is used to store the pixels in a pixmap (a server side off 
   screen image). The pixbuf library does the necessary scaling, alpha 
   transparency and dithering. Typically, this method is called for each
   line of the image so the whole image is not in memory at once. */
JNIEXPORT jboolean JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_setBytePixels(JNIEnv *env, 
                                                           jobject thisObj, 
                                                           jint x, jint y,
                                                           jint w, jint h,
                                                           jobject colorModel,
                                                           jbyteArray pixels,
                                                           jint offset, jint scansize)
{
  AwtImage *imgRep;
  jint m, n;		
  jint pixel;
  jbyte *pixelArray = NULL;
  
  jintArray rgbs = NULL;
  jint* rgbArray = NULL;

  imgRep = AwtImage::getAwtImageForSetPixels (env, thisObj, x, y, w, h,
                                              colorModel, pixels, offset,
                                              scansize);

  CriticalSection::Lock l(imageLock);
  
  if (imgRep == NULL) {
    return JNI_FALSE;
  }

  pixelArray = env->GetByteArrayElements(pixels, NULL);
  
  if (pixelArray == NULL) {
      goto finish;
  }	
  
  int red, green, blue, alpha;
  red = green = blue = alpha = 0;        

  imgRep->currentPtr = imgRep->pixBuffer + (imgRep->bmp_stride * y) + (x * BMP_CHANNELS);
  imgRep->currmaskPtr = imgRep->maskBuffer + (imgRep->mask_stride * y) +
      int((x+1)/8);

  /* Optimization to prevent callbacks into java for index color model. */
  if (env->IsInstanceOf (colorModel, WCachedIDs.
                         java_awt_image_IndexColorModelClass)) {
    
    jint rgb;
    rgbs = (jintArray)env->GetObjectField(colorModel, WCachedIDs.
                                          java_awt_image_IndexColorModel_rgbFID);
    
    if (rgbs == NULL) {
      goto finish;
    }
    
    rgbArray = env->GetIntArrayElements(rgbs, NULL);
    
    if (rgbArray == NULL) {
      goto finish;
    }    
    
    /* For each pixel in the supplied pixel array look up its rgb value 
     * from the colorModel. 
     */
    for (n = 0; n < h; n++) {
        for (m = 0; m < w; m++) {
            //Get the pixel at m, n.
            pixel = ((unsigned char)pixelArray[n * scansize + m + offset]);
            rgb = rgbArray[pixel];
            
            //Get and store red, green, blue values.             
            red = ((rgb & 0xff0000) >> 16);
            green = ((rgb & 0xff00) >> 8);
            blue = (rgb & 0xff);
            alpha = ((rgb >> 24) & 0xff);
            //alpha = rgb & 0xff000000;
            imgRep->SetDIBRGB(blue, green, red);                                    
            imgRep->SetMaskBuffer(alpha, m);            
        }          

        // skip to the end of the row if x + w is smaller than the actual width
        imgRep->skipToEndOfRow(x, w);

        // skip over imgRep->bmp_padding value
        imgRep->bmpPadding();

        // skip over imgRep->mask_padding value for Mask buffer
        imgRep->maskPadding();

        // skip parts of the beginning of the row if x > 0
        imgRep->skipBeginningOfRow(x); 

    }            
    
  }
  /* No optimization possible - call back into Java. */
    else {
        jint rgb;
        for (n = 0; n < h; n++) {
            for (m = 0; m < w; m++) {

                /* Get the pixel at m, n. */
                pixel = ((unsigned char)pixelArray[n * scansize + m + offset]);
                rgb = env->CallIntMethod (colorModel, 
					  WCachedIDs.java_awt_image_ColorModel_getRGBMID, pixel);
			
                if (env->ExceptionCheck()) {
                    goto finish;
                }
                
                /* Get and store red, green, blue values. */
                red = ((rgb & 0xff0000) >> 16);
                green = ((rgb & 0xff00) >> 8);
                blue = (rgb & 0xff);
                alpha = ((rgb >> 24) & 0xff);
                //alpha = rgb & 0xff000000;
                imgRep->SetDIBRGB(blue, green, red);            
                imgRep->SetMaskBuffer(alpha, m);
	    }  

            // skip to the end of the row if x + w is smaller than the actual width
            imgRep->skipToEndOfRow(x, w);

            // skip over imgRep->bmp_padding value
            imgRep->bmpPadding();

            // skip over imgRep->mask_padding value for Mask buffer
            imgRep->maskPadding();

            // skip parts of the beginning of the row if x > 0
            imgRep->skipBeginningOfRow(x); 

        }     
    
    }

 finish:	/* Perform tidying up code and return. */

    if (pixelArray != NULL)
        env->ReleaseByteArrayElements(pixels, pixelArray, JNI_ABORT);
    
    if (rgbArray != NULL)
        env->ReleaseIntArrayElements(rgbs, rgbArray, JNI_ABORT);
    
    return JNI_TRUE;
}

/* As the image is read in and passed through the java image filters this method
   gets called to store the pixels in the native image representation. The 
   PPC pixbuf library is used to store the pixels in a pixmap (a server side
   off screen image). The pixbuf library does the necessary scaling, alpha 
   transparency and dithering. Typically, this method is called for each
   line of the image so the whole image is not in memory at once. */
JNIEXPORT jboolean JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_setIntPixels(JNIEnv *env,
                                                          jobject thisObj,
                                                          jint x, jint y,
                                                          jint w, jint h,
                                                          jobject colorModel,
                                                          jintArray pixels,
                                                          jint offset, jint scansize)
{
    AwtImage *imgRep;
    /* Stores red, green, blue bytes for each pixel */

    /* x, y position in pixel array - same variable names 
       as Javadoc comment for setIntPixels. */
    jint m, n;				
    jint pixel;
    jint *pixelArray = NULL;
    jintArray rgbs = NULL;
    jint *rgbArray = NULL;

    imgRep = AwtImage::getAwtImageForSetPixels (env, thisObj, x, y, w, h,
                                                colorModel, pixels, offset,
                                                scansize);
    CriticalSection::Lock l(imageLock);

    if (imgRep == NULL) {
      return JNI_FALSE;
    }
    
    pixelArray = env->GetIntArrayElements (pixels, NULL);
  
    if (pixelArray == NULL)
	goto finish;  

    /* Optimization to prevent callbacks into java for direct color model. */
    jint alpha, red, green, blue;

    imgRep->currentPtr = imgRep->pixBuffer + (imgRep->bmp_stride * y) + (x * BMP_CHANNELS);
    imgRep->currmaskPtr = imgRep->maskBuffer + (imgRep->mask_stride * y) +
        int((x+1)/8);

    if (env->IsInstanceOf (colorModel, 
			   WCachedIDs.java_awt_image_DirectColorModelClass)) {	
	jint red_mask, red_offset, red_scale;
	jint green_mask, green_offset, green_scale;
	jint blue_mask, blue_offset, blue_scale;
	jint alpha_mask, alpha_offset, alpha_scale;
    
	red_mask = env->GetIntField(colorModel, WCachedIDs.
				    java_awt_image_DirectColorModel_red_maskFID);
	red_offset = env->GetIntField(colorModel, WCachedIDs.
				      java_awt_image_DirectColorModel_red_offsetFID);
	red_scale = env->GetIntField(colorModel, WCachedIDs.
				     java_awt_image_DirectColorModel_red_scaleFID);
    
	green_mask = env->GetIntField(colorModel, WCachedIDs.
				      java_awt_image_DirectColorModel_green_maskFID);
	green_offset = env->GetIntField(colorModel, WCachedIDs.
					java_awt_image_DirectColorModel_green_offsetFID);
	green_scale = env->GetIntField(colorModel, WCachedIDs.
				       java_awt_image_DirectColorModel_green_scaleFID);
    
	blue_mask = env->GetIntField(colorModel, WCachedIDs.
				     java_awt_image_DirectColorModel_blue_maskFID);
	blue_offset = env->GetIntField(colorModel, WCachedIDs.
				       java_awt_image_DirectColorModel_blue_offsetFID);
	blue_scale = env->GetIntField(colorModel, WCachedIDs.
				      java_awt_image_DirectColorModel_blue_scaleFID);
    
	alpha_mask = env->GetIntField(colorModel, WCachedIDs.
				      java_awt_image_DirectColorModel_alpha_maskFID);
	alpha_offset = env->GetIntField(colorModel, WCachedIDs.
					java_awt_image_DirectColorModel_alpha_offsetFID);
	alpha_scale = env->GetIntField(colorModel, WCachedIDs.
				       java_awt_image_DirectColorModel_alpha_scaleFID);
    
	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */
	for (n = 0; n < h; n++) {		
	    for (m = 0; m < w; m++) { /* Get the pixel at m, n. */
		pixel = pixelArray[n * scansize + m + offset];
            
		red = ((pixel & red_mask) >> red_offset);
            
		if (red_scale != 0)
		    red = red * 255 / red_scale;
            
		green = ((pixel & green_mask) >> green_offset);
            
		if (green_scale != 0)
		    green = green * 255 / green_scale;
            
		blue = ((pixel & blue_mask) >> blue_offset);
            
		if (blue_scale != 0)
		    blue = blue * 255 / blue_scale;
            
		if (alpha_mask == 0)
		    alpha = 255;
            
		else {          
		    alpha = ((pixel & alpha_mask) >> alpha_offset);
          
		    if (alpha_scale != 0)
			alpha = alpha * 255 / alpha_scale;
		}
                imgRep->SetDIBRGB(blue, green, red);            
                imgRep->SetMaskBuffer(alpha, m);
	    }  

            // skip to the end of the row if x + w is smaller than the actual width
            imgRep->skipToEndOfRow(x, w);

            // skip over imgRep->bmp_padding value
            imgRep->bmpPadding();

            // skip parts of the beginning of the row if x > 0
            imgRep->skipBeginningOfRow(x); 

            // skip over imgRep->mask_padding value for Mask buffer
            imgRep->maskPadding();
	}
        
    } /* Optimization to prevent callbacks into java for index color model. */
    else if (env->IsInstanceOf(colorModel, WCachedIDs.
			       java_awt_image_IndexColorModelClass)) {	
	jint rgb;
      
	rgbs = (jintArray)env->GetObjectField(colorModel, WCachedIDs.
					      java_awt_image_IndexColorModel_rgbFID);
      
	if (rgbs == NULL) {
	    goto finish;
	}
      
	rgbArray = env->GetIntArrayElements ( rgbs, NULL);
      
	if (rgbArray == NULL) {
	    goto finish;
	}
      
	/* For each pixel in the supplied pixel array look up 
	   its rgb value from the colorModel. */
	for (n = 0; n < h; n++) {
	    for (m = 0; m < w; m++) {
		/* Get the pixel at m, n. */
              
		pixel = pixelArray[n * scansize + m + offset];
		rgb = rgbArray[pixel];
              
		/* Get and store red, green, blue values. */
              
		red = ((rgb & 0xff0000) >> 16);
		green = ((rgb & 0xff00) >> 8);
		blue = (rgb & 0xff);
		alpha = ((rgb >> 24) & 0xff); 
                //alpha = rgb & 0xff000000;
                imgRep->SetDIBRGB(blue, green, red);   
                imgRep->SetMaskBuffer(alpha, m);
	    }  

            // skip to the end of the row if x + w is smaller than the actual width
            imgRep->skipToEndOfRow(x, w);

            // skip over imgRep->bmp_padding value
            imgRep->bmpPadding();

            // skip parts of the beginning of the row if x > 0
            imgRep->skipBeginningOfRow(x); 

            // skip over imgRep->mask_padding value for Mask buffer
            imgRep->maskPadding();
	}

    }
  
    /* No optimization possible - call back into Java. */
  
    else {    
	jint rgb;
            
	for (n = 0; n < h; n++) {	       
	    for (m = 0; m < w; m++) {
              
		/* Get the pixel at m, n. */        
		pixel = pixelArray[n * scansize + m + offset];
		rgb = env->CallIntMethod(colorModel, WCachedIDs.
					 java_awt_image_ColorModel_getRGBMID, 
					 pixel);
              
		if (env->ExceptionCheck ()) {
		    goto finish;
		}
              
		/* Get and store red, green, blue values. */              
		red = ((rgb & 0xff0000) >> 16);
		green = ((rgb & 0xff00) >> 8);
		blue = (rgb & 0xff);
		alpha = ((rgb >> 24) & 0xff); 
                //alpha = rgb & 0xff000000;
                imgRep->SetDIBRGB(blue, green, red);            
                imgRep->SetMaskBuffer(alpha, m);
	    }  

           // skip to the end of the row if x + w is smaller than the actual width
           imgRep->skipToEndOfRow(x, w);

           // skip over imgRep->bmp_padding value
           imgRep->bmpPadding();

           // skip parts of the beginning of the row if x > 0
           imgRep->skipBeginningOfRow(x); 

           // skip over imgRep->mask_padding value for Mask buffer
           imgRep->maskPadding();
	}

    }
  
 finish:	/* Perform tidying up code and return. */
  
    if (pixelArray != NULL)
	env->ReleaseIntArrayElements ( pixels, pixelArray, JNI_ABORT);
  
    if (rgbArray != NULL)
	env->ReleaseIntArrayElements ( rgbs, rgbArray, JNI_ABORT);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_finish(JNIEnv *env, jobject thisObj, 
                                               jboolean force)
{
    return JNI_FALSE;
}

/* Gets the native data for the image representation and the graphics object for
   drawing the image. Also checks to make sure the data is suitable for drawing 
   images and throws exceptions if there is a problem. This function is defined 
   so as to share as much code as possible between the two image drawing methods
   of ImageRepresentation (imageDraw and imageStretch). */

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_imageDraw(JNIEnv * env, 
                                                       jobject thisObj,
                                                       jobject g, 
                                                       jint x, 
                                                       jint y, 
                                                       jobject c)
{
  if (thisObj == NULL || g  == NULL) {
    env->ThrowNew (WCachedIDs.NullPointerExceptionClass, NULL);
    return;
  }

  jint availInfo = 
    env->GetIntField(thisObj,
                     WCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);

  //if ((availInfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO) {
  if ((availInfo & IMAGE_SIZEINFO) != IMAGE_SIZEINFO) {
    return;
  }
  CriticalSection::Lock l(imageLock);
  AwtGraphics *pGraphics = 
      (AwtGraphics *)env->GetIntField(g,
                                      WCachedIDs.PPCGraphics_pDataFID);
  
  AwtImage *pImage = 
      (AwtImage*)env->GetIntField(thisObj,
                                  WCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);
  
    if (pGraphics != NULL && pImage != NULL) {
        pGraphics->DrawImage(env, pImage, x, y, c);
    }

}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_imageStretch(JNIEnv *env, 
                                                          jobject thisObj,
                                                          jobject g, 	
                                                          jint dx1, jint dy1,
                                                          jint dx2, jint dy2,
                                                          jint sx1, jint sy1,
                                                          jint sx2, jint sy2,
                                                          jobject c)
{
  if (thisObj == NULL || g  == NULL) {
    env->ThrowNew (WCachedIDs.NullPointerExceptionClass, NULL);
    return;
  }

  jint availInfo = 
    env->GetIntField(thisObj,
                     WCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);

  //if ((availInfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO) {
  if ((availInfo & IMAGE_SIZEINFO) != IMAGE_SIZEINFO) {
    return;
  }
  CriticalSection::Lock l(imageLock);
  AwtGraphics *pGraphics = 
      (AwtGraphics *)env->GetIntField(g,
                                      WCachedIDs.PPCGraphics_pDataFID);
  
  AwtImage *pImage = 
      (AwtImage*)env->GetIntField(thisObj,
                                  WCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);
  
    if (pGraphics != NULL && pImage != NULL) {
        pGraphics->DrawImage(env, pImage,
			     dx1, dy1, dx2, dy2,
			     sx1, sy1, sx2, sy2,
			     c);
    }
    
}

JNIEXPORT void JNICALL
Java_sun_awt_image_OffScreenImageSource_sendPixels(JNIEnv * env, jobject thisObj)
{
    jobject imageRep = 
        env->GetObjectField(thisObj,
                            WCachedIDs.sun_awt_image_OffScreenImageSource_baseIRFID); 
    
    if (imageRep == NULL) {
        env->ThrowNew (WCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }
    
    jint availInfo = 
        env->GetIntField(imageRep,
                         WCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);
    if ((availInfo & IMAGE_OFFSCRNINFO) != IMAGE_OFFSCRNINFO) {
        return;
    }
    AwtImage *pImage = AwtImage::getAwtImageFromJNI(env,
                                                    imageRep,
                                                    NULL);		
    if (pImage == NULL) {
        env->ThrowNew (WCachedIDs.OutOfMemoryErrorClass, NULL);
	return;
    }
    pImage->sendPixels(env, thisObj);
}

inline jint AwtImage::WinToJavaPixel(USHORT b, USHORT g, USHORT r)
{
    jint value =
      0xFF << 24 | // alpha channel is always turned all the way up
      r << 16 |
      g << 8  |
      b << 0;
    return value;
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_getRGB (JNIEnv * env,
						     jobject thisObj,
						     jobject cm,
						     jint x,
						     jint y) 
{

  // Compute the x,y value from our DIB
  AwtImage *imgRep =  AwtImage::getAwtImageFromJNI(env, thisObj, NULL);
  
  imgRep->currentPtr = imgRep->pixBuffer + (imgRep->bmp_stride * y) + (x * BMP_CHANNELS);
  USHORT blue = *imgRep->currentPtr;
  imgRep->currentPtr++;
  USHORT green = *imgRep->currentPtr;
  imgRep->currentPtr++;
  USHORT red = *imgRep->currentPtr;
  imgRep->currentPtr++;
  
  jint value = AwtImage::WinToJavaPixel(blue, green, red);
  return value;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCImageRepresentation_getRGBs (JNIEnv * env,
						      jobject thisObj,
						      jobject cm,
						      jint startX,
						      jint startY,
						      jint w,
						      jint h,
						      jintArray rgbArray,
						      jint offset,
						      jint scansize) 
{
    jint *pixelArrayElements;
    AwtImage *imgRep =  AwtImage::getAwtImageFromJNI(env, thisObj, NULL);

	if (imgRep != NULL) {

	 if (startX < 0 || startY < 0 || (startX + w) > imgRep->GetWidth() || 
          (startY + h) > imgRep->GetHeight()) {
        env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass, 
           "Value out of bounds");
        return;
     }

  // Compute the x,y value from our DIB
     imgRep->currentPtr = imgRep->pixBuffer + 
      (imgRep->bmp_stride * startY) + (startX * BMP_CHANNELS);
     imgRep->currmaskPtr = imgRep->maskBuffer + (imgRep->mask_stride * startY) +
      int((startX+1)/8);
      if ((pixelArrayElements = 
           env->GetIntArrayElements(rgbArray,
                                    NULL)) == NULL) {
        env->ThrowNew(WCachedIDs.NullPointerExceptionClass, NULL);
        return;
      }

      for (int y = startY; y < startY + h; y++) {
        for (int x  = startX; x < startX + w; x++) {
          // RGB
          USHORT blue = *imgRep->currentPtr; 
          imgRep->currentPtr++;
          USHORT green = *imgRep->currentPtr;
          imgRep->currentPtr++;
          USHORT red = *imgRep->currentPtr;
          imgRep->currentPtr++;
          // Alpha
          int bitpos = (x % 8);
          if (x != 0 && bitpos == 0) {                              
            imgRep->currmaskPtr++;                        
          }                                            
          int alpha = *(imgRep->currmaskPtr);

          jint value = AwtImage::WinToJavaPixel(blue, green, red);
          pixelArrayElements[offset + (y - startY) * scansize +
                            (x - startX)] = value;
        }

        // skip to the end of the row if startX + w is smaller than the actual width
        imgRep->skipToEndOfRow(startX, w);

        // skip over imgRep->bmp_padding value
        imgRep->bmpPadding();

        // skip over imgRep->mask_padding value for Mask buffer
        imgRep->maskPadding();

        // skip parts of the beginning of the row if startX > 0
        imgRep->skipBeginningOfRow(startX);       

      }
      env->ReleaseIntArrayElements(rgbArray, pixelArrayElements, 0);
    } else {
      env->ThrowNew(WCachedIDs.NullPointerExceptionClass, NULL);
    }
}

