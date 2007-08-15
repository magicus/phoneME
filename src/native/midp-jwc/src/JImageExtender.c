/*
 * 
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
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
    unsigned char sa, sam;
    float opacity;
    gxj_screen_buffer *sbuf;
    gxj_screen_buffer screen_buffer;
    unsigned short * srcData;
    unsigned char * srcAlpha;
    
    _MidpImage * img;
    java_imagedata * imgsrc;
    java_graphics * gr;

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
    
    gr = GXAPI_GET_GRAPHICS_PTR(destinationHandle);
    
    if (gr != NULL) {
            sbuf = gxj_get_image_screen_buffer_impl(
                        (gr != NULL && gr->img != NULL)?gr->img->imageData:NULL, 
                        &screen_buffer, destinationHandle);
             sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);
    } else {
       sbuf = NULL;
    }    
    

    x += KNI_GetIntField(destinationHandle, 
                            graphicsFieldIds[GRAPHICS_TRANSX]);
    y += KNI_GetIntField(destinationHandle, 
                          graphicsFieldIds[GRAPHICS_TRANSY]);
    
    img= IMGAPI_GET_IMAGE_PTR(imageHandle);
    imgsrc = img->imageData;
    
    srcData = (unsigned short *) &imgsrc->pixelData->elements[0];
    srcAlpha = (unsigned char *) &imgsrc->alphaData->elements[0];

    if (srcAlpha != NULL && opacity != 0) {
        if (sbuf->alphaData != NULL) {
            for(yy = 0 ; yy < imgsrc->height; yy++) {
                for(xx = 0; xx < imgsrc->width; xx++) {
                    soffset = yy * imgsrc->width + xx;
                    sa =  srcAlpha[soffset] * opacity + 0.5;
                    sam = 255 - sa;
                    if (sa != 0) {
                      offset = (yy + y) * sbuf->width + xx + x;
                        
                        srgb = *((unsigned short *) (srcData + soffset));
    
                        rgb = *((unsigned short *) sbuf->pixelData + offset);
                        
                        r =  rgb >> 11;
                        g = (rgb >> 5) & 0x3f;
                        b =  rgb &  0x1f;
                        
                        sr =  srgb >> 11;
                        sg = (srgb >> 5) & 0x3f;
                        sb =  srgb &  0x1f;
                        
                        dr = (sr * sa + sam * r) / (255);
                        db = (sb * sa + sam * b) / (255);
                        dg = (sg * sa + sam * g) / (255);
                         
                        *((unsigned short *) sbuf->pixelData + offset) = 
                         ((dr & 0x1f) << 11) + ((dg & 0x3f) << 5) + (db & 0x1f);
                        *((unsigned char *) sbuf->alphaData + offset) = sa;
                    }                        
                }
            }
        } else {
            for(yy = 0 ; yy < imgsrc->height; yy++) {
                for(xx = 0; xx < imgsrc->width; xx++) {
                    soffset = yy * imgsrc->width + xx;
                    sa = (int) ((*((unsigned char *)imgsrc->alphaData->elements + 
                            soffset)) & 0xff) * opacity + 0.5;
                    sam = 255 - sa;
                    if (sa != 0)
                      {
                        offset = (yy + y) * sbuf->width + xx + x;
                        
                        srgb = *((unsigned short *) 
                                    imgsrc->pixelData->elements + soffset);
    
                        rgb = *((unsigned short *) sbuf->pixelData + offset);
                        
                        r =  rgb >> 11;
                        g = (rgb >> 5) & 0x3f;
                        b =  rgb &  0x1f;
                        
                        sr =  srgb >> 11;
                        sg = (srgb >> 5) & 0x3f;
                        sb =  srgb &  0x1f;
                        
                        dr = (sr * sa + sam * r) / (255);
                        db = (sb * sa + sam * b) / (255);
                        dg = (sg * sa + sam * g) / (255);
                         
                        *((unsigned short *) sbuf->pixelData + offset) =                                                               ((dr & 0x1f) << 11) + ((dg & 0x3f) << 5) + (db & 0x1f);
                    }                        
                }
            }
        } 
    }
    else {
        for(yy = 0 ; yy < imgsrc->height; yy++) {
            for(xx = 0; xx < imgsrc->width; xx++) {
                offset = (yy + y) * sbuf->width + xx + x;
                *((unsigned short *) (sbuf->pixelData + offset)) =
                  *((unsigned short *) (imgsrc->pixelData + yy * imgsrc->width + 
                                        xx));    
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
    java_imagedata * imgdst;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(imageHandle);

    KNI_GetParameterAsObject(1, imageHandle);

    imgdst = IMGAPI_GET_IMAGE_PTR(imageHandle)->imageData;
    
    if (imgdst->pixelData->elements) {
        count = imgdst->height * imgdst->width;
        for (x = 0; x < count; x++) {
            *((unsigned short *) imgdst->pixelData->elements + x) = 0; 
        }
        if (imgdst->alphaData != NULL && imgdst->alphaData->elements != NULL) {
            alpha_count = imgdst->height * imgdst->width;
            for (x = 0; x < alpha_count; x++) {
                *((char *) imgdst->alphaData->elements + x) = 0;
            }
        }
    }
    
    KNI_EndHandles();
    KNI_ReturnVoid();
}
