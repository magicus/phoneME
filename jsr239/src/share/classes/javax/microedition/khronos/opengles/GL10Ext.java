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
 */

package javax.microedition.khronos.opengles;


/**
 * The <code>GL10Ext</code> interface contains the Java(TM)
 * programming language bindings for the
 * <code>OES_query_matrix</code> extension to OpenGL(R) ES 1.0.
 *
 * <p> The documentation in this class is normative with respect to
 * instance variable names and values, method names and signatures,
 * and exception behavior.  The remaining documentation is placed here
 * for convenience and does not replace the normative documentation
 * found in the OpenGL ES 1.0 specification and the OpenGL
 * specification versions it references.
 */
public interface GL10Ext extends GL {

  // OES_query_matrix extension (optional profile extension in 1.0,
  // deprecated in 1.1)

//   /**
//    * Flag indicating the presence of the <code>OES_query_matrix</code>
//    * extension.
//    */
//   int GL_OES_query_matrix                     = 1;

  /**
   * (<code>OES_query_matrix</code> extension) Return the values of
   * the current matrix.
   *
   * <p><code>glQueryMatrixxOES</code> returns the values of the
   * current matrix. <code>mantissa</code> returns the 16 mantissa
   * values of the current matrix, and <code>exponent</code> returns
   * the correspnding 16 exponent values. The matrix value <i>i</i>
   * is then close to <code>mantissa</code>[<i>i</i>] *
   * 2^<code>exponent</code>[<i>i</i>].
   *
   * <p>Use <code>glMatrixMode</code> and
   * <code>glActiveTexture</code> to select the desired matrix to
   * return.
   *
   * <p>If all are valid (not <code>NaN</code> or <code>Inf</code>),
   * <code>glQueryMatrixxOES</code> returns the status value
   * 0. Otherwise, for every component <i>i</i> which is not valid,
   * the <i>i</i>th bit is set.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glQueryMatrixxOES</code> is available only if the
   * <code>GL_OES_query_matrix</code> extension is supported by your
   * implementation.
   *
   * <p>The implementation is not required to keep track of
   * overflows. If overflows are not tracked, the returned status
   * value is always 0.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetString</code> with argument
   * <code>GL_EXTENSIONS</code>.
   * 
   * @param mantissa Returns the mantissi of the current matrix.
   * @param exponent Returns the exponents of the current matrix.
   * @param mantissaOffset the starting offset within the
   * <code>mantissa</code> array.
   * @param exponentOffset the starting offset within the
   * <code>exponent</code> array.
   * 
   * @return a bitfield indicating which components contain invalid
   * (<code>NaN</code> or <code>Inf</code>) values.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_query_matrix</code> extension.
   * @exception IllegalArgumentException if <code>mantissa</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>exponent</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>mantissaOffset</code>
   * is less than 0.
   * @exception IllegalArgumentException if <code>exponentOffset</code>
   * is less than 0.
   * @exception IllegalArgumentException if <code>mantissa.length -
   * mantissaOffset</code> is less than 16.
   * @exception IllegalArgumentException if <code>exponent.length -
   * exponentOffset</code> is less than 16.
   */
  int glQueryMatrixxOES(int[] mantissa, int mantissaOffset,
			int[] exponent, int exponentOffset);

}
