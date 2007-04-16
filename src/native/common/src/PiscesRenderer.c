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


#include <PiscesRenderer.h>

#include <PiscesUtil.h>

static void lineToImpl(Renderer* rdr, jint x1, jint y1);
static void addEdge(Renderer* rdr, jint x0, jint y0, jint x1, jint y1);

void
prenderer_moveTo(Pipeline* pipeline, jint x0, jint y0) {
    Renderer *rdr = (Renderer*)pipeline->param;

    rdr->_sx0 = rdr->_x0 = x0;
    rdr->_sy0 = rdr->_y0 = y0;
    rdr->_lastOrientation = 0;
}

void
prenderer_lineTo(Pipeline* pipeline, jint x1, jint y1) {
    lineToImpl((Renderer*)pipeline->param, x1, y1);
}

void
prenderer_close(Pipeline* pipeline) {
    jint orientation;
    Renderer *rdr = (Renderer*)pipeline->param;

    orientation = rdr->_lastOrientation;
    if (rdr->_y0 != rdr->_sy0) {
        orientation = (rdr->_y0 < rdr->_sy0) ? 1 : -1;
    }
    if (orientation != rdr->_firstOrientation) {
        ++rdr->_flips;
    }
    lineToImpl(rdr, rdr->_sx0, rdr->_sy0);
}

void
prenderer_lineJoin(Pipeline* pipeline) {
    // IMPL NOTE : to fix warning
    (void) pipeline;
    // do nothing
}

void
prenderer_end(Pipeline* pipeline) {
    // IMPL NOTE : to fix warning
    (void) pipeline;
    // do nothing
}

static void
lineToImpl(Renderer* rdr, jint x1, jint y1) {
    jint orientation;

    // Ignore horizontal lines
    // Next line will count flip
    if (rdr->_y0 == y1) {
        rdr->_x0 = x1;
        return;
    }

    orientation = (rdr->_y0 < y1) ? 1 : -1;
    if (rdr->_lastOrientation == 0) {
        rdr->_firstOrientation = orientation;
    } else if (orientation != rdr->_lastOrientation) {
        ++rdr->_flips;
    }
    rdr->_lastOrientation = orientation;

    // Bias Y by 1 ULP so endpoints never lie on a scanline
    addEdge(rdr, rdr->_x0, rdr->_y0 | 0x1, x1, y1 | 0x1);

    rdr->_x0 = x1;
    rdr->_y0 = y1;
}

static void
addEdge(Renderer* rdr, jint x0, jint y0, jint x1, jint y1) {
    jint orientation;
    jint eminY, emaxY;

    jint newLen = rdr->_edgeIdx + 5;
    REALLOC(rdr->_edges, jint, newLen, rdr->_edges_length * 2);
    ASSERT_ALLOC(rdr->_edges);

    orientation = 1;
    if (y0 > y1) {
        jint tmp = y0;
        y0 = y1;
        y1 = tmp;

        orientation = -1;
    }

    // Skip edges that don't cross a subsampled scanline
    eminY = ((y0 + rdr->_HYSTEP) & rdr->_YMASK);
    emaxY = ((y1 - rdr->_HYSTEP) & rdr->_YMASK);
    if (eminY > emaxY) {
        return;
    }

    if (orientation == -1) {
        jint tmp = x0;
        x0 = x1;
        x1 = tmp;
    }

    rdr->_edges[rdr->_edgeIdx++] = x0;
    rdr->_edges[rdr->_edgeIdx++] = y0;
    rdr->_edges[rdr->_edgeIdx++] = x1;
    rdr->_edges[rdr->_edgeIdx++] = y1;
    rdr->_edges[rdr->_edgeIdx++] = orientation;

    // Update Y bounds of primitive
    if (y0 < rdr->_edgeMinY) {
        rdr->_edgeMinY = y0;
    }
    if (y1 > rdr->_edgeMaxY) {
        rdr->_edgeMaxY = y1;
    }
}
