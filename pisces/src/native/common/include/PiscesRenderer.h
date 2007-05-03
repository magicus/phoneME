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


/**
 *  \file PiscesRenderer.h
 *  Renderer struct declaration. This file is the main PISCES include. I.e. all
 *  PISCES wrappers, should include this file. It provides C - high level API.  
 */
 
#ifndef PISCES_RENDERER_H
#define PISCES_RENDERER_H

#include <PiscesDefs.h>
#include <PiscesSurface.h>
#include <PiscesPipelines.h>
#include <PiscesTransform.h>

/**
 * @defgroup CompositingRules Compositing rules supported by PISCES 
 * When drawing two objects to one pixel area, there are several possibilities
 * how composite color is made of source and destination contributions.
 * Objects can overlap pixel fully and/or partialy. One object could be above
 * the second one and they both can be partialy or fully transparent (alpha).  
 * The way, we count composite color and alpha from theirs contributions is
 * called composite rule (Porter-Duff).
 * @def COMPOSITE_CLEAR
 * @ingroup CompositingRules 
 * Compositing rule COMPOSITE_CLEAR. This rule applied to destination pixel sets 
 * its color to 0x00000000 - transparent black - regardless to source color.
 * @see renderer_setCompositeRule(Renderer *, jint), 
 * renderer_setComposite(Renderer *, jint, jfloat) 
 * @def COMPOSITE_SRC
 * @ingroup CompositingRules  
 * Compositing rule COMPOSITE_SRC. This rule applied to destination pixel sets 
 * its color to source color - regardless to previous color of destination 
 * pixel.
 * @see renderer_setCompositeRule(Renderer *, jint), 
 * renderer_setComposite(Renderer *, jint, jfloat)  
 * @def COMPOSITE_SRC_OVER
 * @ingroup CompositingRules 
 * Compositing rule COMPOSITE_SRC_OVER. This rule is kind of intuitive. When we
 * look through transparent green glass bottle at some object, we can see 
 * mixture of glass and objects colors. Composite color is alpha-weigth average 
 * of source and destination.
 * @see renderer_setCompositeRule(Renderer *, jint), 
 * renderer_setComposite(Renderer *, jint, jfloat)    
 */
//Compositing rules
#define COMPOSITE_CLEAR    0
#define COMPOSITE_SRC      1
#define COMPOSITE_SRC_OVER 2

/**
 * @defgroup WindingRules Winding rules - shape interior  
 * Winding rule determines what part of shape is determined as interior. This is 
 * important to determine what part of shape to fill. 
 * @def WIND_EVEN_ODD
 * @ingroup WindingRules
 * This define represents the even-odd winding rule. To see how this winding 
 * rule works, draw any closed shape and draw a line through the entire shape. 
 * Each time the line crosses the shape's border, increment a counter. When the 
 * counter is even, the line is outside the shape. When the counter is odd,
 * the line is in the interior of the shape.
 * @def WIND_NON_ZERO
 * @ingroup WindingRules
 * This define represents the non-zero winding rule. Similar to even-odd. 
 * We draw line through the entire shape. If intersecting edge is drawn from
 * left-to-right, we add 1 to counter. If it goes from right to left we add -1.
 * Everytime the counter is not zero, we assume it's interior part of shape.      
 */
#define WIND_NON_ZERO 1
#define WIND_EVEN_ODD 0

//Paint methods
/**
 * @defgroup PaintMethods Paint methods in PISCES
 * Paint method says what source color should be used while filling shapes. We 
 * can use solid color for every touched pixel, or we can use gradient-fills or 
 * textures. Setting paint method you can draw any primitive (even line or oval) 
 * filled with gradient or texture. 
 * @see setColor
 * @see setFill
 * @def PAINT_FLAT_COLOR
 * @ingroup PaintMethods 
 * Paint method uses flat color. Source color set by setColor() is used.
 * @see setColor
 * @def PAINT_LINEAR_GRADIENT 
 * @ingroup PaintMethods
 * Paint method. Source color value is precalculated linear gradients color.
 * @see setLinearGradient    
 * @def PAINT_RADIAL_GRADIENT
 * @ingroup PaintMethods
 * Paint method. Source color value is precalculated radial gradients color.
 * @see setRadialGradient
 * @def PAINT_TEXTURE
 * @ingroup PaintMethods 
 * Paint method. Source color value is texture. 
 * @see setTexture
 */  
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

#define INVALID_RENDERER_SURFACE 16

/**
 * @defgroup CycleMethods Gradient cycle methods
 * Gradient cycle methods. Specifies wheteher to repeat gradient fill in cycle 
 * or not. We will explain possible methods on linear gradient behaviour.
 * @see setLinearGradient, setRadialGradient  
 * @def CYCLE_NONE
 * @ingroup CycleMethods 
 * Gradient without repetition. Imagine linear gradient from blue to red color.
 * Color of start point (line perpendicular to vector(start,end)) will be blue. 
 * Color of end point (line) will be red. Between these two points (lines), 
 * there will be smooth color gradient. Outside gradient area everything will be
 * blue or red when CYCLE_NONE used. It works similar way with radial gradient.       
 * @def CYCLE_REPEAT 
 * @ingroup CycleMethods
 * Gradient with repetition. Gradient fill is repeated with period given by 
 * start,end distance.
 * @def CYCLE_REFLECT
 * @ingroup CycleMethods 
 * Gradient is repeated. Start and end color in new cycle are swaped. Gradient 
 * fill is repeated with period given by start,end distance. You can imagine 
 * this as if you'd put mirror to end point (line).
 */
#define CYCLE_NONE 0
#define CYCLE_REPEAT 1
#define CYCLE_REFLECT 2

/**
 * \struct _Renderer 
 * Structure _Renderer encapsulates rendering-state information. Colors, 
 * textures, counted gradient-fills, transformation-matrices, compositing rule,
 * antialiasing, paint method, surface (destination of our drawing) and much 
 * more is tracked by Renderer. Simply, we can say, Renderer knows HOW AND 
 * WHERE TO DRAW. It is also typedefed as Renderer.
 * \typedef Renderer
 * Typedef to struct _Renderer.
 */
typedef struct _Renderer {
    jint _windingRule;

    // Flat color or (Java2D) linear gradient
    jint _paintMode;

    // Current (internal) color
    jint _cred, _cgreen, _cblue, _calpha;

    // Original color
    jint _ored, _ogreen, _oblue, _oalpha;

    // Gradient paint
    jint _lgradient_x0; /// 
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
    /**    
     * @var jint _SUBPIXEL_LG_POSITIONS_X 
     * Number of subpixels in x-axis can be counted as  
     * 2^_SUBPIXEL_LG_POSITIONS_X. Total number of subpixels in one pixel can be
     * computed as 2^_SUBPIXEL_LG_POSITIONS_X * 2^_SUBPIXEL_LG_POSITIONS_Y.
     * @see _SUBPIXEL_LG_POSITIONS_Y, _SUBPIXEL_POSITIONS_X, 
     * _SUBPIXEL_POSITIONS_Y, _MAX_AA_ALPHA
     */      
    jint _SUBPIXEL_LG_POSITIONS_X;
    /**
     * @var jint _SUBPIXEL_LG_POSITIONS_Y
     * Number of subpixels in y-axis can be counted as  
     * 2^_SUBPIXEL_LG_POSITIONS_Y. Total number of subpixels in one pixel can be
     * computed as 2^_SUBPIXEL_LG_POSITIONS_X * 2^_SUBPIXEL_LG_POSITIONS_Y.
     * @see _SUBPIXEL_LG_POSITIONS_X, _SUBPIXEL_POSITIONS_X, 
     * _SUBPIXEL_POSITIONS_Y, _MAX_AA_ALPHA
     */     
    jint _SUBPIXEL_LG_POSITIONS_Y;
    jint _SUBPIXEL_MASK_X;
    jint _SUBPIXEL_MASK_Y;
    /** 
     * @var jint _SUBPIXEL_POSITIONS_X
     * Number of subpixels in x-axis. This is computed as 
     * 2^_SUBPIXEL_LG_POSITIONS_X internaly.
     * @see _SUBPIXEL_POSITIONS_Y, _SUBPIXEL_LG_POSITIONS_X, 
     * _SUBPIXEL_LG_POSITIONS_Y, _MAX_AA_ALPHA     
     */
    jint _SUBPIXEL_POSITIONS_X;
    /** 
     * @var jint _SUBPIXEL_POSITIONS_Y
     * Number of subpixels in y-axis. This is computed as 
     * 2^_SUBPIXEL_LG_POSITIONS_Y internaly.  
     * @see _SUBPIXEL_POSITIONS_Y, _SUBPIXEL_LG_POSITIONS_X, 
     * _SUBPIXEL_LG_POSITIONS_Y, _MAX_AA_ALPHA
     */
    jint _SUBPIXEL_POSITIONS_Y;
    /** 
     * @var jint _MAX_AA_ALPHA
     * Pixel area is divided into _MAX_AA_ALPHA subpixels for antialising. Pixel
     * is fully covered when all of _MAX_AA_ALPHA subpixels are covered. 
     * The more subpixels we use, the better antialising effect is obtained but 
     * more computation power is needed on the contrary.
     * @see _SUBPIXEL_POSITIONS_X, _SUBPIXEL_POSITIONS_Y,
     * _SUBPIXEL_LG_POSITIONS_X, _SUBPIXEL_LG_POSITIONS_Y
     */
    jint _MAX_AA_ALPHA;
    /**
     * @var jint _MAX_AA_ALPHA_DENOM
     * Help variable. This value is precounted for convenience.
     * @see _MAX_AA_ALPHA     
     */          
    jint _MAX_AA_ALPHA_DENOM; ///help variable
    jint _HALF_MAX_AA_ALPHA_DENOM; ///help variable
    jint _XSHIFT;
    jint _YSHIFT;
    jint _YSTEP;
    jint _HYSTEP;
    jint _YMASK;
    jint _AA_ALPHA_SHIFT;


    jint _colorAlphaMap[16*16 + 1];
    jint _paintAlphaMap[256];

    /** 
     * Switches antialiasing support ON/OFF. 
     * @see To switch antialising ON/OFF, use 
     * renderer_setAntialiasing(Renderer*, antialiasingOn).     
     */
    jboolean _antialiasingOn;
    /** 
     * Current compositing rule. Renderers internal variable. To change it use
     * renderer_setCompositeRule(Renderer *, jint) or
     * renderer_setComposite(Renderer *, jint, jfloat).     
     * @see See CompositingRules, renderer_setCompositeRule(Renderer *, jint),
     * renderer_setComposite(Renderer *, jint, jfloat)
     */          
    jint _compositeRule;

    jfloat _compositeAlpha;

    // Bounding boxes
    jint _boundsMinX, _boundsMinY, _boundsMaxX, _boundsMaxY;
    jint _rasterMinX, _rasterMaxX, _rasterMinY, _rasterMaxY;
    jint _bboxX0, _bboxY0, _bboxX1, _bboxY1;

    Surface* _surface;

    // Image layout
    void *_data;
    jint _width, _height;
    jint _imageOffset;
    jint _imageScanlineStride;
    jint _imagePixelStride;
    jint _imageType;
    jboolean _allocated;

    jint _scrOrient;

    void (*_bl_SourceOver)(struct _Renderer *rdr, jint height);
    void (*_bl_PT_SourceOver)(struct _Renderer *rdr, jint height);
    void (*_bl_Source)(struct _Renderer *rdr, jint height);
    void (*_bl_PT_Source)(struct _Renderer *rdr, jint height);

    /**
     * Pointer to function which clears rectangle - ie. sets rectangle data to 
     * transparent black. Implementations are optimized for concrete surface 
     * types. 
     * @param height height of blitting area
     * @see renderer_setCompositeRule()     
     */    
    void (*_bl_Clear)(struct _Renderer *rdr, jint height);
    void (*_bl_PT_Clear)(struct _Renderer *rdr, jint height);

    /**
     * Pointer to bliting function. Bliting function is set due to 
     * composite rule and surface type to appropriate optimized function. _bl_SO
     * is called in paint mode PAINT_FLAT_COLOR - when filling with solid color.    
     * @param height height of blitting area
     * @see renderer_setCompositeRule()             
     */         
    void (*_bl)(struct _Renderer *rdr, jint height);
    /** Pointer to paint bliting function. Bliting function is set due to 
     * composite rule and surface type to appropriate optimized function. 
     * _bl_PT_SO is called in paint mode different from PAINT_FLAT_COLOR, ie.
     * anytime when filling with gradients or textures.          
     * @param height height of blitting area
     * @see renderer_setCompositeRule()     
     */
    void (*_bl_PT)(struct _Renderer *rdr, jint height);

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

    // Paint transform
    Transform6 _paint_transform;

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
    jint _rendererState;

    Pipeline* _defaultPipeline;
    Pipeline _fillerPipeline;
    Pipeline _strokerPipeline;

}
Renderer;

#define INVALIDATE_RENDERER_SURFACE(rdr)        \
        (rdr)->_rendererState |= INVALID_RENDERER_SURFACE;

void prenderer_moveTo(Pipeline* pipeline, jint x0, jint y0);
void prenderer_lineJoin(Pipeline* pipeline);
void prenderer_lineTo(Pipeline* pipeline, jint x1, jint y1);
void prenderer_close(Pipeline* pipeline);
void prenderer_end(Pipeline* pipeline);

#endif
