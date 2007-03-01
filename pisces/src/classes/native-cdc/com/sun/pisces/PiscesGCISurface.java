/* 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.pisces;

import com.sun.me.gci.surface.GCIDrawingSurface;
import com.sun.me.gci.surface.GCISurfaceInfo;

/**
 * Wrapper for GCIDrawingSurface for Pisces rendering. 
 * @see     PiscesShapeRenderer 
 */ 
public final class PiscesGCISurface extends AbstractSurface {
    /** 
     * Pointer to native image data. Accessed from native code.
     * This field is used when GCISurface stores image data in a buffer allocated
     * by native code.         
     */
    private long nativeArray; 
       
    /**
     * Java image data arrays for different image-types. GCIDrawingSurface uses
     * different Java arrays to store data. The real data-type used depends on
     * image-type.        
     * Accessed from native code. 
     */
    private int[]   javaArrayInt;
    private short[] javaArrayShort;
    private byte[]  javaArrayByte;
    
    /** 
     * This property says which of previously declared data arrays
     * (nativeArray, javaArrayInt, javaArrayShort, javaArrayByte) is
     * currently used to store image-data. This is set in acquireSurface()
     * accordingly to underlying GCIDrawingSurface properties. The value 
     * is also used in native code for acquirement of appropriate image memory
     * from java. Values must be one of the following TYPE_OF_ARRAY_. 
     * Another value is errorneous.
     */
    private int typeOfArray; 
    
    /** 
     * Field's @typeOfArray possible values. 
     * Corresponding #defines are used in native code. If you modify them here, 
     * please, make relevant changes to JPiscesGCISurface.c also.
     */   
    public static final int TYPE_OF_ARRAY_NATIVE     =  0;
    public static final int TYPE_OF_ARRAY_JAVA_INT   =  1;
    public static final int TYPE_OF_ARRAY_JAVA_SHORT =  2;
    public static final int TYPE_OF_ARRAY_JAVA_BYTE  =  3;
    
    /** 
     * Underlying GCIDrawingSurface. This is destination of our rendering.    
     */
    private final GCIDrawingSurface gciSurface;

    /**
     * The GCISurfaceInfo of gciSurface held between calls to acquireSurface
     * and releaseSurface.
     */          
    private GCISurfaceInfo gciSurfaceInfo;

    private final int imageType;
    /**
     * How much to right-shift value in bits to get value in pixels.
     */         
    private final int bitsToPixelsShift;
    
    private int offset;
    private int scanlineStride;
    private int pixelStride;
    
    /** 
     * Instantiates wrapper class of GCIDrawingSurface for Pisces rendering.
     * It acquires all needed information from surface and ensures 
     * initialization of native pisces surface structures.          
     * @param surface 
     * @see GCIDrawingSurface            
     */
    public PiscesGCISurface(GCIDrawingSurface surface) {
        gciSurface = surface;
        
        switch (surface.getFormat()) {
            case GCIDrawingSurface.FORMAT_RGB_888:
            case GCIDrawingSurface.FORMAT_XRGB_8888:
            case GCIDrawingSurface.FORMAT_ARGB_8888:
                imageType = RendererBase.TYPE_INT_RGB;
                bitsToPixelsShift = 5;  // 2 ^ 5 = 32
                break;
            case GCIDrawingSurface.FORMAT_RGB_565:
                imageType = RendererBase.TYPE_USHORT_565_RGB;
                bitsToPixelsShift = 4; // 2 ^ 4 = 16
                break;
            case GCIDrawingSurface.FORMAT_GRAY_8:
                imageType = RendererBase.TYPE_BYTE_GRAY;
                bitsToPixelsShift = 3; // 2 ^ 3 = 8
                break;
            case GCIDrawingSurface.FORMAT_ARGB_8888_PRE:
                imageType = RendererBase.TYPE_INT_ARGB_PRE;
                bitsToPixelsShift = 5; // 2 ^ 5 = 32
                break;
            default:
                throw new IllegalArgumentException("Pisces does not support"
                        + " " + surface.getFormat() + " yet") ;
        }

        int width = surface.getWidth();
        int height = surface.getHeight();

        if (surface.isSurfaceInfoDynamic()) {
            initialize(imageType, width, height, true);
        } else {
            // acquire surface once
            acquireSurface();
            initialize(imageType, width, height, false);
        }
    }

    /** 
     * Gets all neccessary information from GCIDrawingSurface.
     * Anytime surface changes at runtime. This method must be called to ensure      
     * pisces and GCI surfaces synchronization.
     */ 
    public void acquireSurface() {
        if (gciSurfaceInfo != null) {
            // we have already acquired the surface
            return;
        }
        gciSurfaceInfo = gciSurface.getSurfaceInfo();    
        if (gciSurfaceInfo == null) {
            throw new RuntimeException("Unable to get surface information");
        }

        offset = gciSurfaceInfo.getBitOffset() >> bitsToPixelsShift;
        scanlineStride = 
                gciSurfaceInfo.getYBitStride() >> bitsToPixelsShift;
        pixelStride =
                gciSurfaceInfo.getXBitStride() >> bitsToPixelsShift; 
        
        if (gciSurface.isNativeSurface()) {
            typeOfArray = TYPE_OF_ARRAY_NATIVE;
            nativeArray = gciSurfaceInfo.getBasePointer();
        } else {            
            switch (gciSurfaceInfo.getPixelArrayType()) {
                case GCISurfaceInfo.TYPE_JAVA_ARRAY_INT:
                    javaArrayInt = (int[])gciSurfaceInfo.getPixelArray();
                    typeOfArray = TYPE_OF_ARRAY_JAVA_INT;
                    break;
                case GCISurfaceInfo.TYPE_JAVA_ARRAY_SHORT:
                    javaArrayShort = (short[])gciSurfaceInfo.getPixelArray();
                    typeOfArray = TYPE_OF_ARRAY_JAVA_SHORT;
                    break;    
                case GCISurfaceInfo.TYPE_JAVA_ARRAY_BYTE:
                    javaArrayByte = (byte[])gciSurfaceInfo.getPixelArray();
                    typeOfArray = TYPE_OF_ARRAY_JAVA_BYTE;
                    break;    
                default:
                    break;    
            }
        }
    }
    
    /** 
     * Detaches (releases) underlying GCIDrawing surface and resets Pisces
     * native surface structures. No rendering is possible after 
     * releaseSurface().     
     */       
    public void releaseSurface(){
        if (gciSurfaceInfo != null) {
            // allow garbage collection
            javaArrayByte = null;
            javaArrayInt = null;
            javaArrayShort = null;

            nativeArray = 0;

            gciSurfaceInfo.release();
            gciSurfaceInfo = null;
        }
    }
    
    /**
     * This method initializes (C-native-code) underlying pisces 
     * surface/renderer structures. 
     * @see JPiscesGCISurface.c source on details.    
     */
    private native void initialize(int imageType, int width, int height, 
                                   boolean isDynamic);
}
