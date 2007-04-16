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

public class Blit {

    public static final int COMPOSITE_SRC_OVER = 1;

    private static final int TYPE_INT_RGB =
        Renderer.TYPE_INT_RGB;
    private static final int TYPE_INT_ARGB =
        Renderer.TYPE_INT_ARGB;
    private static final int TYPE_USHORT_565_RGB =
        Renderer.TYPE_USHORT_565_RGB;
    private static final int TYPE_BYTE_GRAY =
        Renderer.TYPE_BYTE_GRAY;

    private static final int MAX_ALPHA = 256;
    private static final int HALF_ALPHA = MAX_ALPHA >> 1;
    private static final int MIN_ALPHA = 0;
    private static final int ALPHA_SHIFT = 8;

    // Lookup tables to convert 5 or 6 bit color values to 8 bits
    // with proper rounding
    public static int[] convert8To5;
    public static int[] convert8To6;

    static {
        convert8To5 = new int[256];
        convert8To6 = new int[256];

        for (int i = 0; i < 256; i++) {
            convert8To5[i] = (i*31 + 127)/255;
            convert8To6[i] = (i*63 + 127)/255;
        }
    }

    // 8-bit blend against a constant RGB color
    private static void blend888(int[] intData, int iidx,
                                 int aval,
                                 int cred, int cgreen, int cblue) {
        int ival = intData[iidx];
        int red = (ival >> 16) & 0xff;
        int green = (ival >> 8) & 0xff;
        int blue = ival & 0xff;

        int nred = (red << ALPHA_SHIFT) +
            (cred - red)*aval + HALF_ALPHA;
        nred >>= ALPHA_SHIFT;
  
        int ngreen = (green << ALPHA_SHIFT) +
            (cgreen - green)*aval + HALF_ALPHA;
        ngreen >>= ALPHA_SHIFT;
  
        int nblue = (blue << ALPHA_SHIFT) +
            (cblue - blue)*aval + HALF_ALPHA;
        nblue >>= ALPHA_SHIFT;
  
        ival = 0xff000000 | (nred << 16) | (ngreen << 8) | nblue;
        intData[iidx] = ival;
    }


    // 8-bit blend against an ARGB color
    private static void blend8888(int[] intData, int iidx,
                                  int aval,
                                  int sred, int sgreen, int sblue) {
        int ival = intData[iidx];
        int dalpha = (ival >> 24) & 0xff;
        int dred = (ival >> 16) & 0xff;
        int dgreen = (ival >> 8) & 0xff;
        int dblue = ival & 0xff;

        int denom = 256*dalpha + aval*(255 - dalpha);
        if (denom == 0) {
            // dalpha and aval must both be 0
            // The output is transparent black
            intData[iidx] = 0x00000000;
        } else {
            long recip = (1L << 24)/denom;
            long fa = (256 - aval)*dalpha*recip;
            long fb = 255*aval*recip;
            int oalpha = denom >> 8;
            int ored = (int)((fa*dred + fb*sred) >> 24);
            int ogreen = (int)((fa*dgreen + fb*sgreen) >> 24);
            int oblue = (int)((fa*dblue + fb*sblue) >> 24);

            ival = (oalpha << 24) | (ored << 16) | (ogreen << 8) | oblue;
            intData[iidx] = ival;

//          float fsalpha = aval/256.0f; // N.B.: 256 not 255
//          float fsred = sred/255.0f;
//          float fsgreen = sgreen/255.0f;
//          float fsblue = sblue/255.0f;
//          float fdalpha = dalpha/255.0f;
//          float fdred = dred/255.0f;
//          float fdgreen = dgreen/255.0f;
//          float fdblue = dblue/255.0f;
//          float fsred_pre = fsred*fsalpha;
//          float fsgreen_pre = fsgreen*fsalpha;
//          float fsblue_pre = fsblue*fsalpha;
//          float fdred_pre = fdred*fdalpha;
//          float fdgreen_pre = fdgreen*fdalpha;
//          float fdblue_pre = fdblue*fdalpha;
//          float foalpha = fsalpha + fdalpha*(1.0f - fsalpha);
//          float fored_pre = fsred_pre + fdred_pre*(1.0f - fsalpha);
//          float fogreen_pre = fsgreen_pre + fdgreen_pre*(1.0f - fsalpha);
//          float foblue_pre = fsblue_pre + fdblue_pre*(1.0f - fsalpha);
//          float fored = fored_pre/foalpha;
//          float fogreen = fogreen_pre/foalpha;
//          float foblue = foblue_pre/foalpha;
//          int oalpha2 = (int)(foalpha*255.0f);
//          int ored2 = (int)(fored*255.0f);
//          int ogreen2 = (int)(fogreen*255.0f);
//          int oblue2 = (int)(foblue*255.0f);
            
//          if ((Math.abs(oalpha2 - oalpha) > 1) ||
//              (Math.abs(ored2 - ored) > 1) ||
//              (Math.abs(ogreen2 - ogreen) > 1) ||
//              (Math.abs(oblue2 - oblue) > 1)) {
//              System.out.println("\naval = " + aval);
//              System.out.println("sred = " + sred);
//              System.out.println("sgreen = " + sgreen);
//              System.out.println("sblue = " + sblue);
//              System.out.println("dalpha = " + dalpha);
//              System.out.println("dred = " + dred);
//              System.out.println("dgreen = " + dgreen);
//              System.out.println("dblue = " + dblue);

//              System.out.println("oalpha = " + oalpha);
//              System.out.println("oalpha2 = " + oalpha2);
//              System.out.println("ored = " + ored);
//              System.out.println("ored2 = " + ored2);
//              System.out.println("ogreen = " + ogreen);
//              System.out.println("ogreen2 = " + ogreen2);
//              System.out.println("oblue = " + oblue);
//              System.out.println("oblue2 = " + oblue2);
//          }
      }
    }

//     public static void main(String[] args) {
//      int[] intData = new int[1];
//      java.util.Random r = new java.util.Random();

//      System.out.println("Testing general case");
//      for (int i = 0; i < 10000000; i++) {
//          int da = r.nextInt(256);
//          int dr = r.nextInt(256);
//          int dg = r.nextInt(256);
//          int db = r.nextInt(256);

//          int aval = r.nextInt(257);
//          int sr = r.nextInt(256);
//          int sg = r.nextInt(256);
//          int sb = r.nextInt(256);
            
//          intData[0] = (da << 24) | (dr << 16) | (dg << 8) | db;
//          blend8888(intData, 0, aval, sr, sg, sb);
//      }

//      System.out.println("Testing aval = 256, da = 255");
//      for (int i = 0; i < 10000000; i++) {
//          int da = r.nextInt(256);
//          int dr = r.nextInt(256);
//          int dg = r.nextInt(256);
//          int db = r.nextInt(256);

//          int aval = r.nextInt(257);
//          int sr = r.nextInt(256);
//          int sg = r.nextInt(256);
//          int sb = r.nextInt(256);
            
//          aval = 256;
//          da = 255;       

//          intData[0] = (da << 24) | (dr << 16) | (dg << 8) | db;
//          blend8888(intData, 0, aval, sr, sg, sb);
//      }

//      System.out.println("Testing aval = 256, da = 0");
//      for (int i = 0; i < 10000000; i++) {
//          int da = r.nextInt(256);
//          int dr = r.nextInt(256);
//          int dg = r.nextInt(256);
//          int db = r.nextInt(256);

//          int aval = r.nextInt(257);
//          int sr = r.nextInt(256);
//          int sg = r.nextInt(256);
//          int sb = r.nextInt(256);
            
//          aval = 256;
//          da = 0;         

//          intData[0] = (da << 24) | (dr << 16) | (dg << 8) | db;
//          blend8888(intData, 0, aval, sr, sg, sb);
//      }

//      System.out.println("Testing aval = 0, da = 255");
//      for (int i = 0; i < 10000000; i++) {
//          int da = r.nextInt(256);
//          int dr = r.nextInt(256);
//          int dg = r.nextInt(256);
//          int db = r.nextInt(256);

//          int aval = r.nextInt(257);
//          int sr = r.nextInt(256);
//          int sg = r.nextInt(256);
//          int sb = r.nextInt(256);
            
//          aval = 0;
//          da = 255;       

//          intData[0] = (da << 24) | (dr << 16) | (dg << 8) | db;
//          blend8888(intData, 0, aval, sr, sg, sb);
//      }

//      System.out.println("Testing aval = 0, da = 0");
//      for (int i = 0; i < 10000000; i++) {
//          int da = r.nextInt(256);
//          int dr = r.nextInt(256);
//          int dg = r.nextInt(256);
//          int db = r.nextInt(256);

//          int aval = r.nextInt(257);
//          int sr = r.nextInt(256);
//          int sg = r.nextInt(256);
//          int sb = r.nextInt(256);
            
//          aval = 0;
//          da = 0;         

//          intData[0] = (da << 24) | (dr << 16) | (dg << 8) | db;
//          blend8888(intData, 0, aval, sr, sg, sb);
//      }
//      }

    private static void blend565(short[] shortData, int iidx,
                                 int aval,
                                 int cred5, int cgreen6, int cblue5) {
        short sval = shortData[iidx];
        int red = (sval >> 11) & 0x1f;
        int green = (sval >> 5) & 0x3f;
        int blue = sval & 0x1f;

        int nred = (red << ALPHA_SHIFT) +
            (cred5 - red)*aval + HALF_ALPHA;
        nred >>= ALPHA_SHIFT;
  
        int ngreen = (green << ALPHA_SHIFT) +
            (cgreen6 - green)*aval + HALF_ALPHA;
        ngreen >>= ALPHA_SHIFT;
  
        int nblue = (blue << ALPHA_SHIFT) +
            (cblue5 - blue)*aval + HALF_ALPHA;
        nblue >>= ALPHA_SHIFT;
  
        sval = (short)((nred << 11) | (ngreen << 5) | nblue);
        shortData[iidx] = sval;
    }

    private static void blend8(byte[] byteData, int iidx,
                               int aval, int cgray) {
        int gray = byteData[iidx] & 0xff;

        int ngray = (gray << ALPHA_SHIFT) +
            (cgray - gray)*aval + HALF_ALPHA;
        ngray >>= ALPHA_SHIFT;

        byteData[iidx] = (byte)ngray;
    }

    private static void blitSrcOver888(int[] intData,
                                       int imageOffset,
                                       int imageScanlineStride,
                                       int imagePixelStride,
                                       byte[] alpha,
                                       int alphaOffset,
                                       int width, int height,
                                       int[] minTouched, int[] maxTouched,
                                       int[] rowOffsets,
                                       int cred, int cgreen, int cblue,
                                       int calpha, int[] alphaMap) {
        int cval = 0xff000000 | (cred << 16) | (cgreen << 8) | cblue;

        for (int j = 0; j < height; j++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];

            int aidx = alphaOffset + rowOffsets[j] + minX;
            int iidx = imageOffset + minX*imagePixelStride;
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }

            for (int i = 0; i < w; i++, aidx++, iidx += imagePixelStride) {
                int aval = alphaMap[alpha[aidx] & 0xff];
                if (aval == 0) {
                    continue;
                } else if (aval == MAX_ALPHA) {
                    intData[iidx] = cval;
                } else {
                    blend888(intData, iidx, aval, cred, cgreen, cblue);
                }
            }

            imageOffset += imageScanlineStride;
        }
    }

    private static void blitSrcOver8888(int[] intData,
                                        int imageOffset,
                                        int imageScanlineStride,
                                        int imagePixelStride,
                                        byte[] alpha,
                                        int alphaOffset,
                                        int width, int height,
                                        int[] minTouched, int[] maxTouched,
                                        int[] rowOffsets,
                                        int cred, int cgreen, int cblue,
                                        int calpha, int[] alphaMap) {
        int cval = 0xff000000 | (cred << 16) | (cgreen << 8) | cblue;

        for (int j = 0; j < height; j++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];

            int aidx = alphaOffset + rowOffsets[j] + minX;
            int iidx = imageOffset + minX*imagePixelStride;
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }

            for (int i = 0; i < w; i++, aidx++, iidx += imagePixelStride) {
                int aval = alphaMap[alpha[aidx] & 0xff];
                if (aval == 0) {
                    continue;
                } else if (aval == MAX_ALPHA) {
                    intData[iidx] = cval;
                } else {
                    blend8888(intData, iidx, aval, cred, cgreen, cblue);
                }
            }

            imageOffset += imageScanlineStride;
        }
    }

    private static void blitSrcOver565(short[] shortData,
                                       int imageOffset,
                                       int imageScanlineStride,
                                       int imagePixelStride,
                                       byte[] alpha,
                                       int alphaOffset,
                                       int width, int height,
                                       int[] minTouched, int[] maxTouched,
                                       int[] rowOffsets,
                                       int cred5, int cgreen6, int cblue5,
                                       int calpha, int[] alphaMap) {
        short cval = (short)((cred5 << 11) | (cgreen6 << 5) | (cblue5));
        
        for (int j = 0; j < height; j++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];

            int aidx = alphaOffset + rowOffsets[j] + minX;
            int iidx = imageOffset + minX*imagePixelStride;
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }

            for (int i = 0; i < w; i++, aidx++, iidx += imagePixelStride) {
                int aval = alphaMap[alpha[aidx] & 0xff];
                if (aval == MIN_ALPHA) {
                    continue;
                } else if (aval == MAX_ALPHA) {
                    shortData[iidx] = cval;
                } else {
                    blend565(shortData, iidx, aval, cred5, cgreen6, cblue5);
                }
            }

            imageOffset += imageScanlineStride;
        }
    }

    private static void blitSrcOver8(byte[] byteData,
                                     int imageOffset,
                                     int imageScanlineStride,
                                     int imagePixelStride,
                                     byte[] alpha,
                                     int alphaOffset,
                                     int width, int height,
                                     int[] minTouched, int[] maxTouched,
                                     int[] rowOffsets,
                                     int cgray,
                                     int calpha, int[] alphaMap) {
        byte cval = (byte)cgray;

        for (int j = 0; j < height; j++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];

            int aidx = alphaOffset + rowOffsets[j] + minX;
            int iidx = imageOffset + minX*imagePixelStride;
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }
            
            for (int i = 0; i < w; i++, aidx++, iidx += imagePixelStride) {
                int aval = alphaMap[alpha[aidx] & 0xff];
                if (aval == 0) {
                    continue;
                } else if (aval == MAX_ALPHA) {
                    byteData[iidx] = cval;
                } else {
                    blend8(byteData, iidx, aval, cgray);
                }
            }

            imageOffset += imageScanlineStride;
        }
    }

    // blit 888 w/ paint
    private static void blitSrcOver888(int[] intData,
                                       int imageOffset,
                                       int imageScanlineStride,
                                       int imagePixelStride,
                                       byte[] alpha,
                                       int alphaOffset,
                                       int width, int height,
                                       int[] minTouched, int[] maxTouched,
                                       int[] rowOffsets,
                                       int[] paintData,
                                       int paintOffset,
                                       int paintScanlineStride,
                                       int[] alphaMap) {
        for (int j = 0; j < height; j++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];

            int aidx = alphaOffset + rowOffsets[j] + minX;
            int pidx = paintOffset + minX;
            int iidx = imageOffset + minX*imagePixelStride;
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }
            
            for (int i = 0; i < w;
                 i++, aidx++, pidx++, iidx += imagePixelStride) {
                int aval = alphaMap[alpha[aidx] & 0xff];

                // Combine AA alpha with paint alpha
                // IMPL NOTE : This could be sped up somehow...
                int paint = paintData[pidx];
                int calpha = (paint >> 24) & 0xff;
                aval = (aval*calpha + 127)/255;

                if (aval == 0) {
                    continue;
                } else if (aval == MAX_ALPHA) {
                    // Force output alpha to 1
                    intData[iidx] = paintData[pidx] | 0xff000000;
                } else {
                    int cred = (paint >> 16) & 0xff;
                    int cgreen = (paint >> 8) & 0xff;
                    int cblue = paint & 0xff;
                    
                    blend888(intData, iidx, aval, cred, cgreen, cblue);
                }
            }

            paintOffset += paintScanlineStride;
            imageOffset += imageScanlineStride;
        }
    }

    // blit 8888 w/ paint
    private static void blitSrcOver8888(int[] intData,
                                        int imageOffset,
                                        int imageScanlineStride,
                                        int imagePixelStride,
                                        byte[] alpha,
                                        int alphaOffset,
                                        int width, int height,
                                        int[] minTouched, int[] maxTouched,
                                        int[] rowOffsets,
                                        int[] paintData,
                                        int paintOffset,
                                        int paintScanlineStride,
                                        int[] alphaMap) {
        for (int j = 0; j < height; j++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];

            int aidx = alphaOffset + rowOffsets[j] + minX;
            int pidx = paintOffset + minX;
            int iidx = imageOffset + minX*imagePixelStride;
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }
            
            for (int i = 0; i < w;
                 i++, aidx++, pidx++, iidx += imagePixelStride) {
                int aval = alphaMap[alpha[aidx] & 0xff];

                // Combine AA alpha with paint alpha
                // IMPL NOTE : This could be sped up somehow...
                int paint = paintData[pidx];
                int calpha = (paint >> 24) & 0xff;
                aval = (aval*calpha + 127)/255;

                if (aval == 0) {
                    continue;
                } else if (aval == MAX_ALPHA) {
                    intData[iidx] = paintData[pidx];
                } else {
                    int cred = (paint >> 16) & 0xff;
                    int cgreen = (paint >> 8) & 0xff;
                    int cblue = paint & 0xff;
                    
                    blend8888(intData, iidx, aval, cred, cgreen, cblue);
                }
            }

            paintOffset += paintScanlineStride;
            imageOffset += imageScanlineStride;
        }
    }

    // blit 565 w/ paint
    private static void blitSrcOver565(short[] shortData,
                                       int imageOffset,
                                       int imageScanlineStride,
                                       int imagePixelStride,
                                       byte[] alpha,
                                       int alphaOffset,
                                       int width, int height,
                                       int[] minTouched, int[] maxTouched,
                                       int[] rowOffsets,
                                       int[] paintData,
                                       int paintOffset,
                                       int paintScanlineStride,
                                       int[] alphaMap) {
        for (int j = 0; j < height; j++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];

            int aidx = alphaOffset + rowOffsets[j] + minX;
            int pidx = paintOffset + minX;
            int iidx = imageOffset + minX*imagePixelStride;
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }

            for (int i = 0; i < w;
                 i++, aidx++, pidx++, iidx += imagePixelStride) {
                int aval = alphaMap[alpha[aidx] & 0xff];
                int paint = paintData[pidx];
                int calpha = (paint >> 24) & 0xff;
                // IMPL NOTE : This could be sped up somehow...
                aval = (aval*calpha + 127)/255;
                
                if (aval == MIN_ALPHA) {
                    continue;
                }
                
                int cred5 = convert8To5[(paint >> 16) & 0xff];
                int cgreen6 = convert8To6[(paint >> 8) & 0xff];
                int cblue5 = convert8To5[paint & 0xff];

                if (aval == MAX_ALPHA) {
                    shortData[iidx] =
                        (short)((cred5 << 11) | (cgreen6 << 5) | cblue5);
                } else {
                    blend565(shortData, iidx, aval, cred5, cgreen6, cblue5);
                }
            }

            paintOffset += paintScanlineStride;
            imageOffset += imageScanlineStride;
        }
    }

    // blit 8 w/ paint
    private static void blitSrcOver8(byte[] byteData,
                                     int imageOffset,
                                     int imageScanlineStride,
                                     int imagePixelStride,
                                     byte[] alpha,
                                     int alphaOffset,
                                     int width, int height,
                                     int[] minTouched, int[] maxTouched,
                                     int[] rowOffsets,
                                     int[] paintData,
                                     int paintOffset,
                                     int paintScanlineStride,
                                     int[] alphaMap) {
        for (int j = 0; j < height; j++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];

            int aidx = alphaOffset + rowOffsets[j] + minX;
            int pidx = paintOffset + minX;
            int iidx = imageOffset + minX*imagePixelStride;
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }
            
            for (int i = 0; i < w;
                 i++, aidx++, pidx++, iidx += imagePixelStride) {
                int aval = alphaMap[alpha[aidx] & 0xff];
                int paint = paintData[pidx];
                int calpha = (paint >> 24) & 0xff;
                // IMPL NOTE : This could be sped up somehow...
                aval = (aval*calpha + 127)/255;

                if (aval == MIN_ALPHA) {
                    continue;
                }

                int cred = (paint >> 16) & 0xff;
                int cgreen = (paint >> 8) & 0xff;
                int cblue = paint & 0xff;

                // gray = .3*red + .59*green + .11*blue
                int cgray = (19961*cred + 38666*cgreen + 7209*cblue) >> 16;

                if (aval == MAX_ALPHA) {
                    byteData[iidx] = (byte)cgray;
                } else {
                    blend8(byteData, iidx, aval, cgray);
                }
            }

            paintOffset += paintScanlineStride;
            imageOffset += imageScanlineStride;
        }
    }

    private static void blendLine(Object imageData, int imageType,
                                  int imageOffset,
                                  int imageStride,
                                  int length,
                                  int alpha, int red, int green, int blue) {
        if (imageType == TYPE_INT_RGB) {
            int[] intData = (int[])imageData;
            for (int i = 0; i < length; i++) {
                blend888(intData, imageOffset, alpha, red, green, blue);
                imageOffset += imageStride;
            }
        } else if (imageType == TYPE_INT_ARGB) {
            int[] intData = (int[])imageData;
            for (int i = 0; i < length; i++) {
                blend8888(intData, imageOffset, alpha, red, green, blue);
                imageOffset += imageStride;
            }
        } else if (imageType == TYPE_USHORT_565_RGB) {
            short[] shortData = (short[])imageData;
            for (int i = 0; i < length; i++) {
                blend565(shortData, imageOffset, alpha, red, green, blue);
                imageOffset += imageStride;
            }
        } else if (imageType == TYPE_USHORT_565_RGB) {
            byte[] byteData = (byte[])imageData;
            for (int i = 0; i < length; i++) {
                blend8(byteData, imageOffset, alpha, red);
                imageOffset += imageStride;
            }
        }
    }
    
    // Assumes x0, y1, x1, y1 are in supersampled coordinate space
    //
    // All are >= 0
    // x0, x1 < width*SUBPIXEL_POSITIONS_X,
    // y0, y1 < height*SUBPIXEL_POSITIONS_Y
    
    public static void fillRectSrcOver(Renderer rdr,
                                       Object imageData, int imageType,
                                       int imageOffset,
                                       int imageScanlineStride,
                                       int imagePixelStride,
                                       int width, int height,
                                       int x0, int y0, int x1, int y1,
                                       int red, int green, int blue) {
        int SUBPIXEL_LG_POSITIONS_X = rdr.getSubpixelLgPositionsX();
        int SUBPIXEL_LG_POSITIONS_Y = rdr.getSubpixelLgPositionsY();
        int SUBPIXEL_MASK_X = (1 << (SUBPIXEL_LG_POSITIONS_X)) - 1;
        int SUBPIXEL_MASK_Y = (1 << (SUBPIXEL_LG_POSITIONS_Y)) - 1;
        int SUBPIXEL_POSITIONS_X = 1 << (SUBPIXEL_LG_POSITIONS_X);
        int SUBPIXEL_POSITIONS_Y = 1 << (SUBPIXEL_LG_POSITIONS_Y);

        int xmask_l = SUBPIXEL_POSITIONS_X - (x0 & SUBPIXEL_MASK_X);
        if (xmask_l == SUBPIXEL_POSITIONS_X) {
            xmask_l = 0;
        } else {
            x0 += SUBPIXEL_POSITIONS_X;
        }
        int xmask_r = x1 & SUBPIXEL_MASK_X;
        
        int ymask_t = SUBPIXEL_POSITIONS_X - (y0 & SUBPIXEL_MASK_X);
        if (ymask_t == SUBPIXEL_POSITIONS_Y) {
            ymask_t = 0;
        } else {
            y0 += SUBPIXEL_POSITIONS_Y;
        }
        int ymask_b = y1 & SUBPIXEL_MASK_X;
        
        int ix0 = x0 >> SUBPIXEL_LG_POSITIONS_X;
        int ix1 = (x1 >> SUBPIXEL_LG_POSITIONS_X) - 1;
        int iy0 = y0 >> SUBPIXEL_LG_POSITIONS_Y;
        int iy1 = (y1 >> SUBPIXEL_LG_POSITIONS_Y) - 1;

        width = ix1 - ix0 + 1;
        height = iy1 - iy0 + 1;

        int[] intData = null;
        short[] shortData = null;
        byte[] byteData = null;
        int intVal = 0;
        short shortVal = (short)0;
        byte byteVal = (byte)0;

        if (imageType == TYPE_INT_RGB || imageType == TYPE_INT_ARGB) {
            intData = (int[])imageData;
            intVal = 0xff000000 | (red << 16) | (green << 8) | blue;
        } else if (imageType == TYPE_USHORT_565_RGB) {
            shortData = (short[])imageData;
            shortVal = (short)((red << 11) | (green << 5) | (blue));
        } else if (imageType == TYPE_BYTE_GRAY) {
            byteData = (byte[])imageData;
            int gray = (19961*red + 38666*green + 7209*blue) >> 16;
            red = green = blue = gray;
            byteVal = (byte)gray;
        }

        if (width > 0 && height > 0) {
            int offset = imageOffset +
                iy0*imageScanlineStride +
                ix0*imagePixelStride;
            if (intData != null) {
                for (int j = 0; j < height; j++) {
                    int iidx = offset;
                    for (int i = 0; i < width; i++) {
                        intData[iidx] = intVal;
                        iidx += imagePixelStride;
                    }
                    offset += imageScanlineStride;
                }
            } else if (shortData != null) {
                for (int j = 0; j < height; j++) {
                    int iidx = offset;
                    for (int i = 0; i < width; i++) {
                        shortData[iidx] = shortVal;
                        iidx += imagePixelStride;
                    }
                    offset += imageScanlineStride;
                }
            } else if (byteData != null) {
                for (int j = 0; j < height; j++) {
                    int iidx = offset;
                    for (int i = 0; i < width; i++) {
                        byteData[iidx] = byteVal;
                        iidx += imagePixelStride;
                    }
                    offset += imageScanlineStride;
                }
            }
        }

        int RECT_LR_SIDE_ALPHA_SHIFT =
            ALPHA_SHIFT - SUBPIXEL_LG_POSITIONS_X;
        int RECT_TB_SIDE_ALPHA_SHIFT =
            ALPHA_SHIFT - SUBPIXEL_LG_POSITIONS_Y;
        int RECT_CORNER_ALPHA_SHIFT = ALPHA_SHIFT -
            (SUBPIXEL_LG_POSITIONS_X + SUBPIXEL_LG_POSITIONS_Y);

        if (xmask_l != 0) {
            int offset = imageOffset +
                iy0*imageScanlineStride +
                (ix0 - 1)*imagePixelStride;
            int alpha = xmask_l << RECT_LR_SIDE_ALPHA_SHIFT;
            blendLine(imageData, imageType, offset,
                      imageScanlineStride, height,
                      alpha, red, green, blue);
        }

        if (xmask_r != 0) {
            int offset = imageOffset +
                iy0*imageScanlineStride +
                (ix1 + 1)*imagePixelStride;
            int alpha = xmask_r << RECT_LR_SIDE_ALPHA_SHIFT;
            blendLine(imageData, imageType, offset,
                      imageScanlineStride, height,
                      alpha, red, green, blue);
        }

        if (ymask_t != 0) {
            int offset = imageOffset +
                (iy0 - 1)*imageScanlineStride +
                ix0*imagePixelStride;
            int alpha = ymask_t << RECT_TB_SIDE_ALPHA_SHIFT;
            blendLine(imageData, imageType, offset,
                      imagePixelStride, width,
                      alpha, red, green, blue);
        }

        if (ymask_b != 0) {
            int offset = imageOffset +
                (iy1 + 1)*imageScanlineStride +
                ix0*imagePixelStride;
            int alpha = ymask_b << RECT_TB_SIDE_ALPHA_SHIFT;
            blendLine(imageData, imageType, offset,
                      imagePixelStride, width,
                      alpha, red, green, blue);
        }
        
        int alphaUL = xmask_l*ymask_t << RECT_CORNER_ALPHA_SHIFT;
        int alphaUR = xmask_r*ymask_t << RECT_CORNER_ALPHA_SHIFT;
        int alphaLL = xmask_l*ymask_b << RECT_CORNER_ALPHA_SHIFT;
        int alphaLR = xmask_r*ymask_b << RECT_CORNER_ALPHA_SHIFT;

        switch (imageType) {
        case TYPE_INT_RGB:
            if (alphaUL > 0) {
                blend888(intData,
                         imageOffset +
                         (iy0 - 1)*imageScanlineStride +
                         (ix0 - 1)*imagePixelStride,
                         alphaUL, red, green, blue);
            }
            if (alphaUR > 0) {
                blend888(intData,
                         imageOffset + (iy0 - 1)*imageScanlineStride +
                         (ix1 + 1)*imagePixelStride,
                         alphaUR, red, green, blue);
            }
            if (alphaLL > 0) {
                blend888(intData,
                         imageOffset +
                         (iy1 + 1)*imageScanlineStride +
                         (ix0 - 1)*imagePixelStride,
                         alphaLL, red, green, blue);
            }
            if (alphaLR > 0) {
                blend888(intData,
                         imageOffset +
                         (iy1 + 1)*imageScanlineStride +
                         (ix1 + 1)*imagePixelStride,
                         alphaLR, red, green, blue);
            }
            break;

        case TYPE_INT_ARGB:
            if (alphaUL > 0) {
                blend8888(intData,
                          imageOffset +
                          (iy0 - 1)*imageScanlineStride +
                          (ix0 - 1)*imagePixelStride,
                          alphaUL, red, green, blue);
            }
            if (alphaUR > 0) {
                blend8888(intData,
                          imageOffset + (iy0 - 1)*imageScanlineStride +
                          (ix1 + 1)*imagePixelStride,
                          alphaUR, red, green, blue);
            }
            if (alphaLL > 0) {
                blend8888(intData,
                          imageOffset +
                          (iy1 + 1)*imageScanlineStride +
                          (ix0 - 1)*imagePixelStride,
                          alphaLL, red, green, blue);
            }
            if (alphaLR > 0) {
                blend8888(intData,
                          imageOffset +
                          (iy1 + 1)*imageScanlineStride +
                          (ix1 + 1)*imagePixelStride,
                          alphaLR, red, green, blue);
            }
            break;

        case TYPE_USHORT_565_RGB:
            if (alphaUL > 0) {
                blend565(shortData,
                         imageOffset +
                         (iy0 - 1)*imageScanlineStride + 
                         (ix0 - 1)*imagePixelStride,
                         alphaUL, red, green, blue);
            }
            if (alphaUR > 0) {
                blend565(shortData,
                         imageOffset +
                         (iy0 - 1)*imageScanlineStride +
                         (ix1 + 1)*imagePixelStride,
                         alphaUR, red, green, blue);
            }
            if (alphaLL > 0) {
                blend565(shortData,
                         imageOffset +
                         (iy1 + 1)*imageScanlineStride +
                         (ix0 - 1)*imagePixelStride,
                         alphaLL, red, green, blue);
            }
            if (alphaLR > 0) {
                blend565(shortData,
                         imageOffset +
                         (iy1 + 1)*imageScanlineStride +
                         (ix1 + 1)*imagePixelStride,
                         alphaLR, red, green, blue);
            }
            break;

        case TYPE_BYTE_GRAY:
            if (alphaUL > 0) {
                blend8(byteData,
                       imageOffset +
                       (iy0 - 1)*imageScanlineStride + 
                       (ix0 - 1)*imagePixelStride,
                       alphaUL, red);
            }
            if (alphaUR > 0) {
                blend8(byteData,
                         imageOffset +
                         (iy0 - 1)*imageScanlineStride +
                         (ix1 + 1)*imagePixelStride,
                         alphaUR, red);
            }
            if (alphaLL > 0) {
                blend8(byteData,
                         imageOffset +
                         (iy1 + 1)*imageScanlineStride +
                         (ix0 - 1)*imagePixelStride,
                         alphaLL, red);
            }
            if (alphaLR > 0) {
                blend8(byteData,
                       imageOffset +
                       (iy1 + 1)*imageScanlineStride +
                       (ix1 + 1)*imagePixelStride,
                       alphaLR, red);
            }
            break;
        }
    }

    public static void blit(Object imageData, int imageType,
                            int imageOffset,
                            int imageScanlineStride,
                            int imagePixelStride,
                            byte[] alphaData,
                            int alphaOffset,
                            int width, int height,
                            int[] minTouched, int[] maxTouched,
                            int[] rowOffsets,
                            int compositeRule,
                            int red, int green, int blue,
                            int alpha, int[] alphaMap) {
        switch (compositeRule) {
        case COMPOSITE_SRC_OVER:
            switch (imageType) {
            case TYPE_INT_RGB:
                blitSrcOver888((int[])imageData, imageOffset,
                               imageScanlineStride, imagePixelStride,
                               alphaData, alphaOffset,
                               width, height,
                               minTouched, maxTouched, rowOffsets,
                               red, green, blue,
                               alpha, alphaMap);
                return;

            case TYPE_INT_ARGB:
                blitSrcOver8888((int[])imageData, imageOffset,
                                imageScanlineStride, imagePixelStride,
                                alphaData, alphaOffset,
                                width, height,
                                minTouched, maxTouched, rowOffsets,
                                red, green, blue,
                                alpha, alphaMap);
                return;

            case TYPE_USHORT_565_RGB:
                blitSrcOver565((short[])imageData, imageOffset,
                               imageScanlineStride, imagePixelStride,
                               alphaData, alphaOffset,
                               width, height,
                               minTouched, maxTouched, rowOffsets,
                               red, green, blue,
                               alpha, alphaMap);
                return;

            case TYPE_BYTE_GRAY:
                int gray = (int)(0.3f*red + 0.59f*green + 0.11f*blue + 0.5f);
                blitSrcOver8((byte[])imageData, imageOffset,
                             imageScanlineStride, imagePixelStride,
                             alphaData, alphaOffset,
                             width, height,
                             minTouched, maxTouched, rowOffsets,
                             gray,
                             alpha, alphaMap);
                return;
            }
            break;

        default:
            throw new RuntimeException("Unknown composite rule!");
        }

        throw new RuntimeException("Unknown composite/type combination!");
    }

    public static void blit(Object imageData, int imageType,
                            int imageOffset,
                            int imageScanlineStride,
                            int imagePixelStride,
                            byte[] alphaData,
                            int alphaOffset,
                            int width, int height,
                            int[] minTouched, int[] maxTouched,
                            int[] rowOffsets,
                            int compositeRule,
                            int[] paintData,
                            int paintOffset, int paintScanlineStride,
                            int[] alphaMap) {
        switch (compositeRule) {

        case COMPOSITE_SRC_OVER:
            switch (imageType) {
            case TYPE_INT_RGB:
                blitSrcOver888((int[])imageData, imageOffset,
                               imageScanlineStride, imagePixelStride,
                               alphaData, alphaOffset,
                               width, height,
                               minTouched, maxTouched, rowOffsets,
                               paintData, paintOffset,
                               paintScanlineStride,
                               alphaMap);
                return;

            case TYPE_INT_ARGB:
                blitSrcOver8888((int[])imageData, imageOffset,
                                imageScanlineStride, imagePixelStride,
                                alphaData, alphaOffset,
                                width, height,
                                minTouched, maxTouched, rowOffsets,
                                paintData, paintOffset,
                                paintScanlineStride,
                                alphaMap);
                return;

            case TYPE_USHORT_565_RGB:
                blitSrcOver565((short[])imageData, imageOffset,
                               imageScanlineStride, imagePixelStride,
                               alphaData, alphaOffset,
                               width, height,
                               minTouched, maxTouched, rowOffsets,
                               paintData, paintOffset,
                               paintScanlineStride,
                               alphaMap);
                return;
                
            case TYPE_BYTE_GRAY:
                blitSrcOver8((byte[])imageData, imageOffset,
                             imageScanlineStride, imagePixelStride,
                             alphaData, alphaOffset,
                             width, height,
                             minTouched, maxTouched, rowOffsets,
                             paintData, paintOffset,
                             paintScanlineStride,
                             alphaMap);
                return;
            }
            
        default:
            throw new RuntimeException("Unknown composite rule!");
        }
    }
    
    private static void clearRect8888(int[] intData, int imageOffset, 
            int imageScanlineStride, int imagePixelStride,
            int x, int y, int width, int height,
            int calpha, int cred, int cgreen, int cblue) {
        int cval = (calpha << 24) | (cred << 16) | (cgreen << 8) | cblue;
        int scanlineSkip = imageScanlineStride - width * imagePixelStride;
        int iidx = imageOffset + y * imageScanlineStride + x * imagePixelStride;
        
        for (; height > 0; --height) {
            for (int w = width; w > 0; --w) {
                intData[iidx] = cval;
                iidx += imagePixelStride;
            }
            iidx += scanlineSkip;
        }
    }

    private static void clearRect565(short[] shortData, int imageOffset, 
            int imageScanlineStride, int imagePixelStride,
            int x, int y, int width, int height,
            int calpha, int cred, int cgreen, int cblue) {
        short cval = (short)((convert8To5[cred] << 11) | 
                (convert8To6[cgreen] << 5) | convert8To5[cblue]);
        int scanlineSkip = imageScanlineStride - width * imagePixelStride;
        int iidx = imageOffset + y * imageScanlineStride + x * imagePixelStride;
        
        for (; height > 0; --height) {
            for (int w = width; w > 0; --w) {
                shortData[iidx] = cval;
                iidx += imagePixelStride;
            }
            iidx += scanlineSkip;
        }
    }

    private static void clearRect8(byte[] byteData, int imageOffset, 
            int imageScanlineStride, int imagePixelStride,
            int x, int y, int width, int height,
            int calpha, int cred, int cgreen, int cblue) {
        byte cval = (byte)((19961 * cred + 
                38666 * cgreen + 7209 * cblue) >> 16);
        int scanlineSkip = imageScanlineStride - width * imagePixelStride;
        int iidx = imageOffset + y * imageScanlineStride + x * imagePixelStride;
        
        for (; height > 0; --height) {
            for (int w = width; w > 0; --w) {
                byteData[iidx] = cval;
                iidx += imagePixelStride;
            }
            iidx += scanlineSkip;
        }
    }
    
    public static void clearRect(Object imageData, int imageType, 
            int imageOffset, int imageScanlineStride, int imagePixelStride,
            int x, int y, int width, int height,
            int alpha, int red, int green, int blue) {
        switch (imageType) {
            case TYPE_INT_RGB:
            case TYPE_INT_ARGB:
                clearRect8888((int[])imageData, imageOffset, 
                        imageScanlineStride, imagePixelStride, 
                        x, y, width, height, 
                        alpha, red, green, blue);
                break;
            case TYPE_USHORT_565_RGB:
                clearRect565((short[])imageData, imageOffset, 
                        imageScanlineStride, imagePixelStride, 
                        x, y, width, height, 
                        alpha, red, green, blue);
                break;
            case TYPE_BYTE_GRAY:
                clearRect8((byte[])imageData, imageOffset, 
                        imageScanlineStride, imagePixelStride, 
                        x, y, width, height, 
                        alpha, red, green, blue);
                break;
        }
    }
}
