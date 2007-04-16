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


#ifndef PISCES_PIPELINES_H
#define PISCES_PIPELINES_H

#include <PiscesDefs.h>
#include <PiscesTransform.h>

#define JOIN_MITER 0
#define JOIN_ROUND 1
#define JOIN_BEVEL 2

#define CAP_BUTT 0
#define CAP_ROUND 1
#define CAP_SQUARE 2

#define ARC_OPEN 0
#define ARC_CHORD 1
#define ARC_PIE 2

typedef struct _Pipeline {
    void* param;

    jlong transformer_m00, transformer_m01, transformer_m02;
    jlong transformer_m10, transformer_m11, transformer_m12;

    jboolean transformer_scaleAndTranslate;

    jint flattener_flatness, flattener_flatnessSq;
    jint flattener_x0, flattener_y0;

    void (*flattener_next_moveTo)(struct _Pipeline* pipeline,
                                  jint x0, jint y0);
    void (*flattener_next_lineJoin)(struct _Pipeline* pipeline);
    void (*flattener_next_lineTo)(struct _Pipeline* pipeline,
                                  jint x1, jint y1);
    void (*flattener_next_close)(struct _Pipeline* pipeline);
    void (*flattener_next_end)(struct _Pipeline* pipeline);

    jint stroker_lineWidth;
    jint stroker_capStyle;
    jint stroker_joinStyle;
    jint stroker_miterLimit;

    jint stroker_m00, stroker_m01;
    jint stroker_m10, stroker_m11;

    jint stroker_lineWidth2;
    jlong stroker_scaledLineWidth2;

    jint stroker_numPenSegments;
    jint* stroker_pen_dx;
    jint stroker_pen_dx_length;
    jint* stroker_pen_dy;
    jint stroker_pen_dy_length;
    jboolean* stroker_penIncluded;
    jint stroker_penIncluded_length;
    jint* stroker_join;
    jint stroker_join_length;

    jint* stroker_reverse;
    jint stroker_reverse_length;
    jlong stroker_miterLimitSq;

    jint stroker_prev;
    jint stroker_rindex;
    jboolean stroker_started;
    jboolean stroker_lineToOrigin;
    jboolean stroker_joinToOrigin;

    jboolean stroker_joinSegment;

    jint stroker_sx0, stroker_sy0;
    jint stroker_sx1, stroker_sy1;
    jint stroker_x0, stroker_y0;
    jint stroker_x1, stroker_y1;
    jint stroker_mx0, stroker_my0;
    jint stroker_mx1, stroker_my1;
    jint stroker_omx, stroker_omy;
    jint stroker_lx0, stroker_ly0;
    jint stroker_lx1, stroker_ly1;
    jint stroker_lx0p, stroker_ly0p;
    jint stroker_px0, stroker_py0;

    jdouble stroker_m00_2_m01_2;
    jdouble stroker_m10_2_m11_2;
    jdouble stroker_m00_m10_m01_m11;

    void (*stroker_next_moveTo)(struct _Pipeline* pipeline,
                                jint x0, jint y0);
    void (*stroker_next_lineJoin)(struct _Pipeline* pipeline);
    void (*stroker_next_lineTo)(struct _Pipeline* pipeline,
                                jint x1, jint y1);
    void (*stroker_next_close)(struct _Pipeline* pipeline);
    void (*stroker_next_end)(struct _Pipeline* pipeline);

    jint* dasher_dash;
    jint dasher_dash_length;
    jint dasher_startPhase;
    jint dasher_startIdx;

    jint dasher_idx;
    jint dasher_phase;

    jint dasher_sx, dasher_sy;
    jint dasher_x0, dasher_y0;

    jint dasher_m00, dasher_m01;
    jint dasher_m10, dasher_m11;

    jboolean dasher_symmetric;
    jlong dasher_ldet;

    jboolean dasher_firstDashOn;
    jboolean dasher_starting;
    jint dasher_sx1, dasher_sy1;

    jint bbox_minX, bbox_minY;
    jint bbox_maxX, bbox_maxY;

}
Pipeline;

void pipeline_free(Pipeline* pipeline);
void pipeline_initializeFiller(Pipeline* pipeline, void* param);
void pipeline_initializeStroker(Pipeline* pipeline, void* param);
void pipeline_initializeBBox(Pipeline* pipeline, void* param);
void pipeline_setFillerParameters(Pipeline* pipeline,
                                  Transform6* transform);
void pipeline_setStrokerParameters(Pipeline* pipeline, jint lineWidth, 
                                   jint capStyle, jint joinStyle, 
                                   jint miterLimit, jint* dashArray, 
                                   jint dashLength, jint dashPhase, 
                                   Transform6* transform);
void pipeline_setBBoxParameters(Pipeline* pipeline, jint lineWidth, 
                                jint miterLimit, jint m00, jint m01, jint m10, 
                                jint m11, jint m02, jint m12);

void transformer_moveTo(Pipeline* pipeline, jint x0, jint y0);
void transformer_lineJoin(Pipeline* pipeline);
void transformer_lineTo(Pipeline* pipeline, jint x1, jint y1);
void transformer_quadTo(Pipeline* pipeline, jint x1, jint y1,
                        jint x2, jint y2);
void transformer_cubicTo(Pipeline* pipeline, jint x1, jint y1,
                         jint x2, jint y2, jint x3, jint y3);
void transformer_close(Pipeline* pipeline);
void transformer_end(Pipeline* pipeline);

#define PIPELINE_MOVETO(pipeline, x0, y0) \
    transformer_moveTo(pipeline, x0, y0)
#define PIPELINE_LINEJOIN(pipeline) \
    transformer_lineJoin(pipeline)
#define PIPELINE_LINETO(pipeline, x1, y1) \
    transformer_lineTo(pipeline, x1, y1)
#define PIPELINE_QUADTO(pipeline, x1, y1, x2, y2) \
    transformer_quadTo(pipeline, x1, y1, x2, y2)
#define PIPELINE_CUBICTO(pipeline, x1, y1, x2, y2, x3, y3) \
    transformer_cubicTo(pipeline, x1, y1, x2, y2, x3, y3)
#define PIPELINE_CLOSE(pipeline) \
    transformer_close(pipeline)
#define PIPELINE_END(pipeline) \
    transformer_end(pipeline)

void bbox_reset(Pipeline* pipeline);
void bbox_get(Pipeline* pipeline, jint* bbox);

#endif
