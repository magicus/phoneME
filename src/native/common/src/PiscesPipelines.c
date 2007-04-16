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


#include <PiscesPipelines.h>

#include <PiscesMath.h>
#include <PiscesUtil.h>
#include <PiscesRenderer.h>

#include <PiscesSysutils.h>

#define FLATTENER_MAX_CHORD_LENGTH_SQ ((jlong)16L*16L*65536L*65536L)
#define FLATTENER_MIN_CHORD_LENGTH_SQ ((jlong)65536L*65536L/(2L*2L))

#define FLATTENER_LG_FLATNESS 0 // half pixel, 2^(-1)
#define FLATTENER_FLATNESS_SQ_SHIFT (2*(-LG_FLATNESS))

#define ROUND_JOIN_THRESHOLD ((jlong)1000L)
#define ROUND_JOIN_INTERNAL_THRESHOLD ((jlong)1000000000L)

#define STROKER_MOVE_TO 0
#define STROKER_LINE_TO 1
#define STROKER_CLOSE 2

#define DEFAULT_FILLER_FLATNESS (1 << 15)

static jint
math_min(jint a, jint b) {
    return (a < b) ? a : b;
}

static jint
math_max(jint a, jint b) {
    return (a > b) ? a : b;
}

static void transformer_setTransform(Pipeline* pipeline,
                                     Transform6* transform);

static void flattener_setFlatness(Pipeline* pipeline, jint flatness);
static void flattener_moveTo(Pipeline* pipeline, jint x0, jint y0);
static void flattener_lineJoin(Pipeline* pipeline);
static void flattener_lineTo(Pipeline* pipeline, jint x1, jint y1);
static void flattener_quadTo(Pipeline* pipeline, jint x1, jint y1,
                             jint x2, jint y2);
static void flattener_cubicTo(Pipeline* pipeline, jint x1, jint y1,
                              jint x2, jint y2,
                              jint x3, jint y3);
static void flattener_close(Pipeline* pipeline);
static void flattener_end(Pipeline* pipeline);

static void bbox_moveTo(Pipeline* pipeline, jint x0, jint y0);
static void bbox_lineJoin(Pipeline* pipeline);
static void bbox_lineTo(Pipeline* pipeline, jint x1, jint y1);
static void bbox_close(Pipeline* pipeline);
static void bbox_end(Pipeline* pipeline);

static void stroker_setParameters(Pipeline* pipeline, jint lineWidth,
                                  jint capStyle, jint joinStyle, 
                                  jint miterLimit, Transform6* transform);
static void stroker_moveTo(Pipeline* pipeline, jint x0, jint y0);
static void stroker_lineJoin(Pipeline* pipeline);
static void stroker_lineTo(Pipeline* pipeline, jint x1, jint y1);
static void stroker_close(Pipeline* pipeline);
static void stroker_end(Pipeline* pipeline);

static void stroker_lineToImpl(Pipeline* pipeline, jint x1, jint y1,
                               jboolean joinSegment);

static void dasher_setParameters(Pipeline* pipeline, jint* dash,
                                 jint dashLength, jint phase, 
                                 Transform6* transform);
static void dasher_moveTo(Pipeline* pipeline, jint x0, jint y0);
static void dasher_lineJoin(Pipeline* pipeline);
static void dasher_lineTo(Pipeline* pipeline, jint x1, jint y1);
static void dasher_close(Pipeline* pipeline);
static void dasher_end(Pipeline* pipeline);

static jboolean flattener_flatEnough_3(Pipeline* pipeline,
                                       jint x0, jint y0, jint x1, 
                                       jint y1, jint x2, jint y2);
static void flattener_quadToHelper(Pipeline* pipeline,
                                   jint x1, jint y1, jint x2, jint y2);
static jboolean flattener_flatEnough_4(Pipeline* pipeline,
                                       jint x0, jint y0, jint x1, 
                                       jint y1, jint x2, jint y2, 
                                       jint x3, jint y3);
static void flattener_cubicToHelper(Pipeline* pipeline,
                                    jint x1, jint y1, jint x2, 
                                    jint y2, jint x3, jint y3);
static void stroker_computeOffset(Pipeline* pipeline,
                                  jint x0, jint y0, 
                                  jint x1, jint y1, jint* m);
static jboolean stroker_isCCW(jint x0, jint y0, jint x1, jint y1,
                              jint x2, jint y2);
static jboolean stroker_side(jint x, jint y, jint x0, jint y0,
                             jint x1, jint y1);
static void stroker_emitLineTo(Pipeline* pipeline, jint x1, jint y1,
                               jboolean rev);
static jint stroker_computeRoundJoin(Pipeline* pipeline,
                                     jint cx, jint cy, jint xa, jint ya, 
                                     jint xb, jint yb, jint side,
                                     jboolean flip, jint* join);
static void stroker_drawRoundJoin(Pipeline* pipeline, jint x, jint y,
                                  jint omx, jint omy, jint mx, 
                                  jint my, jint side, jboolean flip, 
                                  jboolean rev, jlong threshold);
static void stroker_computeMiter(jint ix0, jint iy0, jint ix1, jint iy1,
                                 jint ix0p, jint iy0p, jint ix1p, jint iy1p, 
                                 jint* m);
static void stroker_drawMiter(Pipeline* pipeline, jint px0, jint py0,
                              jint x0, jint y0, jint x1, jint y1, jint omx, 
                              jint omy, jint mx, jint my, jboolean rev);
static jlong stroker_lineLength(Pipeline* pipeline,
                                jlong ldx, jlong ldy);
static void stroker_finish(Pipeline* pipeline);
static void dasher_goTo(Pipeline* pipeline, jint x1, jint y1);
static void bbox_checkPoint(Pipeline* pipeline, jint x0, jint y0);

void
pipeline_free(Pipeline* pipeline) {
    my_free(pipeline->stroker_pen_dx);
    my_free(pipeline->stroker_pen_dy);
    my_free(pipeline->stroker_penIncluded);
    my_free(pipeline->stroker_join);
    my_free(pipeline->stroker_reverse);
}

void
pipeline_initializeFiller(Pipeline* pipeline, void* param) {
    pipeline->param = param;
    pipeline->flattener_next_moveTo = prenderer_moveTo;
    pipeline->flattener_next_lineJoin = prenderer_lineJoin;
    pipeline->flattener_next_lineTo = prenderer_lineTo;
    pipeline->flattener_next_close = prenderer_close;
    pipeline->flattener_next_end = prenderer_end;
    flattener_setFlatness(pipeline, DEFAULT_FILLER_FLATNESS);
}

void
pipeline_initializeStroker(Pipeline* pipeline, void* param) {
    pipeline->param = param;
    pipeline->stroker_pen_dx = (jint*)PISCESmalloc(16 * sizeof(jint));
    ASSERT_ALLOC(pipeline->stroker_pen_dx);

    pipeline->stroker_pen_dx_length = 16;
    pipeline->stroker_pen_dy = (jint*)PISCESmalloc(16 * sizeof(jint));
    ASSERT_ALLOC(pipeline->stroker_pen_dy);

    pipeline->stroker_pen_dy_length = 16;
    pipeline->stroker_penIncluded = 
                                (jboolean*)PISCESmalloc(16 * sizeof(jboolean));
    ASSERT_ALLOC(pipeline->stroker_penIncluded);

    pipeline->stroker_penIncluded_length = 16;
    pipeline->stroker_join = (jint*)PISCESmalloc(32 * sizeof(jint));
    ASSERT_ALLOC(pipeline->stroker_join);

    pipeline->stroker_join_length = 32;
    pipeline->stroker_reverse = (jint*)PISCESmalloc(128 * sizeof(jint));
    ASSERT_ALLOC(pipeline->stroker_reverse);

    pipeline->stroker_reverse_length = 128;
    pipeline->stroker_next_moveTo = prenderer_moveTo;
    pipeline->stroker_next_lineJoin = prenderer_lineJoin;
    pipeline->stroker_next_lineTo = prenderer_lineTo;
    pipeline->stroker_next_close = prenderer_close;
    pipeline->stroker_next_end = prenderer_end;
    flattener_setFlatness(pipeline, 1 << 16);
}

void
pipeline_initializeBBox(Pipeline* pipeline, void* param) {
    pipeline->param = param;
    pipeline->flattener_next_moveTo = stroker_moveTo;
    pipeline->flattener_next_lineJoin = stroker_lineJoin;
    pipeline->flattener_next_lineTo = stroker_lineTo;
    pipeline->flattener_next_close = stroker_close;
    pipeline->flattener_next_end = stroker_end;

    pipeline->stroker_pen_dx = (jint*)PISCESmalloc(16 * sizeof(jint));
    ASSERT_ALLOC(pipeline->stroker_pen_dx);

    pipeline->stroker_pen_dx_length = 16;
    pipeline->stroker_pen_dy = (jint*)PISCESmalloc(16 * sizeof(jint));
    ASSERT_ALLOC(pipeline->stroker_pen_dy);

    pipeline->stroker_pen_dy_length = 16;
    pipeline->stroker_penIncluded = 
                                (jboolean*)PISCESmalloc(16 * sizeof(jboolean));
    ASSERT_ALLOC(pipeline->stroker_penIncluded);

    pipeline->stroker_penIncluded_length = 16;
    pipeline->stroker_join = (jint*)PISCESmalloc(32 * sizeof(jint));
    ASSERT_ALLOC(pipeline->stroker_join);

    pipeline->stroker_join_length = 32;
    pipeline->stroker_reverse = (jint*)PISCESmalloc(128 * sizeof(jint));
    ASSERT_ALLOC(pipeline->stroker_reverse);

    pipeline->stroker_reverse_length = 128;
    pipeline->stroker_next_moveTo = bbox_moveTo;
    pipeline->stroker_next_lineJoin = bbox_lineJoin;
    pipeline->stroker_next_lineTo = bbox_lineTo;
    pipeline->stroker_next_close = bbox_close;
    pipeline->stroker_next_end = bbox_end;
    flattener_setFlatness(pipeline, 1 << 16);
}

void
pipeline_setFillerParameters(Pipeline* pipeline,
                             Transform6* transform) {
    transformer_setTransform(pipeline, transform);
}

void
pipeline_setStrokerParameters(Pipeline* pipeline,
                              jint lineWidth, jint capStyle, jint joinStyle,
                              jint miterLimit, jint* dashArray, jint dashLength,
                              jint dashPhase, Transform6* transform) {
    Transform6 biasedTransform;
    pisces_transform_assign(&biasedTransform, transform);
    biasedTransform.m02 += PISCES_STROKE_X_BIAS;
    biasedTransform.m12 += PISCES_STROKE_Y_BIAS;
    transformer_setTransform(pipeline, &biasedTransform);
    stroker_setParameters(pipeline, lineWidth, capStyle, joinStyle, miterLimit,
                          transform);
    if (dashArray == NULL) {
        pipeline->dasher_dash = NULL;
        pipeline->flattener_next_moveTo = stroker_moveTo;
        pipeline->flattener_next_lineJoin = stroker_lineJoin;
        pipeline->flattener_next_lineTo = stroker_lineTo;
        pipeline->flattener_next_close = stroker_close;
        pipeline->flattener_next_end = stroker_end;
    } else {
        pipeline->flattener_next_moveTo = dasher_moveTo;
        pipeline->flattener_next_lineJoin = dasher_lineJoin;
        pipeline->flattener_next_lineTo = dasher_lineTo;
        pipeline->flattener_next_close = dasher_close;
        pipeline->flattener_next_end = dasher_end;
        dasher_setParameters(pipeline, dashArray, dashLength, dashPhase,
                             transform);
    }
}

void
pipeline_setBBoxParameters(Pipeline* pipeline,
                           jint lineWidth, jint miterLimit,
                           jint m00, jint m01, jint m10, jint m11, jint m02, 
                           jint m12) {
    Transform6 biasedTransform;
    biasedTransform.m00 = m00;
    biasedTransform.m01 = m01;
    biasedTransform.m10 = m10;
    biasedTransform.m11 = m11;
    biasedTransform.m02 = m02 + PISCES_STROKE_X_BIAS;
    biasedTransform.m12 = m12 + PISCES_STROKE_Y_BIAS;
    transformer_setTransform(pipeline, &biasedTransform);
    stroker_setParameters(pipeline, lineWidth, CAP_SQUARE, JOIN_MITER, 
                          miterLimit, &biasedTransform);
}

void
transformer_moveTo(Pipeline* pipeline, jint x0, jint y0) {
    jlong tx0, ty0;

    if (pipeline->transformer_scaleAndTranslate) {
        tx0 = pipeline->transformer_m00*x0 + pipeline->transformer_m02;
        ty0 = pipeline->transformer_m11*y0 + pipeline->transformer_m12;
    } else {
        tx0 = pipeline->transformer_m00*x0 + pipeline->transformer_m01*y0 
              + pipeline->transformer_m02;
        ty0 = pipeline->transformer_m10*x0 + pipeline->transformer_m11*y0 
              + pipeline->transformer_m12;
    }

    flattener_moveTo(pipeline, (jint)(tx0 >> 16), (jint)(ty0 >> 16));
}

void
transformer_lineJoin(Pipeline* pipeline) {
    flattener_lineJoin(pipeline);
}

void
transformer_lineTo(Pipeline* pipeline, jint x1, jint y1) {
    jlong tx1, ty1;

    if (pipeline->transformer_scaleAndTranslate) {
        tx1 = pipeline->transformer_m00*x1 + pipeline->transformer_m02;
        ty1 = pipeline->transformer_m11*y1 + pipeline->transformer_m12;
    } else {
        tx1 = pipeline->transformer_m00*x1 + pipeline->transformer_m01*y1 
              + pipeline->transformer_m02;
        ty1 = pipeline->transformer_m10*x1 + pipeline->transformer_m11*y1 
              + pipeline->transformer_m12;
    }

    flattener_lineTo(pipeline, (jint)(tx1 >> 16), (jint)(ty1 >> 16));
}

void
transformer_quadTo(Pipeline* pipeline, jint x1, jint y1,
                   jint x2, jint y2) {
    jlong tx1, ty1, tx2, ty2;

    if (pipeline->transformer_scaleAndTranslate) {
        tx1 = pipeline->transformer_m00*x1 + pipeline->transformer_m02;
        ty1 = pipeline->transformer_m11*y1 + pipeline->transformer_m12;
        tx2 = pipeline->transformer_m00*x2 + pipeline->transformer_m02;
        ty2 = pipeline->transformer_m11*y2 + pipeline->transformer_m12;
    } else {
        tx1 = pipeline->transformer_m00*x1 + pipeline->transformer_m01*y1 
              + pipeline->transformer_m02;
        ty1 = pipeline->transformer_m10*x1 + pipeline->transformer_m11*y1 
              + pipeline->transformer_m12;
        tx2 = pipeline->transformer_m00*x2 + pipeline->transformer_m01*y2 
              + pipeline->transformer_m02;
        ty2 = pipeline->transformer_m10*x2 + pipeline->transformer_m11*y2 
              + pipeline->transformer_m12;
    }

    flattener_quadTo(pipeline, (jint)(tx1 >> 16), (jint)(ty1 >> 16),
                     (jint)(tx2 >> 16), (jint)(ty2 >> 16));
}

void
transformer_cubicTo(Pipeline* pipeline, jint x1, jint y1,
                    jint x2, jint y2, jint x3, jint y3) {
    jlong tx1, ty1, tx2, ty2, tx3, ty3;

    if (pipeline->transformer_scaleAndTranslate) {
        tx1 = pipeline->transformer_m00*x1 + pipeline->transformer_m02;
        ty1 = pipeline->transformer_m11*y1 + pipeline->transformer_m12;
        tx2 = pipeline->transformer_m00*x2 + pipeline->transformer_m02;
        ty2 = pipeline->transformer_m11*y2 + pipeline->transformer_m12;
        tx3 = pipeline->transformer_m00*x3 + pipeline->transformer_m02;
        ty3 = pipeline->transformer_m11*y3 + pipeline->transformer_m12;
    } else {
        tx1 = pipeline->transformer_m00*x1 + pipeline->transformer_m01*y1 
              + pipeline->transformer_m02;
        ty1 = pipeline->transformer_m10*x1 + pipeline->transformer_m11*y1 
              + pipeline->transformer_m12;
        tx2 = pipeline->transformer_m00*x2 + pipeline->transformer_m01*y2 
              + pipeline->transformer_m02;
        ty2 = pipeline->transformer_m10*x2 + pipeline->transformer_m11*y2 
              + pipeline->transformer_m12;
        tx3 = pipeline->transformer_m00*x3 + pipeline->transformer_m01*y3 
              + pipeline->transformer_m02;
        ty3 = pipeline->transformer_m10*x3 + pipeline->transformer_m11*y3 
              + pipeline->transformer_m12;
    }

    flattener_cubicTo(pipeline, (jint)(tx1 >> 16), (jint)(ty1 >> 16),
                      (jint)(tx2 >> 16), (jint)(ty2 >> 16),
                      (jint)(tx3 >> 16), (jint)(ty3 >> 16));
}

void
transformer_close(Pipeline* pipeline) {
    flattener_close(pipeline);
}

void
transformer_end(Pipeline* pipeline) {
    flattener_end(pipeline);
}

void
bbox_reset(Pipeline* pipeline) {
    pipeline->bbox_minX = INTEGER_MAX_VALUE;
    pipeline->bbox_minY = INTEGER_MAX_VALUE;
    pipeline->bbox_maxX = INTEGER_MIN_VALUE;
    pipeline->bbox_maxY = INTEGER_MIN_VALUE;
}

void
bbox_get(Pipeline* pipeline, jint* bbox) {
    bbox[MIN_X] = pipeline->bbox_minX;
    bbox[MIN_Y] = pipeline->bbox_minY;
    bbox[MAX_X] = pipeline->bbox_maxX;
    bbox[MAX_Y] = pipeline->bbox_maxY;
}

static void
transformer_setTransform(Pipeline* pipeline, Transform6* transform) {
    pipeline->transformer_m00 = (jlong)transform->m00;
    pipeline->transformer_m01 = (jlong)transform->m01;
    pipeline->transformer_m02 = (jlong)transform->m02 << 16;
    pipeline->transformer_m10 = (jlong)transform->m10;
    pipeline->transformer_m11 = (jlong)transform->m11;
    pipeline->transformer_m12 = (jlong)transform->m12 << 16;

    if (transform->m01 == 0 && transform->m10 == 0) {
        pipeline->transformer_scaleAndTranslate = XNI_TRUE;
    } else {
        pipeline->transformer_scaleAndTranslate = XNI_FALSE;
    }
}

static void
flattener_setFlatness(Pipeline* pipeline, jint flatness) {
    pipeline->flattener_flatness = flatness;
    pipeline->flattener_flatnessSq = (jint)((jlong)flatness*flatness >> 16);
}

static void
flattener_moveTo(Pipeline* pipeline, jint x0, jint y0) {
    pipeline->flattener_next_moveTo(pipeline, x0, y0);
    pipeline->flattener_x0 = x0;
    pipeline->flattener_y0 = y0;
}

static void
flattener_lineJoin(Pipeline* pipeline) {
    pipeline->flattener_next_lineJoin(pipeline);
}

static void
flattener_lineTo(Pipeline* pipeline, jint x1, jint y1) {
    pipeline->flattener_next_lineJoin(pipeline);
    pipeline->flattener_next_lineTo(pipeline, x1, y1);
    pipeline->flattener_x0 = x1;
    pipeline->flattener_y0 = y1;
}

static void
flattener_quadTo(Pipeline* pipeline, jint x1, jint y1,
                 jint x2, jint y2) {
    pipeline->flattener_next_lineJoin(pipeline);
    flattener_quadToHelper(pipeline, x1, y1, x2, y2);
}

static void
flattener_cubicTo(Pipeline* pipeline, jint x1, jint y1,
                  jint x2, jint y2,
                  jint x3, jint y3) {
    pipeline->flattener_next_lineJoin(pipeline);
    flattener_cubicToHelper(pipeline, x1, y1, x2, y2, x3, y3);
}

static void
flattener_close(Pipeline* pipeline) {
    pipeline->flattener_next_lineJoin(pipeline);
    pipeline->flattener_next_close(pipeline);
}

static void
flattener_end(Pipeline* pipeline) {
    pipeline->flattener_next_end(pipeline);
}

static void
stroker_setParameters(Pipeline* pipeline, jint lineWidth,
                      jint capStyle,
                      jint joinStyle,
                      jint miterLimit,
                      Transform6* transform) {
    jdouble dm00, dm01;
    jdouble dm10, dm11;
    jdouble determinant;
    jint newSize, i, numPenSegments;
    jint* pen_dx;
    jint* pen_dy;

    jint m00, m01;
    jint m10, m11;

    jint lineWidth2 = lineWidth >> 1;

    m00 = pipeline->stroker_m00 = transform->m00;
    m01 = pipeline->stroker_m01 = transform->m01;
    m10 = pipeline->stroker_m10 = transform->m10;
    m11 = pipeline->stroker_m11 = transform->m11;

    pipeline->stroker_lineWidth = lineWidth;
    pipeline->stroker_lineWidth2 = lineWidth2;
    pipeline->stroker_scaledLineWidth2 = (jlong)m00*lineWidth2;
    pipeline->stroker_capStyle = capStyle;
    pipeline->stroker_joinStyle = joinStyle;
    pipeline->stroker_miterLimit = miterLimit;

    pipeline->stroker_m00_2_m01_2 = (jdouble)m00*m00 + (jdouble)m01*m01;
    pipeline->stroker_m10_2_m11_2 = (jdouble)m10*m10 + (jdouble)m11*m11;
    pipeline->stroker_m00_m10_m01_m11 = (jdouble)m00*m10 + (jdouble)m01*m11;

    dm00 = m00/65536.0;
    dm01 = m01/65536.0;
    dm10 = m10/65536.0;
    dm11 = m11/65536.0;
    determinant = dm00*dm11 - dm01*dm10;

    if (joinStyle == JOIN_MITER) {
        jdouble limit = (miterLimit/65536.0)*(lineWidth2/65536.0)*determinant;
        jdouble limitSq = limit*limit;
        pipeline->stroker_miterLimitSq = (jlong)(limitSq*65536.0*65536.0);
    }

    numPenSegments = (jint)(3.14159f*lineWidth/65536.0f);
    REALLOC(pipeline->stroker_pen_dx, jint, numPenSegments,
            pipeline->stroker_pen_dx_length * 2);
    ASSERT_ALLOC(pipeline->stroker_pen_dx);

    REALLOC(pipeline->stroker_pen_dy, jint, numPenSegments,
            pipeline->stroker_pen_dy_length * 2);
    ASSERT_ALLOC(pipeline->stroker_pen_dy);

    REALLOC(pipeline->stroker_penIncluded, jboolean, numPenSegments,
            pipeline->stroker_penIncluded_length * 2);
    ASSERT_ALLOC(pipeline->stroker_penIncluded);

    newSize = 2 * numPenSegments;
    REALLOC(pipeline->stroker_join, jint, newSize,
            pipeline->stroker_join_length * 2);
    ASSERT_ALLOC(pipeline->stroker_join);

    pipeline->stroker_numPenSegments = numPenSegments;

    pen_dx = pipeline->stroker_pen_dx;
    pen_dy = pipeline->stroker_pen_dy;
    for (i = 0; i < numPenSegments; i++) {
        jdouble r = lineWidth/2.0;
        jdouble theta = (jdouble)i*2.0*PI_DOUBLE/numPenSegments;

        jdouble _cos = PISCEScos(theta);
        jdouble _sin = PISCESsin(theta);
        pen_dx[i] = (jint)(r*(dm00*_cos + dm01*_sin));
        pen_dy[i] = (jint)(r*(dm10*_cos + dm11*_sin));
    }

    pipeline->stroker_prev = STROKER_CLOSE;
    pipeline->stroker_rindex = 0;
    pipeline->stroker_started = XNI_FALSE;
    pipeline->stroker_lineToOrigin = XNI_FALSE;
}

static void
stroker_moveTo(Pipeline* pipeline, jint x0, jint y0) {
    if (pipeline->stroker_lineToOrigin) {
        // not closing the path, do the previous lineTo
        stroker_lineToImpl(pipeline, pipeline->stroker_sx0, 
                           pipeline->stroker_sy0, 
                           pipeline->stroker_joinToOrigin);
        pipeline->stroker_lineToOrigin = XNI_FALSE;
    }

    if (pipeline->stroker_prev == STROKER_LINE_TO) {
        stroker_finish(pipeline);
    }

    pipeline->stroker_sx0 = pipeline->stroker_x0 = x0;
    pipeline->stroker_sy0 = pipeline->stroker_y0 = y0;
    pipeline->stroker_rindex = 0;
    pipeline->stroker_started = XNI_FALSE;
    pipeline->stroker_joinSegment = XNI_FALSE;
    pipeline->stroker_prev = STROKER_MOVE_TO;
}

static void
stroker_lineJoin(Pipeline* pipeline) {
    pipeline->stroker_joinSegment = XNI_TRUE;
}

static void
stroker_lineTo(Pipeline* pipeline, jint x1, jint y1) {
    if (pipeline->stroker_lineToOrigin) {
        if (x1 == pipeline->stroker_sx0 && y1 == pipeline->stroker_sy0) {
            // staying in the starting point
            return;
        }

        // not closing the path, do the previous lineTo
        stroker_lineToImpl(pipeline, pipeline->stroker_sx0, 
                           pipeline->stroker_sy0,
                           pipeline->stroker_joinToOrigin);
        pipeline->stroker_lineToOrigin = XNI_FALSE;
    } else if (x1 == pipeline->stroker_x0 && y1 == pipeline->stroker_y0) {
        return;
    } else if (x1 == pipeline->stroker_sx0 && y1 == pipeline->stroker_sy0) {
        pipeline->stroker_lineToOrigin = XNI_TRUE;
        pipeline->stroker_joinToOrigin = pipeline->stroker_joinSegment;
        pipeline->stroker_joinSegment = XNI_FALSE;
        return;
    }

    stroker_lineToImpl(pipeline, x1, y1, pipeline->stroker_joinSegment);
    pipeline->stroker_joinSegment = XNI_FALSE;
}

static void
stroker_lineToImpl(Pipeline* pipeline, jint x1, jint y1, jboolean joinSegment) {
    jint offset[2];
    jint mx, my;

    jint x0 = pipeline->stroker_x0;
    jint y0 = pipeline->stroker_y0;

    stroker_computeOffset(pipeline, x0, y0, x1, y1, offset);
    mx = offset[0];
    my = offset[1];

    if (!pipeline->stroker_started) {
        pipeline->stroker_next_moveTo(pipeline, x0 + mx, y0 + my);
        pipeline->stroker_sx1 = x1;
        pipeline->stroker_sy1 = y1;
        pipeline->stroker_mx0 = mx;
        pipeline->stroker_my0 = my;
        pipeline->stroker_started = XNI_TRUE;
    } else {
        jboolean ccw = stroker_isCCW(pipeline->stroker_px0, 
                                     pipeline->stroker_py0,
                                     x0, y0, x1, y1);
        if (joinSegment) {
            if (pipeline->stroker_joinStyle == JOIN_MITER) {
                stroker_drawMiter(pipeline, pipeline->stroker_px0, 
                                  pipeline->stroker_py0, x0, y0, x1, y1, 
                                  pipeline->stroker_omx, pipeline->stroker_omy,
                                  mx, my, ccw);
            } else if (pipeline->stroker_joinStyle == JOIN_ROUND) {
                stroker_drawRoundJoin(pipeline, x0, y0,
                                      pipeline->stroker_omx, 
                                      pipeline->stroker_omy,
                                      mx, my, 0, XNI_FALSE, ccw, 
                                      ROUND_JOIN_THRESHOLD);
            }
        } else {
            // Draw internal joins as round
            stroker_drawRoundJoin(pipeline, x0, y0, pipeline->stroker_omx, 
                                  pipeline->stroker_omy, mx, my, 0, XNI_FALSE, 
                                  ccw, ROUND_JOIN_INTERNAL_THRESHOLD);
        }

        stroker_emitLineTo(pipeline, x0, y0, (jboolean)!ccw);
    }

    stroker_emitLineTo(pipeline, x0 + mx, y0 + my, XNI_FALSE);
    stroker_emitLineTo(pipeline, x1 + mx, y1 + my, XNI_FALSE);

    stroker_emitLineTo(pipeline, x0 - mx, y0 - my, XNI_TRUE);
    stroker_emitLineTo(pipeline, x1 - mx, y1 - my, XNI_TRUE);

    pipeline->stroker_lx0 = x1 + mx;
    pipeline->stroker_ly0 = y1 + my;
    pipeline->stroker_lx0p = x1 - mx;
    pipeline->stroker_ly0p = y1 - my;
    pipeline->stroker_lx1 = x1;
    pipeline->stroker_ly1 = y1;

    pipeline->stroker_omx = mx;
    pipeline->stroker_omy = my;
    pipeline->stroker_px0 = x0;
    pipeline->stroker_py0 = y0;
    pipeline->stroker_x0 = x1;
    pipeline->stroker_y0 = y1;
    pipeline->stroker_prev = STROKER_LINE_TO;
}

static void
stroker_close(Pipeline* pipeline) {
    jint offset[2];
    jint mx, my;
    jboolean ccw;

    jint i;
    jint x0, y0;
    jint sx0, sy0;
    jint mx0, my0;
    jint joinStyle;
    jint* reverse;

    if (pipeline->stroker_lineToOrigin) {
        // ignore the previous lineTo
        pipeline->stroker_lineToOrigin = XNI_FALSE;
    }

    if (!pipeline->stroker_started) {
        stroker_finish(pipeline);
        return;
    }

    x0 = pipeline->stroker_x0;
    y0 = pipeline->stroker_y0;
    sx0 = pipeline->stroker_sx0;
    sy0 = pipeline->stroker_sy0;
    mx0 = pipeline->stroker_mx0;
    my0 = pipeline->stroker_my0;
    joinStyle = pipeline->stroker_joinStyle;

    stroker_computeOffset(pipeline, x0, y0, sx0, sy0, offset);
    mx = offset[0];
    my = offset[1];

    // Draw penultimate join
    ccw = stroker_isCCW(pipeline->stroker_px0, pipeline->stroker_py0,
                        x0, y0, sx0, sy0);
    if (pipeline->stroker_joinSegment) {
        if (joinStyle == JOIN_MITER) {
            stroker_drawMiter(pipeline, pipeline->stroker_px0, 
                              pipeline->stroker_py0, x0, y0, sx0, sy0, 
                              pipeline->stroker_omx, pipeline->stroker_omy,
                              mx, my, ccw);
        } else if (joinStyle == JOIN_ROUND) {
            stroker_drawRoundJoin(pipeline, x0, y0, pipeline->stroker_omx, 
                                  pipeline->stroker_omy, mx, my, 0, XNI_FALSE, 
                                  ccw, ROUND_JOIN_THRESHOLD);
        }
    } else {
        // Draw internal joins as round
        stroker_drawRoundJoin(pipeline, x0, y0, pipeline->stroker_omx, 
                              pipeline->stroker_omy, mx, my, 0, XNI_FALSE, ccw, 
                              ROUND_JOIN_INTERNAL_THRESHOLD);
    }

    pipeline->stroker_next_lineTo(pipeline, x0 + mx, y0 + my);
    pipeline->stroker_next_lineTo(pipeline, sx0 + mx, sy0 + my);

    ccw = stroker_isCCW(x0, y0, sx0, sy0, pipeline->stroker_sx1, 
                        pipeline->stroker_sy1);

    // Draw final join on the outside
    if (!ccw) {
        if (joinStyle == JOIN_MITER) {
            stroker_drawMiter(pipeline, x0, y0, sx0, sy0,
                              pipeline->stroker_sx1, pipeline->stroker_sy1,
                              mx, my, mx0, my0, XNI_FALSE);
        } else if (joinStyle == JOIN_ROUND) {
            stroker_drawRoundJoin(pipeline, sx0, sy0, mx, my,
                                  mx0, my0, 0, XNI_FALSE, XNI_FALSE, 
                                  ROUND_JOIN_THRESHOLD);
        }
    }

    pipeline->stroker_next_lineTo(pipeline, sx0 + mx0, sy0 + my0);
    // same as reverse[0], reverse[1]
    pipeline->stroker_next_lineTo(pipeline, sx0 - mx0, sy0 - my0);  

    // Draw final join on the inside
    if (ccw) {
        if (joinStyle == JOIN_MITER) {
            stroker_drawMiter(pipeline, x0, y0, sx0, sy0,
                              pipeline->stroker_sx1, pipeline->stroker_sy1,
                              -mx, -my, -mx0, -my0, XNI_FALSE);
        } else if (joinStyle == JOIN_ROUND) {
            stroker_drawRoundJoin(pipeline, sx0, sy0, -mx, -my, -mx0, -my0, 0, 
                                  XNI_TRUE, XNI_FALSE, ROUND_JOIN_THRESHOLD);
        }
    }

    pipeline->stroker_next_lineTo(pipeline, sx0 - mx, sy0 - my);
    pipeline->stroker_next_lineTo(pipeline, x0 - mx, y0 - my);
    reverse = pipeline->stroker_reverse;
    for (i = pipeline->stroker_rindex - 2; i >= 0; i -= 2) {
        pipeline->stroker_next_lineTo(pipeline, reverse[i], reverse[i + 1]);
    }

    pipeline->stroker_joinSegment = XNI_FALSE;
    pipeline->stroker_prev = STROKER_CLOSE;
    pipeline->stroker_next_close(pipeline);
}

static void
stroker_end(Pipeline* pipeline) {
    if (pipeline->stroker_lineToOrigin) {
        // not closing the path, do the previous lineTo
        stroker_lineToImpl(pipeline, pipeline->stroker_sx0, 
                           pipeline->stroker_sy0, 
                           pipeline->stroker_joinToOrigin);
        pipeline->stroker_lineToOrigin = XNI_FALSE;
    }

    if (pipeline->stroker_prev == STROKER_LINE_TO) {
        stroker_finish(pipeline);
    }

    pipeline->stroker_next_end(pipeline);
    pipeline->stroker_joinSegment = XNI_FALSE;
    pipeline->stroker_prev = STROKER_MOVE_TO;
}

static void
dasher_setParameters(Pipeline* pipeline, jint* dash, jint dashLength,
                     jint phase, Transform6* transform) {
    // Normalize so 0 <= phase < dash[0]
    jint idx = 0;
    jint d;
    while (phase >= (d = dash[idx])) {
        phase -= d;
        idx = (idx + 1) % dashLength;
    }

    pipeline->dasher_dash = dash;
    pipeline->dasher_dash_length = dashLength;
    pipeline->dasher_startPhase = pipeline->dasher_phase = phase;
    pipeline->dasher_startIdx = idx;

    pipeline->dasher_m00 = transform->m00;
    pipeline->dasher_m01 = transform->m01;
    pipeline->dasher_m10 = transform->m10;
    pipeline->dasher_m11 = transform->m11;
    pipeline->dasher_ldet = ((jlong)transform->m00*transform->m11 -
                             (jlong)transform->m01*transform->m10) >> 16;
    pipeline->dasher_symmetric = (transform->m00 == transform->m11 &&
                                  transform->m10 == -transform->m01);
}

static void
dasher_moveTo(Pipeline* pipeline, jint x0, jint y0) {
    stroker_moveTo(pipeline, x0, y0);
    pipeline->dasher_idx = pipeline->dasher_startIdx;
    pipeline->dasher_phase = pipeline->dasher_startPhase;
    pipeline->dasher_sx = pipeline->dasher_x0 = x0;
    pipeline->dasher_sy = pipeline->dasher_y0 = y0;
    pipeline->dasher_starting = XNI_TRUE;
}

static void
dasher_lineJoin(Pipeline* pipeline) {
    stroker_lineJoin(pipeline);
}

static void
dasher_lineTo(Pipeline* pipeline, jint x1, jint y1) {
    jint* dash = pipeline->dasher_dash;
    jint dash_length = pipeline->dasher_dash_length;

    while (1) {
        jint d = dash[pipeline->dasher_idx] - pipeline->dasher_phase;
        jint lx = x1 - pipeline->dasher_x0;
        jint ly = y1 - pipeline->dasher_y0;

        jlong t;
        jint xsplit, ysplit;

        jint l;

        if (pipeline->dasher_symmetric) {
            l = (jint)(((jlong)piscesmath_ihypot(lx, ly)*65536)
                        /pipeline->dasher_ldet);
        } else {
            jlong la = ((jlong)ly*pipeline->dasher_m00 -
                        (jlong)lx*pipeline->dasher_m10)/pipeline->dasher_ldet;
            jlong lb = ((jlong)ly*pipeline->dasher_m01 -
                        (jlong)lx*pipeline->dasher_m11)/pipeline->dasher_ldet;
            l = (jint)piscesmath_lhypot(la, lb);
        }

        if (l < d) {
            dasher_goTo(pipeline, x1, y1);
            // Advance phase within current dash segment
            pipeline->dasher_phase += l;
            return;
        }

        t = ((jlong)d << 16)/l;
        xsplit = pipeline->dasher_x0 + (jint)(t*(x1 - pipeline->dasher_x0) 
                 >> 16);
        ysplit = pipeline->dasher_y0 + (jint)(t*(y1 - pipeline->dasher_y0) 
                 >> 16);
        dasher_goTo(pipeline, xsplit, ysplit);

        // Advance to next dash segment
        pipeline->dasher_idx = (pipeline->dasher_idx + 1) % dash_length;
        pipeline->dasher_phase = 0;
    }
}

static void
dasher_close(Pipeline* pipeline) {
    dasher_lineTo(pipeline, pipeline->dasher_sx, pipeline->dasher_sy);
    if (pipeline->dasher_firstDashOn) {
        stroker_lineTo(pipeline, pipeline->dasher_sx1, pipeline->dasher_sy1);
    }
}

static void
dasher_end(Pipeline* pipeline) {
    stroker_end(pipeline);
}

static void
bbox_moveTo(Pipeline* pipeline, jint x0, jint y0) {
    bbox_checkPoint(pipeline, x0, y0);
}

static void
bbox_lineJoin(Pipeline* pipeline) {
    // IMPL NOTE : to fix warning
    (void) pipeline;
}

static void
bbox_lineTo(Pipeline* pipeline, jint x1, jint y1) {
    bbox_checkPoint(pipeline, x1, y1);
}

static void
bbox_close(Pipeline* pipeline) {
    // IMPL NOTE : to fix warning
    (void) pipeline;
}

static void
bbox_end(Pipeline* pipeline) {
    // IMPL NOTE : to fix warning
    (void) pipeline;
}

static jboolean
flattener_flatEnough_3(Pipeline* pipeline, jint x0, jint y0,
                       jint x1, jint y1,
                       jint x2, jint y2) {
    jlong num, numsq, df2;

    jlong dx = (jlong)x2 - (jlong)x0;
    jlong dy = (jlong)y2 - (jlong)y0;
    jlong denom2 = dx*dx + dy*dy;
    if (denom2 > FLATTENER_MAX_CHORD_LENGTH_SQ) {
        return XNI_FALSE;
    }

    // Stop dividing if all control points are close together
    if (denom2 < FLATTENER_MIN_CHORD_LENGTH_SQ) {
        jint minx = math_min(math_min(x0, x1), x2);
        jint miny = math_min(math_min(y0, y1), y2);
        jint maxx = math_max(math_max(x0, x1), x2);
        jint maxy = math_max(math_max(y0, y1), y2);

        jlong dx1 = (jlong)maxx - (jlong)minx;
        jlong dy1 = (jlong)maxy - (jlong)miny;
        jlong l2 = dx1*dx1 + dy1*dy1;
        if (l2 < FLATTENER_MIN_CHORD_LENGTH_SQ) {
            return XNI_TRUE;
        }
    }

    num = -dy*x1 + dx*y1 + ((jlong)x0*y2 - (jlong)x2*y0);
    num >>= 16;
    numsq = num*num;
    df2 = denom2*pipeline->flattener_flatnessSq >> 16;
    return numsq < df2;
}

static void
flattener_quadToHelper(Pipeline* pipeline,
                       jint x1, jint y1, jint x2, jint y2) {
    if (flattener_flatEnough_3(pipeline, pipeline->flattener_x0, 
                               pipeline->flattener_y0, x1, y1, x2, y2)) {
        pipeline->flattener_next_lineTo(pipeline, x1, y1);
        pipeline->flattener_next_lineTo(pipeline, x2, y2);
    } else {
        jlong lx1 = (jlong)x1;
        jlong ly1 = (jlong)y1;
        jlong x01 = pipeline->flattener_x0 + lx1; // >> 1
        jlong y01 = pipeline->flattener_y0 + ly1; // >> 1
        jlong x12 = lx1 + x2; // >> 1
        jlong y12 = ly1 + y2; // >> 1

        jlong x012 = x01 + x12; // >> 2
        jlong y012 = y01 + y12; // >> 2

        flattener_quadToHelper(pipeline, (jint)(x01 >> 1), (jint)(y01 >> 1),
                               (jint)(x012 >> 2), (jint)(y012 >> 2));
        flattener_quadToHelper(pipeline, (jint)(x12 >> 1), (jint)(y12 >> 1),
                               x2, y2);
    }

    pipeline->flattener_x0 = x2;
    pipeline->flattener_y0 = y2;
}

// IMPL_NOTE - analyze position of radix points to avoid possibility
// of overflow
static jboolean
flattener_flatEnough_4(Pipeline* pipeline, jint x0, jint y0,
                       jint x1, jint y1,
                       jint x2, jint y2,
                       jint x3, jint y3) {
    jlong df2, cross, num1, num1sq, num2, num2sq;

    jlong dx = (jlong)x3 - (jlong)x0;          // S47.16
    jlong dy = (jlong)y3 - (jlong)y0;          // S47.16
    jlong denom2 = dx*dx + dy*dy; // S31.32
    // Always subdivide curves with a large chord length
    if (denom2 > FLATTENER_MAX_CHORD_LENGTH_SQ) {
        return XNI_FALSE;
    }

    // Stop dividing if all control points are close together
    if (denom2 < FLATTENER_MIN_CHORD_LENGTH_SQ) {
        jint minx = math_min(math_min(math_min(x0, x1), x2), x3);
        jint miny = math_min(math_min(math_min(y0, y1), y2), y3);
        jint maxx = math_max(math_max(math_max(x0, x1), x2), x3);
        jint maxy = math_max(math_max(math_max(y0, y1), y2), y3);

        jlong dx1 = (jlong)maxx - (jlong)minx;
        jlong dy1 = (jlong)maxy - (jlong)miny;
        jlong l2 = dx1*dx1 + dy1*dy1;
        if (l2 < FLATTENER_MIN_CHORD_LENGTH_SQ) {
            return XNI_TRUE;
        }
    }

    // Want to know if num/denom < flatness, so compare
    // numerator^2 against (denominator*flatness)^2 to avoid a square root
    df2 = denom2*pipeline->flattener_flatnessSq >> 16; // S31.32

    cross = (jlong)x0*y3 - (jlong)x3*y0; // S31.32
    num1 = dx*y1 - dy*x1 + cross;      // S31.32
    num1 >>= 16;                       // S47.16
    num1sq = num1*num1;                // S31.32
    if (num1sq > df2) {
        return XNI_FALSE;
    }

    num2 = dx*y2 - dy*x2 + cross;      // S31.32
    num2 >>= 16;                       // S47.16
    num2sq = num2*num2;                // S31.32

    return num2sq < df2;
}

static void
flattener_cubicToHelper(Pipeline* pipeline,
                        jint x1, jint y1,
                        jint x2, jint y2,
                        jint x3, jint y3) {
    if (flattener_flatEnough_4(pipeline,
                               pipeline->flattener_x0,
                               pipeline->flattener_y0,
                               x1, y1, x2, y2, x3, y3)) {
        pipeline->flattener_next_lineTo(pipeline, x1, y1);
        pipeline->flattener_next_lineTo(pipeline, x2, y2);
        pipeline->flattener_next_lineTo(pipeline, x3, y3);
    } else {
        jlong lx1 = (jlong)x1;
        jlong ly1 = (jlong)y1;
        jlong lx2 = (jlong)x2;
        jlong ly2 = (jlong)y2;

        jlong x01 = pipeline->flattener_x0 + lx1; // >> 1
        jlong y01 = pipeline->flattener_y0 + ly1; // >> 1
        jlong x12 = lx1 + lx2; // >> 1
        jlong y12 = ly1 + ly2; // >> 1
        jlong x23 = lx2 + x3; // >> 1
        jlong y23 = ly2 + y3; // >> 1

        jlong x012 = x01 + x12; // >> 2
        jlong y012 = y01 + y12; // >> 2
        jlong x123 = x12 + x23; // >> 2
        jlong y123 = y12 + y23; // >> 2
        jlong x0123 = x012 + x123; // >> 3
        jlong y0123 = y012 + y123; // >> 3

        flattener_cubicToHelper(pipeline, (jint)(x01 >> 1), (jint)(y01 >> 1),
                                (jint)(x012 >> 2), (jint)(y012 >> 2),
                                (jint)(x0123 >> 3), (jint)(y0123 >> 3));
        flattener_cubicToHelper(pipeline, (jint)(x123 >> 2), (jint)(y123 >> 2),
                                (jint)(x23 >> 1), (jint)(y23 >> 1),
                                x3, y3);
    }

    pipeline->flattener_x0 = x3;
    pipeline->flattener_y0 = y3;
}

static void
stroker_computeOffset(Pipeline* pipeline, jint x0, jint y0,
                      jint x1, jint y1, jint* m) {
    jlong lx = (jlong)x1 - (jlong)x0;
    jlong ly = (jlong)y1 - (jlong)y0;

    jint dx, dy;
    if (pipeline->stroker_m00 > 0 
            && (pipeline->stroker_m00 == pipeline->stroker_m11
                || pipeline->stroker_m00 == -pipeline->stroker_m11) 
            && pipeline->stroker_m01 == 0 
            && pipeline->stroker_m10 == 0) {
        jlong ilen = piscesmath_lhypot(lx, ly);
        if (ilen == 0) {
            dx = dy = 0;
        } else {
            dx = (jint)( (ly*pipeline->stroker_scaledLineWidth2)/ilen >> 16);
            dy = (int)(-(lx*pipeline->stroker_scaledLineWidth2)/ilen >> 16);
        }
    } else {
        jdouble dlx = x1 - x0;
        jdouble dly = y1 - y0;

        jdouble a = dly*pipeline->stroker_m00 - dlx*pipeline->stroker_m10;
        jdouble b = dly*pipeline->stroker_m01 - dlx*pipeline->stroker_m11;
        jdouble dh = piscesmath_dhypot(a, b);
        jdouble div = pipeline->stroker_lineWidth2/(65536.0*dh);
        jdouble ddx = dly*pipeline->stroker_m00_2_m01_2 -
                      dlx*pipeline->stroker_m00_m10_m01_m11;
        jdouble ddy = dly*pipeline->stroker_m00_m10_m01_m11 -
                      dlx*pipeline->stroker_m10_2_m11_2;
        dx = (jint)(ddx*div);
        dy = (jint)(ddy*div);
    }

    m[0] = dx;
    m[1] = dy;
}

static jboolean
stroker_isCCW(jint x0, jint y0, jint x1, jint y1, jint x2, jint y2) {
    jint dx0 = x1 - x0;
    jint dy0 = y1 - y0;
    jint dx1 = x2 - x1;
    jint dy1 = y2 - y1;
    return (jlong)dx0*dy1 < (jlong)dy0*dx1;
}

static jboolean
stroker_side(jint x, jint y, jint x0, jint y0, jint x1, jint y1) {
    jlong lx = x;
    jlong ly = y;
    jlong lx0 = x0;
    jlong ly0 = y0;
    jlong lx1 = x1;
    jlong ly1 = y1;

    return (ly0 - ly1)*lx + (lx1 - lx0)*ly + (lx0*ly1 - lx1*ly0) > 0;
}

static void
stroker_emitLineTo(Pipeline* pipeline, jint x1, jint y1,
                   jboolean rev) {
    if (rev) {
        jint rindex = pipeline->stroker_rindex;
        REALLOC(pipeline->stroker_reverse, jint, rindex + 2,
                pipeline->stroker_reverse_length * 2);
        ASSERT_ALLOC(pipeline->stroker_reverse);

        pipeline->stroker_reverse[rindex++] = x1;
        pipeline->stroker_reverse[rindex++] = y1;
        pipeline->stroker_rindex = rindex;
    } else {
        pipeline->stroker_next_lineTo(pipeline, x1, y1);
    }
}

static jint
stroker_computeRoundJoin(Pipeline* pipeline, jint cx, jint cy,
                         jint xa, jint ya,
                         jint xb, jint yb,
                         jint side,
                         jboolean flip,
                         jint* join) {
    jint px, py;
    jint ncoords = 0;

    jboolean centerSide;

    jint i, start, end;
    jint numPenSegments = pipeline->stroker_numPenSegments;
    jint* pen_dx = pipeline->stroker_pen_dx;
    jint* pen_dy = pipeline->stroker_pen_dy;
    jboolean* penIncluded = pipeline->stroker_penIncluded;

    // IMPL NOTE : to fix warning
    (void) flip;

    if (side == 0) {
        centerSide = stroker_side(cx, cy, xa, ya, xb, yb);
    } else {
        centerSide = (side == 1) ? XNI_TRUE : XNI_FALSE;
    }
    for (i = 0; i < numPenSegments; i++) {
        jboolean penSide;

        px = cx + pen_dx[i];
        py = cy + pen_dy[i];

        penSide = stroker_side(px, py, xa, ya, xb, yb);
        if (penSide != centerSide) {
            penIncluded[i] = XNI_TRUE;
        } else {
            penIncluded[i] = XNI_FALSE;
        }
    }

    start = -1;
    end = -1;
    for (i = 0; i < numPenSegments; i++) {
        if (penIncluded[i] &&
                !penIncluded[(i + numPenSegments - 1) % numPenSegments]) {
            start = i;
        }
        if (penIncluded[i] &&
                !penIncluded[(i + 1) % numPenSegments]) {
            end = i;
        }
    }

    if (end < start) {
        end += numPenSegments;
    }

    if (start != -1 && end != -1) {
        jlong dxa = cx + pen_dx[start] - xa;
        jlong dya = cy + pen_dy[start] - ya;
        jlong dxb = cx + pen_dx[start] - xb;
        jlong dyb = cy + pen_dy[start] - yb;

        jboolean rev = (dxa*dxa + dya*dya > dxb*dxb + dyb*dyb);
        jint i = rev ? end : start;
        jint incr = rev ? -1 : 1;
        while (1) {
            jint idx = i % numPenSegments;
            px = cx + pen_dx[idx];
            py = cy + pen_dy[idx];
            join[ncoords++] = px;
            join[ncoords++] = py;
            if (i == (rev ? start : end)) {
                break;
            }
            i += incr;
        }
    }

    return ncoords/2;
}

static void
stroker_drawRoundJoin(Pipeline* pipeline, jint x, jint y,
                      jint omx, jint omy, jint mx, jint my, jint side,
                      jboolean flip, jboolean rev, jlong threshold) {
    jint i;
    jlong domx, domy, len;
    jint bx0, by0;
    jint bx1, by1;
    jint npoints;

    jint* join = pipeline->stroker_join;

    if ((omx == 0 && omy == 0) || (mx == 0 && my == 0)) {
        return;
    }

    domx = (jlong)omx - mx;
    domy = (jlong)omy - my;
    len = domx*domx + domy*domy;
    if (len < threshold) {
        return;
    }

    if (rev) {
        omx = -omx;
        omy = -omy;
        mx = -mx;
        my = -my;
    }

    bx0 = x + omx;
    by0 = y + omy;
    bx1 = x + mx;
    by1 = y + my;

    npoints = stroker_computeRoundJoin(pipeline, x, y, bx0, by0, bx1, by1,
                                       side, flip, join);
    for (i = 0; i < npoints; i++) {
        stroker_emitLineTo(pipeline, join[2*i], join[2*i + 1], rev);
    }
}

// Return the intersection point of the lines (ix0, iy0) -> (ix1, iy1)
// and (ix0p, iy0p) -> (ix1p, iy1p) in m[0] and m[1]
static void
stroker_computeMiter(jint ix0, jint iy0, jint ix1, jint iy1,
                     jint ix0p, jint iy0p, jint ix1p, jint iy1p, jint* m) {
    jlong x0 = ix0;
    jlong y0 = iy0;
    jlong x1 = ix1;
    jlong y1 = iy1;

    jlong x0p = ix0p;
    jlong y0p = iy0p;
    jlong x1p = ix1p;
    jlong y1p = iy1p;

    jlong x10 = x1 - x0;
    jlong y10 = y1 - y0;
    jlong x10p = x1p - x0p;
    jlong y10p = y1p - y0p;

    jlong t;

    jlong den = (x10*y10p - x10p*y10) >> 16;
    if (den == 0) {
        m[0] = ix0;
        m[1] = iy0;
        return;
    }

    t = (x1p*(y0 - y0p) - x0*y10p + x0p*(y1p - y0)) >> 16;
    m[0] = (jint)(x0 + (t*x10)/den);
    m[1] = (jint)(y0 + (t*y10)/den);
}

static void
stroker_drawMiter(Pipeline* pipeline, jint px0, jint py0,
                  jint x0, jint y0, jint x1, jint y1,
                  jint omx, jint omy, jint mx, jint my,
                  jboolean rev) {
    jint miter[2];
    jlong dx, dy;
    jlong a, b;
    jlong lenSq;

    if (mx == omx && my == omy) {
        return;
    }
    if (px0 == x0 && py0 == y0) {
        return;
    }
    if (x0 == x1 && y0 == y1) {
        return;
    }

    if (rev) {
        omx = -omx;
        omy = -omy;
        mx = -mx;
        my = -my;
    }

    stroker_computeMiter(px0 + omx, py0 + omy, x0 + omx, y0 + omy,
                         x0 + mx, y0 + my, x1 + mx, y1 + my, miter);

    // Compute miter length in untransformed coordinates
    dx = (jlong)miter[0] - x0;
    dy = (jlong)miter[1] - y0;
    a = (dy*pipeline->stroker_m00 - dx*pipeline->stroker_m10) >> 16;
    b = (dy*pipeline->stroker_m01 - dx*pipeline->stroker_m11) >> 16;
    lenSq = a*a + b*b;

    if (lenSq < pipeline->stroker_miterLimitSq) {
        stroker_emitLineTo(pipeline, miter[0], miter[1], rev);
    }
}

static jlong
stroker_lineLength(Pipeline* pipeline, jlong ldx, jlong ldy) {
    jlong ldet = ((jlong)pipeline->stroker_m00*pipeline->stroker_m11 -
                  (jlong)pipeline->stroker_m01*pipeline->stroker_m10) >> 16;
    jlong la = ((jlong)ldy*pipeline->stroker_m00 -
                (jlong)ldx*pipeline->stroker_m10)/ldet;
    jlong lb = ((jlong)ldy*pipeline->stroker_m01 -
                (jlong)ldx*pipeline->stroker_m11)/ldet;
    jlong llen = piscesmath_lhypot(la, lb);
    return llen;
}

static void
stroker_finish(Pipeline* pipeline) {
    jint i;
    jint* reverse;
    jint capStyle = pipeline->stroker_capStyle;

    if (capStyle == CAP_ROUND) {
        stroker_drawRoundJoin(pipeline, pipeline->stroker_x0, 
                              pipeline->stroker_y0, pipeline->stroker_omx, 
                              pipeline->stroker_omy, -pipeline->stroker_omx, 
                              -pipeline->stroker_omy, 1, XNI_FALSE, XNI_FALSE, 
                              ROUND_JOIN_THRESHOLD);
    } else if (capStyle == CAP_SQUARE) {
        jlong ldx = (jlong)(pipeline->stroker_px0 - pipeline->stroker_x0);
        jlong ldy = (jlong)(pipeline->stroker_py0 - pipeline->stroker_y0);
        jlong llen = stroker_lineLength(pipeline, ldx, ldy);
        jlong s = (jlong)pipeline->stroker_lineWidth2*65536/llen;

        jint capx = pipeline->stroker_x0 - (jint)(ldx*s >> 16);
        jint capy = pipeline->stroker_y0 - (jint)(ldy*s >> 16);

        pipeline->stroker_next_lineTo(pipeline, capx + pipeline->stroker_omx,
                                      capy + pipeline->stroker_omy);
        pipeline->stroker_next_lineTo(pipeline, capx - pipeline->stroker_omx,
                                      capy - pipeline->stroker_omy);
    }

    reverse = pipeline->stroker_reverse;
    for (i = pipeline->stroker_rindex - 2; i >= 0; i -= 2) {
        pipeline->stroker_next_lineTo(pipeline, reverse[i], reverse[i + 1]);
    }

    if (capStyle == CAP_ROUND) {
        stroker_drawRoundJoin(pipeline, pipeline->stroker_sx0, 
                              pipeline->stroker_sy0, -pipeline->stroker_mx0, 
                              -pipeline->stroker_my0, pipeline->stroker_mx0, 
                              pipeline->stroker_my0, 1, XNI_FALSE, XNI_FALSE, 
                              ROUND_JOIN_THRESHOLD);
    } else if (capStyle == CAP_SQUARE) {
        jlong ldx = (jlong)(pipeline->stroker_sx1 - pipeline->stroker_sx0);
        jlong ldy = (jlong)(pipeline->stroker_sy1 - pipeline->stroker_sy0);
        jlong llen = stroker_lineLength(pipeline, ldx, ldy);
        jlong s = (jlong)pipeline->stroker_lineWidth2*65536/llen;

        jint capx = pipeline->stroker_sx0 - (jint)(ldx*s >> 16);
        jint capy = pipeline->stroker_sy0 - (jint)(ldy*s >> 16);

        pipeline->stroker_next_lineTo(pipeline, capx - pipeline->stroker_mx0,
                                      capy - pipeline->stroker_my0);
        pipeline->stroker_next_lineTo(pipeline, capx + pipeline->stroker_mx0,
                                      capy + pipeline->stroker_my0);
    }

    pipeline->stroker_next_close(pipeline);
    pipeline->stroker_joinSegment = XNI_FALSE;

    pipeline->stroker_prev = STROKER_CLOSE;
    pipeline->stroker_rindex = 0;
    pipeline->stroker_started = XNI_FALSE;
    pipeline->stroker_lineToOrigin = XNI_FALSE;
}

static void
dasher_goTo(Pipeline* pipeline, jint x1, jint y1) {
    if ((pipeline->dasher_idx % 2) == 0) {
        if (pipeline->dasher_starting) {
            pipeline->dasher_sx1 = x1;
            pipeline->dasher_sy1 = y1;
            pipeline->dasher_firstDashOn = XNI_TRUE;
            pipeline->dasher_starting = XNI_FALSE;
        }
        stroker_lineTo(pipeline, x1, y1);
    } else {
        if (pipeline->dasher_starting) {
            pipeline->dasher_firstDashOn = XNI_FALSE;
            pipeline->dasher_starting = XNI_FALSE;
        }
        stroker_moveTo(pipeline, x1, y1);
    }
    pipeline->dasher_x0 = x1;
    pipeline->dasher_y0 = y1;
}

static void
bbox_checkPoint(Pipeline* pipeline, jint x0, jint y0) {
    if (x0 < pipeline->bbox_minX) {
        pipeline->bbox_minX = x0;
    } else if (x0 > pipeline->bbox_maxX) {
        pipeline->bbox_maxX = x0;
    }

    if (y0 < pipeline->bbox_minY) {
        pipeline->bbox_minY = y0;
    } else if (y0 > pipeline->bbox_maxY) {
        pipeline->bbox_maxY = y0;
    }
}
