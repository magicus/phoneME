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

public class Transformer extends PathSink {

    PathSink output;
    long m00, m01, m02;
    long m10, m11, m12;

    boolean scaleAndTranslate;

    public Transformer() {}

    public Transformer(PathSink output,
                       Transform6 transform) {
	if (output instanceof Transformer) {
            // Collapse adjacent transforms
  	    Transformer t = (Transformer)output;
            this.output = t.output;
	    this.m00 = (transform.m00*t.m00 + transform.m10*t.m01) >> 16;
	    this.m01 = (transform.m01*t.m00 + transform.m11*t.m01) >> 16;
	    this.m10 = (transform.m00*t.m10 + transform.m10*t.m11) >> 16;
	    this.m11 = (transform.m01*t.m10 + transform.m11*t.m11) >> 16;
	    this.m02 =  transform.m02*t.m00 + transform.m12*t.m01 + t.m02;
	    this.m12 =  transform.m02*t.m10 + transform.m12*t.m11 + t.m12;
	} else {
            this.output = output;
            setTransform(transform);
	}

        classify();
    }

    public void setTransform(Transform6 transform) {
        this.m00 = (long)transform.m00;
        this.m01 = (long)transform.m01;
        this.m02 = (long)transform.m02 << 16;
        this.m10 = (long)transform.m10;
        this.m11 = (long)transform.m11;
        this.m12 = (long)transform.m12 << 16;

        classify();
    }

    private void classify() {
        if (m01 == 0 && m10 == 0) {
            scaleAndTranslate = true;
        } else {
            scaleAndTranslate = false;
        }
    }

    public void setOutput(PathSink output) {
        this.output = output;
    }

    public void moveTo(int x0, int y0) {
        long tx0, ty0;
        
        if (scaleAndTranslate) {
            tx0 = m00*x0 + m02;
            ty0 = m11*y0 + m12;
        } else {
            tx0 = m00*x0 + m01*y0 + m02;
            ty0 = m10*x0 + m11*y0 + m12;
        }

//   	System.out.println("PT: moveTo " +
//   			   ((tx0 >> 16)/65536.0) + ", " +
//   			   ((ty0 >> 16)/65536.0));
	output.moveTo((int)(tx0 >> 16), (int)(ty0 >> 16));
    }

    public void lineJoin() {
        output.lineJoin();
    }

    public void lineTo(int x1, int y1) {
        long tx1, ty1;

        if (scaleAndTranslate) {
            tx1 = m00*x1 + m02;
            ty1 = m11*y1 + m12;
        } else {
            tx1 = m00*x1 + m01*y1 + m02;
            ty1 = m10*x1 + m11*y1 + m12;
        }

//   	System.out.println("PT: lineTo " +
//   			   ((tx1 >> 16)/65536.0) + ", " +
//   			   ((ty1 >> 16)/65536.0));
	output.lineTo((int)(tx1 >> 16), (int)(ty1 >> 16));
    }

    public void quadTo(int x1, int y1, int x2, int y2) {
        long tx1, ty1, tx2, ty2;

        if (scaleAndTranslate) {
            tx1 = m00*x1 + m02;
            ty1 = m11*y1 + m12;
            tx2 = m00*x2 + m02;
            ty2 = m11*y2 + m12;
        } else {
            tx1 = m00*x1 + m01*y1 + m02;
            ty1 = m10*x1 + m11*y1 + m12;
            tx2 = m00*x2 + m01*y2 + m02;
            ty2 = m10*x2 + m11*y2 + m12;
        }

// 	System.out.println("PT: quadTo " +
//  			   ((tx1 >> 16)/65536.0) + ", " +
//  			   ((ty1 >> 16)/65536.0) + ", " +
//  			   ((tx2 >> 16)/65536.0) + ", " +
//  			   ((ty2 >> 16)/65536.0));
	output.quadTo((int)(tx1 >> 16), (int)(ty1 >> 16),
		      (int)(tx2 >> 16), (int)(ty2 >> 16));
    }

    public void cubicTo(int x1, int y1,
			int x2, int y2,
			int x3, int y3) {
        long tx1, ty1, tx2, ty2, tx3, ty3;

        if (scaleAndTranslate) {
            tx1 = m00*x1 + m02;
            ty1 = m11*y1 + m12;
            tx2 = m00*x2 + m02;
            ty2 = m11*y2 + m12;
            tx3 = m00*x3 + m02;
            ty3 = m11*y3 + m12;
        } else {
            tx1 = m00*x1 + m01*y1 + m02;
            ty1 = m10*x1 + m11*y1 + m12;
            tx2 = m00*x2 + m01*y2 + m02;
            ty2 = m10*x2 + m11*y2 + m12;
            tx3 = m00*x3 + m01*y3 + m02;
            ty3 = m10*x3 + m11*y3 + m12;
        }
//          if (m00 != (1 << 16)) {
//              tx1 = m00*x1;
//              tx2 = m00*x2;
//              tx3 = m00*x3;
//          } else {
//              tx1 = (long)x1 << 16;
//              tx2 = (long)x2 << 16;
//              tx3 = (long)x3 << 16;
//          }

//         if (m11 != (1 << 16)) {
//             ty1 = m11*y1;
//             ty2 = m11*y2;
//             ty3 = m11*y3;
//         } else {
//             ty1 = (long)y1 << 16;
//             ty2 = (long)y2 << 16;
//             ty3 = (long)y3 << 16;
//         }

//         if (m01 != 0) {
//             tx1 += m01*y1;
//             tx2 += m01*y2;
//             tx3 += m01*y3;
//         }

//         if (m10 != 0) {
//             ty1 += m10*x1;
//             ty2 += m10*x2;
//             ty3 += m10*x3;
//         }

//         if (m02 != 0) {
//             tx1 += m02;
//             tx2 += m02;
//             tx3 += m02;
//         }

//         if (m12 != 0) {
//             ty1 += m12;
//             ty2 += m12;
//             ty3 += m12;
//         }

//  	System.out.println("PT: cubicTo " +
//   			   ((tx1 >> 16)/65536.0) + ", " +
//   			   ((ty1 >> 16)/65536.0) + ", " +
//   			   ((tx2 >> 16)/65536.0) + ", " +
//   			   ((ty2 >> 16)/65536.0) + ", " +
//   			   ((tx3 >> 16)/65536.0) + ", " +
//   			   ((ty3 >> 16)/65536.0));
 	output.cubicTo((int)(tx1 >> 16), (int)(ty1 >> 16),
 		       (int)(tx2 >> 16), (int)(ty2 >> 16),
 		       (int)(tx3 >> 16), (int)(ty3 >> 16));
    }
    
    public void close() {
//  	System.out.println("PT: close");
	output.close();
    }

    public void end() {
//  	System.out.println("PT: end");
	output.end();
    }
}
