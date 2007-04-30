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

import java.nio.*;

/**
 * The <code>GL11Ext</code> interface contains the Java(TM)
 * programming language bindings for all optional profile extensions
 * to OpenGL(R) ES 1.1.
 *
 * <p> This interface contains constants and methods associated with the
 * optional profile extensions for OpenGL ES 1.1.  The runtime OpenGL
 * ES engine may or may not implement any particular extensions
 * defined in the extension pack.  Functions that require a particular
 * extension will throw an <code>UnsupportedOperationException</code>
 * if the extension is not available at runtime.
 *
 * <p> The OpenGL ES 1.1 optional profile extensions comprise the
 * following:
 *
 * <ul>
 * <li>OES_draw_texture</li>
 * <li>OES_matrix_palette</li>
 * </ul>
 *
 * <p> The documentation in this class is normative with respect to
 * instance variable names and values, method names and signatures,
 * and exception behavior.  The remaining documentation is placed here
 * for convenience and does not replace the normative documentation
 * found in the OpenGL ES 1.1 specification and the OpenGL
 * specification versions it references.
 */
public interface GL11Ext extends GL {

  /**
   * (1.1 + <code>OES_matrix_palette</code> extension)
   * Enable server-side GL capabilities.
   *
   * <p>The matrix palette extension is enabled if
   * <code>cap</code> assumes the value <code>GL_MATRIX_PALETTE_OES</code>.
   * See <code>glMatrixMode</code>.
   *
   * @see GL11#glEnable(int cap)
   */
  void glEnable(int cap);

  /**
   * (<code>OES_matrix_palette</code> extension)
   * Enable client-side capability.
   *
   * <p> The <code>OES_matrix_palette</code> extension adds three
   * additional values for <code>array</code>:
   *
   * <ul>
   * <li><code>GL_MATRIX_INDEX_ARRAY_OES</code>
   *
   * <p> If enabled, the matrix index array is enabled. See
   * <code>glMatrixIndexPointer</code>.
   *
   * <li><code>GL_WEIGHT_ARRAY_OES</code>
   *
   * <p> If enabled, the weight array is enabled. See
   * <code>glWeightPointer</code>.
   *
   * </ul>
   *
   * @see GL11#glEnableClientState(int array)
   */
  void glEnableClientState(int array);

  /**
   * (<code>OES_draw_texture</code> extension)</li>
   * Set texture parameters.
   * 
   * The <code>OES_draw_texture</code> extension adds an additoanl
   * value for <code>pname</code>,
   * <code>GL_TEXTURE_CROP_RECT_OES</code>, which sets the texture
   * cropping rectangle for use by <code>glDrawTexOES</code>.
   *
   * @see GL11#glTexParameterfv(int target, int pname, float[] params, int offset)
   */
  void glTexParameterfv(int target, int pname, float[] params, int offset);

  // OES_draw_texture extension (optional profile extension in 1.1)

//   /**
//    * Flag indicating the presence of the "draw texture" extension.
//    * <code>OES_draw_texture</code> is an optional profile extension in
//    * OpenGL ES 1.1.
//    */
//   int GL_OES_draw_texture      = 1;

  /**
   * Constant for use with <code>glTexParameter</code>
   * (<code>OES_draw_texture</code> extension).
   */
  int GL_TEXTURE_CROP_RECT_OES = 0x8B9D;

  /**
   * (<code>OES_draw_texture</code> extension)
   * <code>glDrawTexOES</code> draws a texture rectangle to the screen.
   *
   * <p><code>x</code> and <code>y</code> are given directly in window
   * (viewport) coordinates.
   *
   * <p><code>z</code> is mapped to window depth <code>Zw</code> as
   * follows:
   *
   * <pre>
   * If z <= 0 then Zw = n
   * If z >= 1 then Zw = f
   * Otherwise Zw = n + z * (f - n)
   * </pre>
   *
   * where <i>n</i> and <i>f</i> are the near and far values of
   * <code>GL_DEPTH_RANGE</code> respectively.
   *
   * <p><code>width</code> and <code>height</code> specify the width and
   * height of the affected screen rectangle in pixels. These values
   * may be positive or negative; however if either of these are
   * negative, nothing is drawn.
   * 
   * <p>Calling one of the <code>DrawTex</code> functions generates a
   * fragment for each pixel that overlaps the screen rectangle
   * bounded by (<code>x</code>, <code>y</code>) and (<code>x</code> +
   * <code>width</code>), (<code>y</code> + <code>height</code>). For
   * each generated fragment, the depth is given by Zw as defined
   * above, and the color by the current color.
   * 
   * <p>Texture coordinates for each texture unit are computed as follows:
   * 
   * <p>Let <code>X</code> and <code>Y</code> be the screen x and y
   * coordinates of each sample point associated with the
   * fragment. Let <code>Wt</code> and <code>Ht</code> be the width
   * and height in texels of the texture currently bound to the
   * texture unit. (If the texture is a mipmap, let <code>Wt</code>
   * and <code>Ht</code> be the dimensions of the level specified by
   * <code>GL_TEXTURE_BASE_LEVEL</code>). Let <code>Ucr</code>,
   * <code>Vcr</code>, <code>Wcr</code> and <code>Hcr</code> be
   * (respectively) the four integers that make up the texture crop
   * rectangle parameter for the currently bound texture. The fragment
   * texture coordinates <code>(s, t, r, q)</code> are given by:
   * 
   * <pre>
   * s = (Ucr + (X - x) * (Wcr / width)) / Wt
   * t = (Vcr + (Y - y) * (Hcr / height)) / Ht
   * r = 0
   * q = 1
   * </pre>
   * 
   * <h4>Notes</h4>
   * 
   * <p>In the specific case where <code>X</code>, <code>Y</code>,
   * <code>x</code> and <code>y</code> are all integers,
   * <code>Wcr</code>/<code>width</code> and
   * <code>Hcr</code>/<code>height</code> are both equal to one, the
   * base level is used for the texture read, and fragments are
   * sampled at pixel centers, implementations are required to ensure
   * that the resulting <code>u</code>, <code>v</code> texture indices
   * are also integers. This results in a one-to-one mapping of texels
   * to fragments.
   * 
   * <p>Note that <code>Wcr</code> and/or <code>Hcr</code> can be
   * negative. The formulas given above for <code>s</code> and
   * <code>t</code> still apply in this case. The result is that if
   * <code>Wcr</code> is negative, the source rectangle for
   * <code>glDrawTexOES</code> operations lies to the left of the
   * reference point (Ucr, Vcr) rather than to the right of it, and
   * appears right-to-left reversed on the screen after a call to
   * <code>DrawTex</code>. Similarly, if <code>Hcr</code> is negative,
   * the source rectangle lies below the reference point <code>(Ucr,
   * Vcr)</code> rather than above it, and appears upside-down on the
   * screen.
   * 
   * <p>Note also that <code>s</code>, <code>t</code>, <code>r</code>,
   * and <code>q</code> are computed for each fragment as part of
   * <code>glDrawTexOES</code> rendering. This implies that the
   * texture matrix is ignored and has no effect on the rendered
   * result.
   * 
   * <p><code>glDrawTexOES</code> is available only if the
   * <code>OES_draw_texture</code> extension is supported by your
   * implementation.
   * 
   * @param x Specifies the x position of the affected screen rectangle.
   * @param y Specifies the x position of the affected screen rectangle.
   * @param z Specifies the x position of the affected screen rectangle.
   * @param width Specifies the width of the affected screen rectangle
   * in pixels.
   * @param height Specifies the height of the affected screen
   * rectangle in pixels.
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   */
  void glDrawTexfOES(float x, float y, float z, float width, float height);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Short integer version of <code>glDrawTexOES</code>.
   *
   * @see #glDrawTexfOES(float x, float y, float z, float width, float height)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   */
  void glDrawTexsOES(short x, short y, short z, short width, short height);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Integer version of <code>glDrawTexOES</code>.
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   */
  void glDrawTexiOES(int x, int y, int z, int width, int height);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Fixed-point version of <code>glDrawTexOES</code>.
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   */
  void glDrawTexxOES(int x, int y, int z, int width, int height);
  
  /**
   * (<code>OES_draw_texture</code> extension)
   * Floating-point array version of <code>glDrawTexOES</code>.
   *
   * @param coords An array of at least 5 elements containing the x,
   * y, z, width and height values.
   * @param offset the starting offset within the
   * <code>coords</code> array.
   *
   * @see #glDrawTexfOES(float x, float y, float z, float width, float height)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>coords</code> is
   * <code>null</code> or if <code>coords.length - offset</code> is
   * less than 5.
   */
  void glDrawTexfvOES(float[] coords, int offset);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Floating-point <code>Buffer</code> version of
   * <code>glDrawTexOES</code>.
   * 
   * @see #glDrawTexfvOES(float[] coords, int offset)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   * @exception IllegalArgumentException if <code>coords</code> is
   * <code>null</code> or if <code>coords.remaining()</code> is less than 5.
   */
  void glDrawTexfvOES(FloatBuffer coords);
    
  /**
   * (<code>OES_draw_texture</code> extension)
   * Short integer array version of <code>glDrawTexOES</code>.
   *
   * @see #glDrawTexfvOES(float[] coords, int offset)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>coords</code> is
   * <code>null</code> or if <code>coords.length - offset</code> is
   * less than 5.
   */
  void glDrawTexsvOES(short[] coords, int offset);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Short integer <code>Buffer</code> version of
   * <code>glDrawTexOES</code>.
   *
   * @see #glDrawTexfvOES(float[] coords, int offset)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   * @exception IllegalArgumentException if <code>coords</code> is
   * <code>null</code> or if <code>coords.remaining()</code> is less than 5.
   */
  void glDrawTexsvOES(ShortBuffer coords);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Integer array version of <code>glDrawTexOES</code>.
   *
   * @see #glDrawTexfvOES(float[] coords, int offset)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>coords</code> is
   * <code>null</code> or if <code>coords.length - offset</code> is
   * less than 5.
   */
  void glDrawTexivOES(int[] coords, int offset);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Integer <code>Buffer</code> version of
   * <code>glDrawTexOES</code>.
   * 
   * @see #glDrawTexfvOES(float[] coords, int offset)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   * @exception IllegalArgumentException if <code>coords</code> is
   * <code>null</code> or if <code>coords.remaining()</code> is less than 5.
   */
  void glDrawTexivOES(IntBuffer coords);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Fixed-point array version of <code>glDrawTexOES</code>.
   * 
   * @see #glDrawTexfvOES(float[] coords, int offset)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>coords</code> is
   * <code>null</code> or if <code>coords.length - offset</code> is
   * less than 5.
   */
  void glDrawTexxvOES(int[] coords, int offset);

  /**
   * (<code>OES_draw_texture</code> extension)
   * Fixed-point <code>Buffer</code> version of
   * <code>glDrawTexOES</code>.
   * 
   * @see #glDrawTexfvOES(float[] coords, int offset)
   *
   * @exception UnsupportedOperationException if the
   * <code>OES_draw_texture</code> extension is not available.
   * @exception IllegalArgumentException if <code>coords</code> is
   * <code>null</code> or if <code>coords.remaining()</code> is less than 5.
   */
  void glDrawTexxvOES(IntBuffer coords);

  // OES_matrix_palette (optional profile extension in 1.1)

//   /**
//    * Flag indicating the presence of the
//    * <code>OES_matrix_palette</code> extension.
//    */
//   int GL_OES_matrix_palette                    = 1;

  /* OES_matrix_palette */

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MAX_VERTEX_UNITS_OES = 0x86A4;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MAX_PALETTE_MATRICES_OES              = 0x8842;

  /**
   * Constant for use with <code>glMatrixMode</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MATRIX_PALETTE_OES                    = 0x8840;

  /**
   * Constant for use with <code>glEnableClientState</code> and
   * <code>glDisableClientState</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MATRIX_INDEX_ARRAY_OES                = 0x8844;

  /**
   * Constant for use with <code>glEnableClientState</code> and
   * <code>glDisableClientState</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_WEIGHT_ARRAY_OES                      = 0x86AD;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MATRIX_INDEX_ARRAY_SIZE_OES           = 0x8846;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MATRIX_INDEX_ARRAY_TYPE_OES           = 0x8847;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MATRIX_INDEX_ARRAY_STRIDE_OES         = 0x8848;

  /**
   * Constant for use with <code>glGetPointer</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MATRIX_INDEX_ARRAY_POINTER_OES        = 0x8849;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES = 0x8B9E;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_WEIGHT_ARRAY_SIZE_OES                 = 0x86AB;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_WEIGHT_ARRAY_TYPE_OES                 = 0x86A9;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_WEIGHT_ARRAY_STRIDE_OES               = 0x86AA;

  /**
   * Constant for use with <code>glGetPointer</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_WEIGHT_ARRAY_POINTER_OES              = 0x86AC;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_palette</code> extension).
   */
  int GL_WEIGHT_ARRAY_BUFFER_BINDING_OES       = 0x889E;

  /**
   * (<code>OES_matrix_palette</code> extension) Defines which of the
   * palette’s matrices is affected by subsequent matrix operations.
   *
   * <p><code>glCurrentPaletteMatrixOES</code> defines which of the
   * palette’s matrices is affected by subsequent matrix operations
   * when the current matrix mode is
   * <code>GL_MATRIX_PALETTE_OES</code>.
   *
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>index</code> is not between 0 and
   * <code>GL_MAX_PALETTE_MATRICES_OES</code> - 1.
   *
   * @param matrixpaletteindex Specifies the index into the palette's matrices.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_matrix_palette</code> extension.
   */
  void glCurrentPaletteMatrixOES(int matrixpaletteindex);

  /**
   * (<code>OES_matrix_palette</code> extension) Copies the current
   * model view matrix to a matrix in the current matrix palette.
   *
   * <p><code>glLoadPaletteFromModelViewMatrixOES</code> copies the
   * current model view matrix to a matrix in the current matrix
   * palette, as specified by glCurrentPaletteMatrix.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_matrix_palette</code> extension.
   */
  void glLoadPaletteFromModelViewMatrixOES();

  /**
   * (<code>OES_matrix_palette</code> extension) Define an array of
   * matrix indices.
   *
   * <p><code>glMatrixIndexPointer</code> specifies the location and
   * data of an array of matrix indices to use when
   * rendering. <code>size</code> specifies the number of matrix
   * indices per vertex and <code>type</code> the data type of the
   * coordinates. <code>stride</code> specifies the byte stride from
   * one matrix index to the next, allowing vertices and attributes to
   * be packed into a single array or stored in separate
   * arrays. (Single-array storage may be more efficient on some
   * implementations.)
   *
   * <p>These matrices indices are used to blend corresponding matrices
   * for a given vertex.
   *
   * <p>When a matrix index array is specified, <code>size</code>,
   * <code>type</code>, <code>stride</code>, and <code>pointer</code>
   * are saved as client-side state.
   *
   * <p>If the matrix index array is enabled, it is used when
   * <code>glDrawArrays</code>, or <code>glDrawElements</code> is
   * called. To enable and disable the vertex array, call
   * <code>glEnableClientState</code> and
   * <code>glDisableClientState</code> with the argument
   * <code>GL_MATRIX_INDEX_ARRAY_OES</code>. The matrix index array is
   * initially disabled and isn't accessed when
   * <code>glDrawArrays</code> or <code>glDrawElements</code> is
   * called.
   *
   * <p>Use <code>glDrawArrays</code> to construct a sequence of
   * primitives (all of the same type) from prespecified vertex and
   * vertex attribute arrays. Use <code>glDrawElements</code> to
   * construct a sequence of primitives by indexing vertices and
   * vertex attributes.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glMatrixIndexPointer</code> is typically implemented on
   * the client side.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>size</code> is greater than
   * <code>GL_MAX_VERTEX_UNITS_OES</code>.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>type</code>
   * is is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>stride</code> is negative.
   *
   * @param size Specifies the number of matrix indices per
   * vertex. Must be is less than or equal to
   * <code>GL_MAX_VERTEX_UNITS_OES</code>. The initial value is 0.
   * @param type Specifies the data type of each matrix index in the
   * array. Symbolic constant <code>GL_UNSIGNED_BYTE</code> is
   * accepted. The initial value is <code>GL_UNSIGNED_BYTE</code>.
   * @param stride Specifies the byte offset between consecutive
   * matrix indices. If stride is 0, the matrix indices are understood
   * to be tightly packed in the array. The initial value is 0.
   * @param pointer Specifies a buffer containing the first matrix
   * index of the first vertex in the array. The initial value is 0.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_matrix_palette</code> extension.
   * @exception IllegalArgumentException if <code>pointer</code> is
   * <code>null</code>.
   * @exception IllegalStateException if VBOs are enabled.
   */
  void glMatrixIndexPointerOES(int size, int type, int stride, Buffer pointer);

  /**
   * VBO version of <code>glMatrixIndexPointerOES</code>.
   *
   * @see #glMatrixIndexPointerOES(int size, int type, int stride,
   * Buffer pointer)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_matrix_palette</code> extension.
   * @throws IllegalStateException if VBOs are not enabled.
   */
  void glMatrixIndexPointerOES(int size, int type, int stride, int offset);
  
  /**
   * (<code>OES_matrix_palette</code> extension) Define an array of weights.
   *
   * <p><code>glWeightPointer</code> specifies the location and data
   * of an array of weights to use when rendering. <code>size</code>
   * specifies the number of weights per vertex and <code>type</code>
   * the data type of the coordinates. <code>stride</code> specifies
   * the byte stride from one weight to the next allowing vertices and
   * attributes to be packed into a single array or stored in separate
   * arrays. (Single-array storage may be more efficient on some
   * implementations.)
   *
   * <p>These weights are used to blend corresponding matrices for a
   * given vertex.
   *
   * <p>When a weight array is specified, <code>size</code>,
   * <code>type</code>, <code>stride</code>, and <code>pointer</code>
   * are saved as client-side state.
   *
   * <p>If the weight array is enabled, it is used when
   * <code>glDrawArrays</code>, or <code>glDrawElements</code> is
   * called. To enable and disable the vertex array, call
   * <code>glEnableClientState</code> and
   * <code>glDisableClientState</code> with the argument
   * <code>GL_WEIGHT_ARRAY_OES</code>. The weight array is initially
   * disabled and isn't accessed when <code>glDrawArrays</code> or
   * <code>glDrawElements</code> is called.
   *
   * <p>Use <code>glDrawArrays</code> to construct a sequence of
   * primitives (all of the same type) from prespecified vertex and
   * vertex attribute arrays. Use <code>glDrawElements</code> to
   * construct a sequence of primitives by indexing vertices and
   * vertex attributes.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glWeightPointer</code> is typically implemented on the
   * client side.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>size</code> is greater than
   * <code>GL_MAX_VERTEX_UNITS_OES</code>.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>type</code>
   * is is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>stride</code> is negative.
   *
   * @param size Specifies the number of weights per vertex. Must be
   * is less than or equal to
   * <code>GL_MAX_VERTEX_UNITS_OES</code>. The initial value is 0.
   * @param type Specifies the data type of each weight in the
   * array. Symbolic constant <code>GL_FIXED</code> is
   * accepted. However, the common profile also accepts the symbolic
   * constant <code>GL_FLOAT</code> as well. The initial value is
   * <code>GL_FIXED</code> for the common lite profile, or
   * <code>GL_FLOAT</code> for the common profile.
   * @param stride Specifies the byte offset between consecutive
   * weights. If <code>stride</code> is 0, the weights are understood
   * to be tightly packed in the array. The initial value is 0.
   * @param pointer Specifies a <code>Buffer</code> containing the
   * weights of each vertex in the array. The initial value is 0.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_matrix_palette</code> extension.
   * @exception IllegalArgumentException if <code>pointer</code> is
   * <code>null</code>.
   * @exception IllegalStateException if VBOs are enabled.
   */
  void glWeightPointerOES(int size, int type, int stride, Buffer pointer);

  /**
   * (<code>OES_matrix_palette</code> extension) VBO version of
   * <code>glWeightPointerOES</code>.
   *
   * @see #glWeightPointerOES(int size, int type, int stride, Buffer pointer)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_matrix_palette</code> extension.
   * @exception IllegalStateException if VBOs are not enabled.
   */
  void glWeightPointerOES(int size, int type, int stride, int offset);

}
