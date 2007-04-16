/*
 * 
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
 */
 

package com.sun.pisces;

public final class NativeSurface implements Surface {
    private class NativeSurfaceDestination implements SurfaceDestination {
        public void drawSurface(Surface ps, int srcX, int srcY, 
                int dstX, int dstY, int width, int height, float opacity) {
            int srcW = ps.getWidth();
            int srcH = ps.getHeight();
            int dstW = getWidth();
            int dstH = getHeight();
            
            if (srcX < 0) {
                dstX -= srcX;
                width += srcX;
                srcX = 0;
            }
            
            if (srcY < 0) {
                dstY -= srcY;
                height += srcY;
                srcY = 0;
            }
            
            if ((srcX + width) > srcW) {
                width = srcW - srcX;
            }
            
            if ((srcY + height) > srcH) {
                height = srcH - srcY;
            }

            if ((width < 0) || (height < 0) || (opacity == 0)) {
                return;
            }

            if (ps instanceof NativeSurface) {
                NativeSurface ns = (NativeSurface)ps;                
                drawRGBImpl(ns.getData(), srcY * srcW + srcX, srcW, dstX, dstY, 
                        width, height, opacity);
                return;
            }

            if (dstX < 0) {
                srcX -= dstX;
                width += dstX;
                dstX = 0;
            }
            
            if (dstY < 0) {
                srcY -= dstY;
                height += dstY;
                dstY = 0;
            }

            if ((dstX + width) > dstW) {
                width = dstW - dstX;
            }
            
            if ((dstY + height) > dstH) {
                height = dstH - dstY;
            }
            
            if ((width > 0) && (height > 0)) {
                int size = width * height;
                int[] srcRGB = new int[size];

                ps.getRGB(srcRGB, 0, width, srcX, srcY, width, height);
                drawRGBImpl(srcRGB, 0, width, dstX, dstY, width, height, 
                        opacity);
            }
        }
    
        public void drawRGB(int[] argb, int offset, int scanLength, 
                int x, int y, int width, int height, float opacity) {
            drawRGBImpl(argb, offset, scanLength, x, y, width, height, opacity);
        }
    }
    
    private final int width;
    private final int height;
    private final int[] data;

    public NativeSurface(int width, int height) {
        this(null, width, height);
    }
    
    public NativeSurface(int[] data, int width, int height) {
        this.data = (data != null) ? data : new int[width * height];
        this.width = width;
        this.height = height;
    }
    
    public SurfaceDestination createSurfaceDestination() {
        return new NativeSurfaceDestination();
    }

    public int getWidth() {
        return width;
    }
    
    public int getHeight() {
        return height;
    }
    
    public void getRGB(int[] argb, int offset, int scanLength, 
            int x, int y, int width, int height) {
        if ((argb == data) && (offset == 0) && (scanLength == this.width) &&
                (x == 0) && (y == 0) && 
                (width == this.width) && (height == this.height)) {
            return;
        }

        int dstX = 0;
        int dstY = 0;
        
        if (x < 0) {
            dstX -= x;
            width += x;
            x = 0;
        }

        if (y < 0) {
            dstY -= y;
            height += y;
            y = 0;
        }

        if ((x + width) > this.width) {
            width = this.width - x;
        }

        if ((y + height) > this.height) {
            height = this.height - y;
        }

        if ((width > 0) && (height > 0)) {
            offset += dstY * scanLength + dstX;    
            transfer(argb, offset, scanLength, 
                    data, y * this.width + x, this.width, 
                    width, height);
        }
    }
    
    public void setRGB(int[] argb, int offset, int scanLength, 
            int x, int y, int width, int height) {
        if ((argb == data) && (offset == 0) && (scanLength == this.width) &&
                (x == 0) && (y == 0) &&
                (width == this.width) && (height == this.height)) {
            return;
        }

        int srcX = 0;
        int srcY = 0;
        
        if (x < 0) {
            srcX -= x;
            width += x;
            x = 0;
        }

        if (y < 0) {
            srcY -= y;
            height += y;
            y = 0;
        }

        if ((x + width) > this.width) {
            width = this.width - x;
        }

        if ((y + height) > this.height) {
            height = this.height - y;
        }

        if ((width > 0) && (height > 0)) {
            offset += srcY * scanLength + srcX;    
            transfer(data, y * this.width + x, this.width,
                    argb, offset, scanLength, 
                    width, height);
        }
    }
    
    int[] getData() {
        return data;
    }
    
    private void drawRGBImpl(int[] argb, int offset, int scanLength, 
            int x, int y, int width, int height, float opacity) {
        int srcX = 0;
        int srcY = 0;
        
        if (x < 0) {
            srcX -= x;
            width += x;
            x = 0;
        }

        if (y < 0) {
            srcY -= y;
            height += y;
            y = 0;
        }

        if ((x + width) > this.width) {
            width = this.width - x;
        }

        if ((y + height) > this.height) {
            height = this.height - y;
        }

        if ((width > 0) && (height > 0)) {
            offset += srcY * scanLength + srcX;    
            paint(data, y * this.width + x, this.width,
                    argb, offset, scanLength, 
                    width, height, opacity);
        }
    }
    
    private static void transfer(int[] dstRGB, int dstOffset, int dstScanLength,
            int[] srcRGB, int srcOffset, int srcScanLength,
            int width, int height) {
        int srcScanRest = srcScanLength - width;
        int dstScanRest = dstScanLength - width;
        for (; height > 0; --height) {
            for (int w = width; w > 0; --w) {
                dstRGB[dstOffset++] = srcRGB[srcOffset++];
            }
            srcOffset += srcScanRest;
            dstOffset += dstScanRest;
        }
    }

    private static void paint(int[] dstRGB, int dstOffset, int dstScanLength,
            int[] srcRGB, int srcOffset, int srcScanLength,
            int width, int height, float opacity) {
        int srcScanRest = srcScanLength - width;
        int dstScanRest = dstScanLength - width;

        int op = (int)(0x100 * opacity);    
        for (; height > 0; --height) {
            for (int w = width; w > 0; --w) {
                int salpha = ((srcRGB[srcOffset] >> 24) & 0xff) * op;
                if (salpha == 0xff00) {
                    dstRGB[dstOffset++] = srcRGB[srcOffset++];        
                } else {
                    int dval = dstRGB[dstOffset];
                    int dalpha = (dval >> 24) & 0xff;
                    int anom = 255 * salpha;
                    int bnom = dalpha * (0xff00 - salpha);
                    int denom = anom + bnom;
                    if (denom > 0) {
                        long recip = ((long)1 << 32) / denom;
                        long fa = anom * recip;
                        long fb = bnom * recip;
                        int sval = srcRGB[srcOffset];
                        int oalpha = ((257 * denom) >> 24) & 0xff;
                        int ored = (int)((fa * ((sval >> 16) & 0xff) + 
                            fb * ((dval >> 16) & 0xff)) >> 32);
                        int ogreen = (int)((fa * ((sval >> 8) & 0xff) + 
                            fb * ((dval >> 8) & 0xff)) >> 32);
                        int oblue = (int)((fa * (sval & 0xff) + 
                            fb * (dval & 0xff)) >> 32);

                        dstRGB[dstOffset] = (oalpha << 24) | 
                                (ored << 16) | (ogreen << 8) | oblue;
                    }
                    ++srcOffset;
                    ++dstOffset;
                }
            }
            srcOffset += srcScanRest;
            dstOffset += dstScanRest;
        }
    }
}
