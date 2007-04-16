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
 *  \file PiscesRenderer.inl
 *  Renderer related inline-functions implementation. C high level API.  
 */

#include <PiscesRenderer.h>

#include <PiscesUtil.h>
#include <PiscesMath.h>
#include <PiscesBlit.h>
#include <PiscesTransform.h>

#include <PiscesSysutils.h>

#define DEFAULT_SUBPIXEL_LG_POSITIONS_X 3
#define DEFAULT_SUBPIXEL_LG_POSITIONS_Y 3

#define PISCES_ACV (jlong)(65536.0 * 0.22385762508460333)

#define INVALID_COLOR_ALPHA_MAP 1
#define INVALID_PAINT_ALPHA_MAP 2
#define VALIDATE_ALPHA_MAP 4
#define INVALID_INTERNAL_COLOR 8


#define INVALID_COMPOSITE_DEPENDED_ROUTINES 32
#define INVALID_PAINT_DEPENDED_ROUTINES 64

#define INVALID_BLITTING_MASK (VALIDATE_ALPHA_MAP |                            \
                               INVALID_INTERNAL_COLOR |                        \
                               INVALID_RENDERER_SURFACE |                      \
                               INVALID_COMPOSITE_DEPENDED_ROUTINES |           \
                               INVALID_PAINT_DEPENDED_ROUTINES)
                               
#define INVALID_ALL (INVALID_COLOR_ALPHA_MAP |                                 \
                     INVALID_PAINT_ALPHA_MAP |                                 \
                     INVALID_BLITTING_MASK)

#define VALIDATE_SURFACE(rdr)                                                  \
    if ((rdr)->_rendererState & INVALID_RENDERER_SURFACE) {                    \
            updateRendererSurface(rdr);                                        \
    }

#define VALIDATE_BLITTING(rdr)                                                 \
    if ((rdr)->_rendererState & INVALID_BLITTING_MASK) {                       \
        jint __state = (rdr)->_rendererState;                                  \
                                                                               \
        if (__state & INVALID_RENDERER_SURFACE) {                              \
            updateRendererSurface(rdr);                                        \
        }                                                                      \
                                                                               \
        if (__state & INVALID_INTERNAL_COLOR) {                                \
            updateInternalColor(rdr);                                          \
        }                                                                      \
                                                                               \
        if (__state & VALIDATE_ALPHA_MAP) {                                    \
            validateAlphaMap(rdr);                                             \
            rdr->_rendererState &= ~VALIDATE_ALPHA_MAP;                        \
        }                                                                      \
                                                                               \
        if (__state & INVALID_COMPOSITE_DEPENDED_ROUTINES) {                   \
            /* Optimization: validates also INVALID_PAINT_DEPENDED_ROUTINES */ \
            updateCompositeDependedRoutines(rdr);                              \
        } else if (__state & INVALID_PAINT_DEPENDED_ROUTINES) {                \
            updatePaintDependedRoutines(rdr);                                  \
        }                                                                      \
                                                                               \
        assert((rdr->_rendererState & INVALID_BLITTING_MASK) == 0);            \
    }

static INLINE Renderer* renderer_create(Surface* surface);
static INLINE void renderer_dispose(Renderer* rdr);

static INLINE void renderer_beginRendering1(Renderer* rdr, jint windingRule);
static INLINE void renderer_beginRendering5(Renderer* rdr, jint minX, jint minY,
        jint width, jint height, jint windingRule);
static INLINE void renderer_endRendering(Renderer* rdr);

static INLINE void renderer_setAntialiasing(Renderer* rdr,
        jboolean antialiasingOn);
static INLINE jboolean renderer_getAntialiasing(Renderer* rdr);
static INLINE void renderer_setClip(Renderer* rdr, jint minX, jint minY,
                                    jint width, jint height);
static INLINE void renderer_resetClip(Renderer* rdr);
static INLINE void renderer_setTransform(Renderer* rdr,
        const Transform6* transform);
static INLINE Transform6* renderer_getTransform(Renderer* rdr);
static INLINE void renderer_setStroke0(Renderer* rdr);
static INLINE void renderer_setStroke6(Renderer* rdr, jint lineWidth,
                                       jint capStyle, jint joinStyle,
                                       jint miterLimit, jint* dashArray,
                                       jint dashArray_length, jint dashPhase);
static INLINE void renderer_setFill(Renderer* rdr);
              
static INLINE void renderer_setCompositeRule(Renderer *rdr, jint compositeRule);
static INLINE void renderer_setComposite(Renderer* rdr, jint compositeRule, 
                                         jfloat alpha);
static INLINE void renderer_setColor(Renderer* rdr, jint red, jint green,
                                     jint blue, jint alpha);
static INLINE void renderer_setLinearGradient(Renderer* rdr, jint x0, jint y0,
        jint x1, jint y1, jint* colors,
        Transform6 *transform);
static INLINE void renderer_setRadialGradient(Renderer* rdr, jint cx, jint cy,
        jint fx, jint fy, jint radius,
        jint* colors,
        Transform6 *transform);
static INLINE void renderer_setTexture(Renderer* rdr, jint* data, jint width,
                                       jint height, jboolean repeat,
                                       const Transform6* transform);

static INLINE void renderer_moveTo(Renderer* rdr, jint x0, jint y0);
static INLINE void renderer_lineJoin(Renderer* rdr);
static INLINE void renderer_lineTo(Renderer* rdr, jint x1, jint y1);
static INLINE void renderer_quadTo(Renderer* rdr, jint x1, jint y1, jint x2,
                                   jint y2);
static INLINE void renderer_cubicTo(Renderer* rdr, jint x1, jint y1, jint x2,
                                    jint y2, jint x3, jint y3);
static INLINE void renderer_close(Renderer* rdr);
static INLINE void renderer_end(Renderer* rdr);

static INLINE void renderer_clearRect(Renderer* rdr, jint x, jint y, jint w,
                                      jint h);
static INLINE void renderer_drawLine(Renderer* rdr, jint x0, jint y0, jint x1,
                                     jint y1);
static INLINE void renderer_drawRect(Renderer* rdr, jint x, jint y, jint w,
                                     jint h);
static INLINE void renderer_fillRect(Renderer* rdr, jint x, jint y, jint w,
                                     jint h);
static INLINE void renderer_drawOval(Renderer* rdr, jint x, jint y, jint w,
                                     jint h);
static INLINE void renderer_fillOval(Renderer* rdr, jint x, jint y, jint w,
                                     jint h);
static INLINE void renderer_drawRoundRect(Renderer* rdr, jint x, jint y, jint w,
        jint h, jint aw, jint ah);
static INLINE void renderer_fillRoundRect(Renderer* rdr, jint x, jint y, jint w,
        jint h, jint aw, jint ah);
static INLINE void renderer_drawArc(Renderer* rdr, jint x, jint y, jint width,
                                    jint height, jint startAngle, jint arcAngle,
                                    jint arcType);
static INLINE void renderer_fillArc(Renderer* rdr, jint x, jint y, jint width,
                                    jint height, jint startAngle, jint arcAngle,
                                    jint arcType);

static Renderer* createCommon(Surface* surface);
static void setPaintMode(Renderer* rdr, jint newPaintMode);
static void setAntialiasing(Renderer* rdr, jint subpixelLgPositionsX,
                            jint subpixelLgPositionsY);
static void beginRenderingImpl(Renderer* rdr, jint boundsX, jint boundsY,
                               jint boundsWidth, jint boundsHeight,
                               jint windingRule);
static jint clamp(jint x, jint min, jint max);
static void endRenderingImpl(Renderer* rdr);
static void computeCrossingsForEdge(Renderer *rdr, jint index,
                                    jint boundsMinY, jint boundsMaxY);
static void computeBounds(Renderer* rdr);
static void renderStrip(Renderer* rdr);
static void clearAlpha(jbyte* alpha, jint alphaOffset, jint alphaStride,
                       jint width, jint height, jint *minTouched,
                       jint *maxTouched);
static void emitRow(Renderer* rdr, jint minX, jint maxX, jboolean forceOutput);
static void setCrossingsExtents(Renderer* rdr, jint minY, jint maxY,
                                jint maxXEntries);
static void resetCrossings(Renderer* rdr);
static void crossingListFinished(Renderer* rdr);
static void sortCrossings3(jint* x, jint off, jint len);
static void sortCrossings(Renderer* rdr);
static void addCrossing(Renderer* rdr, jint y, jint x, jint orientation);
static void iterateCrossings(Renderer* rdr);
static jboolean hasMoreCrossingRows(Renderer* rdr);
static void validateStroker(Renderer* rdr);
static void validateFiller(Renderer* rdr);
static void fillOrDrawRect(Pipeline* pipeline, jint x, jint y, jint w, jint h);
static void emitQuadrants(Pipeline* pipeline, jint cx, jint cy,
                          jint* points, jint nPoints);
static void emitOval(Pipeline* pipeline, jint cx, jint cy, jint rx, jint ry,
                     jint nPoints, jboolean reverse);
static void emitOffsetOval(Pipeline* pipeline, jint cx, jint cy,
                           jint rx, jint ry, jint lw2, jint nPoints,
                           jboolean inside);
static void fillOrDrawOval(Renderer* rdr, jint x, jint y, jint w, jint h,
                           jboolean hollow);
static void emitRoundRect(Pipeline* pipeline, jint x, jint y, jint w, jint h,
                          jint aw, jint ah, jboolean reverse);
static void fillOrDrawRoundRect(Renderer* rdr, jint x, jint y, jint w, jint h,
                                jint aw, jint ah, jboolean stroke);
static void emitArc(Pipeline* pipeline, jint cx, jint cy, jint rx, jint ry,
                    jint startAngle, jint endAngle, jint nPoints);
static void fillOrDrawArc(Renderer* rdr, jint x, jint y, jint width,
                          jint height, jint startAngle, jint arcAngle,
                          jint arcType, jboolean stroke);

static void updateInternalColor(Renderer* rdr);
static void validateAlphaMap(Renderer* rdr);

static void updateRendererSurface(Renderer* rdr);

static void updateSurfaceDependedRoutines(Renderer* rdr);
static void updateCompositeDependedRoutines(Renderer* rdr);
static void updatePaintDependedRoutines(Renderer* rdr);

static INLINE Renderer* 
renderer_create(Surface* surface) {
    return createCommon(surface);
}

static INLINE void
renderer_dispose(Renderer* rdr) {
    if (rdr->_allocated) {
        my_free(rdr->_data);
    }

    my_free(rdr->_rowAA);
    my_free(rdr->_crossings);
    my_free(rdr->_crossingIndices);
    my_free(rdr->_edges);
    my_free(rdr->_ovalPoints);

    my_free(rdr->_texture_intData);
    my_free(rdr->_dashArray);

    my_free(rdr->_paint);

    pipeline_free(&rdr->_fillerPipeline);
    pipeline_free(&rdr->_strokerPipeline);

    my_free(rdr);
}

static INLINE void
renderer_beginRendering1(Renderer* rdr, jint windingRule) {
    jint minX, minY;
    jint maxX, maxY;
    jint width, height;

    VALIDATE_SURFACE(rdr);

    rdr->_inSubpath = XNI_FALSE;

    minX = MAX(0, rdr->_clip_bbMinX);
    minY = MAX(0, rdr->_clip_bbMinY);

    maxX = MIN(rdr->_width, rdr->_clip_bbMaxX);
    maxY = MIN(rdr->_height, rdr->_clip_bbMaxY);

    width = maxX - minX;
    height = maxY - minY;
    beginRenderingImpl(rdr, minX, minY, width, height, windingRule);
}

static INLINE void
renderer_beginRendering5(Renderer* rdr, jint minX, jint minY,
                         jint width, jint height, jint windingRule) {
    jint maxX = minX + width;
    jint maxY = minY + height;

    VALIDATE_SURFACE(rdr);

    rdr->_inSubpath = XNI_FALSE;

    minX = MAX(minX, 0);
    minX = MAX(minX, rdr->_clip_bbMinX);

    minY = MAX(minY, 0);
    minY = MAX(minY, rdr->_clip_bbMinY);

    maxX = MIN(maxX, rdr->_width);
    maxX = MIN(maxX, rdr->_clip_bbMaxX);

    maxY = MIN(maxY, rdr->_height);
    maxY = MIN(maxY, rdr->_clip_bbMaxY);

    width = maxX - minX;
    height = maxY - minY;
    beginRenderingImpl(rdr, minX, minY, width, height, windingRule);
}

static INLINE void
renderer_endRendering(Renderer* rdr) {
    /* end() */
    if (rdr->_inSubpath && rdr->_isPathFilled) {
        /* close() */
        PIPELINE_CLOSE(rdr->_defaultPipeline);
    }
    rdr->_inSubpath = XNI_FALSE;
    PIPELINE_END(rdr->_defaultPipeline);

    /* myEndRendering() */
    endRenderingImpl(rdr);
}

/**
 * Switches rdrs antialiasing support ON/OFF. Number of subpixels is reset
 * to default. If you want to use different number of subpixels than default, 
 * then you have to call renderer_setAntialiasing(Renderer * , jint, jint) after
 * every call of this function.
 * @see renderer_setAntialiasing(Renderer * , jint, jint), _MAX_AA_ALPHA
 * @param rdr pointer to renderer structure
 * @param antialisingOn if true antialiasing support is turned on. If false, it
 * is switched off - graphics will not be antialiased.  
 */  
static INLINE void
renderer_setAntialiasing(Renderer* rdr, jboolean antialiasingOn) {
    jint samplesX;
    jint samplesY;

    if (antialiasingOn) {
        samplesX = DEFAULT_SUBPIXEL_LG_POSITIONS_X;
        samplesY = DEFAULT_SUBPIXEL_LG_POSITIONS_Y;
    } else {
        samplesX = 0;
        samplesY = 0;
    }
    
    rdr->_antialiasingOn = antialiasingOn;
    setAntialiasing(rdr, samplesX, samplesY);
    // TODO: invalidate(); ??
}
/**
 * Returns true if (*rdr) has antialiasing support switched on. False otherwise.
 * @param rdr pointer to renderer structure
 */ 
static INLINE jboolean
renderer_getAntialiasing(Renderer* rdr) {
    return rdr->_antialiasingOn;
}

/**
 * This function sets clip-rect. Any part of object which is determined outside
 * of clip-rect is cliped == not drawn to destination surface.
 * @param rdr pointer to renderer structure
 * @param minX clip-rects upper-left corner x-coordinate (S15.16)
 * @param minY clip-rects upper-left corner y-coordinate (S15.16)
 * @param width clip-rects width (S15.16)
 * @param height clip-rects height (S15.16)
 * @see renderer_resetClip(Renderer *)
 */   
static INLINE void
renderer_setClip(Renderer* rdr, jint minX, jint minY, jint width, jint height) {
    rdr->_clip_bbMinX = minX;
    rdr->_clip_bbMinY = minY;
    rdr->_clip_bbMaxX = minX + width;
    rdr->_clip_bbMaxY = minY + height;
}

/**
 * This function resets clip-rect to world. This means, that clip-rect becomes
 * as large as possible. Nothing you draw to your surface is cliped.
 * @param rdr pointer to renderer structure
 * @see renderer_setClip(Renderer *, jint, jint, jint, jint) 
 */
static INLINE void
renderer_resetClip(Renderer* rdr) {
    rdr->_clip_bbMinX = INTEGER_MIN_VALUE;
    rdr->_clip_bbMinY = INTEGER_MIN_VALUE;
    rdr->_clip_bbMaxX = INTEGER_MAX_VALUE;
    rdr->_clip_bbMaxY = INTEGER_MAX_VALUE;
}

/**
 * Sets 2D transformation to be used in current rendering session. Any shape 
 * drawn in current rendering session is transformed (translated, rotated, ...).
 * In addition, fills (gradients, textures) are transformed too.  
 * @param rdr pointer to renderer structure to which we want to apply 
 * transformation to
 * @param transform new transformation matrix  
 * @see Transform6
 * @todo for now (because of Perseus test framework) transformation to fills is 
 * applied only to textures (rdr->_paintMode == PAINT_TEXTURE); 
 * it should be applied in other modes too (PAINT_LINEAR_GRADIENT,  
 * PAINT_RADIAL_GRADIENT)
 * @see renderer_getTransform(Renderer *),  Transform6
 */ 
static INLINE void
renderer_setTransform(Renderer* rdr, const Transform6* transform) {
    Transform6 compoundTransform;
    //jfloat fx0, fx1, fy0, fy1, fdx, fdy, flensq, t;
    //jfloat a00, a01, a02, a10, a11, a12;

    /* Assign transformation to shapes (lines, arcs and other primitives)*/
    pisces_transform_assign(&rdr->_transform, transform);

    /* Let's assign the same transformation to fills, too.*/ 
    if (rdr->_paintMode == PAINT_LINEAR_GRADIENT) {
        /*   pisces_transform_assign(&compoundTransform, &rdr->_transform);
           pisces_transform_multiply(&compoundTransform, 
                                     &rdr->_gradient_transform);
           pisces_transform_assign(&rdr->_gradient_transform, 
                                   &compoundTransform);

           pisces_transform_invert(&compoundTransform);
           pisces_transform_assign(&rdr->_gradient_inverse_transform, 
                                   &compoundTransform);
         
           a00 = rdr->_gradient_inverse_transform.m00/65536.0f;
           a01 = rdr->_gradient_inverse_transform.m01/65536.0f;
           a02 = rdr->_gradient_inverse_transform.m02/65536.0f;
           a10 = rdr->_gradient_inverse_transform.m10/65536.0f;
           a11 = rdr->_gradient_inverse_transform.m11/65536.0f;
           a12 = rdr->_gradient_inverse_transform.m12/65536.0f;

           fx0 = rdr->_lg_x0/65536.0f;
           fx1 = rdr->_lg_x1/65536.0f;
           fy0 = rdr->_lg_y0/65536.0f;
           fy1 = rdr->_lg_y1/65536.0f;

           fdx = fx1 - fx0;
           fdy = fy1 - fy0;
           flensq = fdx*fdx + fdy*fdy;
           t = fdx*fx0 + fdy*fy0;

           rdr->_lg_mx = (jlong)(65536.0f*(a00*fdx + a10*fdy)/flensq);
           rdr->_lg_my = (jlong)(65536.0f*(a01*fdx + a11*fdy)/flensq);
           rdr->_lg_b = (jlong)(65536.0f*(a02*fdx + a12*fdy - t)/flensq);
         */
    } else if (rdr->_paintMode == PAINT_RADIAL_GRADIENT) {
        /*  pisces_transform_assign(&compoundTransform, &rdr->_transform);
          pisces_transform_multiply(&compoundTransform, 
                                    &rdr->_gradient_transform);
          pisces_transform_assign(&rdr->_gradient_transform, 
                                  &compoundTransform);

          pisces_transform_invert(&compoundTransform);
          pisces_transform_assign(&rdr->_gradient_inverse_transform, 
                                  &compoundTransform);
        */
    } else if (rdr->_paintMode == PAINT_TEXTURE) {
        pisces_transform_assign(&compoundTransform, &rdr->_transform);
        pisces_transform_multiply(&compoundTransform, &rdr->_paint_transform);
        pisces_transform_invert(&compoundTransform);

        rdr->_texture_m00 = compoundTransform.m00;
        rdr->_texture_m01 = compoundTransform.m01;
        rdr->_texture_m10 = compoundTransform.m10;
        rdr->_texture_m11 = compoundTransform.m11;
        rdr->_texture_m02 = compoundTransform.m02 +
                            (rdr->_texture_m00 >> 1) + (rdr->_texture_m01 >> 1);
        rdr->_texture_m12 = compoundTransform.m12 +
                            (rdr->_texture_m10 >> 1) + (rdr->_texture_m11 >> 1);

        rdr->_texture_interpolate = XNI_TRUE;
        rdr->_texture_m02 -= 32768;   // interpolate == true
        rdr->_texture_m12 -= 32768;   // interpolate == true

    }
    
    //Let's recalculate filler and stroker for next use.  
    rdr->_validFiller = XNI_FALSE;
    rdr->_validStroker = XNI_FALSE;
}

/**
 * Returns renderers (rdr) transformation matrix.
 * @param rdr pointer to Renderer structure
 * @returns  renderers transformation matrix
 * @see renderer_setTransform(Renderer* , const Transform6*), Transform6
 */ 
static INLINE Transform6*
renderer_getTransform(Renderer* rdr) {
    return &rdr->_transform;
}

static INLINE void
renderer_setStroke0(Renderer* rdr) {
    validateStroker(rdr);
    rdr->_isPathFilled = XNI_FALSE;
    rdr->_defaultPipeline = &rdr->_strokerPipeline;
}

static INLINE void
renderer_setStroke6(Renderer* rdr, jint lineWidth, jint capStyle,
                    jint joinStyle, jint miterLimit, jint* dashArray, 
                    jint dashArray_length, jint dashPhase) {
    my_free(rdr->_dashArray);

    rdr->_lineWidth = lineWidth;
    rdr->_capStyle = capStyle;
    rdr->_joinStyle = joinStyle;
    rdr->_miterLimit = miterLimit;
    rdr->_dashArray = dashArray;
    rdr->_dashArray_length = dashArray_length;
    rdr->_dashPhase = dashPhase;
    rdr->_validStroker = XNI_FALSE;

    /* setStroke(); */
    validateStroker(rdr);
    rdr->_isPathFilled = XNI_FALSE;
    rdr->_defaultPipeline = &rdr->_strokerPipeline;
}

static INLINE void
renderer_setFill(Renderer* rdr) {
    validateFiller(rdr);
    rdr->_isPathFilled = XNI_TRUE;
    rdr->_defaultPipeline = &rdr->_fillerPipeline;
}

static INLINE void
renderer_setColor(Renderer* rdr, jint red, jint green, jint blue, jint alpha) {
    if ((rdr->_ored != red) ||
            (rdr->_ogreen != green) ||
            (rdr->_oblue != blue) ||
            (rdr->_oalpha != alpha)) {
        rdr->_rendererState |= INVALID_INTERNAL_COLOR;
        if (rdr->_oalpha != alpha) {
            rdr->_rendererState |= VALIDATE_ALPHA_MAP |
                                   INVALID_COLOR_ALPHA_MAP |
                                   INVALID_PAINT_ALPHA_MAP;
        }
        
        rdr->_ored = red;
        rdr->_ogreen = green;
        rdr->_oblue = blue;
        rdr->_oalpha = alpha;
    }

    setPaintMode(rdr, PAINT_FLAT_COLOR);
}

/**
 * This function sets composite rule of rdr. It initializes bliting functions 
 * pointers for current surface-type too. 
 * @param rdr pointer to Renderer structure 
 * @param compositeRule compositing rule    
 * @see For supported compositeRule values see CompositingRules, 
 * renderer_setComposite(Renderer *, jint, jfloat)
 */
static INLINE void
renderer_setCompositeRule(Renderer* rdr, jint compositeRule) {
    if (rdr->_compositeRule != compositeRule) {
        // composite mode COMPOSITE_CLEAR changes the internal color
        rdr->_rendererState |= INVALID_INTERNAL_COLOR | 
                               INVALID_COMPOSITE_DEPENDED_ROUTINES;

        if ((compositeRule == COMPOSITE_SRC_OVER) ||
                ((rdr->_imageType == TYPE_INT_ARGB) ||
                   (rdr->_imageType == TYPE_INT_ARGB_PRE))) {
            // need to recalculate the alpha map
            // see the implementation of validateAlphaMap
            rdr->_rendererState |= VALIDATE_ALPHA_MAP |
                                   INVALID_COLOR_ALPHA_MAP |
                                   INVALID_PAINT_ALPHA_MAP;
        }
        
        rdr->_compositeRule = compositeRule;
    }
}
/**
 * Function sets Renderers compositing rule to compositeRule and composite alpha 
 * to alpha. 
 * @param compositeRule compositing rule
 * @param alpha composite alpha. Value must be from 0.0 to 1.0.  
 * @see For supported values of compositeRule see CompositingRules, 
 * renderer_setComposite(Renderer *, jint, jfloat) 
 */
static INLINE void
renderer_setComposite(Renderer* rdr, jint compositeRule, jfloat alpha) {
    renderer_setCompositeRule(rdr, compositeRule);
    
    if (rdr->_compositeAlpha != alpha) {
        rdr->_rendererState |= INVALID_INTERNAL_COLOR |
                               VALIDATE_ALPHA_MAP |
                               INVALID_COLOR_ALPHA_MAP |
                               INVALID_PAINT_ALPHA_MAP;
        rdr->_compositeAlpha = alpha;
    }
}

static INLINE void
renderer_setLinearGradient(Renderer* rdr,
                           jint x0, jint y0, jint x1, jint y1,
                           jint *colors,
                           Transform6 *transform) {
    jfloat fx0, fx1, fy0, fy1, fdx, fdy, flensq, t;
    jfloat a00, a01, a02, a10, a11, a12;

    //Transform6 compoundTransform;

    //pisces_transform_assign(&rdr->_paint_transform, transform);

    //pisces_transform_assign(&compoundTransform, &rdr->_transform);
    //pisces_transform_multiply(&compoundTransform, transform);
    //pisces_transform_invert(&compoundTransform);


    //a00 = compoundTransform.m00/65536.0f;
    //a01 = compoundTransform.m01/65536.0f;
    //a02 = compoundTransform.m02/65536.0f;
    //a10 = compoundTransform.m10/65536.0f;
    //a11 = compoundTransform.m11/65536.0f;
    //a12 = compoundTransform.m12/65536.0f;

    //rev. 226 begins
    pisces_transform_assign(&rdr->_gradient_transform, transform);
    pisces_transform_assign(&rdr->_gradient_inverse_transform, transform);
    pisces_transform_invert(&rdr->_gradient_inverse_transform);

    a00 = rdr->_gradient_inverse_transform.m00/65536.0f;
    a01 = rdr->_gradient_inverse_transform.m01/65536.0f;
    a02 = rdr->_gradient_inverse_transform.m02/65536.0f;
    a10 = rdr->_gradient_inverse_transform.m10/65536.0f;
    a11 = rdr->_gradient_inverse_transform.m11/65536.0f;
    a12 = rdr->_gradient_inverse_transform.m12/65536.0f;
    //rev. 226 ends.

    fx0 = x0/65536.0f;
    fx1 = x1/65536.0f;
    fy0 = y0/65536.0f;
    fy1 = y1/65536.0f;
    fdx = fx1 - fx0;
    fdy = fy1 - fy0;
    flensq = fdx*fdx + fdy*fdy;
    t = fdx*fx0 + fdy*fy0;

    rdr->_lg_x0 = x0;
    rdr->_lg_y0 = y0;
    rdr->_lg_x1 = x1;
    rdr->_lg_y1 = y1;

    rdr->_lg_mx = (jlong)(65536.0f*(a00*fdx + a10*fdy)/flensq);
    rdr->_lg_my = (jlong)(65536.0f*(a01*fdx + a11*fdy)/flensq);
    rdr->_lg_b = (jlong)(65536.0f*(a02*fdx + a12*fdy - t)/flensq);

    setPaintMode(rdr, PAINT_LINEAR_GRADIENT);
    memcpy(rdr->_gradient_colors, colors, GRADIENT_MAP_SIZE*sizeof(jint));
}

static INLINE void
renderer_setRadialGradient(Renderer* rdr,
                           jint cx, jint cy, jint fx, jint fy, jint radius,
                           jint *colors,
                           Transform6 *transform) {

//  Transform6 compoundTransform;

//  pisces_transform_assign(&rdr->_paint_transform, transform);

//  pisces_transform_assign(&compoundTransform, &rdr->_transform);
//  pisces_transform_multiply(&compoundTransform, transform);
//  pisces_transform_assign(&rdr->_gradient_transform, &compoundTransform);
//  pisces_transform_invert(&compoundTransform);
//  pisces_transform_assign(&rdr->_gradient_inverse_transform, 
//                          &compoundTransform);

//rev. 226 begins
    pisces_transform_assign(&rdr->_gradient_transform, transform);
    pisces_transform_assign(&rdr->_gradient_inverse_transform, transform);
    pisces_transform_invert(&rdr->_gradient_inverse_transform);
//rev. 226 ends

    rdr->_rg_cx = cx;
    rdr->_rg_cy = cy;
    rdr->_rg_fx = fx;
    rdr->_rg_fy = fy;
    rdr->_rg_radius = radius;

    setPaintMode(rdr, PAINT_RADIAL_GRADIENT);
    memcpy(rdr->_gradient_colors, colors, GRADIENT_MAP_SIZE*sizeof(jint));
}

static INLINE void
renderer_setTexture(Renderer* rdr, jint* data, jint width, jint height,
                    jboolean repeat, const Transform6* transform) {
    Transform6 compoundTransform;

    jint iw, wshift;
    jint ih, hshift;

    pisces_transform_assign(&rdr->_paint_transform, transform);

    pisces_transform_assign(&compoundTransform, &rdr->_transform);
    pisces_transform_multiply(&compoundTransform, transform);
    pisces_transform_invert(&compoundTransform);

    setPaintMode(rdr, PAINT_TEXTURE);

    my_free(rdr->_texture_intData);
    rdr->_texture_intData = data;
    rdr->_texture_imageWidth = width;
    rdr->_texture_imageHeight = height;
    rdr->_texture_stride = width + 2;
    rdr->_texture_repeat = repeat;

    rdr->_texture_m00 = compoundTransform.m00;
    rdr->_texture_m01 = compoundTransform.m01;
    rdr->_texture_m10 = compoundTransform.m10;
    rdr->_texture_m11 = compoundTransform.m11;
    rdr->_texture_m02 = compoundTransform.m02 +
                        (rdr->_texture_m00 >> 1) + (rdr->_texture_m01 >> 1);
    rdr->_texture_m12 = compoundTransform.m12 +
                        (rdr->_texture_m10 >> 1) + (rdr->_texture_m11 >> 1);

    rdr->_texture_interpolate = XNI_TRUE;
    rdr->_texture_m02 -= 32768;   // interpolate == true
    rdr->_texture_m12 -= 32768;   // interpolate == true

    iw = width;
    wshift = 0;
    while (iw > 0) {
        iw >>= 1;
        ++wshift;
    }
    rdr->_texture_wmask = 0xffffffff << (wshift - 1);

    ih = height;
    hshift = 0;
    while (ih > 0) {
        ih >>= 1;
        ++hshift;
    }
    rdr->_texture_hmask = 0xffffffff << (hshift - 1);
}

static INLINE void
renderer_moveTo(Renderer* rdr, jint x0, jint y0) {
    if (rdr->_inSubpath && rdr->_isPathFilled) {
        PIPELINE_CLOSE(rdr->_defaultPipeline);
    }
    rdr->_inSubpath = XNI_FALSE;
    PIPELINE_MOVETO(rdr->_defaultPipeline, x0, y0);
}

static INLINE void
renderer_lineJoin(Renderer* rdr) {
    PIPELINE_LINEJOIN(rdr->_defaultPipeline);
}

static INLINE void
renderer_lineTo(Renderer* rdr, jint x1, jint y1) {
    rdr->_inSubpath = XNI_TRUE;
    PIPELINE_LINETO(rdr->_defaultPipeline, x1, y1);
}

static INLINE void
renderer_quadTo(Renderer* rdr, jint x1, jint y1, jint x2, jint y2) {
    rdr->_inSubpath = XNI_TRUE;
    PIPELINE_QUADTO(rdr->_defaultPipeline, x1, y1, x2, y2);
}

static INLINE void
renderer_cubicTo(Renderer* rdr, jint x1, jint y1, jint x2, jint y2,
                 jint x3, jint y3) {
    rdr->_inSubpath = XNI_TRUE;
    PIPELINE_CUBICTO(rdr->_defaultPipeline, x1, y1, x2, y2, x3, y3);
}

static INLINE void
renderer_close(Renderer* rdr) {
    rdr->_inSubpath = XNI_FALSE;
    PIPELINE_CLOSE(rdr->_defaultPipeline);
}

static INLINE void
renderer_end(Renderer* rdr) {
    if (rdr->_inSubpath && rdr->_isPathFilled) {
        PIPELINE_CLOSE(rdr->_defaultPipeline);
    }
    rdr->_inSubpath = XNI_FALSE;
    PIPELINE_END(rdr->_defaultPipeline);
}

static INLINE void
renderer_clearRect(Renderer* rdr, jint x, jint y, jint w, jint h) {
    jint maxX = x + w;
    jint maxY = y + h;

    VALIDATE_BLITTING(rdr);

    x = MAX(x, 0);
    x = MAX(x, rdr->_clip_bbMinX);

    y = MAX(y, 0);
    y = MAX(y, rdr->_clip_bbMinY);

    maxX = MIN(maxX, rdr->_width);
    maxX = MIN(maxX, rdr->_clip_bbMaxX);

    maxY = MIN(maxY, rdr->_height);
    maxY = MIN(maxY, rdr->_clip_bbMaxY);

    rdr->_clearRect(rdr, x, y, maxX - x, maxY - y);
}

static INLINE void
renderer_drawLine(Renderer* rdr, jint x0, jint y0, jint x1, jint y1) {
    Pipeline* pipeline;

    validateStroker(rdr);
    pipeline = &rdr->_strokerPipeline;

    renderer_beginRendering1(rdr, WIND_NON_ZERO);
    PIPELINE_MOVETO(pipeline, x0, y0);
    PIPELINE_LINETO(pipeline, x1, y1);
    PIPELINE_END(pipeline);
    /** myEndRendering() **/
    endRenderingImpl(rdr);
}

static INLINE void
renderer_drawRect(Renderer* rdr, jint x, jint y, jint w, jint h) {
    if (w > 0 && h > 0) {
        // If dashing is disabled, and using mitered joins,
        // simply draw two opposing rect outlines separated
        // by linewidth
        if (rdr->_dashArray == NULL) {
            if ((rdr->_joinStyle == JOIN_MITER)
                    && (rdr->_miterLimit >= PISCES_SQRT_TWO)) {
                jint x0 = x + PISCES_STROKE_X_BIAS;
                jint y0 = y + PISCES_STROKE_Y_BIAS;
                jint x1 = x0 + w;
                jint y1 = y0 + h;

                jint lw = rdr->_lineWidth;
                jint m = lw/2;

                Pipeline* pipeline;

                validateFiller(rdr);
                pipeline = &rdr->_fillerPipeline;

                renderer_beginRendering1(rdr, WIND_NON_ZERO);
                PIPELINE_MOVETO(pipeline, x0 - m, y0 - m);
                PIPELINE_LINETO(pipeline, x1 + m, y0 - m);
                PIPELINE_LINETO(pipeline, x1 + m, y1 + m);
                PIPELINE_LINETO(pipeline, x0 - m, y1 + m);
                PIPELINE_CLOSE(pipeline);

                // Hollow out interior if w and h are greater than linewidth
                if ((x1 - x0) > lw && (y1 - y0) > lw) {
                    PIPELINE_MOVETO(pipeline, x0 + m, y0 + m);
                    PIPELINE_LINETO(pipeline, x0 + m, y1 - m);
                    PIPELINE_LINETO(pipeline, x1 - m, y1 - m);
                    PIPELINE_LINETO(pipeline, x1 - m, y0 + m);
                    PIPELINE_CLOSE(pipeline);
                }

                PIPELINE_END(pipeline);

                /** myEndRendering() **/
                endRenderingImpl(rdr);
                return;
            } else if (rdr->_joinStyle == JOIN_ROUND) {
                // todo - accelerate hollow rects with round joins
            }
        }

        validateStroker(rdr);
        fillOrDrawRect(&rdr->_strokerPipeline, x, y, w, h);
    }
}

static INLINE void
renderer_fillRect(Renderer* rdr, jint x, jint y, jint w, jint h) {
    if (w > 0 && h > 0) {
        // Renderer will detect aligned rectangles
        validateFiller(rdr);
        fillOrDrawRect(&rdr->_fillerPipeline, x, y, w, h);
    }
}

static INLINE void
renderer_drawOval(Renderer* rdr, jint x, jint y, jint w, jint h) {
    fillOrDrawOval(rdr, x, y, w, h, XNI_TRUE);
}

static INLINE void
renderer_fillOval(Renderer* rdr, jint x, jint y, jint w, jint h) {
    fillOrDrawOval(rdr, x, y, w, h, XNI_FALSE);
}

static INLINE void
renderer_drawRoundRect(Renderer* rdr, jint x, jint y, jint w, jint h,
                       jint aw, jint ah) {
    if (w > 0 && h > 0) {
        fillOrDrawRoundRect(rdr, x, y, w, h, aw, ah, XNI_TRUE);
    }
}

static INLINE void
renderer_fillRoundRect(Renderer* rdr, jint x, jint y, jint w, jint h,
                       jint aw, jint ah) {
    if (w > 0 && h > 0) {
        fillOrDrawRoundRect(rdr, x, y, w, h, aw, ah, XNI_FALSE);
    }
}

static INLINE void
renderer_drawArc(Renderer* rdr, jint x, jint y, jint width, jint height,
                 jint startAngle, jint arcAngle, jint arcType) {
    fillOrDrawArc(rdr, x, y, width, height, startAngle, arcAngle, arcType,
                  XNI_TRUE);
}

static INLINE void
renderer_fillArc(Renderer* rdr, jint x, jint y, jint width, jint height,
                 jint startAngle, jint arcAngle, jint arcType) {
    fillOrDrawArc(rdr, x, y, width, height, startAngle, arcAngle, arcType,
                  XNI_FALSE);
}

static Renderer*
createCommon(Surface* surface) {
    Renderer* rdr = (Renderer*)my_malloc(Renderer, 1);
    Transform6* tr;

    ASSERT_ALLOC_POINTER(rdr);

    tr = &rdr->_transform;

    rdr->_windingRule = WIND_NON_ZERO;
    rdr->_edgeMinY = INTEGER_MAX_VALUE;
    rdr->_edgeMaxY = INTEGER_MIN_VALUE;
    rdr->_crossingMinX = INTEGER_MAX_VALUE;
    rdr->_crossingMaxX = INTEGER_MIN_VALUE;
    rdr->_crossingsSorted = XNI_FALSE;

    rdr->_edges = (jint*)PISCESmalloc(256 * sizeof(jint));
    ASSERT_ALLOC_POINTER(rdr->_edges);

    rdr->_edges_length = 256;

    rdr->_ovalPoints = (jint*)PISCESmalloc(256 * sizeof(jint));
    ASSERT_ALLOC_POINTER(rdr->_ovalPoints);

    rdr->_ovalPoints_length = 256;

    // initialize image type to an invalid value (will be corrected later)
    rdr->_imageType = -1;

    // initialize composite mode
    rdr->_compositeRule = COMPOSITE_SRC_OVER;
    rdr->_compositeAlpha = 1.0;

    // initialize paint mode
    rdr->_paintMode = PAINT_FLAT_COLOR;

    // initialize surface reference
    rdr->_surface = surface;

    rdr->_antialiasingOn = XNI_TRUE;
    setAntialiasing(rdr,
                    DEFAULT_SUBPIXEL_LG_POSITIONS_X,
                    DEFAULT_SUBPIXEL_LG_POSITIONS_Y);

    // initialize the clip region
    rdr->_clip_bbMinX = INTEGER_MIN_VALUE;
    rdr->_clip_bbMinY = INTEGER_MIN_VALUE;
    rdr->_clip_bbMaxX = INTEGER_MAX_VALUE;
    rdr->_clip_bbMaxY = INTEGER_MAX_VALUE;

    // initialize transform
    tr->m00 = 1 << 16;
    tr->m11 = 1 << 16;

    // initialize stroker parameters
    rdr->_lineWidth = 1 << 16;
    rdr->_capStyle = CAP_BUTT;
    rdr->_joinStyle = JOIN_MITER;
    rdr->_miterLimit = 10 << 16;

    // initialize renderer state
    rdr->_rendererState = INVALID_ALL;

    // initialize filler pipeline
    pipeline_initializeFiller(&rdr->_fillerPipeline, rdr);

    // initialize stroker pipeline
    pipeline_initializeStroker(&rdr->_strokerPipeline, rdr);

    // set fill by default
    renderer_setFill(rdr);

    return rdr;
}

static void 
updateInternalColor(Renderer* rdr) {
    if (rdr->_compositeRule == COMPOSITE_CLEAR) {
        rdr->_cred = 0;
        rdr->_cgreen = 0;
        rdr->_cblue = 0;
        rdr->_calpha = 0;
        
        rdr->_rendererState &= ~INVALID_INTERNAL_COLOR;
        return;
    }
    
    // for PBP-TCK test compliance - api.java_awt.alphaComposite.FieldSrcConst
    if (rdr->_compositeAlpha == 0.0f || rdr->_oalpha == 0) {
        rdr->_cred = 0;
        rdr->_cgreen = 0;
        rdr->_cblue = 0;
        
        rdr->_calpha = 0;
        rdr->_rendererState &= ~INVALID_INTERNAL_COLOR;
        return;
    }

    switch (rdr->_imageType) {
        case TYPE_USHORT_565_RGB:
            rdr->_cred = _Pisces_convert8To5[rdr->_ored];
            rdr->_cgreen = _Pisces_convert8To6[rdr->_ogreen];
            rdr->_cblue = _Pisces_convert8To5[rdr->_oblue];
            break;
        default:
            // TYPE_INT_RGB
            // TYPE_INT_ARGB
            // TYPE_INT_ARGB_PRE
            // TYPE_BYTE_GRAY
            rdr->_cred = rdr->_ored;
            rdr->_cgreen = rdr->_ogreen;
            rdr->_cblue = rdr->_oblue;
            break;
    }
    
    rdr->_calpha = (jint)(rdr->_oalpha * rdr->_compositeAlpha + 0.5f);
    rdr->_rendererState &= ~INVALID_INTERNAL_COLOR;
}

static void
updateRendererSurface(Renderer* rdr) {
    Surface* surface = rdr->_surface;
  
    rdr->_width = 
            surface->width;
    rdr->_height = 
            surface->height;
    rdr->_data = 
            surface->data;
    rdr->_imageOffset = 
            surface->offset;
    rdr->_imageScanlineStride = 
            surface->scanlineStride;
    rdr->_imagePixelStride = 
            surface->pixelStride;

    if (rdr->_imageType != surface->imageType) {
        if ((rdr->_compositeRule != COMPOSITE_SRC_OVER) && 
                ((surface->imageType == TYPE_INT_ARGB) || 
                    (surface->imageType == TYPE_INT_ARGB_PRE))) {
            // need to recalculate the alpha map
            // see the implementation of validateAlphaMap
            rdr->_rendererState |= VALIDATE_ALPHA_MAP |
                                   INVALID_COLOR_ALPHA_MAP |
                                   INVALID_PAINT_ALPHA_MAP;
        }
    
        rdr->_imageType = surface->imageType;
        updateSurfaceDependedRoutines(rdr);
    }
    
    rdr->_rendererState &= ~INVALID_RENDERER_SURFACE;
}

static void
updateSurfaceDependedRoutines(Renderer* rdr) {
    switch (rdr->_imageType) {
        case TYPE_INT_RGB:
            rdr->_bl_SourceOver = blitSrcOver888;
            rdr->_bl_PT_SourceOver = blitPTSrcOver888;
            rdr->_bl_Source = blitSrc888;
            rdr->_bl_PT_Source = blitPTSrc888;
            rdr->_bl_Clear = blitSrc888;
            rdr->_bl_PT_Clear = blitSrc888;
            rdr->_clearRect = clearRect8888;
            break;
        case TYPE_INT_ARGB:
            rdr->_bl_SourceOver = blitSrcOver8888;
            rdr->_bl_PT_SourceOver = blitPTSrcOver8888;
            rdr->_bl_Source = blitSrc8888;
            rdr->_bl_PT_Source = blitPTSrc8888;
            rdr->_bl_Clear = blitSrc8888;
            rdr->_bl_PT_Clear = blitSrc8888;
            rdr->_clearRect = clearRect8888;
            break;
        case TYPE_INT_ARGB_PRE:
            rdr->_bl_SourceOver = blitSrcOver8888_pre;
            rdr->_bl_PT_SourceOver = blitPTSrcOver8888_pre;
            rdr->_bl_Source = blitSrc8888_pre;
            rdr->_bl_PT_Source = blitPTSrc8888_pre;
            rdr->_bl_Clear = blitSrc8888_pre;
            rdr->_bl_PT_Clear = blitSrc8888_pre;
            rdr->_clearRect = clearRect8888;
            break;  
        case TYPE_USHORT_565_RGB:
            rdr->_bl_SourceOver = blitSrcOver565;
            rdr->_bl_PT_SourceOver = blitPTSrcOver565;
            rdr->_bl_Source = blitSrc565;
            rdr->_bl_PT_Source = blitPTSrc565;
            rdr->_bl_Clear = blitSrc565;
            rdr->_bl_PT_Clear = blitSrc565;
            rdr->_clearRect = clearRect565;
            break;
        case TYPE_BYTE_GRAY:
            rdr->_bl_SourceOver = blitSrcOver8;
            rdr->_bl_PT_SourceOver = blitPTSrcOver8;
            rdr->_bl_Source = blitSrc8;
            rdr->_bl_PT_Source = blitPTSrc8;
            rdr->_bl_Clear = blitSrc8;
            rdr->_bl_PT_Clear = blitSrc8;
            rdr->_clearRect = clearRect8;
            break;  
        default:
            // unsupported!
            break;
    }
    
    updateCompositeDependedRoutines(rdr);
}

static void
updateCompositeDependedRoutines(Renderer* rdr) {
    switch (rdr->_compositeRule) {
        case COMPOSITE_SRC_OVER:
            rdr->_bl = rdr->_bl_SourceOver;
            rdr->_bl_PT = rdr->_bl_PT_SourceOver;
            break;
        case COMPOSITE_SRC:
            rdr->_bl = rdr->_bl_Source;
            rdr->_bl_PT = rdr->_bl_PT_Source;
            break;
        case COMPOSITE_CLEAR:
            rdr->_bl = rdr->_bl_Clear;
            rdr->_bl_PT = rdr->_bl_PT_Clear;
            break;
        default:
            // unsupported!
            break;
    }

    updatePaintDependedRoutines(rdr);
    rdr->_rendererState &= ~INVALID_COMPOSITE_DEPENDED_ROUTINES;
}

static void
updatePaintDependedRoutines(Renderer* rdr) {
    switch (rdr->_paintMode) {
        case PAINT_LINEAR_GRADIENT:
            rdr->_genPaint = genLinearGradientPaint;
            rdr->_emitRows = rdr->_bl_PT;
            break;
        case PAINT_RADIAL_GRADIENT:
            rdr->_genPaint = genRadialGradientPaint;
            rdr->_emitRows = rdr->_bl_PT;
            break;
        case PAINT_TEXTURE:
            rdr->_genPaint = genTexturePaint;
            rdr->_emitRows = rdr->_bl_PT;
            break;
        case PAINT_FLAT_COLOR:
            rdr->_genPaint = NULL;
            rdr->_emitRows = rdr->_bl;
            break;
        default:
            // unsupported!
            break;
    }

    rdr->_rendererState &= ~INVALID_PAINT_DEPENDED_ROUTINES;
}

static void 
setPaintMode(Renderer* rdr, jint newPaintMode) {
    if (rdr->_paintMode != newPaintMode) {
        my_free(rdr->_texture_intData);
        rdr->_texture_intData = NULL;
        
        // when changing paint mode, the alpha maps should be checked
        rdr->_rendererState |= VALIDATE_ALPHA_MAP | 
                               INVALID_PAINT_DEPENDED_ROUTINES;
        rdr->_paintMode = newPaintMode;
    }
}

/*
   This function maps percentage of pixel overlaping to coresponding alpha.
   If we have _SUBPIXEL_POSITIONS_X x _SUBPIXEL_POSITIONS_Y subpixel grid,
   MAX_AA_ALPHA = _SUBPIXEL_POSITIONS_X * _SUBPIXEL_POSITIONS_Y then. Value of
   rdr->_calpha has meaning of max alpha. i.e. alpha of pixel when fully 
   overlapped (100% covered).
*/

static void
validateAlphaMap(Renderer* rdr) {
    switch (rdr->_paintMode) {
        case PAINT_FLAT_COLOR:
            if (rdr->_rendererState & INVALID_COLOR_ALPHA_MAP) {
                if ((rdr->_compositeRule == COMPOSITE_SRC_OVER) || 
                        (rdr->_imageType == TYPE_INT_ARGB || 
                            rdr->_imageType == TYPE_INT_ARGB_PRE)) {
                    jint i;
                    for (i = 0; i <= rdr->_MAX_AA_ALPHA; i++) {
                        rdr->_colorAlphaMap[i] = 
                                (256 * i * rdr->_calpha + 
                                    rdr->_HALF_MAX_AA_ALPHA_DENOM) /
                                rdr->_MAX_AA_ALPHA_DENOM;
                    }
                }
                
                rdr->_rendererState &= ~INVALID_COLOR_ALPHA_MAP;
            }
            break;
 
        default:       
            // PAINT_LINEAR_GRADIENT
            // PAINT_RADIAL_GRADIENT
            // PAINT_TEXTURE
            if (rdr->_rendererState & INVALID_PAINT_ALPHA_MAP) {
                if ((rdr->_compositeRule == COMPOSITE_SRC_OVER) || 
                        ((rdr->_compositeRule == COMPOSITE_SRC) &&
                        ((rdr->_imageType == TYPE_INT_ARGB) || 
                            (rdr->_imageType == TYPE_INT_ARGB_PRE)))) {
                    jint i;
                    jfloat compositeAlpha = rdr->_compositeAlpha;
                
                    if (compositeAlpha == 1.0f) {
                        for (i = 0; i < 256; i++) {
                            rdr->_paintAlphaMap[i] = i;
                        }
                    } else {
                        for (i = 0; i < 256; i++) {
                            // adding 0.5 improves precision over just 
                            // truncating
                            rdr->_paintAlphaMap[i] = 
                                    (jint)(i * compositeAlpha + 0.5f);
                        }
                    }
                }

                rdr->_rendererState &= ~INVALID_PAINT_ALPHA_MAP;
            }
            break;
    }
}

/**
 * This method initializes antialising variables. All constants for given 
 * subpixel-count are calculated.
*/
static void
setAntialiasing(Renderer* rdr, jint subpixelLgPositionsX,
                jint subpixelLgPositionsY) {
    rdr->_SUBPIXEL_LG_POSITIONS_X = subpixelLgPositionsX;
    rdr->_SUBPIXEL_LG_POSITIONS_Y = subpixelLgPositionsY;
    rdr->_SUBPIXEL_MASK_X = (1 << (rdr->_SUBPIXEL_LG_POSITIONS_X)) - 1;
    rdr->_SUBPIXEL_MASK_Y = (1 << (rdr->_SUBPIXEL_LG_POSITIONS_Y)) - 1;
    rdr->_SUBPIXEL_POSITIONS_X = 1 << (rdr->_SUBPIXEL_LG_POSITIONS_X);
    rdr->_SUBPIXEL_POSITIONS_Y = 1 << (rdr->_SUBPIXEL_LG_POSITIONS_Y);
    rdr->_MAX_AA_ALPHA = (rdr->_SUBPIXEL_POSITIONS_X 
                          * rdr->_SUBPIXEL_POSITIONS_Y);
    rdr->_MAX_AA_ALPHA_DENOM = 255 * rdr->_MAX_AA_ALPHA;
    rdr->_HALF_MAX_AA_ALPHA_DENOM = rdr->_MAX_AA_ALPHA_DENOM/2;
    rdr->_XSHIFT = 16 - rdr->_SUBPIXEL_LG_POSITIONS_X;
    rdr->_YSHIFT = 16 - rdr->_SUBPIXEL_LG_POSITIONS_Y;
    rdr->_YSTEP = 1 << rdr->_YSHIFT;
    rdr->_HYSTEP = 1 << (rdr->_YSHIFT - 1);
    rdr->_YMASK = ~(rdr->_YSTEP - 1);
    
    // how many bytes to left-shift an alpha value from antialiasing to get
    // a number in the range <0, 256>
    rdr->_AA_ALPHA_SHIFT = 8 - (subpixelLgPositionsX + subpixelLgPositionsY);

    rdr->_rendererState |= VALIDATE_ALPHA_MAP |
                           INVALID_COLOR_ALPHA_MAP |
                           INVALID_PAINT_ALPHA_MAP;
}

static void
beginRenderingImpl(Renderer* rdr, jint boundsX, jint boundsY,
                   jint boundsWidth, jint boundsHeight, jint windingRule) {
    rdr->_boundsMinX = boundsX << 16;
    rdr->_boundsMinY = boundsY << 16;
    rdr->_boundsMaxX = (boundsX + boundsWidth) << 16;
    rdr->_boundsMaxY = (boundsY + boundsHeight) << 16;
    rdr->_windingRule = windingRule;

    rdr->_firstOrientation = 0;
    rdr->_lastOrientation = 0;
    rdr->_flips = 0;
    rdr->_edgeIdx = 0;

    rdr->_edgeMinY = INTEGER_MAX_VALUE;
    rdr->_edgeMaxY = INTEGER_MIN_VALUE;
}

static jint
clamp(jint x, jint min, jint max) {
    if (x < min) {
        return min;
    } else if (x > max) {
        return max;
    }
    return x;
}

static void
endRenderingImpl(Renderer* rdr) {
    jint minY, maxY, iminY, imaxY, bminY, bmaxY;
    jint yextent, size, bmax, rows, last;
    jint i, index, maxIdx, fidx, tidx;
    jint bMinX, bMinY, bMaxX, bMaxY;

    VALIDATE_BLITTING(rdr);

    if (rdr->_flips == 0) {
        rdr->_bboxX0 = rdr->_bboxY0 = 0;
        rdr->_bboxX1 = rdr->_bboxY1 = -1;
        return;
    }

    if (rdr->_paintMode == PAINT_FLAT_COLOR &&
            rdr->_calpha == 255 &&
            rdr->_edgeIdx == 10 &&
            rdr->_edges[0] == rdr->_edges[2] &&
            rdr->_edges[1] == rdr->_edges[6] &&
            rdr->_edges[3] == rdr->_edges[8] &&
            rdr->_edges[5] == rdr->_edges[7] &&
            ABS(rdr->_edges[0] - rdr->_edges[5]) > MIN_QUAD_OPT_WIDTH) {

        int x0 = rdr->_edges[0] >> rdr->_XSHIFT;
        int y0 = rdr->_edges[1] >> rdr->_YSHIFT;
        int x1 = rdr->_edges[5] >> rdr->_XSHIFT;
        int y1 = rdr->_edges[3] >> rdr->_YSHIFT;

        if (x0 > x1) {
            jint tmp = x0;
            x0 = x1;
            x1 = tmp;
        }
        if (y0 > y1) {
            jint tmp = y0;
            y0 = y1;
            y1 = tmp;
        }

        /* Clip to image bounds in supersampled coordinates */

        bMinX = rdr->_boundsMinX >> rdr->_XSHIFT;
        bMinY = rdr->_boundsMinY >> rdr->_YSHIFT;
        bMaxX = rdr->_boundsMaxX >> rdr->_XSHIFT;
        bMaxY = rdr->_boundsMaxY >> rdr->_YSHIFT;

        x0 = clamp(x0, bMinX, bMaxX);
        x1 = clamp(x1, bMinX, bMaxX);
        y0 = clamp(y0, bMinY, bMaxY);
        y1 = clamp(y1, bMinY, bMaxY);

        fillRectSrcOver(rdr, rdr->_data, rdr->_imageType,
                        rdr->_imageOffset,
                        rdr->_imageScanlineStride,
                        rdr->_imagePixelStride,
                        rdr->_scrOrient,
                        rdr->_width, rdr->_height,
                        x0, y0, x1, y1,
                        rdr->_cred, rdr->_cgreen, rdr->_cblue);

        /* Check for error in memory allocation */
        if (XNI_TRUE == readMemErrorFlag()) {
            return;
        }

        rdr->_bboxX0 = x0 >> rdr->_SUBPIXEL_LG_POSITIONS_X;
        rdr->_bboxY0 = y0 >> rdr->_SUBPIXEL_LG_POSITIONS_Y;
        rdr->_bboxX1 = (x1 + rdr->_SUBPIXEL_POSITIONS_X - 1) >>
                       rdr->_SUBPIXEL_LG_POSITIONS_X;
        rdr->_bboxY1 = (y1 + rdr->_SUBPIXEL_POSITIONS_Y - 1) >>
                       rdr->_SUBPIXEL_LG_POSITIONS_Y;

        return;
    }

    minY = MAX(rdr->_edgeMinY, rdr->_boundsMinY);
    maxY = MIN(rdr->_edgeMaxY, rdr->_boundsMaxY);
    if (minY > maxY) {
        rdr->_bboxX0 = rdr->_bboxY0 = 0;
        rdr->_bboxX1 = rdr->_bboxY1 = -1;

        return;
    }

    iminY = (minY >> rdr->_YSHIFT) & ~rdr->_SUBPIXEL_MASK_Y;
    imaxY = (maxY >> rdr->_YSHIFT) | rdr->_SUBPIXEL_MASK_Y;
    yextent = (imaxY - iminY) + 1;
    size = rdr->_flips*yextent;
    bmax = (rdr->_boundsMaxY >> rdr->_YSHIFT) - 1;
    if (imaxY > bmax) {
        imaxY = bmax;
    }

    rdr->_bboxX0 = INTEGER_MAX_VALUE;
    rdr->_bboxX1 = INTEGER_MIN_VALUE;

    rdr->_bboxY0 = iminY >> rdr->_SUBPIXEL_LG_POSITIONS_Y;
    rdr->_bboxY1 = (imaxY + rdr->_SUBPIXEL_POSITIONS_Y - 1) >>
                   rdr->_SUBPIXEL_LG_POSITIONS_Y;

    rows = DEFAULT_CROSSINGS_SIZE/(rdr->_flips*rdr->_SUBPIXEL_POSITIONS_Y);
    rows = MIN(rows, yextent);
    rows = MAX(rows, 1);
    for (i = iminY; i <= imaxY; i += rows*rdr->_SUBPIXEL_POSITIONS_Y) {
        last = MIN(i + rows*rdr->_SUBPIXEL_POSITIONS_Y - 1, imaxY);

        setCrossingsExtents(rdr, i, last, rdr->_flips);
        /* Check for error in memory allocation */
        if (XNI_TRUE == readMemErrorFlag()) {
            return;
        }

        bminY = i << rdr->_YSHIFT;
        bmaxY = (last << rdr->_YSHIFT) | ~rdr->_YMASK;

        maxIdx = rdr->_edgeIdx;
        for (index = 0; index < maxIdx; index += 5) {
            if (rdr->_edges[index + 3] < bminY) {
                rdr->_edgeIdx -= 5;
                fidx = rdr->_edgeIdx;
                tidx = index;
                memcpy(&rdr->_edges[tidx], &rdr->_edges[fidx], sizeof(jint)*5);
                maxIdx -= 5;
                index -= 5;
                continue;
            }

            if (rdr->_edges[index + 1] > bmaxY) {
                continue;
            }

            computeCrossingsForEdge(rdr, index, bminY, bmaxY);
            /* Check for error in memory allocation */
            if (XNI_TRUE == readMemErrorFlag()) {
                return;
            }

        }

        computeBounds(rdr);
        if (rdr->_rasterMaxX < rdr->_rasterMinX) {
            continue;
        }

        rdr->_bboxX0 = MIN(rdr->_bboxX0,
                           rdr->_rasterMinX >> rdr->_SUBPIXEL_LG_POSITIONS_X);
        rdr->_bboxX1 = MAX(rdr->_bboxX1,
                           (rdr->_rasterMaxX + rdr->_SUBPIXEL_POSITIONS_X - 1)
                           >> rdr->_SUBPIXEL_LG_POSITIONS_X);
        renderStrip(rdr);
        /* Check for error in memory allocation */
        if (XNI_TRUE == readMemErrorFlag()) {
            return;
        }
    }

    crossingListFinished(rdr);
}

static void
computeCrossingsForEdge(Renderer *rdr, jint index,
                        jint boundsMinY, jint boundsMaxY) {
    jint iy0, iy1, clipy0, clipy1, minY, maxY;
    jint ix0, ix1, dx, dy;
    jint orientation, y;
    jlong lx, xstep;

    iy0 = rdr->_edges[index + 1];
    iy1 = rdr->_edges[index + 3];

    clipy0 = MAX(iy0, boundsMinY);
    clipy1 = MIN(iy1, boundsMaxY);
    minY = ((clipy0 + rdr->_HYSTEP) & rdr->_YMASK) + rdr->_HYSTEP;
    maxY = ((clipy1 - rdr->_HYSTEP) & rdr->_YMASK) + rdr->_HYSTEP;
    if (minY > maxY) {
        return;
    }

    ix0 = rdr->_edges[index];
    ix1 = rdr->_edges[index + 2];

    dx = ix1 - ix0;
    dy = iy1 - iy0;

    orientation = rdr->_edges[index + 4];
    y = minY;
    lx = (jlong)(y - iy0)*dx/dy + ix0;
    addCrossing(rdr, y >> rdr->_YSHIFT, (jint)(lx >> rdr->_XSHIFT), 
                orientation);
    /* Check for error in memory allocation */
    if (XNI_TRUE == readMemErrorFlag()) {
        return;
    }

    y += rdr->_YSTEP;
    if (y > maxY) {
        return;
    }

    xstep = ((jlong)rdr->_YSTEP*dx)/dy;
    for (; y <= maxY; y += rdr->_YSTEP) {
        lx += xstep;
        addCrossing(rdr, y >> rdr->_YSHIFT, (jint)(lx >> rdr->_XSHIFT), 
                    orientation);
        /* Check for error in memory allocation */
        if (XNI_TRUE == readMemErrorFlag()) {
            return;
        }
    }
}

static void
computeBounds(Renderer* rdr) {
    rdr->_rasterMinX = rdr->_crossingMinX & ~rdr->_SUBPIXEL_MASK_X;
    rdr->_rasterMaxX = rdr->_crossingMaxX | rdr->_SUBPIXEL_MASK_X;
    rdr->_rasterMinY = rdr->_crossingMinY & ~rdr->_SUBPIXEL_MASK_Y;
    rdr->_rasterMaxY = rdr->_crossingMaxY | rdr->_SUBPIXEL_MASK_Y;

    if (rdr->_rasterMinX > rdr->_rasterMaxX ||
            rdr->_rasterMinY > rdr->_rasterMaxY) {
        rdr->_rasterMinX = 0;
        rdr->_rasterMaxX = -1;
        rdr->_rasterMinY = 0;
        rdr->_rasterMaxY = -1;
        return;
    }

    if (rdr->_rasterMinX < rdr->_boundsMinX >> rdr->_XSHIFT) {
        rdr->_rasterMinX = rdr->_boundsMinX >> rdr->_XSHIFT;
    }
    if (rdr->_rasterMinY < rdr->_boundsMinY >> rdr->_YSHIFT) {
        rdr->_rasterMinY = rdr->_boundsMinY >> rdr->_YSHIFT;
    }
    if (rdr->_rasterMaxX > rdr->_boundsMaxX >> rdr->_XSHIFT) {
        rdr->_rasterMaxX = rdr->_boundsMaxX >> rdr->_XSHIFT;
    }
    if (rdr->_rasterMaxY > rdr->_boundsMaxY >> rdr->_YSHIFT) {
        rdr->_rasterMaxY = rdr->_boundsMaxY >> rdr->_YSHIFT;
    }
}

static void
renderStrip(Renderer* rdr) {
    jint width, mask, y, prevY;
    jint minX, maxX;
    jint sum, prev;
    jint j;
    jint crxo, crx, crorientation;
    jint x0, x1, x, xmax, xmaxm1;

    // a temporary jint pointer to write as int's
    jint* jintptr = NULL;

    width = (rdr->_rasterMaxX - rdr->_rasterMinX + 1) 
             >> rdr->_SUBPIXEL_LG_POSITIONS_X;
    rdr->_alphaWidth = width;
    // ALLOC3(rdr->_rowAA, jshort, NUM_ALPHA_ROWS*width + 1);
    ALLOC3(rdr->_rowAA, jbyte, NUM_ALPHA_ROWS*width + 1);
    ASSERT_ALLOC(rdr->_rowAA);

    ALLOC3(rdr->_paint, jint, (NUM_ALPHA_ROWS*width + 1)*((jint)sizeof(jint)));
    ASSERT_ALLOC(rdr->_paint);

    mask = (rdr->_windingRule == WIND_EVEN_ODD) ? 0x1 : ~0x0;

    y = 0;
    prevY = rdr->_rasterMinY - 1;

    rdr->_currX = rdr->_rasterMinX >> rdr->_SUBPIXEL_LG_POSITIONS_X;
    rdr->_currY = rdr->_rasterMinY >> rdr->_SUBPIXEL_LG_POSITIONS_Y;
    rdr->_currImageOffset = rdr->_imageOffset +
                            rdr->_currY*rdr->_imageScanlineStride +
                            rdr->_currX*rdr->_imagePixelStride;
    rdr->_rowAAOffset = 0;
    rdr->_rowNum = 0;

    minX = INTEGER_MAX_VALUE;
    maxX = INTEGER_MIN_VALUE;

    iterateCrossings(rdr);
    while (hasMoreCrossingRows(rdr)) {
        y = rdr->_crossingY;

        for (j = prevY + 1; j < y; j++) {
            if (((j & rdr->_SUBPIXEL_MASK_Y) == rdr->_SUBPIXEL_MASK_Y) ||
                    (j == rdr->_rasterMaxY)) {
                emitRow(rdr, 0, -1, XNI_FALSE);
            }
        }
        prevY = y;

        if (rdr->_crossingRowIndex < rdr->_crossingRowCount) {
            jint lx, hx, x0, x1;

            lx = rdr->_crossings[rdr->_crossingRowOffset 
                                 + rdr->_crossingRowIndex];
            lx >>= 1;
            hx = rdr->_crossings[rdr->_crossingRowOffset 
                                 + rdr->_crossingRowCount - 1];
            hx >>= 1;
            x0 = lx > rdr->_rasterMinX ? lx : rdr->_rasterMinX;
            x1 = hx < rdr->_rasterMaxX ? hx : rdr->_rasterMaxX;
            x0 -= rdr->_rasterMinX;
            x1 -= rdr->_rasterMinX;
            x0 >>= rdr->_SUBPIXEL_LG_POSITIONS_X;
            x1 >>= rdr->_SUBPIXEL_LG_POSITIONS_X;

            minX = MIN(minX, x0);
            maxX = MAX(maxX, x1);
        }

        sum = 0;
        prev = rdr->_rasterMinX;
        while (rdr->_crossingRowIndex < rdr->_crossingRowCount) {
            crxo = rdr->_crossings[rdr->_crossingRowOffset +
                                   rdr->_crossingRowIndex];
            rdr->_crossingRowIndex++;

            crx = crxo >> 1;
            crorientation = ((crxo & 0x1) == 0x1) ? 1 : -1;
            if ((sum & mask) != 0) {
                x0 = MAX(prev, rdr->_rasterMinX);
                x1 = MIN(crx, rdr->_rasterMaxX);
                if (x1 > x0) {
                    x0 -= rdr->_rasterMinX;
                    x1 -= rdr->_rasterMinX;

                    x = x0 >> rdr->_SUBPIXEL_LG_POSITIONS_X;
                    xmaxm1 = (x1 - 1) >> rdr->_SUBPIXEL_LG_POSITIONS_X;
                    xmax = x1 >> rdr->_SUBPIXEL_LG_POSITIONS_X;

                    if (x == xmaxm1) {
                        rdr->_rowAA[x + rdr->_rowAAOffset] += x1 - x0;
                    } else if ((xmax-x) <= 32) {
                        jbyte *tmpRowAA = &rdr->_rowAA[rdr->_rowAAOffset + x];
                        *tmpRowAA++ += rdr->_SUBPIXEL_POSITIONS_X 
                                       - (x0 & rdr->_SUBPIXEL_MASK_X);
                        x++;

                        while (x < xmax) {
                            *tmpRowAA++ += rdr->_SUBPIXEL_POSITIONS_X;
                            x++;
                        }

                        *tmpRowAA += x1 & rdr->_SUBPIXEL_MASK_X;
                    } else {
                        jint count;
                        jint rowAdd = (rdr->_SUBPIXEL_POSITIONS_X << 24) | 
                                      (rdr->_SUBPIXEL_POSITIONS_X << 16) | 
                                      (rdr->_SUBPIXEL_POSITIONS_X << 8)  | 
                                       rdr->_SUBPIXEL_POSITIONS_X;

                        jbyte *tmpRowAA = &rdr->_rowAA[rdr->_rowAAOffset + x];

                        /* Have to record this as we could clobber the last byte 
                           with the int writes */
                        jbyte tmpAA = tmpRowAA[xmax-x];

                        *tmpRowAA++ += rdr->_SUBPIXEL_POSITIONS_X - 
                                       (x0 & rdr->_SUBPIXEL_MASK_X);
                        x++;

                        /* assume as there are at least 8 values this won't 
                        overrun */
                        while ((int)tmpRowAA & 0x03) {
                            *tmpRowAA++ += rdr->_SUBPIXEL_POSITIONS_X;
                            x++;
                        }

                        /* tmpRowAA should be aligned */
                        count = (xmax - x) >> 2;

                        // a temporary jint pointer to write as int's
                        jintptr = (jint*) tmpRowAA;

                        while (count--) {
                            *jintptr++ += rowAdd;

                            x+=sizeof(jint);
                        }

                        // further operations are to a jbyte pointer
                        // so assign back to original
                        tmpRowAA = (jbyte*)jintptr;

                        while (x < xmax) {
                            *tmpRowAA++ += rdr->_SUBPIXEL_POSITIONS_X;
                            x++;
                        }

                        *tmpRowAA = tmpAA + (x1 & rdr->_SUBPIXEL_MASK_X);
                    }
                }
            }
            sum += crorientation;
            prev = crx;
        }

        if (((y & rdr->_SUBPIXEL_MASK_Y) == rdr->_SUBPIXEL_MASK_Y) ||
                (y == rdr->_rasterMaxY)) {
            emitRow(rdr, minX, maxX, XNI_FALSE);
            minX = INTEGER_MAX_VALUE;
            maxX = INTEGER_MIN_VALUE;
        }
    }

    for (j = prevY + 1; j <= rdr->_rasterMaxY; j++) {
        if (((j & rdr->_SUBPIXEL_MASK_Y) == rdr->_SUBPIXEL_MASK_Y) ||
                (j == rdr->_rasterMaxY)) {
            emitRow(rdr, minX, maxX, XNI_FALSE);
        }
    }

    if (rdr->_rowAAOffset != 0) {
        emitRow(rdr, minX, maxX, XNI_TRUE);
    }
}

static void
clearAlpha(jbyte* alpha, jint alphaOffset, jint alphaStride,
           jint width, jint height, jint *minTouched, jint *maxTouched) {
    jint j, minX, maxX, aidx, w;
//  jint i, j, minX, maxX, aidx, w;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];

        if (maxX >= minX) {
            w = maxX - minX + 1;
            if (w + minX > width) {
                w = width - minX;
            }

            aidx = alphaOffset + minX;
            memset(&alpha[aidx], 0, w);
        }

        alphaOffset += alphaStride;
    }
}

static void
emitRow(Renderer* rdr, jint minX, jint maxX, jboolean forceOutput) {
    rdr->_minTouched[rdr->_rowNum] = minX;
    rdr->_maxTouched[rdr->_rowNum] = maxX;

    rdr->_rowAAOffset += rdr->_alphaWidth;
    rdr->_rowNum++;
    if (forceOutput || rdr->_rowNum == NUM_ALPHA_ROWS) {
        if (rdr->_genPaint) {
            rdr->_genPaint(rdr, rdr->_rowNum);
        }

        rdr->_emitRows(rdr, rdr->_rowNum);
        clearAlpha(rdr->_rowAA, 0, rdr->_alphaWidth,
                   rdr->_alphaWidth, rdr->_rowNum,
                   rdr->_minTouched, rdr->_maxTouched);

        rdr->_currY += rdr->_rowNum;
        rdr->_currImageOffset += rdr->_rowNum*rdr->_imageScanlineStride;
        rdr->_rowAAOffset = 0;
        rdr->_rowNum = 0;
    }
}

static void
setCrossingsExtents(Renderer* rdr, jint minY, jint maxY, jint maxXEntries) {
    jint yextent;

    yextent = maxY - minY + 1;

    ALLOC(rdr->_crossings, jint, yextent*maxXEntries,
          DEFAULT_CROSSINGS_SIZE);
    ASSERT_ALLOC(rdr->_crossings);

    ALLOC(rdr->_crossingIndices, jint, yextent, DEFAULT_INDICES_SIZE);
    ASSERT_ALLOC(rdr->_crossingIndices);

    rdr->_crossingMinY = minY;
    rdr->_crossingMaxY = maxY;
    rdr->_crossingMaxXEntries = maxXEntries;
    resetCrossings(rdr);
}

static void
resetCrossings(Renderer* rdr) {
    jint yextent, start;
    jint i;

    yextent = rdr->_crossingMaxY - rdr->_crossingMinY + 1;
    start = 0;
    for (i = 0; i < yextent; i++) {
        rdr->_crossingIndices[i] = start;
        start += rdr->_crossingMaxXEntries;
    }
    rdr->_crossingMinX = INTEGER_MAX_VALUE;
    rdr->_crossingMaxX = INTEGER_MIN_VALUE;
    rdr->_numCrossings = 0;
    rdr->_crossingsSorted = XNI_FALSE;
}

static void
crossingListFinished(Renderer* rdr) {
    SHRINK(rdr->_crossings, jint, DEFAULT_CROSSINGS_SIZE);
    ASSERT_ALLOC(rdr->_crossings);

    SHRINK(rdr->_crossingIndices, jint, DEFAULT_INDICES_SIZE);
    ASSERT_ALLOC(rdr->_crossingIndices);
}

static void
sortCrossings3(jint* x, jint off, jint len) {
    jint i, j, xj, xjm1;

    for (i = off + 1; i < off + len; i++) {
        j = i;
        xj = x[j];
//    xjm1;

        while (j > off && (xjm1 = x[j - 1]) > xj) {
            x[j] = xjm1;
            x[j - 1] = xj;
            j--;
        }
    }
}

static void
sortCrossings(Renderer* rdr) {
    jint start, i;

    start = 0;
    for (i = 0; i <= rdr->_crossingMaxY - rdr->_crossingMinY; i++) {
        sortCrossings3(rdr->_crossings, start,
                       rdr->_crossingIndices[i] - start);
        start += rdr->_crossingMaxXEntries;
    }
}

static void
addCrossing(Renderer* rdr, jint y, jint x, jint orientation) {
    jint index;

    if (x < rdr->_crossingMinX) {
        rdr->_crossingMinX = x;
    }
    if (x > rdr->_crossingMaxX) {
        rdr->_crossingMaxX = x;
    }

    index = rdr->_crossingIndices[y - rdr->_crossingMinY]++;
    x <<= 1;
    rdr->_crossings[index] = (orientation == 1) ? (x | 0x1) : x;
    ++rdr->_numCrossings;
}

static void
iterateCrossings(Renderer* rdr) {
    if (!rdr->_crossingsSorted) {
        sortCrossings(rdr);
        rdr->_crossingsSorted = XNI_TRUE;
    }
    rdr->_crossingY = rdr->_crossingMinY - 1;
    rdr->_crossingRowOffset = -rdr->_crossingMaxXEntries;
}

static jboolean
hasMoreCrossingRows(Renderer* rdr) {
    jint y;

    if (++rdr->_crossingY <= rdr->_crossingMaxY) {
        rdr->_crossingRowOffset += rdr->_crossingMaxXEntries;
        y = rdr->_crossingY - rdr->_crossingMinY;
        rdr->_crossingRowCount = rdr->_crossingIndices[y] -
                                 y*rdr->_crossingMaxXEntries;
        rdr->_crossingRowIndex = 0;
        return XNI_TRUE;
    } else {
        return XNI_FALSE;
    }
}

static void
validateStroker(Renderer* rdr) {
    if (!rdr->_validStroker) {
        pipeline_setStrokerParameters(&rdr->_strokerPipeline, rdr->_lineWidth, 
                                      rdr->_capStyle, rdr->_joinStyle,
                                      rdr->_miterLimit, rdr->_dashArray, 
                                      rdr->_dashArray_length, rdr->_dashPhase, 
                                      &rdr->_transform);
        rdr->_validStroker = XNI_TRUE;
    }
}

static void
validateFiller(Renderer* rdr) {
    if (!rdr->_validFiller) {
        pipeline_setFillerParameters(&rdr->_fillerPipeline, &rdr->_transform);
        rdr->_validFiller = XNI_TRUE;
    }
}

static void
fillOrDrawRect(Pipeline* pipeline, jint x, jint y, jint w, jint h) {
    jint x0 = x;
    jint y0 = y;
    jint x1 = x0 + w;
    jint y1 = y0 + h;

    renderer_beginRendering1((Renderer*)pipeline->param, WIND_NON_ZERO);
    PIPELINE_MOVETO(pipeline, x0, y0);
    PIPELINE_LINETO(pipeline, x1, y0);
    PIPELINE_LINETO(pipeline, x1, y1);
    PIPELINE_LINETO(pipeline, x0, y1);
    PIPELINE_CLOSE(pipeline);
    PIPELINE_END(pipeline);
    /** myEndRendering() **/
    endRenderingImpl((Renderer*)pipeline->param);
}

// Emit quarter-arc about a central point (cx, cy).
// Each quadrant is suitably reflected
static void
emitQuadrants(Pipeline* pipeline, jint cx, jint cy,
              jint* points, jint nPoints) {
    jint pass;

    // Emit quarter-arc once for each quadrant, suitably
    // reflected
    for (pass = 0; pass < 4; pass++) {
        jint xsign = 1;
        jint ysign = 1;
        jint incr, idx, j;
        if (pass == 1 || pass == 2) xsign = -1;
        if (pass == 2 || pass == 3) ysign = -1;
        incr = 2*xsign*ysign;
        idx = (incr > 0) ? 0 : 2*nPoints - 2;

        for (j = 0; j < nPoints; j++) {
            PIPELINE_LINETO(pipeline, cx + xsign*points[idx],
                            cy + ysign*points[idx + 1]);
            idx += incr;
        }
    }
}

static void
emitOval(Pipeline* pipeline, jint cx, jint cy, jint rx, jint ry,
         jint nPoints, jboolean reverse) {
    Renderer* rdr = (Renderer*)pipeline->param;

    jint i = reverse ? nPoints - 1 : 1;
    jint incr = reverse ? -1 : 1;
    jint* points;
    jint j;

    jint idx = 0;
    jint nSize;

    PIPELINE_MOVETO(pipeline, cx + rx, cy);

    nPoints /= 4;
    nSize = 2*nPoints;
    REALLOC(rdr->_ovalPoints, jint, nSize, rdr->_ovalPoints_length * 2);
    ASSERT_ALLOC(rdr->_ovalPoints);

    points = rdr->_ovalPoints;
    for (j = 0; j < nPoints; j++) {
        jint theta = i*PISCES_TWO_PI/(4*nPoints);
        jint ox = piscesmath_cos(theta);
        jint oy = piscesmath_sin(theta);
        points[idx++] = (jint)((jlong)rx*ox >> 16);
        points[idx++] = (jint)((jlong)ry*oy >> 16);

        i += incr;
    }

    emitQuadrants(pipeline, cx, cy, points, nPoints);
    PIPELINE_CLOSE(pipeline);
}

// Emit the outline of an oval, offset by pen radius lw2
// The interior path may self-intersect, but this is handled
// by using a WIND_NON_ZERO wining rule
static void
emitOffsetOval(Pipeline* pipeline, jint cx, jint cy, jint rx, jint ry,
               jint lw2, jint nPoints, jboolean inside) {
    Renderer* rdr = (Renderer*)pipeline->param;

    jint i = inside ? nPoints - 1 : 1;
    jint incr = inside ? -1 : 1;
    jint* points;
    jint j;

    jdouble drx = rx/65536.0;
    jdouble dry = ry/65536.0;
    jdouble dlw2 = lw2/65536.0;

    jint idx = 0;
    jint nSize;

    PIPELINE_MOVETO(pipeline, cx + rx + lw2*incr, cy);

    nPoints /= 4;
    nSize = 2*nPoints;
    REALLOC(rdr->_ovalPoints, jint, nSize, rdr->_ovalPoints_length * 2);
    ASSERT_ALLOC(rdr->_ovalPoints);

    points = rdr->_ovalPoints;
    for (j = 0; j < nPoints; j++) {
        jdouble dtheta = i*(PI_DOUBLE/2.0)/nPoints;
        jdouble cosTheta = PISCEScos(dtheta);
        jdouble sinTheta = PISCESsin(dtheta);
        jdouble drxSinTheta = drx*sinTheta;
        jdouble dryCosTheta = dry*cosTheta;
        jdouble den = dlw2/PISCESsqrt(drxSinTheta*drxSinTheta + 
                                      dryCosTheta*dryCosTheta);
        jdouble dpx = cosTheta*(drx + incr*dry*den);
        jdouble dpy = sinTheta*(dry + incr*drx*den);

        jint px = (jint)(dpx*65536.0);
        jint py = (jint)(dpy*65536.0);

        points[idx++] = px;
        points[idx++] = py;

        i += incr;
    }

    emitQuadrants(pipeline, cx, cy, points, nPoints);
    PIPELINE_CLOSE(pipeline);
}

static void
fillOrDrawOval(Renderer* rdr, jint x, jint y, jint w, jint h, jboolean hollow) {
    jint w2, h2;
    jint cx, cy;
    jint wl, hl;
    jint lineWidth2;
    jint nPoints;
    jint tmp;

    if (w <= 0 || h <= 0) {
        return;
    }

    w2 = w >> 1;
    h2 = h >> 1;
    cx = x + w2;
    cy = y + h2;
    lineWidth2 = hollow ? rdr->_lineWidth/2 : 0;

    wl = w2 + lineWidth2;
    hl = h2 + lineWidth2;
    tmp = MAX(wl, hl) >> 13;
    nPoints = MAX(16, tmp);

    renderer_beginRendering1(rdr, WIND_NON_ZERO);

    // Stroke the outline if dashing
    if (hollow && rdr->_dashArray != NULL) {
        validateStroker(rdr);
        emitOval(&rdr->_strokerPipeline, cx, cy, w2, h2, nPoints, XNI_FALSE);
        PIPELINE_END(&rdr->_strokerPipeline);
        /** myEndRendering() **/
        endRenderingImpl(rdr);
        return;
    }

    if (!rdr->_antialiasingOn) {
        cx += PISCES_STROKE_X_BIAS;
    }

    validateFiller(rdr);

    if (w == h) {
        emitOval(&rdr->_fillerPipeline, cx, cy, wl, hl, nPoints, XNI_FALSE);
    } else {
        emitOffsetOval(&rdr->_fillerPipeline, cx, cy, w2, h2, lineWidth2,
                       nPoints, XNI_FALSE);
    }

    // Draw interior in the reverse direction
    if (hollow) {
        wl = w2 - lineWidth2;
        hl = h2 - lineWidth2;

        if (wl > 0 && hl > 0) {
            if (w == h) {
                emitOval(&rdr->_fillerPipeline, cx, cy, wl, hl, nPoints, 
                         XNI_TRUE);
            } else {
                emitOffsetOval(&rdr->_fillerPipeline, cx, cy, w2, h2, 
                               lineWidth2, nPoints, XNI_TRUE);
            }
        }
    }

    PIPELINE_END(&rdr->_fillerPipeline);
    /** myEndRendering() **/
    endRenderingImpl(rdr);
}

static void
emitRoundRect(Pipeline* pipeline, jint x, jint y, jint w, jint h,
              jint aw, jint ah, jboolean reverse) {
    jint xw = x + w;
    jint yh = y + h;

    jint aw2 = aw >> 1;
    jint ah2 = ah >> 1;
    jint acvaw = (jint)(PISCES_ACV*aw >> 16);
    jint acvah = (jint)(PISCES_ACV*ah >> 16);
    jint xacvaw = x + acvaw;
    jint xw_acvaw = xw - acvaw;
    jint yacvah = y + acvah;
    jint yh_acvah = yh - acvah;
    jint xaw2 = x + aw2;
    jint xw_aw2 = xw - aw2;
    jint yah2 = y + ah2;
    jint yh_ah2 = yh - ah2;

    PIPELINE_MOVETO(pipeline, x, yah2);
    if (reverse) {
        PIPELINE_CUBICTO(pipeline, x, yacvah, xacvaw, y, xaw2, y);
        PIPELINE_LINETO(pipeline, xw_aw2, y);
        PIPELINE_CUBICTO(pipeline, xw_acvaw, y, xw, yacvah, xw, yah2);
        PIPELINE_LINETO(pipeline, xw, yh_ah2);
        PIPELINE_CUBICTO(pipeline, xw, yh_acvah, xw_acvaw, yh, xw_aw2, yh);
        PIPELINE_LINETO(pipeline, xaw2, yh);
        PIPELINE_CUBICTO(pipeline, xacvaw, yh, x, yh_acvah, x, yh_ah2);
    } else {
        PIPELINE_LINETO(pipeline, x, yh_ah2);
        PIPELINE_CUBICTO(pipeline, x, yh_acvah, xacvaw, yh, xaw2, yh);
        PIPELINE_LINETO(pipeline, xw_aw2, yh);
        PIPELINE_CUBICTO(pipeline, xw_acvaw, yh, xw, yh_acvah, xw, yh_ah2);
        PIPELINE_LINETO(pipeline, xw, yah2);
        PIPELINE_CUBICTO(pipeline, xw, yacvah, xw_acvaw, y, xw_aw2, y);
        PIPELINE_LINETO(pipeline, xaw2, y);
        PIPELINE_CUBICTO(pipeline, xacvaw, y, x, yacvah, x, yah2);
    }
    PIPELINE_CLOSE(pipeline);
    PIPELINE_END(pipeline);
}

static void
fillOrDrawRoundRect(Renderer* rdr, jint x, jint y, jint w, jint h,
                    jint aw, jint ah, jboolean stroke) {
    Pipeline* pipeline;
    if (stroke) {
        validateStroker(rdr);
        pipeline = &rdr->_strokerPipeline;
    } else {
        validateFiller(rdr);
        pipeline = &rdr->_fillerPipeline;
    }

    if (aw < 0) aw = -aw;
    if (aw > w) aw = w;
    if (ah < 0) ah = -ah;
    if (ah > h) ah = h;

    // If stroking but not dashing, draw the outer and inner
    // contours explicitly as round rects
    //
    // Note - this only works if aw == ah since the result of tracing
    // a circle with a circular pen is a larger circle, but the result
    // of tracing an ellipse with a circular pen is not (generally)
    // an ellipse...
    if (stroke && rdr->_dashArray == NULL && aw == ah) {
        jint lineWidth = rdr->_lineWidth;
        jint lineWidth2 = lineWidth >> 1;

        validateFiller(rdr);

        renderer_beginRendering1(rdr, WIND_NON_ZERO);

        x += PISCES_STROKE_X_BIAS;
        y += PISCES_STROKE_Y_BIAS;

        emitRoundRect(&rdr->_fillerPipeline, x - lineWidth2, y - lineWidth2,
                      w + lineWidth, h + lineWidth, aw + lineWidth, 
                      ah + lineWidth, XNI_FALSE);

        // Empty out inner rect
        w -= lineWidth;
        h -= lineWidth;
        if (w > 0 && h > 0) {
            emitRoundRect(&rdr->_fillerPipeline, x + lineWidth2, y + lineWidth2,
                          w, h, aw - lineWidth, ah - lineWidth,
                          XNI_TRUE);
        }
        /** myEndRendering() **/
        endRenderingImpl(rdr);
    } else {
        renderer_beginRendering1(rdr, WIND_NON_ZERO);
        emitRoundRect(pipeline, x, y, w, h, aw, ah, XNI_FALSE);
        /** myEndRendering() **/
        endRenderingImpl(rdr);
    }
}

static void
emitArc(Pipeline* pipeline, jint cx, jint cy, jint rx, jint ry,
        jint startAngle, jint endAngle, jint nPoints) {
    jboolean first = XNI_TRUE;
    jint i;

    for (i = 0; i < nPoints; i++) {
        jint theta = startAngle + i*(endAngle - startAngle)/(nPoints - 1);
        jint ox = piscesmath_cos(theta);
        jint oy = piscesmath_sin(theta);

        jint lx = cx + (jint)((jlong)rx*ox >> 16);
        jint ly = cy - (jint)((jlong)ry*oy >> 16);
        if (first) {
            PIPELINE_MOVETO(pipeline, lx, ly);
            first = XNI_FALSE;
        } else {
            PIPELINE_LINETO(pipeline, lx, ly);
        }
    }
}

static void
fillOrDrawArc(Renderer* rdr, jint x, jint y, jint width, jint height,
              jint startAngle, jint arcAngle, jint arcType, jboolean stroke) {
    jint w2, h2;
    jint cx, cy;
    jint endAngle;
    jint nPoints;
    jint tmp;

    Pipeline* pipeline;
    if (stroke) {
        validateStroker(rdr);
        pipeline = &rdr->_strokerPipeline;
    } else {
        validateFiller(rdr);
        pipeline = &rdr->_fillerPipeline;
    }

    if (width <= 0 || height <= 0) {
        return;
    }

    w2 = width >> 1;
    h2 = height >> 1;
    cx = x + w2;
    cy = y + h2;

    startAngle = (jint)(((jlong)startAngle*PISCES_TWO_PI)/(360*65536));
    arcAngle = (jint)(((jlong)arcAngle*PISCES_TWO_PI)/(360*65536));

    endAngle = startAngle + arcAngle;

    tmp = MAX(w2, h2) >> 16;
    nPoints = MAX(16, tmp);

    renderer_beginRendering1(rdr, WIND_NON_ZERO);
    emitArc(pipeline, cx, cy, w2, h2, startAngle, endAngle, nPoints);
    if (arcType == ARC_PIE) {
        PIPELINE_LINETO(pipeline, cx, cy);
    }
    if (!stroke || (arcType == ARC_CHORD) || (arcType == ARC_PIE)) {
        PIPELINE_CLOSE(pipeline);
    }
    PIPELINE_END(pipeline);
    /** myEndRendering() **/
    endRenderingImpl(rdr);
}
