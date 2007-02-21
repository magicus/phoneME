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


#ifndef PISCES_RENDERER_H
#define PISCES_RENDERER_H

#include <PiscesDefs.h>
#include <PiscesSurface.h>
#include <PiscesPipelines.h>
#include <PiscesTransform.h>

#define TYPE_INT_RGB 1
#define TYPE_INT_ARGB 2
#define TYPE_USHORT_565_RGB 8
#define TYPE_BYTE_GRAY 10
#define WIND_NON_ZERO 1
#define WIND_EVEN_ODD 0

#define PAINT_FLAT_COLOR 0
#define PAINT_LINEAR_GRADIENT 1
#define PAINT_RADIAL_GRADIENT 2
#define PAINT_TEXTURE 3

#define LG_GRADIENT_MAP_SIZE 8
#define GRADIENT_MAP_SIZE (1 << LG_GRADIENT_MAP_SIZE)

#define DEFAULT_INDICES_SIZE 8192
#define DEFAULT_CROSSINGS_SIZE (32*1024)
#define NUM_ALPHA_ROWS 8
#define MIN_QUAD_OPT_WIDTH (100 << 16)

#define CYCLE_NONE 0
#define CYCLE_REPEAT 1
#define CYCLE_REFLECT 2

typedef struct _Renderer {
  jint _windingRule;

  // Flat color or (Java2D) linear gradient
  jint _paintMode;

  // Current color 
  jint _cred, _cgreen, _cblue, _calpha;
  
  // Gradient paint
  jint _lgradient_x0;
  jint _lgradient_y0;
  jint _lgradient_x1;
  jint _lgradient_y1;
  jint _lgradient_c0;
  jint _lgradient_c1;
  jint _lgradient_cyclic;

  jfloat _lgradient_fx0;
  jfloat _lgradient_fy0;
  jfloat _lgradient_fx1;
  jfloat _lgradient_fy1;

  /* Gradient delta for one pixel step in X */
  jfloat _lgradient_dgdx;
  /* Gradient delta for one pixel step in Y */
  jfloat _lgradient_dgdy;

  /*
   * Color and alpha for gradient value g is located in
   * color map at index (int)(g*scale + bias)
   */
  jfloat _lgradient_scale;
  jfloat _lgradient_bias;

  jint _lgradient_color_565[GRADIENT_MAP_SIZE];
  jint _lgradient_color_888[GRADIENT_MAP_SIZE];
  jint _lgradient_color_8  [GRADIENT_MAP_SIZE];

  // Antialiasing
  jint _SUBPIXEL_LG_POSITIONS_X;
  jint _SUBPIXEL_LG_POSITIONS_Y;
  jint _SUBPIXEL_MASK_X;
  jint _SUBPIXEL_MASK_Y;
  jint _SUBPIXEL_POSITIONS_X;
  jint _SUBPIXEL_POSITIONS_Y;
  jint _MAX_AA_ALPHA;
  jint _MAX_AA_ALPHA_DENOM;
  jint _HALF_MAX_AA_ALPHA_DENOM;
  jint _XSHIFT;
  jint _YSHIFT;
  jint _YSTEP;
  jint _HYSTEP;
  jint _YMASK;

  jint _alphaMap[16*16 + 1];

  jboolean _antialiasingOn;

  // Current composite rule
  jint _compositeRule;

  // Bounding boxes
  jint _boundsMinX, _boundsMinY, _boundsMaxX, _boundsMaxY;
  jint _rasterMinX, _rasterMaxX, _rasterMinY, _rasterMaxY;
  jint _bboxX0, _bboxY0, _bboxX1, _bboxY1;

  // Image layout 
  void *_data;
  jint *_intData;
  jshort *_shortData;
  jbyte *_byteData;
  jint _width, _height;
  jint _imageOffset;
  jint _imageScanlineStride;
  jint _imagePixelStride;
  jint _imageType;
  jboolean _allocated;

  jint _scrOrient;

  void (*_bl_SO)(struct _Renderer *rdr, jint height);
  void (*_bl_PT_SO)(struct _Renderer *rdr, jint height);

  void (*_clearRect)(struct _Renderer *rdr, jint x, jint y, jint w, jint h);
  void (*_emitRows)(struct _Renderer *rdr, jint height);
  void (*_genPaint)(struct _Renderer *rdr, jint height);

  // Edge list
  jint *_edges;
  jint _edges_length;
  jint _edgeIdx;
  jint _edgeMinY;
  jint _edgeMaxY;

  // Oval points
  jint *_ovalPoints;
  jint _ovalPoints_length;

  // AA buffer
  jbyte *_rowAA;
  jint _rowAA_length;
  jint _rowAAOffset;
  jint _rowNum;
  jint _alphaWidth;
  jint _minTouched[NUM_ALPHA_ROWS];
  jint _maxTouched[NUM_ALPHA_ROWS];
  jint _currX, _currY;
  jint _currImageOffset;

  // Paint buffer
  jint *_paint;
  jint _paint_length;

  // Gradient transform
  Transform6 _gradient_transform;
  Transform6 _gradient_inverse_transform;

  // New-style linear gradient geometry
  jint _lg_x0, _lg_y0, _lg_x1, _lg_y1; // Raw coordinates
  jlong _lg_mx, _lg_my, _lg_b;         // g(x, y) = x*mx + y*my + b

  // Radial gradient geometry
  jint _rg_cx, _rg_cy, _rg_fx, _rg_fy, _rg_radius;

  // Gradient color map
  jint _gradient_colors[GRADIENT_MAP_SIZE];
  jint _gradient_cycleMethod;

  // X crossing tables
  jint *_crossings;
  jint _crossings_length;
  jint *_crossingIndices;
  jint _crossingIndices_length;
  jint _crossingMinY;
  jint _crossingMaxY;
  jint _crossingMinX;
  jint _crossingMaxX;
  jint _crossingMaxXEntries;
  jint _numCrossings;
  jboolean _crossingsSorted;

  // Crossing iterator
  jint _crossingY;
  jint _crossingRowCount;
  jint _crossingRowOffset;
  jint _crossingRowIndex;
  
  // Current drawing position, i.e., final point of last segment
  jint _x0, _y0; 

  // Position of most recent 'moveTo' command
  jint _sx0, _sy0;

  // Track the number of vertical extrema of the incoming edge list
  // in order to determine the maximum number of crossings of a
  // scanline
  jint _firstOrientation;
  jint _lastOrientation;
  jint _flips;
  
  // Texture paint
  jint* _texture_intData;
  jint _texture_imageWidth;
  jint _texture_imageHeight;
  jint _texture_stride;
  jboolean _texture_repeat;
  jlong _texture_m00, _texture_m01, _texture_m02;
  jlong _texture_m10, _texture_m11, _texture_m12;
  jint _texture_wmask, _texture_hmask;
  jboolean _texture_interpolate;

  // Current bounding box for all primitives
  jint _clip_bbMinX;
  jint _clip_bbMinY;
  jint _clip_bbMaxX;
  jint _clip_bbMaxY;
 
  jboolean _inSubpath;
  jboolean _isPathFilled;
  
  jint _lineWidth;
  jint _capStyle;
  jint _joinStyle;
  jint _miterLimit;
  jint* _dashArray;
  jint _dashArray_length;
  jint _dashPhase;

  Transform6 _transform;

  jboolean _validFiller;
  jboolean _validStroker;

  Pipeline* _defaultPipeline;
  Pipeline _fillerPipeline;
  Pipeline _strokerPipeline;
  
} Renderer; 

jboolean renderer_moduleInitialize();
void renderer_moduleFinalize();

void prenderer_moveTo(Pipeline* pipeline, jint x0, jint y0);
void prenderer_lineJoin(Pipeline* pipeline);
void prenderer_lineTo(Pipeline* pipeline, jint x1, jint y1);
void prenderer_close(Pipeline* pipeline);
void prenderer_end(Pipeline* pipeline);

#endif
