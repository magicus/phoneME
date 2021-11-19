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
#include<stdio.h>

#include <JAbstractSurface.h>

#include <KNIUtil.h>
#include <midpPiscesUtils.h>

#include <PiscesUtil.h>
#include <PiscesSysutils.h>
#include <commonKNIMacros.h>

#define SURFACE_NATIVE_PTR 0
#define SURFACE_GRAPHICS 1
#define SURFACE_LAST SURFACE_GRAPHICS

#define GRAPHICS_TRANSX 0
#define GRAPHICS_TRANSY 1
#define GRAPHICS_LAST GRAPHICS_TRANSY

static jboolean fieldIdsInitialized = KNI_FALSE;

static jfieldID graphicsFieldIds[GRAPHICS_LAST + 1];

static jboolean
initializeGDFieldIds() {
    static const FieldDesc graphicsFieldDesc[GRAPHICS_LAST + 2] = {
                                                       { "transX", "I" },
                                                       { "transY", "I" },
                                                       { NULL, NULL}
                                                      };            
    jboolean retVal;

    if (fieldIdsInitialized) {
        return KNI_TRUE;
    }
    
    retVal = KNI_FALSE;
    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHndl);
    KNI_FindClass("javax/microedition/lcdui/Graphics", graphicsHndl);
    if (!KNI_IsNullHandle(graphicsHndl) && 
        initializeFieldIds(graphicsFieldIds, graphicsHndl, graphicsFieldDesc)) {
        retVal = KNI_TRUE;
        fieldIdsInitialized = KNI_TRUE;
    }
    KNI_EndHandles();
    return retVal;
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_ImageExtender_drawImageInternal() {  
    int w, h, offset, xx, yy, x, y;
    int soffset;
    unsigned short rgb, srgb;
    int r, g, b;
    int dr, dg, db;
    int sr, sg, sb;
    int sa, sam;
    float opacity;

    VDC vdc, svdc;
    VDC* pVDC;
    _MidpImage * img;
    
    initializeGDFieldIds();
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(destinationHandle);
    KNI_DeclareHandle(imageHandle);
    
    opacity = KNI_GetParameterAsFloat(7);
    KNI_GetParameterAsObject(6, imageHandle);
    w = KNI_GetParameterAsInt(5);
    h = KNI_GetParameterAsInt(4);
    y = KNI_GetParameterAsInt(3);
    x = KNI_GetParameterAsInt(2);
    KNI_GetParameterAsObject(1, destinationHandle);


    pVDC = setupVDC(destinationHandle, &vdc);
    pVDC = getVDC(pVDC);

    x += KNI_GetIntField(destinationHandle, 
                            graphicsFieldIds[GRAPHICS_TRANSX]);
    y += KNI_GetIntField(destinationHandle, 
                            graphicsFieldIds[GRAPHICS_TRANSY]);
    
    // We need to acquire alphaData, because setupVDC had set it to NULL
    img = (_MidpImage *) getMidpGraphicsPtr(destinationHandle)->img;
        
    if (img != NULL && img->alphaData != NULL) {
       pVDC->alphaData = PISCES_GET_DATA_POINTER(img->alphaData);        
    }
    
    
    setupImageVDC(imageHandle, &svdc);
    if (svdc.alphaData != NULL && opacity != 0) {
        if (pVDC->alphaData != NULL) {
            for(yy = 0 ; yy < svdc.height; yy++) {
                for(xx = 0; xx < svdc.width; xx++) {
                    soffset = yy * svdc.width + xx;
                    sa = (int) (*(svdc.alphaData + soffset) & 0xff) * opacity + 0.5;
                    sam = 255 - sa;
                    if (sa != 0) {
                        offset = (yy + y) * pVDC->width + xx + x;
                        
                        srgb = *((unsigned short *) (svdc.hdc + soffset));
    
                        rgb = *((unsigned short *) (pVDC->hdc + offset));
                        
                        r =  rgb >> 11;
                        g = (rgb >> 5) & 0x3f;
                        b =  rgb &  0x1f;
                        
                        sr =  srgb >> 11;
                        sg = (srgb >> 5) & 0x3f;
                        sb =  srgb &  0x1f;
                        
                        dr = (sr * sa + sam * r) / (255);
                        db = (sb * sa + sam * b) / (255);
                        dg = (sg * sa + sam * g) / (255);
                         
                        *((unsigned short *) (pVDC->hdc + offset)) =
                        ((dr & 0x1f) << 11) + ((dg & 0x3f) << 5) + (db & 0x1f);
                        
                        *(pVDC->alphaData + offset) = sa;
                    }                        
                }
            }
        } else {
        
            for(yy = 0 ; yy < svdc.height; yy++) {
                for(xx = 0; xx < svdc.width; xx++) {
                    soffset = yy * svdc.width + xx;
                    sa = (int) (*(svdc.alphaData + soffset) & 0xff) * 
                            opacity + 0.5;
                    sam = 255 - sa;
                    if (sa != 0) {
                        offset = (yy + y) * pVDC->width + xx + x;
                        
                        srgb = *((unsigned short *) (svdc.hdc + soffset));
    
                        rgb = *((unsigned short *) (pVDC->hdc + offset));
                        
                        r =  rgb >> 11;
                        g = (rgb >> 5) & 0x3f;
                        b =  rgb &  0x1f;
                        
                        sr =  srgb >> 11;
                        sg = (srgb >> 5) & 0x3f;
                        sb =  srgb &  0x1f;
                        
                        dr = (sr * sa + sam * r) / (255);
                        db = (sb * sa + sam * b) / (255);
                        dg = (sg * sa + sam * g) / (255);
                         
                        *((unsigned short *) (pVDC->hdc + offset)) = 
                         ((dr & 0x1f) << 11) + ((dg & 0x3f) << 5) + (db & 0x1f);
                    }                        
                }
            }
        } 
    }
    else {
        for(yy = 0 ; yy < svdc.height; yy++) {
            for(xx = 0; xx < svdc.width; xx++) {
                offset = (yy + y) * pVDC->width + xx + x;
                *((unsigned short *) (pVDC->hdc + offset)) = 
                        *((unsigned short *) (svdc.hdc + yy * svdc.width + xx));    
            }
        }
    }
    KNI_EndHandles();
    KNI_ReturnVoid();    
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_ImageExtender_cleanImageInternal() {          
    int x;
    int count, alpha_count;
    VDC svdc;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(imageHandle);

    KNI_GetParameterAsObject(1, imageHandle);
    
    setupImageVDC(imageHandle, &svdc);
    
    if (svdc.hdc != NULL) {
        count = svdc.height * svdc.width;
        for (x = 0; x < count; x++) {
            *((unsigned short *) (svdc.hdc + x)) = 0; 
        }
        if (svdc.alphaData != NULL) {
            alpha_count = svdc.height * svdc.width;
            count = (svdc.height * svdc.width) % 4;
            for (x = 0; x < alpha_count; x++) {
                *((char *) (svdc.alphaData + x)) = 0;
            }
        }
    }
    
    KNI_EndHandles();
    KNI_ReturnVoid();    
}
