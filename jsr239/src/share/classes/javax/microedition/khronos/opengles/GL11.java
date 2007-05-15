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

 * The <code>GL11</code> interface contains the Java(TM) programming
 * language bindings for OpenGL(R) ES 1.1 core functionality.
 *
 * <p> The <code>OES_matrix_get</code>, <code>OES_point_sprite</code> and
 * <code>OES_point_size_array</code> core extensions are defined as
 * part of this interface.
 *
 * <p> Methods that are also present in OpenGL ES 1.0 are included in
 * this interface by inheritance, but are not separately documented.
 * Instead, the documentation from the <code>GL10</code> interface
 * specifies the functional differences between 1.0 and 1.1.
 *
 * <p> See the <code>GL</code> interface for a description of how to
 * obtain an instance of this interface.
 */
public interface GL11 extends GL10 {

  // Begin GL 1.1

  // ClipPlaneName

  /** OpenGL ES 1.1 constant. */
  int GL_CLIP_PLANE0                          = 0x3000;
  /** OpenGL ES 1.1 constant. */
  int GL_CLIP_PLANE1                          = 0x3001;
  /** OpenGL ES 1.1 constant. */
  int GL_CLIP_PLANE2                          = 0x3002;
  /** OpenGL ES 1.1 constant. */
  /** OpenGL ES 1.1 constant. */
  int GL_CLIP_PLANE3                          = 0x3003;
  /** OpenGL ES 1.1 constant. */
  int GL_CLIP_PLANE4                          = 0x3004;
  /** OpenGL ES 1.1 constant. */
  int GL_CLIP_PLANE5                          = 0x3005;

  // GetPName

  /** OpenGL ES 1.1 constant. */
  int GL_CURRENT_COLOR                        = 0x0B00;
  /** OpenGL ES 1.1 constant. */
  int GL_CURRENT_NORMAL                       = 0x0B02;
  /** OpenGL ES 1.1 constant. */
  int GL_CURRENT_TEXTURE_COORDS               = 0x0B03;
  /** OpenGL ES 1.1 constant. */
  int GL_POINT_SIZE                           = 0x0B11;
  /** OpenGL ES 1.1 constant. */
  int GL_POINT_SIZE_MIN                       = 0x8126;
  /** OpenGL ES 1.1 constant. */
  int GL_POINT_SIZE_MAX                       = 0x8127;
  /** OpenGL ES 1.1 constant. */
  int GL_POINT_FADE_THRESHOLD_SIZE            = 0x8128;
  /** OpenGL ES 1.1 constant. */
  int GL_POINT_DISTANCE_ATTENUATION           = 0x8129;
  /** OpenGL ES 1.1 constant. */
  int GL_LINE_WIDTH                           = 0x0B21;
  /** OpenGL ES 1.1 constant. */
  int GL_CULL_FACE_MODE                       = 0x0B45;
  /** OpenGL ES 1.1 constant. */
  int GL_FRONT_FACE                           = 0x0B46;
  /** OpenGL ES 1.1 constant. */
  int GL_SHADE_MODEL                          = 0x0B54;
  /** OpenGL ES 1.1 constant. */
  int GL_DEPTH_RANGE                          = 0x0B70;
  /** OpenGL ES 1.1 constant. */
  int GL_DEPTH_WRITEMASK                      = 0x0B72;
  /** OpenGL ES 1.1 constant. */
  int GL_DEPTH_CLEAR_VALUE                    = 0x0B73;
  /** OpenGL ES 1.1 constant. */
  int GL_DEPTH_FUNC                           = 0x0B74;
  /** OpenGL ES 1.1 constant. */
  int GL_STENCIL_CLEAR_VALUE                  = 0x0B91;
  /** OpenGL ES 1.1 constant. */
  int GL_STENCIL_FUNC                         = 0x0B92;
  /** OpenGL ES 1.1 constant. */
  int GL_STENCIL_VALUE_MASK                   = 0x0B93;
  /** OpenGL ES 1.1 constant. */
  int GL_STENCIL_FAIL                         = 0x0B94;
  /** OpenGL ES 1.1 constant. */
  int GL_STENCIL_PASS_DEPTH_FAIL              = 0x0B95;
  /** OpenGL ES 1.1 constant. */
  int GL_STENCIL_PASS_DEPTH_PASS              = 0x0B96;
  /** OpenGL ES 1.1 constant. */
  int GL_STENCIL_REF                          = 0x0B97;
  /** OpenGL ES 1.1 constant. */
  int GL_STENCIL_WRITEMASK                    = 0x0B98;
  /** OpenGL ES 1.1 constant. */
  int GL_MATRIX_MODE                          = 0x0BA0;
  /** OpenGL ES 1.1 constant. */
  int GL_VIEWPORT                             = 0x0BA2;
  /** OpenGL ES 1.1 constant. */
  int GL_MODELVIEW_STACK_DEPTH                = 0x0BA3;
  /** OpenGL ES 1.1 constant. */
  int GL_PROJECTION_STACK_DEPTH               = 0x0BA4;
  /** OpenGL ES 1.1 constant. */
  int GL_TEXTURE_STACK_DEPTH                  = 0x0BA5;
  /** OpenGL ES 1.1 constant. */
  int GL_MODELVIEW_MATRIX                     = 0x0BA6;
  /** OpenGL ES 1.1 constant. */
  int GL_PROJECTION_MATRIX                    = 0x0BA7;
  /** OpenGL ES 1.1 constant. */
  int GL_TEXTURE_MATRIX                       = 0x0BA8;
  /** OpenGL ES 1.1 constant. */
  int GL_ALPHA_TEST_FUNC                      = 0x0BC1;
  /** OpenGL ES 1.1 constant. */
  int GL_ALPHA_TEST_REF                       = 0x0BC2;
  /** OpenGL ES 1.1 constant. */
  int GL_BLEND_DST                            = 0x0BE0;
  /** OpenGL ES 1.1 constant. */
  int GL_BLEND_SRC                            = 0x0BE1;
  /** OpenGL ES 1.1 constant. */
  int GL_LOGIC_OP_MODE                        = 0x0BF0;
  /** OpenGL ES 1.1 constant. */
  int GL_SCISSOR_BOX                          = 0x0C10;
  /** OpenGL ES 1.1 constant. */
  int GL_COLOR_CLEAR_VALUE                    = 0x0C22;
  /** OpenGL ES 1.1 constant. */
  int GL_COLOR_WRITEMASK                      = 0x0C23;
  /** OpenGL ES 1.1 constant. */
  int GL_MAX_CLIP_PLANES                      = 0x0D32;
  /** OpenGL ES 1.1 constant. */
  int GL_POLYGON_OFFSET_UNITS                 = 0x2A00;
  /** OpenGL ES 1.1 constant. */
  int GL_POLYGON_OFFSET_FACTOR                = 0x8038;
  /** OpenGL ES 1.1 constant. */
  int GL_TEXTURE_BINDING_2D                   = 0x8069;
  /** OpenGL ES 1.1 constant. */
  int GL_VERTEX_ARRAY_SIZE                    = 0x807A;
  /** OpenGL ES 1.1 constant. */
  int GL_VERTEX_ARRAY_TYPE                    = 0x807B;
  /** OpenGL ES 1.1 constant. */
  int GL_VERTEX_ARRAY_STRIDE                  = 0x807C;
  /** OpenGL ES 1.1 constant. */
  int GL_NORMAL_ARRAY_TYPE                    = 0x807E;
  /** OpenGL ES 1.1 constant. */
  int GL_NORMAL_ARRAY_STRIDE                  = 0x807F;
  /** OpenGL ES 1.1 constant. */
  int GL_COLOR_ARRAY_SIZE                     = 0x8081;
  /** OpenGL ES 1.1 constant. */
  int GL_COLOR_ARRAY_TYPE                     = 0x8082;
  /** OpenGL ES 1.1 constant. */
  int GL_COLOR_ARRAY_STRIDE                   = 0x8083;
  /** OpenGL ES 1.1 constant. */
  int GL_TEXTURE_COORD_ARRAY_SIZE             = 0x8088;
  /** OpenGL ES 1.1 constant. */
  int GL_TEXTURE_COORD_ARRAY_TYPE             = 0x8089;
  /** OpenGL ES 1.1 constant. */
  int GL_TEXTURE_COORD_ARRAY_STRIDE           = 0x808A;

  /** OpenGL ES 1.1 constant. */
  int GL_VERTEX_ARRAY_POINTER                 = 0x808E;
  /** OpenGL ES 1.1 constant. */
  int GL_NORMAL_ARRAY_POINTER                 = 0x808F;
  /** OpenGL ES 1.1 constant. */
  int GL_COLOR_ARRAY_POINTER                  = 0x8090;
  /** OpenGL ES 1.1 constant. */
  int GL_TEXTURE_COORD_ARRAY_POINTER          = 0x8092;

  /** OpenGL ES 1.1 constant. */
  int GL_SAMPLE_BUFFERS                       = 0x80A8;
  /** OpenGL ES 1.1 constant. */
  int GL_SAMPLES                              = 0x80A9;
  /** OpenGL ES 1.1 constant. */
  int GL_SAMPLE_COVERAGE_VALUE                = 0x80AA;
  /** OpenGL ES 1.1 constant. */
  int GL_SAMPLE_COVERAGE_INVERT               = 0x80AB;

  // HintTarget

  /** OpenGL ES 1.1 constant. */
  int GL_GENERATE_MIPMAP_HINT                 = 0x8192;

  // TextureParameterName

  /** OpenGL ES 1.1 constant. */
  int GL_GENERATE_MIPMAP                      = 0x8191;

  // TextureUnit

  /** OpenGL ES 1.1 constant. */
  int GL_ACTIVE_TEXTURE                       = 0x84E0;
  /** OpenGL ES 1.1 constant. */
  int GL_CLIENT_ACTIVE_TEXTURE                = 0x84E1;

  // Buffer Objects

  /** OpenGL ES 1.1 constant. */
  int GL_ARRAY_BUFFER                         = 0x8892;
  /** OpenGL ES 1.1 constant. */
  int GL_ELEMENT_ARRAY_BUFFER                 = 0x8893;

  /** OpenGL ES 1.1 constant. */
  int GL_ARRAY_BUFFER_BINDING                 = 0x8894;
  /** OpenGL ES 1.1 constant. */
  int GL_ELEMENT_ARRAY_BUFFER_BINDING         = 0x8895;
  /** OpenGL ES 1.1 constant. */
  int GL_VERTEX_ARRAY_BUFFER_BINDING          = 0x8896;
  /** OpenGL ES 1.1 constant. */
  int GL_NORMAL_ARRAY_BUFFER_BINDING          = 0x8897;
  /** OpenGL ES 1.1 constant. */
  int GL_COLOR_ARRAY_BUFFER_BINDING           = 0x8898;
  /** OpenGL ES 1.1 constant. */
  int GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING   = 0x889A;

  /** OpenGL ES 1.1 constant. */
  int GL_STATIC_DRAW                          = 0x88E4;
  /** OpenGL ES 1.1 constant. */
  int GL_DYNAMIC_DRAW                         = 0x88E8;

  /** OpenGL ES 1.1 constant. */
  int GL_WRITE_ONLY                           = 0x88B9;

  /** OpenGL ES 1.1 constant. */
  int GL_BUFFER_SIZE                          = 0x8764;
  /** OpenGL ES 1.1 constant. */
  int GL_BUFFER_USAGE                         = 0x8765;
  /** OpenGL ES 1.1 constant. */
  int GL_BUFFER_ACCESS                        = 0x88BB;

  // Texture combine + dot3

  /** OpenGL ES 1.1 constant. */
  int GL_SUBTRACT                             = 0x84E7;
  /** OpenGL ES 1.1 constant. */
  int GL_COMBINE                              = 0x8570;
  /** OpenGL ES 1.1 constant. */
  int GL_COMBINE_RGB                          = 0x8571;
  /** OpenGL ES 1.1 constant. */
  int GL_COMBINE_ALPHA                        = 0x8572;
  /** OpenGL ES 1.1 constant. */
  int GL_RGB_SCALE                            = 0x8573;
  /** OpenGL ES 1.1 constant. */
  int GL_ADD_SIGNED                           = 0x8574;
  /** OpenGL ES 1.1 constant. */
  int GL_INTERPOLATE                          = 0x8575;
  /** OpenGL ES 1.1 constant. */
  int GL_CONSTANT                             = 0x8576;
  /** OpenGL ES 1.1 constant. */
  int GL_PRIMARY_COLOR                        = 0x8577;
  /** OpenGL ES 1.1 constant. */
  int GL_PREVIOUS                             = 0x8578;
  /** OpenGL ES 1.1 constant. */
  int GL_OPERAND0_RGB                         = 0x8590;
  /** OpenGL ES 1.1 constant. */
  int GL_OPERAND1_RGB                         = 0x8591;
  /** OpenGL ES 1.1 constant. */
  int GL_OPERAND2_RGB                         = 0x8592;
  /** OpenGL ES 1.1 constant. */
  int GL_OPERAND0_ALPHA                       = 0x8598;
  /** OpenGL ES 1.1 constant. */
  int GL_OPERAND1_ALPHA                       = 0x8599;
  /** OpenGL ES 1.1 constant. */
  int GL_OPERAND2_ALPHA                       = 0x859A;

  /** OpenGL ES 1.1 constant. */
  int GL_ALPHA_SCALE                          = 0x0D1C;

  /** OpenGL ES 1.1 constant. */
  int GL_SRC0_RGB                             = 0x8580;
  /** OpenGL ES 1.1 constant. */
  int GL_SRC1_RGB                             = 0x8581;
  /** OpenGL ES 1.1 constant. */
  int GL_SRC2_RGB                             = 0x8582;
  /** OpenGL ES 1.1 constant. */
  int GL_SRC0_ALPHA                           = 0x8588;
  /** OpenGL ES 1.1 constant. */
  int GL_SRC1_ALPHA                           = 0x8589;
  /** OpenGL ES 1.1 constant. */
  int GL_SRC2_ALPHA                           = 0x858A;

  /** OpenGL ES 1.1 constant. */
  int GL_DOT3_RGB                             = 0x86AE;
  /** OpenGL ES 1.1 constant. */
  int GL_DOT3_RGBA                            = 0x86AF;

  // Functions

  // Buffers

  /**  
   * Generate buffer object names.
   *
   * <p><code>glGenBuffers</code> returns <code>n</code> buffer object
   * names in <code>buffers</code>.
   *
   * <p>These names are marked as used, for the purposes of
   * <code>glGenBuffers</code> only, but they acquire buffer state
   * only when they are first bound, just as if they were unused.
   *
   * <h4>Errors</h4>
   *
   * <p>GL_INVALID_VALUE is generated if <code>n</code> is negative.
   *
   * @param n Specifies the number of buffer object names to be generated.
   * @param buffers Specifies an array in which the generated buffer
   * object names are stored.
   * @param offset the starting offset within the
   * <code>buffers</code> array.
   *
   * @exception IllegalArgumentException if <code>buffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if
   * <code>buffers.length - offset</code> is less than <code>n</code>.
   */
  void glGenBuffers(int n, int[] buffers, int offset);

  /**  
   * Integer <code>Buffer</code> version of
   * <code>glGenBuffers</code>.
   * 
   * @see #glGenBuffers(int n, int[] buffers, int offset)
   *
   * @exception IllegalArgumentException if <code>buffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>buffers.remaining()</code> is less than <code>n</code>.
   */
  void glGenBuffers(int n, IntBuffer buffers);

  /**  
   * Bind a named buffer to a target.
   *
   * <p><code>glBindBuffer</code> lets you create or use a named
   * buffer. Calling <code>glBindBuffer</code> with
   * <code>target</code> set to <code>GL_ARRAY_BUFFER</code> or
   * <code>GL_ELEMENT_ARRAY_BUFFER</code> and <code>buffer</code> set
   * to the buffer name, binds the buffer name to the target.
   *
   * <p>Buffer names are unsigned integers. The value 0 is reserved
   * for GL.
   *
   * <p>When a buffer is bound to a target, any previous binding for
   * that target is automatically broken.
   *
   * <p>If an unused buffer name is specified, a buffer object is
   * created. The resulting buffer object is a new state vector,
   * initialized with a zero-sized memory buffer, and with the
   * following state values.
   * 
   * <p><code>GL_BUFFER_SIZE</code> initialized to 0.
   * <p><code>GL_BUFFER_USAGE</code> initialized to
   * <code>GL_STATIC_DRAW</code>.
   * <p><code>GL_BUFFER_ACCESS</code>
   * initialized to <code>GL_WRITE_ONLY</code>.
   * <p><code>glBindBuffer</code> may also be used to bind an existing
   * buffer object. If the bind is successful no change is made to the
   * state of the newly bound buffer object.
   * 
   * <p>While a buffer is bound, GL operations on the target to which
   * it is bound affect the bound buffer object, and queries of the
   * target to which a buffer object is bound return state from the
   * bound object.
   * 
   * <h4>Notes</h4>
   * 
   * <p>In the initial state the reserved name zero is bound to
   * <code>GL_ARRAY_BUFFER</code> and
   * <code>GL_ELEMENT_ARRAY_BUFFER</code>. There is no buffer object
   * corresponding to the name zero, so client attempts to modify or
   * query buffer object state for the target
   * <code>GL_ARRAY_BUFFER</code> or
   * <code>GL_ELEMENT_ARRAY_BUFFER</code> while zero is bound will
   * generate GL errors.
   * 
   * <p>While a buffer object is bound, any GL operations on that
   * object affect any other bindings of that object.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> is not one of the allowable values.
   * 
   * @param target Specifies the target to which the buffer is
   * bound. Which must be <code>GL_ARRAY_BUFFER</code> or
   * <code>GL_ELEMENT_ARRAY_BUFFER</code>.
   * @param buffer Specifies the name of a buffer.
   */
  void glBindBuffer(int target, int buffer);

  /**  
   * Creates and initializes the data store of a buffer object.
   *
   * <code>glBufferData</code> lets you create and initialize the data
   * store of a buffer object.
   * 
   * <p>If <code>data</code> is non-<code>null</code>, then the source
   * data is copied to the buffer object’s data store. If
   * <code>data</code> is <code>null</code>, then the contents of the
   * buffer object’s data store are undefined.
   * 
   * <p>The options for <code>usage</code> are:
   * 
   * <ul>
   * 
   * <li><code>GL_STATIC_DRAW</code></li>
   * 
   * <p> Where the data store contents will be specified once by the
   * application, and used many times as the source for GL drawing
   * commands.
   * 
   * <li><code>GL_DYNAMIC_DRAW</code></li>
   * 
   * <p> Where the data store contents will be respecified repeatedly
   * by the application, and used many times as the source for GL
   * drawing commands.
   * 
   * </ul>
   * 
   * <p><code>glBufferData</code> deletes any existing data store, and
   * sets the values of the buffer object’s state variables thus:
   * 
   * <p><code>GL_BUFFER_SIZE</code> initialized to <code>size</code>.
   * <p><code>GL_BUFFER_USAGE</code> initialized to <code>usage</code>.
   * <p><code>GL_BUFFER_ACCESS</code> initialized to
   * <code>GL_WRITE_ONLY</code>.
   * 
   * <p>Clients must align data elements consistent with the
   * requirements of the client platform, with an additional
   * base-level requirement that an offset within a buffer to a datum
   * comprising N basic machine units be a multiple of N.
   * 
   * <h4>Notes</h4>
   * 
   * <p><code>usage</code> is provided as a performance hint only. The
   * specified usage value does not constrain the actual usage pattern
   * of the data store.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> is not one of the allowable values.
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>usage</code> is not one of the allowable values.
   * 
   * <p><code>GL_OUT_OF_MEMORY</code> is generated if the GL is unable
   * to create a data store of the requested size.
   * 
   * @param target Specifies the buffer object target, which must be
   * <code>GL_ARRAY_BUFFER</code> or
   * <code>GL_ELEMENT_ARRAY_BUFFER</code>.
   * @param size Specifies the size of the data store in basic machine
   * units.
   * @param data Specifies the source data in client memory.
   * @param usage Specifies the expected application usage pattern of
   * the data store. Accepted values are <code>GL_STATIC_DRAW</code>
   * and <code>GL_DYNAMIC_DRAW</code>.
   *
   * @exception IllegalArgumentException if <code>data</code> is
   * <code>non-null</code> but is not a direct buffer.
   *
   * <!-- Need revisit - buffer type mixing - byte order issues -->
   */
  void glBufferData(int target, int size, Buffer data, int usage);

  /**  
   * Modifies some or all of the data contained in a buffer object's
   * data store.
   * 
   * <p><code>glBufferSubData</code> lets you modify some or all of
   * the data contained in a buffer object's data store.
   * 
   * <h4>Errors</h4>
   * 
   * <p>GL_INVALID_ENUM is generated if <code>target</code> is not one of the
   * allowable values.
   * 
   * <p>GL_INVALID_VALUE is generated if <code>offset</code> or
   * <code>size</code> is less than zero, or if <code>offset +
   * size</code> is greater than the value of
   * <code>GL_BUFFER_SIZE</code>.
   * 
   * @param target Specifies the buffer object <code>target</code>. Must be
   * <code>GL_ARRAY_BUFFER</code> or
   * <code>GL_ELEMENT_ARRAY_BUFFER</code>.
   * @param offset Specifies the starting offset of the data to be
   * replaced in bytes.
   * @param size Specifies the size of the data to be replaced in bytes.
   * @param data Specifies a region of client memory <code>size</code>
   * bytes in length, containing the data that replace the specified
   * buffer range.
   *
   * @exception IllegalArgumentException if <code>data</code> is
   * <code>null</code>.
   *
   * <!-- Need revisit - buffer type mixing - byte order issues -->
   */
  void glBufferSubData(int target, int offset, int size, Buffer data);

  /**  
   * Delete named buffer objects.
   * 
   * <p><code>glDeleteBuffers</code> deletes <code>n</code> buffer
   * objects named by the elements of the array buffers. After a
   * buffer object is deleted, it has no contents, and its name is
   * free for reuse.
   * 
   * <p><code>glDeleteBuffers</code> silently ignores zero and names
   * that do not correspond to existing buffer objects.
   * 
   * <h4>Notes</h4>
   * 
   * <p>If a buffer object is deleted while it is bound, all bindings
   * to that object in the current context (i.e., in the thread that
   * called <code>glDeleteBuffers</code>) are reset to zero. Bindings
   * to that buffer in other contexts and other threads are not
   * affected, but attempting to use a deleted buffer in another
   * thread produces undefined results, including but not limited to
   * possible GL errors and rendering corruption. Using a deleted
   * buffer in another context or thread may not, however, result in
   * program termination.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_VALUE</code> is generated if <code>n</code>
   * is negative.
   * 
   * @param n Specifies the number of buffer objects to be deleted.
   * @param buffers Specifies an array of buffer object names to be
   * deleted.
   * @param offset the starting offset within the
   * <code>buffers</code> array.
   *
   * @exception IllegalArgumentException if <code>buffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if
   * <code>buffers.length - offset</code> is less than <code>n</code>.
   */
  void glDeleteBuffers(int n, int[] buffers, int offset);

  /**  
   * Integer <code>Buffer</code> version of
   * <code>glDeleteBuffers</code>.
   * 
   * @see #glDeleteBuffers(int n, int[] buffers, int offset)
   *
   * @exception IllegalArgumentException if <code>buffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>buffers.remaining()</code> is less than <code>n</code>.
   */
  void glDeleteBuffers(int n, IntBuffer buffers);

  // Clip Planes
  
  /**  
   * Specify a plane against which all geometry is clipped.
   *
   * <p>Geometry is always clipped against the boundaries of a
   * six-plane frustum in x, y, and z. <code>glClipPlane</code> allows
   * the specification of additional planes, not necessarily
   * perpendicular to the x, y, or z axis, against which all geometry
   * is clipped. To determine the maximum number of additional
   * clipping planes, call <code>glGet</code> with argument
   * <code>GL_MAX_CLIP_PLANES</code>. All implementations support at
   * least one such clipping planes. Because the resulting clipping
   * region is the intersection of the defined half-spaces, it is
   * always convex.
   * 
   * <p><code>glClipPlane</code> specifies a half-space using a
   * four-component plane equation. When <code>glClipPlane</code> is
   * called, equation is transformed by the inverse of the modelview
   * matrix and stored in the resulting eye coordinates. Subsequent
   * changes to the modelview matrix have no effect on the stored
   * plane-equation components. If the dot product of the eye
   * coordinates of a vertex with the stored plane equation components
   * is positive or zero, the vertex is in with respect to that
   * clipping plane. Otherwise, it is out.
   * 
   * <p>To enable and disable clipping planes, call
   * <code>glEnable</code> and glDisable with the argument
   * <code>GL_CLIP_PLANE</code><i>i</i>, where <i>i</i> is the plane number.
   * 
   * <p>All clipping planes are initially defined as (0, 0, 0, 0) in
   * eye coordinates and are disabled.
   * 
   * <h4>Notes</h4>
   * 
   * <p>It is always the case that <code>GL_CLIP_PLANE</code><i>i</i> =
   * <code>GL_CLIP_PLANE0</code> + <i>i</i>.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>plane</code> is not an accepted value.
   * 
   * <h4>Associated Gets</h4>
   * 
   * <p><code>glGetClipPlane</code>, <code>glIsEnabled</code> with
   * argument <code>GL_CLIP_PLANE</code><i>i</i>.
   * 
   * @param plane Specifies which clipping plane is being
   * positioned. Symbolic names of the form
   * <code>GL_CLIP_PLANE</code><i>i</i> where <code>0 < <i>i</i> <
   * GL_MAX_CLIP_PLANES</code> are accepted.
   * @param equation Specifies an array of four values. These values
   * are interpreted as a plane equation.
   * @param offset the starting offset within the
   * <code>equation</code> array.
   *
   * @exception IllegalArgumentException if <code>equation</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if
   * <code>equation.length - offset</code> is less than 4.
   */
  void glClipPlanef(int plane, float[] equation, int offset);

  /**  
   * Floating-point <code>Buffer</code> version of
   * <code>glClipPlane</code>.
   *
   * @see #glClipPlanef(int plane, float[] equation, int offset)
   *
   * @exception IllegalArgumentException if <code>equation</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>equation.remaining()</code> is less than 4.
   */
  void glClipPlanef(int plane, FloatBuffer equation);

  /**  
   * Fixed-point array version of <code>glClipPlane</code>.
   *
   * @see #glClipPlanef(int plane, float[] equation, int offset)
   *
   * @exception IllegalArgumentException if <code>equation</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if
   * <code>equation.length - offset</code> is less than 4.
   */
  void glClipPlanex(int plane, int[] equation, int offset);

  /**  
   * Fixed-point <code>Buffer</code> version of
   * <code>glClipPlane</code>.
   *
   * @see #glClipPlanef(int plane, float[] equation, int offset)
   *
   * @exception IllegalArgumentException if <code>equation</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>equation.remaining()</code> is less than 4.
   */
  void glClipPlanex(int plane, IntBuffer equation);

  // Getters

  /**
   * Floating-point array version of <code>glGet</code>.
   *
   * @see #glGetIntegerv(int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetFloatv(int pname, float[] params, int offset);

  /**  
   * Floating-point <code>Buffer</code> version of
   * <code>glGet</code>.
   * 
   * @see #glGetIntegerv(int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetFloatv(int pname, FloatBuffer params);

  /**  
   * Boolean array version of <code>glGet</code>.
   * 
   * @see #glGetIntegerv(int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetBooleanv(int pname, boolean[] params, int offset);
 
  /**
   * Boolean <code>Buffer</code> version of
   * <code>glGet</code>.  A value of 0 represents <code>false</code>
   * and a value of 1 represents <code>true</code>.
   *
   * @see #glGetIntegerv(int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetBooleanv(int pname, IntBuffer params);
 
  /**  
   * Fixed-point array version of <code>glGet</code>.
   * 
   * @see #glGetIntegerv(int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetFixedv(int pname, int[] params, int offset);

  /**  
   * Fixed-point <code>Buffer</code> version of
   * <code>glGet</code>.
   * 
   * @see #glGetIntegerv(int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetFixedv(int pname, IntBuffer params);

  /**  
   * Return buffer parameter values.
   * 
   * <p><code>glGetBufferParameter</code> returns in
   * <code>params</code> the value or values of the buffer object
   * parameter specified as <code>pname</code>. <code>target</code>
   * defines the target buffer object, which must be
   * <code>GL_ARRAY_BUFFER</code>.
   * 
   * <ul>
   * 
   * <li><code>GL_BUFFER_SIZE</code></li>
   * 
   * <p> Returns the size of the data store in bytes.
   * 
   * <li><code>GL_BUFFER_USAGE</code></li>
   * 
   * <p> Returns the expected application usage pattern of the data
   * store. Possible values are:
   * 
   * <li><code>GL_STATIC_DRAW</code></li>
   * 
   * <p> Where the data store contents will be specified once by the
   * application, and used many times as the source for GL drawing
   * commands.
   * 
   * <li><code>GL_DYNAMIC_DRAW</code></li>
   * 
   * <p> Where the data store contents will be respecified repeatedly
   * by the application, and used many times as the source for GL
   * drawing commands.
   * 
   * <li><code>GL_BUFFER_ACCESS</code></li>
   * 
   * <p> Returns the access capability for the data store. Will always
   * be <code>GL_WRITE_ONLY</code>.
   * 
   * </ul>
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> or <code>pname</code> is not one of the
   * accepted defined values.
   * 
   * @param target Specifies the buffer object target. Which must be
   * <code>GL_ARRAY_BUFFER</code>.
   * @param pname Specifies the symbolic name of a buffer object
   * parameter. Which can be either <code>GL_BUFFER_SIZE</code>,
   * <code>GL_BUFFER_USAGE</code>, or <code>GL_BUFFER_ACCESS</code>.
   * @param params returns the buffer object parameters.
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetBufferParameteriv(int target, int pname, int[] params, int offset);

  /**  
   * Integer <code>Buffer</code> version of
   * <code>glGetBufferParameter</code>.
   *
   * @see #glGetBufferParameteriv(int target, int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
   void glGetBufferParameteriv(int target, int pname, IntBuffer params);

  /**
   * Return the coefficients of the specified clipping
   * plane.
   * 
   * <p><code>glGetClipPlane</code> returns in <code>equation</code>
   * the four coefficients of the plane equation for
   * <code>plane</code>.
   * 
   * <h4>Notes</h4>
   * 
   * <p>It is always the case that <code>GL_CLIP_PLANE</code><i>i</i> =
   * <code>GL_CLIP_PLANE0</code> + <i>i</i>.
   * 
   * <p>If an error is generated, no change is made to the contents of
   * equation.  <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>plane</code> is not an accepted value.
   * 
   * @param plane Specifies a clipping plane. The number of clipping
   * planes depends on the implementation, but at least six clipping
   * planes are supported. They are identified by symbolic names of
   * the form <code>GL_CLIP_PLANE</code><i>i</i> where <code>0 <
   * <i>i</i> < GL_MAX_CLIP_PLANES</code>
   * @param equation Returns four values that are the coefficients of
   * the plane equation of plane in eye coordinates. The initial value
   * is (0, 0, 0, 0).
   * @param offset the starting offset within the
   * <code>equation</code> array.
   * 
   * @exception IllegalArgumentException if <code>equation</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if
   * <code>equation.length - offset</code> is less than 4.
   */
  void glGetClipPlanef(int plane, float[] equation, int offset);

  /**  
   * Floating-point <code>Buffer</code> version of
   * <code>glGetClipPlane</code>.
   *
   * @see #glGetClipPlanef(int plane, float[] equation, int offset)
   *
   * @exception IllegalArgumentException if <code>equation</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>equation.remaining()</code> is less than 4.
   */
   void glGetClipPlanef(int plane, FloatBuffer equation);

  /**  
   * Fixed-point array version of
   * <code>glGetClipPlane</code>.
   *
   * @see #glGetClipPlanef(int plane, float[] equation, int offset)
   *
   * @exception IllegalArgumentException if <code>equation</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>equation.length -
   * offset</code> is less than 4.
   */
   void glGetClipPlanex(int plane, int[] equation, int offset);

  /**  
   * Fixed-point <code>Buffer</code> version of
   * <code>glGetClipPlane</code>.
   *
   * @see #glGetClipPlanef(int plane, float[] equation, int offset)
   *
   * @exception IllegalArgumentException if <code>equation</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>equation.remaining()</code> is less than 4.
   */
   void glGetClipPlanex(int plane, IntBuffer equation);

  /**  
   * Return light source parameter values.
   * 
   * <p><code>glGetLight</code> returns in params the value or values
   * of a light source parameter. light names the light and is a
   * symbolic name of the form <code>GL_LIGHT</code><i>i</i> for <code>0 < i
   * < GL_MAX_LIGHTS</code> where <code>GL_MAX_LIGHTS</code> is an
   * implementation dependent constant that is greater than or equal
   * to eight. <code>pname</code> specifies one of ten light source parameters,
   * again by symbolic name.
   * 
   * The ten light parameters are defined:
   * 
   * <ul>
   * 
   * <li><code>GL_AMBIENT</code></li>
   * 
   * <p> <code>params</code> returns four values that
   * specify the ambient RGBA intensity of the light. Both fixed-point
   * and floating-point values are mapped directly. Neither
   * fixed-point nor floating-point values are clamped. The initial
   * ambient light intensity is (0, 0, 0, 1).
   * 
   * <li><code>GL_DIFFUSE</code></li>
   * 
   * <p> <code>params</code> returns four values that
   * specify the diffuse RGBA intensity of the light. Both fixed-point
   * and floating-point values are mapped directly. Neither
   * fixed-point nor floating-point values are clamped. The initial
   * value for <code>GL_LIGHT0</code> is (1, 1, 1, 1). For other
   * lights, the initial value is (0, 0, 0, 0).
   * 
   * <li><code>GL_SPECULAR</code></li>
   * 
   * <p> <code>params</code> returns four values that
   * specify the specular RGBA intensity of the light. Both
   * fixed-point and floating-point values are mapped
   * directly. Neither fixed-point nor floating-point values are
   * clamped. The initial value for <code>GL_LIGHT0</code> is (1, 1,
   * 1, 1). For other lights, the initial value is (0, 0, 0, 0).
   * 
   * <li><code>GL_EMISSION</code></li>
   * 
   * <p> <code>params</code> returns four values
   * representing the emitted light intensity of the material. Both
   * fixed-point and floating-point values are mapped directly. The
   * initial value is (0, 0, 0, 1).
   * 
   * <li><code>GL_SPOT_DIRECTION</code></li>
   * 
   * <p> <code>params</code> returns three values
   * that specify the direction of the light in homogeneous object
   * coordinates. Both fixed-point and floating-point values are
   * mapped directly. Neither fixed-point nor floating-point values
   * are clamped. The initial direction is (0, 0, -1).
   * 
   * <li><code>GL_SPOT_EXPONENT</code></li>
   * 
   * <p> <code>params</code> returns a single value that specifies the
   * intensity distribution of the light. Fixed-point and
   * floating-point values are mapped directly. Only values in the
   * range <code>[0, 128]</code> are accepted. The initial spot
   * exponent is 0.
   * 
   * <li><code>GL_SPOT_CUTOFF</code></li>
   * 
   * <p> <code>params</code> returns a single value that specifies the
   * maximum spread angle of a light source. Fixed-point and
   * floating-point values are mapped directly. Only values in the
   * range <code>[0, 90]</code> and the special value 180 are
   * accepted. If the angle between the direction of the light and the
   * direction from the light to the vertex being lighted is greater
   * than the spot cutoff angle, the light is completely
   * masked. Otherwise, its intensity is controlled by the spot
   * exponent and the attenuation factors. The initial spot cutoff is
   * 180.
   *
   * <li><code>GL_CONSTANT_ATTENUATION</code>,
   * <code>GL_LINEAR_ATTENUATION</code>,
   * <code>GL_QUADRATIC_ATTENUATION</code></li>
   * 
   * <p> <code>params</code> returns a single value
   * that specifies one of the three light attenuation
   * factors. Fixed-point and floating-point values are mapped
   * directly. Only nonnegative values are accepted. If the light is
   * positional, rather than directional, its intensity is attenuated
   * by the reciprocal of the sum of the constant factor, the linear
   * factor times the distance between the light and the vertex being
   * lighted, and the quadratic factor times the square of the same
   * distance. The initial attenuation factors are (1, 0, 0).
   * 
   * </ul>
   * 
   * <h4>Notes</h4>
   * 
   * <p>It is always the case that <code>GL_LIGHT</code><i>i</i> =
   * <code>GL_LIGHT0</code> + <i>i</i>.
   * 
   * <p>If an error is generated, no change is made to the contents of
   * params.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if light or
   * <code>pname</code> is not an accepted value.
   * 
   * @param light Specifies a light source. The number of possible
   * lights depends on the implementation, but at least eight lights
   * are supported. They are identified by symbolic names of the form
   * <code>GL_LIGHT</code><i>i</i> where <code>0 < <i>i</i> <
   * GL_MAX_LIGHTS</code>.
   * @param pname Specifies a light source parameter for
   * light. Accepted symbolic names are <code>GL_AMBIENT</code>,
   * <code>GL_DIFFUSE</code>, <code>GL_SPECULAR</code>,
   * <code>GL_EMISSION</code>, <code>GL_SPOT_DIRECTION</code>,
   * <code>GL_SPOT_EXPONENT</code>, <code>GL_SPOT_CUTOFF</code>,
   * <code>GL_CONSTANT_ATTENUATION</code>,
   * <code>GL_LINEAR_ATTENUATION</code>, and
   * <code>GL_QUADRATIC_ATTENUATION</code>.
   * @param params returns the requested data.
   * @param offset the starting offset within the
   * <code>params</code> array. 
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetLightfv(int light, int pname, float[] params, int offset);

  /**  
   * Floating-point <code>Buffer</code> version of
   * <code>glGetLight</code>.
   * 
   * @see #glGetLightfv(int light, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetLightfv(int light, int pname, FloatBuffer params);

  /**  
   * Fixed-point array version of <code>glGetLight</code>.
   *
   * @see #glGetLightfv(int light, int pname, float[] params, int offset)
   * 
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetLightxv(int light, int pname, int[] params, int offset);
  
  /**  
   * Fixed-point <code>Buffer</code> version of
   * <code>glGetLight</code>.
   *
   * @see #glGetLightfv(int light, int pname, float[] params, int offset)
   * 
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetLightxv(int light, int pname, IntBuffer params);

  /**  
   * Return material parameters values.
   * 
   * <p><code>glGetMaterial</code> returns in <code>params</code> the
   * value or values of parameter <code>pname</code> of material
   * <code>face</code>.
   * 
   * <p>Five parameters are defined:
   * 
   * <ul>
   * 
   * <li><code>GL_AMBIENT</code></li>
   * 
   * <p> <code>params</code> returns four values that specify the ambient RGBA
   * reflectance of the material. The values are not clamped. The
   * initial ambient reflectance is (0.2, 0.2, 0.2, 1.0).
   * 
   * <li><code>GL_DIFFUSE</code></li>
   * 
   * <p> <code>params</code> returns four values that specify the diffuse RGBA
   * reflectance of the material. The values are not clamped. The
   * initial diffuse reflectance is (0.8, 0.8, 0.8, 1.0).
   * 
   * <li><code>GL_SPECULAR</code></li>
   * 
   * <p> <code>params</code> returns four values that specify the specular RGBA
   * reflectance of the material. The values are not clamped. The
   * initial specular reflectance is (0, 0, 0, 1).
   * 
   * <li><code>GL_EMISSION</code></li>
   * 
   * <p> <code>params</code> returns four values that specify the RGBA emitted
   * light intensity of the material. The values are not clamped. The
   * initial emission intensity is (0, 0, 0, 1).
   * 
   * <li><code>GL_SHININESS</code></li>
   * 
   * <p> <code>params</code> returns a single value that specifies the RGBA
   * specular exponent of the material. The initial specular exponent
   * is 0.
   * 
   * </ul>
   * 
   * <h4>Notes</h4>
   * 
   * <p>If an error is generated, no change is made to the contents of
   * <code>params</code>.
   *
   * <h4>Errors</h4>
   * 
   * <code>GL_INVALID_ENUM</code> is generated if <code>face</code> or
   * <code>pname</code> is not an accepted value.
   * 
   * @param face Specifies which of the two materials is being
   * queried. <code>GL_FRONT</code> or <code>GL_BACK</code> are
   * accepted, representing the front and back materials,
   * respectively.
   * @param pname Specifies the material parameter to return. Accepted
   * symbolic names are <code>GL_AMBIENT</code>,
   * <code>GL_DIFFUSE</code>, <code>GL_SPECULAR</code>,
   * <code>GL_EMISSION</code>, and <code>GL_SHININESS</code>.
   * @param params returns the requested data.
   * @param offset the starting offset within the
   * <code>params</code> array.
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetMaterialfv(int face, int pname, float[] params, int offset);

  /**  
   * Floating-point <code>Buffer</code> version of
   * <code>glGetMaterial</code>.
   * 
   * @see #glGetMaterialfv(int face, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetMaterialfv(int face, int pname, FloatBuffer params);

  /**  
   * Fixed-point array version of
   * <code>glGetMaterial</code>.
   * 
   * @see #glGetMaterialfv(int face, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetMaterialxv(int face, int pname, int[] params, int offset);

  /**  
   * Fixed-point <code>Buffer</code> version of
   * <code>glGetMaterial</code>.
   * 
   * @see #glGetMaterialfv(int face, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetMaterialxv(int face, int pname, IntBuffer params);

  /**
   * Return the <code>Buffer</code> associated with the specified pointer.
   *
   * <p> <code>glGetPointer</code> returns pointer information in the
   * form of a <code>Buffer</code> reference. <code>pname</code> is a
   * symbolic constant indicating the pointer to be returned, and
   * <code>params</code> is an array of <code>Buffer</code>s in which
   * the returned data will be placed in element 0.
   *
   * <h4>Notes</h4>
   *
   * <p>The pointers are all client-side state.
   *
   * <p>The initial value for each pointer is <code>null</code>.
   * <h4>Errors</h4>
   *
   * <p> <code>GL_INVALID_ENUM</code> is generated if
   * <code>pname</code> is not an accepted value.
   *
   * @param pname Specifies the array or buffer pointer to be
   * returned. Accepted symbolic names are
   * <code>GL11.GL_COLOR_ARRAY_POINTER</code>,
   * <code>GL11.GL_NORMAL_ARRAY_POINTER</code>,
   * <code>GL11.GL_TEXTURE_COORD_ARRAY_POINTER</code>,
   * <code>GL11.GL_VERTEX_ARRAY_POINTER</code>,
   * <code>GL11.GL_POINT_SIZE_ARRAY_POINTER_OES</code>,
   * <code>GL11Ext.GL_MATRIX_INDEX_ARRAY_POINTER_OES</code>, and
   * <code>GL11Ext.GL_WEIGHT_ARRAY_POINTER_OES</code>.
   * @param params a non-<code>null</code> array of <code>Buffer</code>s
   * containing at least one element.
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code> or has length less than 1.
   */
  void glGetPointerv(int pname, Buffer[] params);

  /**  
   * Return texture environment parameters.
   * 
   * <p><code>glGetTexParameter</code> returns in <code>params</code>
   * selected values of a texture environment that was specified with
   * <code>glTexEnv</code>. <code>target</code> specifies a texture
   * environment. Currently, only one texture environment is defined
   * and supported: <code>GL_TEXTURE_ENV</code>.
   * 
   * <p><code>pname</code> names a specific texture environment parameter, as
   * follows:
   * 
   * <li><code>GL_TEXTURE_ENV_MODE</code></li>
   * 
   * <p> <code>params</code> returns the single-valued texture
   * environment mode, a symbolic constant. The initial value is
   * <code>GL_MODULATE</code>.
   * 
   * <li><code>GL_TEXTURE_ENV_COLOR</code></li>
   * 
   * <p> <code>params</code> returns four fixed or floating-point
   * values that are the texture environment color. The initial value
   * is (0, 0, 0, 0).
   * 
   * <h4>Notes</h4>
   * 
   * <p>If an error is generated, no change is made to the contents of
   * <code>params</code>.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> or <code>pname</code> is not one of the
   * accepted defined values.
   * 
   * @param env Specifies a texture environment, which must be
   * <code>GL_TEXTURE_ENV</code>.
   * @param pname Specifies the symbolic name of a texture environment
   * parameter.  <code>pname</code> can be one of the following:
   * <code>GL_TEXTURE_ENV_MODE</code>,
   * <code>GL_TEXTURE_ENV_COLOR</code>,
   * <code>GL_TEXTURE_WRAP_S</code>, <code>GL_TEXTURE_WRAP_T</code>,
   * or <code>GL_GENERATE_MIPMAP</code>.
   * @param params returns the requested data.
   * @param offset the starting offset within the
   * <code>params</code> array.
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexEnvfv(int env, int pname, float[] params, int offset);

  /**  
   * Floating-point <code>Buffer</code> version of
   * <code>glGetTexEnv</code>.
   *
   * @see #glGetTexEnvfv(int env, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexEnvfv(int env, int pname, FloatBuffer params);

  /**  
   * Integer array version of <code>glGetTexEnv</code>.
   *
   * @see #glGetTexEnvfv(int env, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexEnviv(int env, int pname, int[] params, int offset);

  /**  
   * Integer <code>Buffer</code> version of
   * <code>glGetTExEnv</code>.
   *
   * @see #glGetTexEnvfv(int env, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexEnviv(int env, int pname, IntBuffer params);

  /**  
   * Fixed-point version of <code>glGetTexEnv</code>.
   *
   * @see #glGetTexEnvfv(int env, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexEnvxv(int env, int pname, int[] params, int offset);

  /**  
   * Fixed-point <code>Buffer</code> version of
   * <code>glGetTexEnv</code>.
   * 
   * @see #glGetTexEnvfv(int env, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexEnvxv(int env, int pname, IntBuffer params);

  /**  
   * Return texture parameter values.
   * 
   * <p><code>glGetTexParameter</code> returns in <code>params</code>
   * the value or values of the texture parameter specified as
   * <code>pname</code>. <code>target</code> defines the target
   * texture, which must be <code>GL_TEXTURE_2D</code> which specifies
   * two-dimensional texturing. <code>pname</code> accepts the same
   * symbols as <code>glTexParameter</code>, with the same
   * interpretations:
   * 
   * <ul>
   * 
   * <li><code>GL_TEXTURE_MIN_FILTER</code></li>
   * 
   * <p> Returns the texture minifying function. Which can one of the
   * following: <code>GL_NEAREST</code>, <code>GL_LINEAR</code>,
   * <code>GL_NEAREST_MIPMAP_NEAREST</code>,
   * <code>GL_LINEAR_MIPMAP_NEAREST</code>,
   * <code>GL_NEAREST_MIPMAP_LINEAR</code>, or
   * <code>GL_LINEAR_MIPMAP_LINEAR</code>. The initial value is
   * <code>GL_NEAREST_MIPMAP_LINEAR</code>.
   * 
   * <li><code>GL_TEXTURE_MAG_FILTER</code></li>
   * 
   * <p> Returns the texture magnification function. Which can be
   * either <code>GL_NEAREST</code> or <code>GL_LINEAR</code>. The
   * initial value is <code>GL_LINEAR</code>.
   * 
   * <li><code>GL_TEXTURE_WRAP_S</code></li>
   * 
   * <p> Returns the wrap parameter for texture coordinate s. Which
   * can be either: <code>GL_CLAMP</code>,
   * <code>GL_CLAMP_TO_EDGE</code>, or <code>GL_REPEAT</code>. The
   * initial value is <code>GL_REPEAT</code>.
   * 
   * <li><code>GL_TEXTURE_WRAP_T</code></li>
   * 
   * <p> Returns the wrap parameter for texture coordinate t. Which
   * can be either: <code>GL_CLAMP</code>,
   * <code>GL_CLAMP_TO_EDGE</code>, or <code>GL_REPEAT</code>. The
   * initial value is <code>GL_REPEAT</code>.
   * 
   * <li><code>GL_GENERATE_MIPMAP</code></li>
   * 
   * <p> Returns the automatic mipmap generation parameter. The
   * initial value is <code>GL_FALSE</code>.
   * 
   * </ul>
   * 
   * <h4>Notes</h4>
   * 
   * <p>If an error is generated, no change is made to the contents of params.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> or <code>pname</code> is not one of the
   * accepted defined values.
   * 
   * @param target Specifies the target texture, which must be
   * <code>GL_TEXTURE_2D</code>.
   * @param pname Specifies the symbolic name of a texture
   * parameter. <code>pname</code> can be one of the following:
   * <code>GL_TEXTURE_MIN_FILTER</code>,
   * <code>GL_TEXTURE_MAG_FILTER</code>,
   * <code>GL_TEXTURE_WRAP_S</code>, <code>GL_TEXTURE_WRAP_T</code>,
   * or <code>GL_GENERATE_MIPMAP</code>.
   * @param params returns the texture parameters.
   * @param offset the starting offset within the
   * <code>params</code> array.
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexParameterfv(int target, int pname, float[] params, int offset);

  /**  
   * Floating-point <code>Buffer</code> version of
   * <code>glGetTexParameter</code>.
   * 
   * @see #glGetTexParameterfv(int target, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexParameterfv(int target, int pname, FloatBuffer params);

  /**
   * Integer array version of
   * <code>glGetTexParameter</code>.
   * 
   * @see #glGetTexParameterfv(int target, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexParameteriv(int target, int pname, int[] params, int offset);

  /**  
   * Integer <code>Buffer</code> version of
   * <code>glGetTexParameter</code>.
   * 
   * @see #glGetTexParameterfv(int target, int pname, float[] params, int offset)
   * 
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexParameteriv(int target, int pname, IntBuffer params);

  /**  
   * Fixed-point array version of
   * <code>glGetTexParameter</code>.
   * 
   * @see #glGetTexParameterfv(int target, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexParameterxv(int target, int pname, int[] params, int offset);

  /**  
   * Fixed-point <code>Buffer</code> version of
   * <code>glGetTexParameter</code>.
   * 
   * @see #glGetTexParameterfv(int target, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexParameterxv(int target, int pname, IntBuffer params);

  /**  
   * Determine if a name corresponds to a buffer object.
   * 
   * <p><code>glIsBuffer</code> returns <code>true</code> if
   * <code>buffer</code> is currently the name of a buffer object. If
   * buffer is zero, or is a non-zero value that is not currently the
   * name of a buffer object, <code>glIsBuffer</code> returns
   * <code>false</code>.
   * 
   * @param buffer Specifies a value that may be the name of a buffer object.
   *
   * @return <code>true</code> if <code>buffer</code> is the name of a
   * buffer object.
   */
  boolean glIsBuffer(int buffer);

  /**  
   * Test whether a capability is enabled.
   * 
   * <p><code>glIsEnabled</code> returns <code>true</code> if
   * <code>cap</code> is an enabled capability and returns
   * <code>false</code> otherwise.
   * 
   * <p>The following capabilities are accepted for cap:
   * 
   * <pre>
   * Constant                        See function:
   *                 
   * GL_ALPHA_TEST                   glAlphaFunc
   * GL_ARRAY_BUFFER_BINDING         glBindBuffer
   * GL_BLEND                        glBlendFunc, glLogicOp
   * GL_CLIP_PLANE                   glClipPlane
   * GL_COLOR_ARRAY                  glColorPointer
   * GL_COLOR_LOGIC_OP               glLogicOp
   * GL_COLOR_MATERIAL               glColorMaterial
   * GL_CULL_FACE                    glCullFace
   * GL_DEPTH_TEST                   glDepthFunc, glDepthRange
   * GL_DITHER                       glEnable
   * GL_FOG                          glFog
   * GL_LIGHT                        glLight, glLightModel
   * GL_LIGHTING                     glLight, glLightModel, glMaterial
   * GL_LINE_SMOOTH                  glLineWidth
   * GL_MATRIX_PALETTE_OES           glMatrixMode
   * GL_MATRIX_INDEX_ARRAY_OES       glEnableClientState
   * GL_MULTISAMPLE                  glEnable
   * GL_NORMAL_ARRAY                 glNormalPointer
   * GL_NORMALIZE                    glNormal
   * GL_POINT_SIZE_ARRAY_OES         glEnableClientState
   * GL_POINT_SMOOTH                 glPointSize
   * GL_POINT_SPRITE_OES             glEnable, glTexEnv
   * GL_POLYGON_OFFSET_FILL          glPolygonOffset
   * GL_RESCALE_NORMAL               glEnable
   * GL_SAMPLE_ALPHA_TO_COVERAGE     glEnable
   * GL_SAMPLE_ALPHA_TO_ONE          glEnable
   * GL_SAMPLE_COVERAGE              glEnable
   * GL_SCISSOR_TEST                 glScissor
   * GL_STENCIL_TEST                 glStencilFunc, glStencilOp
   * GL_TEXTURE_2D                   glTexImage2D
   * GL_TEXTURE_COORD_ARRAY          glTexCoordPointer
   * GL_WEIGHT_ARRAY_OES             glEnableClientState
   * GL_VERTEX_ARRAY                 glVertexPointer
   * </pre>
   * 
   * <h4>Notes</h4>
   * 
   * <p>If an error is generated, <code>glIsEnabled</code> returns 0.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>cap</code>
   * is not an accepted value.
   * 
   * @param cap Specifies a symbolic constant indicating a GL capability.
   *
   * @return <code>true</code> if the given capability is enabled.
   */
  boolean glIsEnabled(int cap);

  /**  
   * Determine if a name corresponds to a texture.
   * 
   * <p><code>glIsTexture</code> returns <code>true</code> if
   * <code>texture</code> is currently the name of a texture. If
   * <code>texture</code> is zero, or is a non-zero value that is not
   * currently the name of a texture, or if an error occurs,
   * glIsTexture returns <code>false</code>.
   * 
   * @param texture Specifies a value that may be the name of a texture.
   *
   * @return <code>true</code> if <code>texture</code> is the name of a
   * texture object.
   */
  boolean glIsTexture(int texture);
  
  // Points

  /**
   * Specify parameters for point rasterization.
   * 
   * <p><code>glPointParameter</code> assigns values to point parameters.
   * 
   * <p><code>glPointParameter</code> takes two
   * arguments. <code>pname</code> specifies which of several
   * parameters will be modified. <code>param</code> specifies what
   * value will be assigned to the specified parameter.
   * 
   * <p>The parameters that can be specified using
   * <code>glPointParameter</code>, and their interpretations are as
   * follows:
   * 
   * <ul>
   * 
   * <li><code>GL_POINT_SIZE_MIN</code></li>
   * 
   * <p> <code>param</code> specifies the lower bound to which the
   * derived point size is clamped.
   * 
   * <li><code>GL_POINT_SIZE_MAX</code></li>
   * 
   * <p> <code>param</code> specifies the upper bound to which the
   * derived point size is clamped.
   * 
   * <li><code>GL_POINT_FADE_THRESHOLD_SIZE</code></li>
   * 
   * <p> <code>param</code> specifies to the point fade threshold.
   * 
   * </ul>
   * 
   * <h4>Notes</h4>
   * 
   * <p>If the point size lower bound is greater than the upper bound, then
   * the point size after clamping is undefined.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>pname</code> is not an accepted value.
   * <p><code>GL_INVALID_VALUE</code> is generated if assigned values
   * for <p><code>GL_POINT_SIZE_MIN</code>,
   * <code>GL_POINT_SIZE_MAX</code>, or
   * <code>GL_POINT_FADE_THRESHOLD_SIZE</code> are less then zero.
   * 
   * @param pname Specifies the single-valued parameter to be
   * updated. Can be either <code>GL_POINT_SIZE_MIN</code>,
   * <code>GL_POINT_SIZE_MAX</code>, or
   * <code>GL_POINT_FADE_THRESHOLD_SIZE</code>.
   * @param param Specifies the value that the parameter will be set
   * to.
   */
  void glPointParameterf(int pname, float param);

  /**
   * Fixed-point version of <code>glPointParameter</code>.
   *
   * @see #glPointParameterf(int pname, float param)
   */
  void glPointParameterx(int pname, int param);

  /**  
   * Specify parameters for point rasterization (array
   * version).
   * 
   * <p><code>glPointParameter</code> assigns values to point parameters.
   * 
   * <p><code>glPointParameter</code> takes two
   * arguments. <code>pname</code> specifies which of several
   * parameters will be modified. <code>params</code> specifies what
   * values will be assigned to the specified parameter.
   * 
   * <p>The parameters that can be specified using
   * <code>glPointParameter</code>, and their interpretations are as
   * follows:
   * 
   * <ul>
   * 
   * <li><code>GL_POINT_SIZE_MIN</code></li>
   * 
   * <p> <code>params</code> contains the lower bound to which the
   * derived point size is clamped.
   * 
   * <li><code>GL_POINT_SIZE_MAX</code></li>
   * 
   * <p> <code>params</code> contains the upper bound to which the
   * derived point size is clamped.
   * 
   * <li><code>GL_POINT_FADE_THRESHOLD_SIZE</code></li>
   * 
   * <p> <code>params</code> contains the point fade
   * threshold.
   * 
   * <li><code>GL_POINT_DISTANCE_ATTENUATION</code></li>
   * 
   * <p> <code>params</code> contains the distance attenuation
   * function coefficients a, b, and c.
   * 
   * </ul>
   * 
   * <h4>Notes</h4>
   * 
   * <p>If the point size lower bound is greater than the upper bound,
   * then the point size after clamping is undefined.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>pname</code> is not an accepted value.
   * <p><code>GL_INVALID_VALUE</code> is generated if assigned values
   * for <p><code>GL_POINT_SIZE_MIN</code>,
   * <code>GL_POINT_SIZE_MAX</code>, or
   * <code>GL_POINT_FADE_THRESHOLD_SIZE</code> are less then zero.
   * 
   * @param pname Specifies the parameter to be updated. Can be either
   * <code>GL_POINT_SIZE_MIN</code>, <code>GL_POINT_SIZE_MAX</code>,
   * <code>GL_POINT_FADE_THRESHOLD_SIZE</code>, or
   * <code>GL_POINT_DISTANCE_ATTENUATION</code>.
   * @param params Specifies an array of values that
   * <code>pname</code> will be set to.
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glPointParameterfv(int pname, float[] params, int offset);

  /**  
   * Floating-point <code>Buffer</code> version of
   * <code>glPointParameter</code>.
   * 
   * @see #glPointParameterfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glPointParameterfv(int pname, FloatBuffer params);

  /**  
   * Fixed-point array version of
   * <code>glPointParameter</code>.
   *
   * @see #glPointParameterfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glPointParameterxv(int pname, int[] params, int offset);

  /**  
   * Fixed-point <code>Buffer</code> version of
   * <code>glPointParameterfv</code>.
   *
   * @see #glPointParameterfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glPointParameterxv(int pname, IntBuffer params);
  
  // Color

  /**  
   * Byte version of <code>glColor</code>. The bytes are
   * interpreted as unsigned values by bitwise ANDing them with the
   * value <code>0xff</code>.
   *
   * @see #glColor4f(float red, float green, float blue, float alpha)
   */
  void glColor4ub(byte red, byte green, byte blue, byte alpha);

  // Textures

  /**
   * Integer version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvf(int target, int pname, float param)
   */
  void glTexEnvi(int target, int pname, int param);

  /**  
   * Integer array version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvfv(int target, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexEnviv(int target, int pname, int[] params, int offset);

  /**  
   * Integer <code>Buffer</code> version of
   * <code>glTexEnvfv</code>.
   *
   * @see #glTexEnvfv(int target, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexEnviv(int target, int pname, IntBuffer params);

  /**  
   * Floating-point array version of
   * <code>glTexParameter</code>.
   *
   * @see #glTexParameterf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexParameterfv(int target, int pname, float[] params, int offset);

  /**
   * Floating-point <code>Buffer</code> version of
   * <code>glTexParameter</code>.
   *
   * @see #glTexParameterf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexParameterfv(int target, int pname, FloatBuffer params);

  /**  
   * Integer version of <code>glTexParameter</code>.
   * 
   * @see #glTexParameterf(int target, int pname, float param)
   */
  void glTexParameteri(int target, int pname, int param);

  /**  
   * Integer array version of <code>glTexParameter</code>.
   * 
   * @see #glTexParameterf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexParameteriv(int target, int pname, int[] params, int offset);

  /**  
   * Integer <code>Buffer</code> version of
   * <code>glTexParameter</code>.
   * 
   * @see #glTexParameterf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexParameteriv(int target, int pname, IntBuffer params);

  /**  
   * Fixed-point array version of
   * <code>glTexParameter</code>.
   * 
   * @see #glTexParameterf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexParameterxv(int target, int pname, int[] params, int offset);

  /**
   * Fixed-point <code>Buffer</code> version of
   * <code>glTexParameter</code>.
   * 
   * @see #glTexParameterf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexParameterxv(int target, int pname, IntBuffer params);

  /**
   * VBO version of <code>glColorPointer</code>.
   *
   * @see #glBindBuffer
   * @see #glColorPointer(int size, int type, int stride, Buffer pointer)
   *
   * @exception IllegalStateException if VBOs are not enabled. 
   * @exception ArrayIndexOutOfBoundsException if <code>offset</code>
   * is less than 0 or greater than or equal to the length of the
   * currently bound buffer.
   */
  void glColorPointer(int size, int type, int stride, int offset);

  /**
   * VBO version of <code>glNormalPointer</code>.
   *
   * @see #glBindBuffer
   * @see #glNormalPointer(int type, int stride, Buffer pointer)
   *
   * @exception IllegalStateException if VBOs are not enabled.
   * @exception ArrayIndexOutOfBoundsException if <code>offset</code>
   * is less than 0 or greater than or equal to the length of the
   * currently bound buffer.
   */
  void glNormalPointer(int type, int stride, int offset);

  /**
   * VBO version of <code>glTexCoordPointer</code>.
   *
   * @see #glBindBuffer
   * @see #glTexCoordPointer(int size, int type, int stride, Buffer pointer)
   *
   * @exception IllegalStateException if VBOs are not enabled.
   * @exception ArrayIndexOutOfBoundsException if <code>offset</code>
   * is less than 0 or greater than or equal to the length of the
   * currently bound buffer.
   */
  void glTexCoordPointer(int size, int type, int stride, int offset);

  /**
   * VBO version of <code>glVertexPointer</code>.
   *
   * @see #glBindBuffer
   * @see #glVertexPointer(int size, int type, int stride, Buffer pointer)
   *
   * @exception IllegalStateException if VBOs are not enabled.
   * @exception ArrayIndexOutOfBoundsException if <code>offset</code>
   * is less than 0 or greater than or equal to the length of the
   * currently bound buffer.
   */
  void glVertexPointer(int size, int type, int stride, int offset);

  /**
   * VBO version of <code>glDrawElements</code>.
   *
   * @see #glBindBuffer
   * @see #glDrawElements(int mode, int count, int type, Buffer indices)
   * 
   * @exception IllegalStateException if the most recent call to
   * <code>glBindBuffer</code> for the
   * <code>GL_ELEMENT_ARRAY_BUFFER</code> target had a
   * <code>buffer</code> parameter of 0 (i.e., an index buffer is not
   * bound).
   * @exception ArrayIndexOutOfBoundsException if any index in the
   * sequence of indices from <code>offset</code> to <code>offset +
   * count - 1</code> would result in a reference to an entry outside
   * of the currently bound index or data (vertex, color, normal,
   * texture coordinate array, weight, matrix index, or point size)
   * array, where <code>indices</code> refers to the set of values
   * bound to the <code>GL_ELEMENT_ARRAY_BUFFER</code> target.
   */
  void glDrawElements(int mode, int count, int type, int offset);

  /// End 1.1

  /// Begin 1.1 core extensions

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_get</code> extension).
   */
  int GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES = 0x898D;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_get</code> extension).
   */
  int GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES  = 0x898E;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_matrix_get</code> extension).
   */
  int GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES     = 0x898F;

  // OES_point_sprite extension (required profile extension in 1.1)

  /**
   * Constant for use with <code>glTexEnv</code> and
   * <code>glEnable</code> (<code>OES_point_sprite</code>
   * extension).
   */
  int GL_POINT_SPRITE_OES  = 0x8861;

  /**
   * Constant for use with <code>glTexEnv</code>
   * (<code>OES_point_sprite</code> extension).
   */
  int GL_COORD_REPLACE_OES = 0x8862;

  // OES_point_size_array extension (required profile extension in 1.1)

  /**
   * Constant for use with <code>glEnableClientState</code> and
   * <code>glDisableClientState</code>
   * (<code>OES_point_size_array</code> extension).
   */
  int GL_POINT_SIZE_ARRAY_OES                = 0x8B9C;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_point_size_array</code> extension).
   */
  int GL_POINT_SIZE_ARRAY_TYPE_OES           = 0x898A;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_point_size_array</code> extension).
   */
  int GL_POINT_SIZE_ARRAY_STRIDE_OES         = 0x898B;

  /**
   * Constant for use with <code>glGetPointer</code>
   * (<code>OES_point_size_array</code> extension).
   */
  int GL_POINT_SIZE_ARRAY_POINTER_OES        = 0x898C;

  /**
   * Constant for use with <code>glGet</code>
   * (<code>OES_point_size_array</code> extension).
   */
  int GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES = 0x8B9F;
  
  /**
   * (<code>OES_point_size_array</code> extension) Define an array of
   * point sizes.
   * 
   * <p><code>glPointSizePointer</code> specifies the location and
   * data of an array of point sizes to use when rendering
   * points. <code>type</code> is the data type of the
   * coordinates. <code>stride</code> specifies the byte stride from
   * one point size to the next, allowing vertices and attributes to
   * be packed into a single array or stored in separate
   * arrays. (Single-array storage may be more efficient on some
   * implementations.)
   * 
   * <p>The point sizes supplied in the point size arrays will be the
   * sizes used to render both points and point sprites.
   * 
   * <p>Distance-based attenuation works in conjunction with
   * <code>GL_POINT_SIZE_ARRAY_OES</code>. If distance-based
   * attenuation is enabled the point size from the point size array
   * will be attenuated as defined by <code>glPointParameter</code>,
   * to compute the final point size.
   * 
   * <p>When a point size array is specified, <code>type</code>,
   * <code>stride</code>, and <code>pointer</code> are saved as
   * client-side state.
   * 
   * <p>If the point size array is enabled, it is used to control the
   * sizes used to render points and point sprites. To enable and
   * disable the point size array, call
   * <code>glEnableClientState</code> and
   * <code>glDisableClientState</code> with the argument
   * <code>GL_POINT_SIZE_ARRAY_OES</code>. The point size array is
   * initially disabled.
   * 
   * <h4>Notes</h4>
   * 
   * <p>If point size array is enabled, the point size defined by
   * <code>glPointSize</code> is ignored.
   * 
   * <p><code>glPointSizePointer</code> is typically implemented on
   * the client side.
   *
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>type</code>
   * is is not an accepted value.
   * 
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>stride</code> is negative.
   *
   * @param type Specifies the data type of each point size in the
   * array. Symbolic constants <code>GL_FIXED</code> and
   * <code>GL_FLOAT</code> are accepted. The initial value is
   * <code>GL_FLOAT</code>.
   * @param stride Specifies the byte offset between consecutive point
   * sizes. If <code>stride</code> is 0, the point sizes are
   * understood to be tightly packed in the array. The initial value
   * is 0.
   * @param pointer Specifies a <code>Buffer</code> containing the
   * point sizes starting at the first vertex in the array. The
   * initial value is 0.
   *
   * @exception IllegalArgumentException if <code>pointer</code> is
   * <code>null</code>.
   * @exception IllegalStateException if VBOs are enabled.
   */
  void glPointSizePointerOES(int type, int stride, Buffer pointer);

  /**
   * (<code>OES_point_size_array</code> extension) VBO version of
   * <code>glPointSizePointerOES</code>.
   *
   * @see #glPointSizePointerOES(int type, int stride, Buffer pointer)
   *
   * @exception IllegalStateException if VBOs are not enabled.
   */
  void glPointSizePointerOES(int type, int stride, int offset);

  // End 1.1 core extensions

}

