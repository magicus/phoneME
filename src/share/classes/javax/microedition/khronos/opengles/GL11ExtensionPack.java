/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 * The <code>GL11ExtensionPack</code> interface contains the Java(TM)
 * programming language bindings for the OpenGL ES 1.1 Extension Pack.
 * The runtime OpenGL ES engine may or may not implement any
 * particular extensions defined in the extension pack.  Functions
 * that require a particular extension will throw an
 * <code>UnsupportedOperationException</code> if the extension is not
 * available at runtime.
 *
 * <p> The OpenGL ES 1.1 Extension Pack consists of the following extensions:
 *
 * <ul>
 * <li>OES_texture_env_crossbar</li>
 * <li>OES_texture_mirrored_repeat</li>
 * <li>OES_texture_cube_map</li>
 * <li>OES_blend_subtract</li>
 * <li>OES_blend_func_separate</li>
 * <li>OES_blend_equation_separate</li>
 * <li>OES_stencil_wrap</li>
 * <li>OES_extended_matrix_palette</li>
 * <li>OES_framebuffer_object</li>
 * </ul>
 *
 * <p> The specification for the OpenGL ES 1.1 Extension Pack may be
 * found at <a
 * href="http://www.khronos.org/cgi-bin/fetch/fetch.cgi?opengles_spec_1_1_extension_pack">http://www.khronos.org/cgi-bin/fetch/fetch.cgi?opengles_spec_1_1_extension_pack</a>.
 *
 * <p> The documentation in this class is normative with respect to
 * instance variable names and values, method names and signatures,
 * and exception behavior.  The remaining documentation is placed here
 * for convenience and does not replace the normative documentation
 * found in the OpenGL ES 1.1 Extension Pack specification, OpenGL ES
 * specification, relevant extension specifications, and the OpenGL
 * specification versions referenced by any of the preceding
 * specifications.
 */
public interface GL11ExtensionPack extends GL {

//   /**
//    * Flag indicating the presence of the "texture env crossbar"
//    * extension.  <code>OES_texture_env_crossbar</code> is an optional
//    * profile extension in OpenGL ES 1.1, and is part of the OpenGL ES
//    * 1.1 Extension Pack.
//    *
//    * <p> The value of this flag does not imply the presence of the
//    * extension in the runtime enviroment.
//    */
//   int GL_OES_texture_env_crossbar                         = 1;

  /**
   * (<code>OES_texture_env_crossbar</code> extension)
   * Set texture environment parameters.
   *
   * <p> The <code>OES_texture_env_crossbar</code> extension adds the
   * capability to use the texture color from other texture units as
   * sources to the <code>COMBINE</code> texture function. OpenGL ES
   * 1.1 defined texture combine functions which could use the color
   * from the current texture unit as a source. This extension adds
   * the ability to use the color from any texture unit as a source.
   *
   * <p> The tables that define arguments for <code>COMBINE_RGB</code>
   * and <code>COMBINE_ALPHA</code> functions are extended to include
   * <code>TEXTURE</code><i>n</i>:
   *
   * <pre>
   * SRCn_RGB           OPERANDn_RGB              Argument
   *
   * TEXTURE            SRC_COLOR                 Cs
   *                    ONE_MINUS_SRC_COLOR       1 - Cs
   *                    SRC_ALPHA                 As
   *                    ONE_MINUS_SRC_ALPHA       1 - As
   *
   * TEXTUREn           SRC_COLOR                 Cs^n
   *                    ONE_MINUS_SRC_COLOR       1 - Cs^n
   *                    SRC_ALPHA                 As^n
   *                    ONE_MINUS_SRC_ALPHA       1 - As^n
   *
   * CONSTANT           SRC_COLOR                 Cc
   *                    ONE_MINUS_SRC_COLOR       1 - Cc
   *                    SRC_ALPHA                 Ac
   *                    ONE_MINUS_SRC_ALPHA       1 - Ac
   *
   * PRIMARY_COLOR      SRC_COLOR                 Cf
   *                    ONE_MINUS_SRC_COLOR       1 - Cf
   *                    SRC_ALPHA                 Af
   *                    ONE_MINUS_SRC_ALPHA       1 - Af
   *
   * PREVIOUS           SRC_COLOR                 Cp
   *                    ONE_MINUS_SRC_COLOR       1 - Cp
   *                    SRC_ALPHA                 Ap
   *                    ONE_MINUS_SRC_ALPHA       1 - Ap
   * </pre>
   *
   * @param param additionally accept <code>TEXTURE</code><i>n</i>,
   * where <i>n</i> is a number between 0 and 31, inclusive.
   *
   * @see GL11#glTexEnvf(int target, int pname, float param)
   */
  void glTexEnvf(int target, int pname, float param);

  /**
   * (<code>OES_texture_env_crossbar</code> extension)
   * Floating-point array version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexEnvfv(int target, int pname, float[] params, int offset);

  /**
   * (<code>OES_texture_env_crossbar</code> extension)
   * Floating-point <code>Buffer</code> version of
   * <code>glTexEnv</code>.
   *
   * @see #glTexEnvf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexEnvfv(int target, int pname, FloatBuffer params);

  /**
   * (<code>OES_texture_env_crossbar</code> extension)
   * Integer version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvf(int target, int pname, float param)
   */
  void glTexEnvx(int target, int pname, int param);

  /**
   * (<code>OES_texture_env_crossbar</code> extension)
   * Fixed-point array version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexEnvxv(int target, int pname, int[] params, int offset);

  /**
   * (<code>OES_texture_env_crossbar</code> extension)
   * Fixed-point <code>Buffer</code> version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvf(int target, int pname, float param)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexEnvxv(int target, int pname, IntBuffer params);    

//   /**
//    * Flag indicating the presence of the "texture mirrored repeat"
//    * extension.  <code>OES_texture_mirrored_repeat</code> is an optional
//    * profile extension in OpenGL ES 1.1, and is part of the OpenGL ES
//    * 1.1 Extension Pack.
//    *
//    * <p> The value of this flag does not imply the presence of the
//    * extension in the runtime enviroment.
//    */
//   int GL_OES_texture_mirrored_repeat                      = 1;

  /**
   * Constant for use with <code>glTexParameter</code> and
   * <code>glGetTexParameter</code>
   * (<code>OES_texture_mirrored_repeat</code> extension).
   */
  int GL_MIRRORED_REPEAT                                  = 0x8370;

  /**
   * (<code>OES_texture_mirrored_repeat</code> extension)
   * Set texture parameters.
   *
   * <p> An additional option is accepted for
   * <code>GL_TEXTURE_WRAP_S</code> and
   * <code>GL_TEXTURE_WRAP_T</code></li> parameters.
   * <code>GL_MIRRORED_REPEAT</code> effectively uses a texture map
   * twice as large as the original image im which the additional
   * half, for each coordinate, of the new image is a mirror image of
   * the original image.
   *
   * @see GL11#glTexParameterf(int target, int pname, float param)
   */
  void glTexParameterf(int target, int pname, float param);

//   /**
//    * Flag indicating the presence of the "texture cube map" extension.
//    * <code>OES_texture_cube_map</code> is an optional profile
//    * extension in OpenGL ES 1.1, and is part of the OpenGL ES 1.1
//    * Extension Pack.
//    *
//    * <p> The value of this flag does not imply the presence of the
//    * extension in the runtime enviroment.
//    */
//   int GL_OES_texture_cube_map                             = 1;

  /**
   * Constant for use with <code>glTexGen</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_NORMAL_MAP                                       = 0x8511;

  /**
   * Constant for use with <code>glTexGen</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_REFLECTION_MAP                                   = 0x8512;

  /**
   * Constant for use with <code>glBindTexture</code>,
   * <code>glTexParameter</code>, <code>glEnable</code>,
   * <code>glDisable</code>, and <code>glIsEnabled</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_CUBE_MAP                                 = 0x8513;

  /**
   * Constant for use with <code>glGetInteger</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_BINDING_CUBE_MAP                         = 0x8514;

  // Need revisit - value
  /**
   * Constant for use with <code>glTexGen</code> for the
   * <code>OES_texture_cube_map</code> extension.
   */
  int GL_STR                                              = -1;

  /**
   * Constant for use with <code>glTexGen</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_GEN_MODE                                 = 0x2500;

  /**
   * Constant for use with <code>glEnable</code> and
   * <code>glDisable</code> (<code>OES_texture_cube_map</code>
   * extension).
   */
  int GL_TEXTURE_GEN_STR                                  = 0x8D60;
  
  /**
   * Constant for use with <code>glTexImage2D</code> and
   * <code>glCompressedTexImage2D</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_CUBE_MAP_POSITIVE_X                      = 0x207D;

  /**
   * Constant for use with <code>glTexImage2D</code> and
   * <code>glCompressedTexImage2D</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_CUBE_MAP_NEGATIVE_X                      = 0x207E;

  /**
   * Constant for use with <code>glTexImage2D</code> and
   * <code>glCompressedTexImage2D</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_CUBE_MAP_POSITIVE_Y                      = 0x207F;

  /**
   * Constant for use with <code>glTexImage2D</code> and
   * <code>glCompressedTexImage2D</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_CUBE_MAP_NEGATIVE_Y                      = 0x2080;

  /**
   * Constant for use with <code>glTexImage2D</code> and
   * <code>glCompressedTexImage2D</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_CUBE_MAP_POSITIVE_Z                      = 0x2081;

  /**
   * Constant for use with <code>glTexImage2D</code> and
   * <code>glCompressedTexImage2D</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_TEXTURE_CUBE_MAP_NEGATIVE_Z                      = 0x2082;

  /**
   * Constant for use with <code>glGetIntegerv</code>
   * (<code>OES_texture_cube_map</code> extension).
   */
  int GL_MAX_CUBE_MAP_TEXTURE_SIZE                        = 0x851C;

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Bind a named texture to a texturing target.
   *
   * <p> The <code>OES_texture_cube_map</code> extension allows the
   * value <code>GL_TEXTURE_CUBE_MAP</code> to be passed to the
   * <code>target</code> parameter.
   *
   * @param target additionally accepts
   * <code>GL_TEXTURE_CUBE_MAP</code>.
   *
   * @see GL11#glBindTexture(int target, int texture)
   */
  void glBindTexture(int target, int texture);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Specify a two-dimensional compressed texture image.
   *
   * <p> The <code>OES_texture_cube_map</code> extension allows the
   * values
   * <code>GL_TEXTURE_CUBE_MAP_{POSITIVE,NEGATIVE}_{X,Y,Z}</code> to
   * be passed to the <code>target</code> parameter.
   *
   * @param target additionally accepts the constants
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Y</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Z</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Y</code>, and
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Z</code>.
   *
   * @see GL11#glCompressedTexImage2D
   *
   * @exception IllegalArgumentException if <code>data</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>data.remaining()</code> is less than
   * <code>imageSize</code>.
   */
  void glCompressedTexImage2D(int target, int level,
                              int internalformat,
                              int width, int height,
                              int border, int imageSize,
                              Buffer data);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Specify a two-dimensional texture image with pixels from the
   * color buffer.
   *
   * <p> The <code>OES_texture_cube_map</code> extension allows the
   * values
   * <code>GL_TEXTURE_CUBE_MAP_{POSITIVE,NEGATIVE}_{X,Y,Z}</code> to
   * be passed to the <code>target</code> parameter.
   *
   * @param target additionally accepts the constants
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Y</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Z</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Y</code>, and
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Z</code>.
   *
   * @see GL11#glCompressedTexImage2D
   */
  void glCopyTexImage2D(int target, int level,
                        int internalformat,
                        int x, int y,
                        int width, int height,
                        int border);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Enable server-side GL capabilities.
   *
   * <p>Cube-map texturing is enabled if <code>cap</code> assumes the
   * value <code>GL_TEXTURE_CUBE_MAP</code>.
   *
   * <p>If enabled, cube-map texturing is performed for the active
   * texture unit. See <code>GL.glActiveTexture</code>,
   * <code>GL.glTexImage2D</code>, <code>glCompressedTexImage2D</code>,
   * and <code>glCopyTexImage2D</code>.
   *
   * <p>Texture coordinates with be generated if <code>cap</code> is
   * equal to <code>GL_TEXTURE_GEN_STR</code>.
   *
   * @see GL11#glEnable(int cap)
   */
  void glEnable(int cap);

  /**
   * (1.1 + <code>OES_texture_cube_map</code>,
   * <code>OES_blend_subtract</code>,
   * <code>OES_blend_func_separate</code>, and
   * <code>OES_blend_equation_separate</code> extensions) Return the
   * value or values of a selected parameter.
   *
   * <p> The extensions in the GL 1.1 Extension Pack add the following
   * constants as possible values for <code>pname</code>:
   *
   * <ul>
   *
   * <li><code>GL_BLEND_DST_ALPHA</code> (1.1 +
   * <code>OES_blend_func_separate</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the destination blend function set by
   * <code>glBlendFunc</code>, or the destination alpha blend function
   * set by <code>glBlendFuncSeparate</code>. See
   * <code>glBlendFunc</code> and <code>glBlendFuncSeparate</code>.
   *
   * <li><code>GL_BLEND_DST_RGB</code> (1.1 +
   * <code>OES_blend_func_separate</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the destination blend function set by
   * <code>glBlendFunc</code>, or the destination RGB blend function
   * set by <code>glBlendFuncSeparate</code>. See
   * <code>glBlendFunc</code> and <code>glBlendFuncSeparate</code>.
   *
   * <li><code>GL_BLEND_EQUATION</code> (1.1 +
   * <code>OES_blend_subtract</code> and
   * <code>OES_blend_equation_separate</code> extensions)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the blend equation set by
   * <code>glBlendEquation</code>, or the RGB blend equation set by
   * <code>glBlendEquationSeparate</code>. See
   * <code>glBlendEquation</code> and
   * <code>glBlendEquationSeparate</code>.
   *
   * <li><code>GL_BLEND_EQUATION_ALPHA</code> (1.1 +
   * <code>OES_blend_equation_separate</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the blend equation set by
   * <code>glBlendEquation</code>, or the alpha blend equation set by
   * <code>glBlendEquationSeparate</code>. See
   * <code>glBlendEquation</code> and
   * <code>glBlendEquationSeparate</code>.
   *
   * <li><code>GL_BLEND_EQUATION_RGB</code> (1.1 +
   * <code>OES_blend_subtract</code> and
   * <code>OES_blend_equation_separate</code> extensions)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the blend equation set by
   * <code>glBlendEquation</code>, or the RGB blend equation set by
   * <code>glBlendEquationSeparate</code>. See
   * <code>glBlendEquation</code> and
   * <code>glBlendEquationSeparate</code>.
   *
   * <li><code>GL_BLEND_SRC</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the source blend function set by <code>glBlendFunc</code>,
   * or the source RGB blend function set by
   * <code>glBlendFuncSeparate</code>. See <code>glBlendFunc</code> and
   * <code>glBlendFuncSeparate</code>.
   *
   * <li><code>GL_BLEND_SRC_ALPHA</code> (1.1 +
   * <code>OES_blend_func_separate</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the source blend function set by <code>glBlendFunc</code>,
   * or the source alpha blend function set by
   * <code>glBlendFuncSeparate</code>. See <code>glBlendFunc</code> and
   * <code>glBlendFuncSeparate</code>.
   *
   * <li><code>GL_BLEND_SRC_RGB</code> (1.1 +
   * <code>OES_blend_func_separate</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the source blend function set by <code>glBlendFunc</code>,
   * or the source RGB blend function set by
   * <code>glBlendFuncSeparate</code>. See <code>glBlendFunc</code> and
   * <code>glBlendFuncSeparate</code>.
   *
   * <li><code>GL_MAX_CUBE_MAP_TEXTURE_SIZE</code> (1.1 +
   * <code>OES_texture_cube_map</code> extension)</li>
   * 
   * <p><code>params</code> returns one value, the maximum dimension
   * of any face in a cube map texture.  The value must be at least
   * 64. See <code>glTexImage2D</code>,
   * <code>glCompressedTexImage2D</code>, and
   * <code>glCopyTexImage2D</code>.
   *
   * <li><code>GL_MAX_PALETTE_MATRICES_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   *
   * <p><code>params</code> returns the size of the matrix
   * palette. The initial value is 32 if the
   * <code>OES_extended_matrix_palette</code> extension is present.
   *
   * <li><code>GL_MAX_VERTEX_UNITS_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   *
   * <p><code>params</code> returns the number of matrices per
   * vertex. The initial value is 4 if the
   * <code>OES_extended_matrix_palette</code> extension is present.
   *
   * </ul>
   *
   * @see GL11#glGetIntegerv(int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetIntegerv(int pname, int[] params, int offset);

  /**
   * (1.1 + <code>OES_texture_cube_map</code>,
   * <code>OES_blend_subtract</code>,
   * <code>OES_blend_func_separate</code>, and
   * <code>OES_blend_equation_separate</code> extensions)
   * Integer <code>Buffer</code> version of
   * <code>getGetIntegerv</code>.
   *
   * @see #glGetIntegerv(int pname, int[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetIntegerv(int pname, IntBuffer params);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Specify texture coordinate generation function.
   *
   * <p>The <code>OES_texture_cube_map</code> extension provides a new
   * texture generation scheme for cube map textures. Instead of the
   * current texture providing a 2D lookup into a 2D texture image,
   * the texture is a set of six 2D images representing the faces of a
   * cube. The <code>(s,t,r)</code> texture coordinates are treated as
   * a direction vector emanating from the center of a cube. At
   * texture generation time, the interpolated per-fragment
   * <code>(s,t,r)</code> selects one cube face 2D image based on the
   * largest magnitude coordinate (the major axis). A new 2D
   * <code>(s,t)</code> is calculated by dividing the two other
   * coordinates (the minor axes values) by the major axis value. Then
   * the new <code>(s,t)</code> is used to lookup into the selected 2D
   * texture image face of the cube map.
   *
   * <p>Unlike a standard 2D texture that have just one target, a cube
   * map texture has six targets, one for each of its six 2D texture
   * image cube faces. All these targets must be consistent, complete,
   * and have equal width and height.
   *
   * <p>This extension also provides two new texture coordinate
   * generation modes for use in conjunction with cube map
   * texturing. The reflection map mode generates texture coordinates
   * <code>(s,t,r)</code> matching the vertex?s eyespace reflection
   * vector. The reflection map mode is useful for environment mapping
   * without the singularity inherent in sphere mapping. The normal
   * map mode generates texture coordinates <code>(s,t,r)</code>
   * matching the vertex?s transformed eyespace normal. The normal map
   * mode is useful for sophisticated cube map texturing-based diffuse
   * lighting models.
   *
   * <p>The intent of the new texgen functionality is that an
   * application using cube map texturing can use the new texgen modes
   * to automatically generate the reflection or normal vectors used
   * to look up into the cube map texture.
   *
   * <p>The following texgen modes are supported: <code>GL_REFLECTION
   * MAP</code> and <code>GL_NORMAL MAP</code>. The
   * <code>GL_SPHERE_MAP</code>, <code>GL_OBJECT LINEAR</code>, and
   * <code>GL_EYE LINEAR</code> texgen modes are not supported. Texgen
   * supports a new coord value <code>GL_STR</code>.  This allows the
   * application to specify the texgen mode for the appropriate
   * coordinates in a single call. Texgen with coord values of
   * <code>GL_S</code>, <code>GL_T</code>, <code>GL_R</code> and
   * <code>GL_Q</code> are not supported.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>param</code> or <code>pname</code> is not an accepted
   * value, or if <code>pname</code> is
   * <code>GL_TEXTURE_GEN_MODE</code> and <code>params</code> is not
   * <code>GL_REFLECTION_MAP</code> or <code>GL_NORMAL_MAP</code>.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetTexGen</code>, <code>glIsEnabled</code> with
   * argument <code>GL_TEXTURE_GEN_STR</code>.
   *
   * @param coord Specifies the texture coordinate or coordinates for
   * which a generation function is being specified. At present, only
   * <code>GL_STR</code> is accepted.
   * @param pname Specifies a single-valued integer texture coordinate
   * generation parameter. At present, only
   * <code>GL_TEXTURE_GEN_MODE</code> is accepted.
   * @param param Specifies the value that <code>pname</code> will be
   * set to.  If <code>pname</code> is <code>GL_TEXTURE_GEN_MODE</code>, then
   * <code>GL_REFLECTION_MAP</code> and <code>GL_NORMAL_MAP</code> are
   * accepted.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   */
  void glTexGeni(int coord, int pname, int param);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Floating-point version of <code>glTexGen</code>.
   *
   * @see #glTexGeni(int coord, int pname, int param)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   */
  void glTexGenf(int coord, int pname, float param);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Fixed-point version of <code>glTexGen</code>.
   *
   * @see #glTexGeni(int coord, int pname, int param)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   */
  void glTexGenx(int coord, int pname, int param);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Floating-point array version of <code>glTexGen</code>.
   *
   * @see #glTexGeni(int coord, int pname, int param)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexGenfv(int coord, int pname, float[] params, int offset);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Floating-point <code>Buffer</code> version of
   * <code>glTexGen</code>.
   *
   * @see #glTexGeni(int coord, int pname, int param)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexGenfv(int coord, int pname, FloatBuffer params);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Integer array version of <code>glTexGen</code>.
   *
   * @see #glTexGeni(int coord, int pname, int param)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexGeniv(int coord, int pname, int[] params, int offset);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Integer <code>Buffer</code> version of <code>glTexGen</code>.
   *
   * @see #glTexGeni(int coord, int pname, int param)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexGeniv(int coord, int pname, IntBuffer params);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Fixed-point array version of <code>glTexGen</code>.
   *
   * @see #glTexGeni(int coord, int pname, int param)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glTexGenxv(int coord, int pname, int[] params, int offset);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Fixed-point <code>Buffer</code> version of <code>glTexGen</code>.
   *
   * @see #glTexGeni(int coord, int pname, int param)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexGenxv(int coord, int pname, IntBuffer params);

  // GetTexGen

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Get texture coordinate generation parameters.
   *
   * This method queries texture coordinate generation parameters set
   * using <code>glTexGen</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>param</code> or <code>pname</code> is not an accepted
   * value, or if <code>pname</code> is
   * <code>GL_TEXTURE_GEN_MODE</code> and <code>params</code> is not
   * <code>GL_REFLECTION_MAP</code> or <code>GL_NORMAL_MAP</code>.
   *
   * @param coord Specifies the texture coordinate or coordinates for
   * which a generation parameter is being requested. At present, only
   * <code>GL_STR</code> is accepted.
   * @param pname Specifies a single-valued integer texture coordinate
   * generation parameter. At present, only
   * <code>GL_TEXTURE_GEN_MODE</code> is accepted.
   * @param params Returns the value or values of the specified
   * parameter.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexGeniv(int coord, int pname, int[] params, int offset);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Integer <code>Buffer</code> version of <code>glGetTexGen</code>.
   *
   * @see #glGetTexGeniv(int coord, int pname, int[] params, int offset)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexGeniv(int coord, int pname, IntBuffer params);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Floating-point array version of <code>glGetTexGen</code>.
   *
   * @see #glGetTexGeniv(int coord, int pname, int[] params, int offset)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexGenfv(int coord, int pname, float[] params, int offset);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Fixed-point array version of <code>glGetTexGen</code>.
   *
   * @see #glGetTexGeniv(int coord, int pname, int[] params, int offset)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetTexGenxv(int coord, int pname, int[] params, int offset);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Fixed-point <code>Buffer</code> version of <code>glGetTexGen</code>.
   *
   * @see #glGetTexGeniv(int coord, int pname, int[] params, int offset)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexGenxv(int coord, int pname, IntBuffer params);

  /**
   * (1.1 + <code>OES_texture_cube_map</code> extension)
   * Floating-point <code>Buffer</code> version of <code>glGetTexGen</code>.
   *
   * @see #glGetTexGeniv(int coord, int pname, int[] params, int offset)
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_texture_cube_map</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetTexGenfv(int coord, int pname, FloatBuffer params);

//   /**
//    * Flag indicating the presence of the "blend subtract" extension.
//    * <code>OES_blend_subtract</code> is an optional profile extension
//    * in OpenGL ES 1.1, and is part of the OpenGL ES 1.1 Extension
//    * Pack.
//    *
//    * <p> The value of this flag does not imply the presence of the
//    * extension in the runtime enviroment.
//    */
//   int GL_OES_blend_subtract                               = 1;

  /**
   * Constant for use with <code>glBlendEquation</code>
   * (<code>OES_blend_subtract</code> extension).
   */
  int GL_FUNC_ADD                                         = 0x8006;

  /**
   * Constant for use with <code>glBlendEquation</code>
   * (<code>OES_blend_subtract</code> extension).
   */
  int GL_FUNC_SUBTRACT                                    = 0x800A;

  /**
   * Constant for use with <code>glBlendEquation</code>
   * (<code>OES_blend_subtract</code> extension).
   */
  int GL_FUNC_REVERSE_SUBTRACT                            = 0x800B;

  
  
  /**
   * (1.1 + <code>OES_blend_subtract</code> extension) Specify the
   * blending equation.
   *
   * <p>Blending is controlled by <code>glBlendEquation</code>.
   *
   * <p><code>glBlendEquation</code> <code>mode</code>
   * <code>GL_FUNC_ADD</code> defines the blend equation as
   *
   * <pre>C = Cs * S + Cd * D,</pre>
   *
   * where <code>Cs</code> and <code>Cd</code> are the source and
   * destination colors, <code>S</code> and <code>D</code> are the
   * quadruplets of weighting factors determined by the blend
   * functions <code>glBlendFunc</code> and
   * <code>glBlendFuncSeparate</code>, and C is the new color
   * resulting from blending.
   *
   * <p>If <code>mode</code> is <code>GL_FUNC_SUBTRACT</code>, the
   * blending equation is defined as
   *
   * <pre>C = Cs * S - Cd * D.</pre>
   *
   * <p>If <code>mode</code> is <code>GL_FUNC_REVERSE_SUBTRACT</code>,
   * the blend equation is defined as
   *
   * <pre>C = Cd * D - Cs * S.</pre>
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>mode</code>
   * is not an accepted value.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_BLEND_EQUATION</code> returns the blend equation.
   *
   * @param mode Specifies the blend
   * equation. <code>GL_FUNC_ADD</code>,
   * <code>GL_FUNC_SUBTRACT</code>, and
   * <code>GL_FUNC_REVERSE_SUBTRACT</code> are accepted.
   *
   * @see GL11#glBlendFunc
   * @see #glBlendFuncSeparate
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_blend_subtract</code> extension.
   */
  void glBlendEquation(int mode);

//   /**
//    * Flag indicating the presence of the "blend func separate"
//    * extension.  <code>OES_blend_func_separate</code> is an optional
//    * profile extension in OpenGL ES 1.1, and is part of the OpenGL ES
//    * 1.1 Extension Pack.
//    *
//    * <p> The value of this flag does not imply the presence
//    * of the extension in the runtime enviroment.
//    */
//   int GL_OES_blend_func_separate                          = 1;

  /**
   * Constant for use with <code>glBlendFuncSeparate</code> 
   * (<code>OES_blend_func_separate</code> extension).
   */
  int GL_BLEND_DST_RGB                                    = 0x80C8;

  /**
   * Constant for use with <code>glBlendFuncSeparate</code> 
   * (<code>OES_blend_func_separate</code> extension).
   */
  int GL_BLEND_SRC_RGB                                    = 0x80C9;

  /**
   * Constant for use with <code>glBlendFuncSeparate</code> 
   * (<code>OES_blend_func_separate</code> extension).
   */
  int GL_BLEND_DST_ALPHA                                  = 0x80CA;

  /**
   * Constant for use with <code>glBlendFuncSeparate</code> 
   * (<code>OES_blend_func_separate</code> extension).
   */
  int GL_BLEND_SRC_ALPHA                                  = 0x80CB;

  /**
   * (1.1 + <code>GL_OES_blend_func_separate</code> extension)
   * Apply different blend factors to RGB and alpha.
   *
   * <p><code>glBlendFuncSeparate</code> allows different blend factors to be
   * applied to RGB and alpha. The blend factors are those defined for
   * <code>glBlendFunc</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>srcRGB</code>, <code>dstRGB</code>, <code>srcAlpha</code>,
   * or <code>dstAlpha</code> is not an accepted value.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_BLEND_SRC_RGB</code> or <code>GL_BLEND_SRC</code>
   * returns the source RGB blend function.
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_BLEND_DST_RGB</code> or <code>GL_BLEND_DST</code>
   * returns the destination RGB blend function.
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_BLEND_SRC_ALPHA</code> returns the source alpha blend
   * function.
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_BLEND_DST_ALPHA</code> returns the destination alpha
   * blend function.
   *
   * @param srcRGB Source RGB blend function. Must be one of the blend
   * functions accepted by <code>glBlendFunc</code>.
   * @param dstRGB Destination RGB blend function. Must be one of the blend
   * functions accepted by <code>glBlendFunc</code>.
   * @param srcAlpha Source alpha blend function. Must be one of the
   * blend functions accepted by <code>glBlendFunc</code>.
   * @param dstAlpha Destination alpha blend function. Must be one of
   * the blend functions accepted by <code>glBlendFunc</code>.
   *
   * @see GL11#glBlendFunc
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_blend_func_separate</code> extension.
   */
  void glBlendFuncSeparate(int srcRGB, int dstRGB,
			   int srcAlpha, int dstAlpha);

//   /**
//    * Flag indicating the presence of the "blend equation separate"
//    * extension.  <code>OES_blend_equation_separate</code> is an
//    * optional profile extension in OpenGL ES 1.1, and is part of the
//    * OpenGL ES 1.1 Extension Pack.
//    *
//    * <p> The value of this flag does not imply the presence
//    * of the extension in the runtime enviroment.
//    */
//   int GL_OES_blend_equation_separate                      = 1;

  /**
   * Constant for use with <code>glBlendEquationSeparate</code> and
   * <code>glGetInteger</code>
   * (<code>OES_blend_equation_separate</code> extension).
   */
  int GL_BLEND_EQUATION                                   = 0x8009;

  /**
   * Constant for use with <code>glBlendEquationSeparate</code> and
   * <code>glGetInteger</code>
   * (<code>OES_blend_equation_separate</code> extension).  Synonym
   * for <code>GL_BLEND_EQUATION</code>.
   */
  int GL_BLEND_EQUATION_RGB                               = 0x8009;

  /**
   * Constant for use with <code>glBlendEquationSeparate</code> and
   * <code>glGetInteger</code>
   * (<code>OES_blend_equation_separate</code> extension).
   */
  int GL_BLEND_EQUATION_ALPHA                             = 0x883D;

  /**
   * (1.1 + <code>OES_blend_equation_separate</code> extension)
   * Provide different blend equations for RGB and alpha.
   *
   * <p><code>glBlendEquationSeparate</code> allows different blend
   * equations to be provideded for RGB and alpha. The blend equations
   * are those defined for <code>glBlendEquation</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>modeRGB</code>, or <code>modeAlpha</code> is not an
   * accepted value.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_BLEND_EQUATION_RGB</code> or
   * <code>GL_BLEND_EQUATION</code> returns the RGB blend equation.
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_BLEND_EQUATION_ALPHA</code> returns the alpha blend
   * equation.
   *
   * @param modeRGB RGB blend equation. Must be one of the blend
   * equations accepted by <code>glBlendEquation</code>.
   * @param modeAlpha Alpha blend equation. Must be one of the blend
   * equations accepted by <code>glBlendEquation</code>.
   *
   * @see #glBlendEquation
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_blend_equation_separate</code> extension.
   */
  void glBlendEquationSeparate(int modeRGB, int modeAlpha);

//   /**
//    * Flag indicating the presence of the "stencil wrap" extension.
//    * <code>OES_stencil_wrap</code> is an optional profile extension in
//    * OpenGL ES 1.1, and is part of the OpenGL ES 1.1 Extension Pack.
//    *
//    * <p> The value of this flag does not imply the presence of the
//    * extension in the runtime enviroment. 
//   */
//   int GL_OES_stencil_wrap                                 = 1;

  /**
   * Constant for use with <code>glStencilOp</code>
   * (<code>OES_stencil_wrap</code> extension).
   */
  int GL_INCR_WRAP                                        = 0x8507;

  /**
   * Constant for use with <code>glStencilOp</code>
   * (<code>OES_stencil_wrap</code> extension).
   */
  int GL_DECR_WRAP                                        = 0x8508;

  /**
   * (<code>OES_stencil_wrap</code> extension)
   * Set stencil test actions.
   *
   * <p> Two additional actions are defined by the
   * <code>OES_stencil_wrap</code> extension:
   *
   * <ul>
   * 
   * <li>(<code>OES_stencil_wrap</code> extension)
   * <code>GL_DECR_WRAP</code></li>
   *
   * <p>Decrements the current stencil buffer value, wrapping around
   * to the maximum representable unsigned value if less than 0.
   *
   * <li>(<code>OES_stencil_wrap</code> extension)
   * <code>GL_INCR</code></li>
   *
   * <p>Increments the current stencil buffer value, wrapping around
   * to 0 if greater than the maximum representable unsigned value.
   *
   * </ul>
   *
   * @see GL11#glStencilOp(int fail, int zfail, int zpass)
   */
  void glStencilOp(int fail, int zfail, int zpass);


//   /**
//    * Flag indicating the presence of the "extended matrix palette"
//    * extension. <code>OES_extended_matrix_palette</code> is an
//    * optional profile extension in OpenGL ES 1.1, and is part of the
//    * OpenGL ES 1.1 Extension Pack.
//    *
//    * <p> The value of this flag does not imply the presence of the
//    * extension in the runtime enviroment. 
//    */
//   int GL_OES_extended_matrix_palette                      = 1;

//   /**
//    * Flag indicating the presence of the "framebuffer object"
//    * extension.  <code>OES_framebuffer_object</code> is an optional
//    * profile extension in OpenGL ES 1.1, and is part of the OpenGL ES
//    * 1.1 Extension Pack.
//    *
//    * <p> The value of this flag does not imply the presence of the
//    * extension in the runtime enviroment.
//    */
//   int GL_OES_framebuffer_object                           = 1;

  /**
   * Constant accepted by the <code>target</code> parameter of
   * <code>glBindFramebufferOES</code>,
   * <code>glCheckFramebufferStatusOES</code>,
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_OES                                  = 0x8D40;

  /**
   * Constant accepted by the <code>target</code> parameter of
   * <code>glBindRenderbufferOES</code>,
   * <code>glRenderbufferStorageOES</code>, and
   * <code>glGetRenderbufferParameterivOES</code>, and returned by
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_OES                                 = 0x8D41;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_WIDTH_OES                           = 0x8D42;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_HEIGHT_OES                          = 0x8D43;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_INTERNAL_FORMAT_OES                 = 0x8D44;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_RED_SIZE_OES                        = 0x8D50;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_GREEN_SIZE_OES                      = 0x8D51;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_BLUE_SIZE_OES                       = 0x8D52;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_ALPHA_SIZE_OES                      = 0x8D53;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_DEPTH_SIZE_OES                      = 0x8D54;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetRenderbufferParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_STENCIL_SIZE_OES                    = 0x8D55;
  
  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES           = 0x8CD0;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES           = 0x8CD1;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES         = 0x8CD2;

  /**
   * Constant accepted by the <code>pname</code> parameter of
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES = 0x8CD3;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT0_OES                            = 0x8CE0;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT1_OES                            = 0x8CE1;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT2_OES                            = 0x8CE2;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT3_OES                            = 0x8CE3;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT4_OES                            = 0x8CE4;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT5_OES                            = 0x8CE5;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT6_OES                            = 0x8CE6;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT7_OES                            = 0x8CE7;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT8_OES                            = 0x8CE8;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT9_OES                            = 0x8CE9;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT10_OES                           = 0x8CEA;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT11_OES                           = 0x8CEB;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT12_OES                           = 0x8CEC;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT13_OES                           = 0x8CED;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT14_OES                           = 0x8CEE;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_COLOR_ATTACHMENT15_OES                           = 0x8CEF;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_DEPTH_ATTACHMENT_OES                             = 0x8D00;

  /**
   * Constant accepted by the <code>attachment</code> parameter of
   * <code>glFramebufferTexture2DOES</code>,
   * <code>glFramebufferRenderbufferOES</code>, and
   * <code>glGetFramebufferAttachmentParameterivOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_STENCIL_ATTACHMENT_OES                           = 0x8D20;

  /**
   * Constant returned by <code>glCheckFramebufferStatusOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_COMPLETE_OES                         = 0x8CD5;

  /**
   * Constant returned by <code>glCheckFramebufferStatusOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES            = 0x8CD6;

  /**
   * Constant returned by <code>glCheckFramebufferStatusOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES    = 0x8CD7;

  /**
   * Constant returned by <code>glCheckFramebufferStatusOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES            = 0x8CD9;

  /**
   * Constant returned by <code>glCheckFramebufferStatusOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_INCOMPLETE_FORMATS_OES               = 0x8CDA;

  /**
   * Constant returned by <code>glCheckFramebufferStatusOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_OES           = 0x8CDB;

  /**
   * Constant returned by <code>glCheckFramebufferStatusOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_OES           = 0x8CDC;

  /**
   * Constant returned by <code>glCheckFramebufferStatusOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_UNSUPPORTED_OES                      = 0x8CDD;

  /**
   * Constant accepted by <code>glGetIntegerv</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_FRAMEBUFFER_BINDING_OES                          = 0x8CA6;

  /**
   * Constant accepted by <code>glGetIntegerv</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RENDERBUFFER_BINDING_OES                         = 0x8CA7;

  /**
   * Constant accepted by <code>glGetIntegerv</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_MAX_COLOR_ATTACHMENTS_OES                        = 0x8CDF;

  /**
   * Constant accepted by <code>glGetIntegerv</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_MAX_RENDERBUFFER_SIZE_OES                        = 0x84E8;

  /**
   * Constant returned by <code>glGetError</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_INVALID_FRAMEBUFFER_OPERATION_OES                = 0x0506;

  /**
   * Constant accepted by the <code>internalformat</code> parameter of
   * <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RGB565_OES                                       = 0x8D62;

  /**
   * Constant accepted by the <code>internalformat</code> parameter of
   * <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RGBA4                                            = 0x8056;

  /**
   * Constant accepted by the <code>internalformat</code> parameter of
   * <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RGB5_A1                                          = 0x8057;

  /**
   * Constant accepted by the <code>internalformat</code> parameter of
   * <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_DEPTH_COMPONENT16                                = 0x81A5;

  /**
   * Constant optionally accepted by the <code>internalformat</code>
   * parameter of <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RGBA8                                            = 0x8058;

  /**
   * Constant optionally accepted by the <code>internalformat</code>
   * parameter of <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_RGB8                                             = 0x8051;

  /**
   * Constant optionally accepted by the <code>internalformat</code>
   * parameter of <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_DEPTH_COMPONENT24                                = 0x81A6;

  /**
   * Constant optionally accepted by the <code>internalformat</code>
   * parameter of <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */
  int GL_DEPTH_COMPONENT32                                = 0x81A7;

  /**
   * Constant optionally accepted by the <code>internalformat</code>
   * parameter of <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */    
  int GL_STENCIL_INDEX1_OES                               = 0x8D46;

  /**
   * Constant optionally accepted by the <code>internalformat</code>
   * parameter of <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */    
  int GL_STENCIL_INDEX4_OES                               = 0x8D47;

  /**
   * Constant optionally accepted by the <code>internalformat</code>
   * parameter of <code>glRenderbufferStorageOES</code>
   * (<code>OES_framebuffer_object</code> extension).
   */    
  int GL_STENCIL_INDEX8_OES                               = 0x8D48;

/*
  int GL_STENCIL_INDEX16_OES                              = 0x8D49;
*/

  /**
   * Constant (<code>OES_framebuffer_object</code> extension).
   */    
  int GL_STENCIL_INDEX                                    = 0x1901;

  /**
   * Constant (<code>OES_framebuffer_object</code> extension).
   */    
  int GL_DEPTH_COMPONENT                                  = 0x1902;

  /**
   * (<code>OES_framebuffer_object</code> extension) Determine whether a
   * token represents a renderbuffer.
   *
   * <p> Returns <code>true</code> if <code>renderbuffer</code> is the
   * name of a renderbuffer object.  If <code>renderbuffer</code> is
   * zero, or if <code>renderbuffer</code> is a non-zero value that is
   * not the name of a renderbuffer object,
   * <code>glIsRenderbufferOES</code> returns <code>false</code>.
   *
   * @param renderbuffer an integer.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  boolean glIsRenderbufferOES(int renderbuffer);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Bind a renderbuffer.
   *

   * <p> A renderbuffer is a data storage object containing a single
   * image of a renderable internal format.  GL provides the methods
   * described below to allocate and delete a renderbuffer's image,
   * and to attach a renderbuffer's image to a framebuffer object.
   *
   * <p> The name space for renderbuffer objects is the unsigned
   * integers, with zero reserved for the GL.  A renderbuffer object
   * is created by binding an unused name to
   * <code>GL_RENDERBUFFER_OES</code>.  The binding is effected by
   * setting <code>target</code> to <code>GL_RENDERBUFFER_OES</code>
   * and <code>renderbuffer</code> set to the unused name.  If
   * <code>renderbuffer</code> is not zero, then the resulting
   * renderbuffer object is a new state vector, initialized with a
   * zero-sized memory buffer, and comprising the state values listed
   * in Table 8.nnn of the <code>EXT_framebuffer_object</code>
   * specification.  Any previous binding to <code>target</code> is
   * broken.
   *
   * <p> <code>glBindRenderbufferOES</code> may also be used to bind an
   * existing renderbuffer object.  If the bind is successful, no
   * change is made to the state of the newly bound renderbuffer
   * object, and any previous binding to <code>target</code> is
   * broken.
   *
   * <p> While a renderbuffer object is bound, GL operations on the target
   * to which it is bound affect the bound renderbuffer object, and
   * queries of the target to which a renderbuffer object is bound
   * return state from the bound object.
   *
   * <p> The name zero is reserved.  A renderbuffer object cannot be
   * created with the name zero.  If <code>renderbuffer</code> is
   * zero, then any previous binding to <code>target</code> is broken
   * and the <code>target</code> binding is restored to the initial
   * state.
   *
   * <p> In the initial state, the reserved name zero is bound to
   * <code>GL_RENDERBUFFER_OES</code>.  There is no renderbuffer
   * object corresponding to the name zero, so client attempts to
   * modify or query renderbuffer state for the target
   * <code>GL_RENDERBUFFER_OES</code> while zero is bound will
   * generate GL errors.
   *
   * <p> Using <code>glGetIntegerv</code>, the current
   * <code>GL_RENDERBUFFER_OES</code> binding can be queried as
   * <code>GL_RENDERBUFFER_BINDING_OES</code>.
   *
   * @param target the value <code>GL_RENDERBUFFER_OES</code>.
   * @param renderbuffer Specifies the name of a renderbuffer.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  void glBindRenderbufferOES(int target, int renderbuffer);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Delete a renderbuffer.
   *
   * <p> Renderbuffer objects are deleted by calling
   * glDeleteRenderbuffersOES where <code>renderbuffers</code>
   * contains <code>n</code> names of renderbuffer objects to be
   * deleted.  After a renderbuffer object is deleted, it has no
   * contents, and its name is again unused.  If a renderbuffer that
   * is currently bound to <code>GL_RENDERBUFFER_OES</code> is
   * deleted, it is as though <code>glBindRenderbufferOES</code> had
   * been executed with the <code>target</code>
   * <code>GL_RENDERBUFFER_OES</code> and <code>name</code> of zero.
   * Additionally, special care must be taken when deleting a
   * renderbuffer if the image of the renderbuffer is attached to a
   * framebuffer object.  Unused names in
   * <code>renderbuffers</code> are silently ignored, as is the value
   * zero.
   *
   * @param n the number of renderbuffers to be deleted.
   * @param renderbuffers an array of <code>n</code> renderbuffer names.
   * @param offset the starting offset within the
   * <code>renderbuffers</code> array.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>renderbuffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>renderbuffers.length
   * - offset</code> is less than <code>n</code>.
   */
  void glDeleteRenderbuffersOES(int n, int[] renderbuffers, int offset);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Integer <code>Buffer</code> version of
   * <code>glDeleteRenderbuffersOES</code>.
   *
   * @see #glDeleteRenderbuffersOES(int n, int[] renderbuffers, int offset)
   *
   * @param n the number of renderbuffers to be deleted.
   * @param renderbuffers an <code>IntBuffer</code> containing
   * renderbuffer names.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>renderbuffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>renderbuffers.limit() - renderbuffers.position()</code> is
   * less than <code>n</code>.
   */
  void glDeleteRenderbuffersOES(int n, IntBuffer renderbuffers);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Generate renderbuffer names.
   *
   * <p> The command <code>glGenRenderbuffersOES</code> returns
   * <code>n</code> previously unused renderbuffer object names in
   * <code>renderbuffers</code>.  These names are marked as used, for
   * the purposes of <code>glGenRenderbuffersOES</code> only, but they
   * acquire renderbuffer state only when they are first bound, just
   * as if they were unused.
   *
   * @param n the number of renderbuffer names to be generated.
   * @param renderbuffers an array to be filled in with <code>n</code>
   * renderbuffer names.
   * @param offset the starting offset within the
   * <code>renderbuffers</code> array.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>renderbuffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>renderbuffers.length
   * - offset</code> is less than <code>n</code>.
   */
  void glGenRenderbuffersOES(int n, int[] renderbuffers, int offset);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Integer <code>Buffer</code> version of
   * <code>glGenRenderbuffersOES</code>.
   *
   * @see #glGenRenderbuffersOES(int n, int[] renderbuffers, int offset)
   *
   * @param n the number of renderbuffer names to be generated.
   * @param renderbuffers an <code>IntBuffer</code> to be filled in
   * with <code>n</code> renderbuffer names.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>renderbuffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>renderbuffers.limit() - renderbuffers.position()</code> is
   * less than <code>n</code>.
   */
  void glGenRenderbuffersOES(int n, IntBuffer renderbuffers);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Establish the layout of a renderbuffer object's image.
   *
   * <p> The command <code>glRenderbufferStorageOES</code> establishes
   * the data storage, format, and dimensions of a renderbuffer
   * object's image.  <code>target</code> must be
   * <code>GL_RENDERBUFFER_OES</code>.  <code>internalformat</code>
   * must be one of the sized internal formats from the following
   * tables which has a base internal format of <code>GL_RGB</code>,
   * <code>GL_RGBA</code>, <code>GL_DEPTH_COMPONENT</code>, or
   * <code>GL_STENCIL_INDEX</code>.
   *
   * <p> The following formats are required:
   *
   * <pre>
   * Sized                 Base               Size in
   * Internal Format       Internal format    Bits
   * ---------------       ---------------    ----
   * RGB565_OES            RGB                16
   * RGBA4                 RGBA               16
   * RGB5_A1               RGBA               16
   * DEPTH_COMPONENT_16    DEPTH_COMPONENT    16
   * </pre>
   *
   * <p> The following formats are optional:
   *
   * <pre>
   * Sized                 Base               Size in
   * Internal Format       Internal format    Bits
   * ---------------       ---------------    ----
   * RGBA8                 RGBA               32
   * RGB8                  RGB                24
   * DEPTH_COMPONENT_24    DEPTH_COMPONENT    24
   * DEPTH_COMPONENT_32    DEPTH_COMPONENT    32
   * STENCIL_INDEX1_OES    STENCIL_INDEX      1
   * STENCIL_INDEX4_OES    STENCIL_INDEX      4
   * STENCIL_INDEX8_OES    STENCIL_INDEX      8
   * </pre>
   *
   * <p> The optional formats are described by the
   * <code>OES_rgb8_rgba8</code>, <code>OES_depth24</code>,
   * <code>OES_depth32</code>, <code>OES_stencil1</code>,
   * <code>OES_stencil4</code>, and <code>OES_stencil8</code>
   * extensions.
   *
   * <p> If <code>glRenderbufferStorageOES</code> is called with an
   * <code>internalformat</code> value that is not supported by the
   * OpenGL ES implementation, a <code>GL_INVALID_ENUM</code> error
   * will be generated.
   *
   * <p> <code>width</code> and
   * <code>height</code> are the dimensions in pixels of the
   * renderbuffer.  If either <code>width</code> or
   * <code>height</code> is greater than
   * <code>GL_MAX_RENDERBUFFER_SIZE_OES</code>, then the error
   * <code>GL_INVALID_VALUE</code> is generated.  If the GL is unable
   * to create a data store of the requested size, the error
   * <code>GL_OUT_OF_MEMORY</code> is
   * generated. <code>glRenderbufferStorageOES</code> deletes any
   * existing data store for the renderbuffer and the contents of the
   * data store after calling <code>glRenderbufferStorageOES</code>
   * are undefined.
   *
   * @param target the value <code>GL_RENDERBUFFER_OES</code>.
   * @param internalformat one of <code>GL_RGB</code>,
   * <code>GL_RGBA</code>, <code>GL_DEPTH_COMPONENT</code>,
   * <code>GL_STENCIL_INDEX</code>, or an internal format that has one
   * of those as a base format.
   * @param width the width of the image.
   * @param height the height of the image.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  void glRenderbufferStorageOES(int target, int internalformat,
				int width, int height);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Query a renderbuffer parameter.
   *
   * <p> <code>target</code> must be <code>GL_RENDERBUFFER_OES</code>.
   * <code>pname</code> must be one of the symbolic values in the
   * table below.
   *
   * <p> If the renderbuffer currently bound to <code>target</code> is
   * zero, then <code>GL_INVALID_OPERATION</code> is generated.
   *
   * <p> Upon successful return from
   * <code>glGetRenderbufferParameterivOES</code>, if
   * <code>pname</code> is <code>GL_RENDERBUFFER_WIDTH_OES</code>,
   * <code>GL_RENDERBUFFER_HEIGHT_OES</code>, or
   * <code>GL_RENDERBUFFER_INTERNAL_FORMAT_OES</code>, then
   * <code>params</code> will contain the width in pixels, height in
   * pixels, or internal format, respectively, of the image of the
   * renderbuffer currently bound to <code>target</code>.
   *
   * <p> Upon successful return from
   * <code>glGetRenderbufferParameterivOES</code>, if
   * <code>pname</code> is <code>GL_RENDERBUFFER_RED_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_GREEN_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_BLUE_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_ALPHA_SIZE_OES</code>,
   * <code>GK_RENDERBUFFER_DEPTH_SIZE_OES</code>, or
   * <code>GL_RENDERBUFFER_STENCIL_SIZE_OES</code>, then
   * <code>params</code> will contain the actual resolutions, (not the
   * resolutions specified when the image array was defined), for the
   * red, green, blue, alpha depth, or stencil components,
   * respectively, of the image of the renderbuffer currently bound to
   * <code>target</code>.
   *
   * <p> Otherwise, <code>GL_INVALID_ENUM</code> is generated.
   *
   * <p> The values in the first column of the table below should be
   * prefixed with <code>GL_RENDERBUFFER_</code> and suffixed with
   * <code>OES</code>.  The get command for all values is
   * <code>glGetRenderbufferParameterivOES</code>.  All take on
   * positive integral values.
   *
   * <pre>
   * Get               Initial
   * Value             Value    Description
   * ---------------   -------  ----------------------
   * WIDTH             0        width of renderbuffer
   * 
   * HEIGHT            0        height of renderbuffer
   *
   * INTERNAL_FORMAT   GL_RGBA  internal format
   *                            of renderbuffer
   *
   * RED_SIZE          0        size in bits of
   *                            renderbuffer image's
   *                            red component
   *
   * GREEN_SIZE        0        size in bits of
   *                            renderbuffer image's
   *                            green component
   *
   * BLUE_SIZE         0        size in bits of
   *                            renderbuffer image's
   *                            blue component
   *
   * ALPHA_SIZE        0        size in bits of
   *                            renderbuffer image's
   *                            alpha component
   *
   * DEPTH_SIZE        0        size in bits of
   *                            renderbuffer image's
   *                            depth component
   *
   * STENCIL_SIZE      0        size in bits of
   *                            renderbuffer image's
   *                            stencil component
   * </pre>
   *
   * @param target the value <code>GL_RENDERBUFFER_OES</code>.
   * @param pname one of <code>GL_RENDERBUFFER_WIDTH_OES</code>,
   * <code>GL_RENDERBUFFER_HEIGHT_OES</code>,
   * <code>GL_RENDERBUFFER_INTERNAL_FORMAT_OES</code>,
   * <code>GL_RENDERBUFFER_RED_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_GREEN_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_BLUE_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_ALPHA_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_DEPTH_SIZE_OES</code>, or
   * <code>GL_RENDERBUFFER_STENCIL_SIZE_OES</code>.
   * @param params an array into which renderbuffer parameters will be
   * written.
   * @param offset the starting offset within the
   * <code>params</code> array.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetRenderbufferParameterivOES(int target, int pname,
				       int[] params, int offset);
  
  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Integer <code>Buffer</code> version of
   * <code>glGetRenderbufferParameterivOES</code>.
   *
   * @see #glGetRenderbufferParameterivOES(int target, int pname,
   * int[] params, int offset)
   *
   * @param target the value <code>GL_RENDERBUFFER_OES</code>.
   * @param pname one of <code>GL_RENDERBUFFER_WIDTH_OES</code>,
   * <code>GL_RENDERBUFFER_HEIGHT_OES</code>,
   * <code>GL_RENDERBUFFER_INTERNAL_FORMAT_OES</code>,
   * <code>GL_RENDERBUFFER_RED_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_GREEN_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_BLUE_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_ALPHA_SIZE_OES</code>,
   * <code>GL_RENDERBUFFER_DEPTH_SIZE_OES</code>, or
   * <code>GL_RENDERBUFFER_STENCIL_SIZE_OES</code>.
   * @param params an <code>IntBuffer</code> into which renderbuffer
   * parameters will be written.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetRenderbufferParameterivOES(int target, int pname,
				       IntBuffer params);

  /**
   * (<code>OES_framebuffer_object</code> extension) Determine whether a
   * token represents a framebuffer.
   *
   * <p> Returns <code>true</code> if <code>framebuffer</code> is the
   * name of a framebuffer object.  If <code>framebuffer</code> is
   * zero, or if <code>framebuffer</code> is a non-zero value that is
   * not the name of a framebuffer object,
   * <code>glIsFrambufferOES</code> returns <code>false</code>.
   *
   * @param framebuffer an integer.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  boolean glIsFramebufferOES(int framebuffer);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Bind a framebuffer.

   * <p> The operations described in chapter 4 of the OpenGL 1.5
   * specification affect the images attached to the framebuffer
   * object bound to the target GL_FRAMEBUFFER_OES.  By default,
   * framebuffer bound to the target GL_FRAMEBUFFER_OES is zero,
   * specifying the default implementation dependent framebuffer
   * provided by the windowing system.  When the framebuffer bound to
   * target GL_FRAMEBUFFER_OES is not zero, but instead names a
   * application-created framebuffer object, then operations
   * affect the application-created framebuffer object
   * rather than the default framebuffer.
   *
   * <p> The namespace for framebuffer objects is the unsigned
   * integers, with zero reserved by the GL to refer to the default
   * framebuffer.  A framebuffer object is created by binding an
   * unused name to the target <code>GL_FRAMEBUFFER_OES</code>.  The
   * binding is effected by calling <code>glBindFramebufferOES</code>
   * with <code>target</code> set to <code>GL_FRAMEBUFFER_OES</code>
   * and <code>framebuffer</code> set to the unused name.  The
   * resulting framebuffer object is a new state vector, comprising
   * all the state values listed in the table below.  There are
   * <code>GL_MAX_COLOR_ATTACHMENTS_OES</code> color attachment
   * points, plus one each for the depth and stencil attachment
   * points.
   *
   * <code>glBindFramebufferOES</code> may also be used to bind an
   * existing framebuffer object to <code>target</code>.  If the bind
   * is successful no change is made to the state of the bound
   * framebuffer object and any previous binding to
   * <code>target</code> is broken.  The current
   * <code>GL_FRAMEBUFFER_OES</code> binding can be queried using
   * <code>glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES)</code>.
   *
   * <p> While a framebuffer object is bound to the target
   * <code>GL_FRAMEBUFFER_OES</code>, GL operations on the target to
   * which it is bound affect the images attached to the bound
   * framebuffer object, and queries of the target to which it is
   * bound return state from the bound object. The framebuffer object
   * bound to the target <code>GL_FRAMEBUFFER_OES</code> is used as
   * the destination of fragment operations and as the source of pixel
   * reads such as <code>glReadPixels</code>.
   *
   * In the initial state, the reserved name zero is bound to the
   * target <code>GL_FRAMEBUFFER_OES</code>.  There is no
   * application-created framebuffer object corresponding to the name
   * zero.  Instead, the name zero refers to the
   * window-system-provided framebuffer.  All queries and operations
   * on the framebuffer while the name zero is bound to the target
   * <code>GL_FRAMEBUFFER_OES</code> operate on this default
   * framebuffer.  On some implementations, the properties of the
   * default window-system-provided framebuffer can change over time
   * (e.g., in response to window-system events such as attaching the
   * context to a new window-system drawable.)
   *
   * <p> Application-created framebuffer objects (i.e., those with a
   * non-zero name) differ from the default window-system-provided
   * framebuffer in a few important ways.  First and foremost, unlike
   * the window-system-provided framebuffer,
   * application-created-framebuffers have modifiable attachment
   * points for each logical buffer in the framebuffer.
   * Framebuffer-attachable images can be attached to and detached
   * from these attachment points.  Also, the size and format of the
   * images attached to application-created framebuffers are
   * controlled entirely within the GL interface, and are not affected
   * by window-system events, such as pixel format selection, window
   * resizes, and display mode changes.
   *
   * <p> Additionally, when rendering to or reading from an
   * application created-framebuffer object:
   *
   * <ul>
   *
   * <li>The pixel ownership test always succeeds.  In other words,
   * application-created framebuffer objects own all of their
   * pixels.</li>
   *
   * <li>There are no visible color buffer bitplanes.  This means
   * there is no color buffer corresponding to the back, front, left,
   * or right color bitplanes.</li>
   *
   * <li>The only color buffer bitplanes are the ones defined by the
   * framebuffer attachment points named
   * <code>COLOR_ATTACHMENT0_OES</code> through
   * <code>COLOR_ATTACHMENT</code><i>n</i><code>_OES</code>.</li>
   *
   * <li>The only depth buffer bitplanes are the ones defined by the
   * framebuffer attachment point
   * <code>DEPTH_ATTACHMENT_OES</code>.</li>
   * 
   * <li>The only stencil buffer bitplanes are the ones defined by the
   * framebuffer attachment point
   * <code>STENCIL_ATTACHMENT_OES</code>.</li>
   *
   * </ul>
   *
   * @param target the value <code>GL_FRAMEBUFFER_OES</code>.
   * @param framebuffer the framebuffer to be bound.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  void glBindFramebufferOES(int target, int framebuffer);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Delete framebuffer objects.
   *
   * <p> Framebuffer objects are deleted by calling
   * <code>glDeleteFramebuffersOES</code>. <code>framebuffers</code>
   * contains <code>n</code> names of framebuffer objects to be
   * deleted.  After a framebuffer object is deleted, it has no
   * attachments, and its name is again unused.  If a framebuffer that
   * is currently bound to the target <code>GL_FRAMEBUFFER_OES</code>
   * is deleted, it is as though <code>glBindFramebufferOES</code> had
   * been executed with the <code>target</code> of
   * <code>GL_FRAMEBUFFER_OES</code> and <code>framebuffer</code> of
   * zero.  Unused names in <code>framebuffers</code> are silently
   * ignored, as is the value zero.
   *
   * @param n the number of framebuffers to be deleted.
   * @param framebuffers a list of framebuffers.
   * @param offset the starting offset within the
   * <code>framebuffers</code> array.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>framebuffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>framebuffers.length
   * - offset</code> is less than <code>n</code>.
   */
  void glDeleteFramebuffersOES(int n, int[] framebuffers, int offset);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Integer <code>Buffer</code> version of
   * <code>glDeleteFramebuffersOES</code>.
   *
   * @see #glDeleteFramebuffersOES(int n, int[] framebuffers, int offset)
   *
   * @param n
   * @param framebuffers
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>framebuffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>framebuffers.limit()
   * - framebuffers.position()</code> is less than <code>n</code>.
   */
  void glDeleteFramebuffersOES(int n, IntBuffer framebuffers);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Generate framebuffer names.
   *
   * <p> The command <code>glGenFramebuffersOES</code> returns
   * <code>n</code> previously unused framebuffer object names in
   * <code>ids</code>.  These names are marked as used, for the
   * purposes of <code>glGenFramebuffersOES</code> only, but they
   * acquire state and type only when they are first bound, just as if
   * they were unused.
   *
   * @param n the number of framebuffer names to be generated.
   * @param framebuffers an array to be filled in with <code>n</code>
   * framebuffer names.
   * @param offset the starting offset within the
   * <code>framebuffers</code> array.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>framebuffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>framebuffers.length
   * - offset</code> is less than <code>n</code>.
   */
  void glGenFramebuffersOES(int n, int[] framebuffers, int offset);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Integer <code>Buffer</code> version of
   * <code>glGenFramebuffersOES</code>.
   *
   * @see #glGenFramebuffersOES(int n, int[] framebuffers, int offset)
   *
   * @param n the number of framebuffer names to be generated.
   * @param framebuffers an <code>IntBuffer</code> to be filled in
   * with <code>n</code> framebuffer names.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>framebuffers</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>framebuffers.limit()
   * - framebuffers.position()</code> is less than <code>n</code>.
   */
  void glGenFramebuffersOES(int n, IntBuffer framebuffers);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Check the status of a framebuffer.
   *
   * <p> Returns one of <code>GL_FRAMEBUFFER_COMPLETE_OES</code>,
   * <code>GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES</code>,
   * <code>GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES</code>,
   * <code>GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES</code>,
   * <code>GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_OES</code>,
   * <code>GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_OES</code>, or
   * <code>GL_FRAMEBUFFER_UNSUPPORTED_OES</code>.
   *
   * @param target a framebuffer.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  int glCheckFramebufferStatusOES(int target);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Attach an image from a texture object to a framebuffer.
   *
   * <p> To render directly into a texture image, a specified image from a
   * texture object can be attached as one of the logical buffers of the
   * currently bound framebuffer object by calling this method.
   *
   * <p> GL_INVALID_OPERATION is generated if the current value of
   * <code>GL_FRAMEBUFFER_BINDING_OES</code> is zero when
   * <code>glFramebufferTexture2DOES</code> is called.
   * <code>attachment</code> must be one of the attachment points of
   * the framebuffer, either
   * <code>GL_COLOR_ATTACHMENT</code><i>n</i><code>_OES</code> (where
   * <i>n</i> is from 0 to <code>GL_MAX_COLOR_ATTACHMENTS_OES</code> -
   * 1), <code>GL_DEPTH_ATTACHMENT_OES</code>, or
   * <code>GL_STENCIL_ATTACHMENT_OES</code>.
   *
   * <p> If <code>texture</code> is zero, then <code>textarget</code>
   * and <code>level</code> are ignored.  If <code>texture</code> is
   * not zero, then <code>texture</code> must either name an existing
   * texture object with an target of <code>textarget</code>, or
   * <code>texture</code> must name an existing cube map texture and
   * <code>textarget</code> must be one of:
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Y</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Z</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Y</code>, or
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Z</code>.  Otherwise,
   * <code>GL_INVALID_OPERATION</code> is generated.
   *
   * <p> <code>level</code> specifies the mipmap level of the texture
   * image to be attached to the framebuffer and must be 0 (indicating
   * the base MIPmap level).
   *
   * <p> If <code>textarget</code> is one of
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Y</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Z</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Y</code>, or
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Z</code>, then
   * <code>level</code> must be greater than or equal to zero and less
   * than or equal to log base 2 of
   * <code>GL_MAX_CUBE_MAP_TEXTURE_SIZE</code>. For all other values of
   * <code>textarget</code>, <code>level</code> must be greater than or
   * equal to zero and no larger than log base 2 of
   * <code>GL_MAX_TEXTURE_SIZE</code>.  Otherwise,
   * <code>GL_INVALID_VALUE</code> is generated.
   *
   * <p> If <code>texture</code> is not zero, then
   * <code>textarget</code> must be one of:
   * <code>GL_TEXTURE_2D</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Y</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Z</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Y</code>, or
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Z</code>.
   *
   * <p> If <code>texture</code> is not zero, and if
   * <code>glFramebufferTexture2DOES</code> is successful, then the
   * specified texture image will be used as the logical buffer
   * identified by <code>attachment</code> of the framebuffer currently
   * bound to <code>target</code>.  The value of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES</code> for the
   * specified attachment point is set to <code>GL_TEXTURE</code> and
   * the value of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES</code> is set to
   * <texture>.  Additionally, the value of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES</code> for the
   * named attachment point is set to <code>level</code>.  If
   * <code>texture</code> is a cubemap texture then, the value of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES</code>
   * the named attachment point is set to <code>textarget</code>.  All
   * other state values of the attachment point specified by
   * <code>attachment</code> are set to their default values listed in
   * the table below.  No change is made to the state of the texture
   * object, and any previous attachment to the <code>attachment</code>
   * logical buffer of the framebuffer object bound to framebuffer
   * <code>target</code> is broken.  If, on the other hand, the
   * attachment is not successful, then no change is made to the state
   * of either the texture object or the framebuffer object.
   *
   * <p> Calling <code>glFramebufferTexture2DOES</code> with
   * <code>texture</code> name zero will detach the image identified by
   * <code>attachment</code>, if any, in the framebuffer currently
   * bound to <code>target</code>.  All state values of the attachment
   * point specified by <code>attachment</code> are set to their
   * default values listed in the table below.
   *
   * <p> If a texture object is deleted while its image is attached to
   * one or more attachment points in the currently bound framebuffer,
   * then it is as if <code>glFramebufferTexture2DOES</code> had been
   * called, with a <code>texture</code> of 0, for each attachment
   * point to which this image was attached in the currently bound
   * framebuffer.  In other words, this texture image is first detached
   * from all attachment points in the currently bound framebuffer.
   * Note that the texture image is specifically <b>not</b> detached from
   * any other framebuffer objects.  Detaching the texture image from
   * any other framebuffer objects is the responsibility of the
   * application.
   *
   * <p> The entire texture image, including border texels, if any, can be
   * addressed with window-coordinates in the following range:
   *
   * <pre>
   * 0 <= window_x < (texture_width  + (2 * border)),
   * </pre>
   * and
   * <pre>
   * 0 <= window_y < (texture_height + (2 * border))
   * </pre>
   *
   * <p> The values in the first column of the table below should be
   * prefixed with <code>GL_RENDERBUFFER_</code> and suffixed with
   * <code>OES</code>.  The get command for all values is
   * <code>glGetRenderbufferParameterivOES</code>.  All take on
   * positive integral values.
   *
   * <pre>
   * Get                        Initial
   * Value                      Value         Description
   * ---------------            -------       ----------------------
   * ATTACHMENT_OBJECT_TYPE     GL_NONE       type of
   *                                          image attached to
   *                                          framebuffer attachment
   *                                          point
   *
   * ATTACHMENT_OBJECT_NAME     0             name of object
   *                                          attached to
   *                                          framebuffer attachment
   *                                          point
   *
   * ATTACHMENT_TEXTURE_LEVEL   0             mipmap level of
   *                                          texture image
   *                                          attached, if object
   *                                          attached is texture.
   *
   * ATTACHMENT_TEXTURE_        GL_TEXTURE_   cubemap face of
   * CUBE_MAP_FACE              CUBE_MAP_     texture image
   *                            POSITIVE_X    attached, if object
   *                                          attached is cubemap
   *                                          texture.
   * </pre>
   *
   * @param target the value <code>GL_FRAMEBUFFER_OES</code>.
   * @param attachment the framebuffer attachment, one of
   * <code>GL_COLOR_ATTACHMENT_</code><i>n</i><code>_OES</code> (where
   * <i>n</i> is from 0 to <code>GL_MAX_COLOR_ATTACHMENTS_OES</code> -
   * 1), <code>GL_DEPTH_ATTACHMENT_OES</code>, or
   * <code>GL_STENCIL_ATTACHMENT_OES</code>.
   * @param textarget one of <code>GL_TEXTURE_2D</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Y</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Z</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Y</code>, or
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Z</code>.
   * @param texture the texture, or zero.
   * @param level the texture MIPmap level, which must be 0.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  void glFramebufferTexture2DOES(int target, int attachment,
				 int textarget, int texture,
				 int level);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Attach a renderbuffer to a framebuffer.
   *
   * <p> A renderbuffer can be attached as one of the logical buffers
   * of the currently bound framebuffer object by calling
   * <code>glFramebufferRenderbufferOES</code>. <code>target</code>
   * must be <code>GL_FRAMEBUFFER_OES</code>.
   * <code>GL_INVALID_OPERATION</code> is generated if the current
   * value of <code>GL_FRAMEBUFFER_BINDING_OES</code> is zero when
   * <code>glFramebufferRenderbufferOES</code> is called.
   * <code>attachment</code> should be set to one of the attachment
   * points of the framebuffer, one of
   * <code>GL_COLOR_ATTACHMENT</code><i>n</i><code>_OES</code> (where
   * <i>n</i> is from 0 to <code>GL_MAX_COLOR_ATTACHMENTS_OES</code> -
   * 1), <code>GL_DEPTH_ATTACHMENT_OES</code>, or
   * <code>GL_STENCIL_ATTACHMENT_OES</code>.
   * <code>renderbuffertarget</code> must be
   * <code>GL_RENDERBUFFER_OES</code> and <code>renderbuffer</code>
   * should be set to the name of the renderbuffer object to be
   * attached to the framebuffer.  <code>renderbuffer</code> must be
   * either zero or the name of an existing renderbuffer object of
   * type <code>renderbuffertarget</code>, otherwise
   * <code>GL_INVALID_OPERATION</code> is generated.  If
   * <code>renderbuffer</code> is zero, then the value of
   * <code>renderbuffertarget</code> is ignored.
   *
   * <p> If <code>renderbuffer</code> is not zero and if
   * <code>glFramebufferRenderbufferOES</code> is successful, then the
   * renderbuffer named <code>renderbuffer</code> will be used as the
   * logical buffer identified by <code>attachment</code> of the
   * framebuffer currently bound to <code>target</code>.  The value of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES</code> for the
   * specified attachment point is set to
   * <code>GL_RENDERBUFFER_OES</code> and the value of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES</code> is set to
   * <code>renderbuffer</code>. All other state values of the
   * attachment point specified by <code>attachment</code> are set to
   * their default values listed in the table below. No change is made
   * to the state of the renderbuffer object and any previous
   * attachment to the <code>attachment</code> logical buffer of the
   * framebuffer object bound to framebuffer <code>target</code> is
   * broken.  If, on the other hand, the attachment is not successful,
   * then no change is made to the state of either the renderbuffer
   * object or the framebuffer object.
   *
   * <p> Calling <code>glFramebufferRenderbufferOES</code> with the
   * <code>renderbuffer</code> name zero will detach the image, if
   * any, identified by <code>attachment</code>, in the framebuffer
   * currently bound to <code>target</code>.  All state values of the
   * attachment point specified by <code>attachment</code> in the
   * object bound to <code>target</code> are set to their default
   * values listed in the table below.
   *
   * <p> If a renderbuffer object is deleted while its image is
   * attached to one or more attachment points in the currently bound
   * framebuffer, then it is as if
   * <code>glFramebufferRenderbufferOES</code> had been called, with a
   * <code>renderbuffer</code> of 0, for each attachment point to
   * which this image was attached in the currently bound framebuffer.
   * In other words, this renderbuffer image is first detached from
   * all attachment points in the currently bound framebuffer.  Note
   * that the renderbuffer image is specifically *not* detached from
   * any non-bound framebuffers.  Detaching the image from any
   * non-bound framebuffers is the responsibility of the application.
   *
   * <p> The values in the first column of the table below should be
   * prefixed with <code>GL_RENDERBUFFER_</code> and suffixed with
   * <code>OES</code>.  The get command for all values is
   * <code>glGetRenderbufferParameterivOES</code>.  All take on
   * positive integral values.
   *
   * <pre>
   * Get                        Initial
   * Value                      Value         Description
   * ---------------            -------       ----------------------
   * ATTACHMENT_OBJECT_TYPE     GL_NONE       type of
   *                                          image attached to
   *                                          framebuffer attachment
   *                                          point
   *
   * ATTACHMENT_OBJECT_NAME     0             name of object
   *                                          attached to
   *                                          framebuffer attachment
   *                                          point
   *
   * ATTACHMENT_TEXTURE_LEVEL   0             mipmap level of
   *                                          texture image
   *                                          attached, if object
   *                                          attached is texture.
   *
   * ATTACHMENT_TEXTURE_        GL_TEXTURE_   cubemap face of
   * CUBE_MAP_FACE              CUBE_MAP_     texture image
   *                            POSITIVE_X    attached, if object
   *                                          attached is cubemap
   *                                          texture.
   * </pre>
   *
   * @param target the value <code>GL_FRAMEBUFFER_OES</code>.
   * @param attachment one of
   * <code>GL_COLOR_ATTACHMENT</code><i>n</i><code>_OES</code> (where
   * <i>n</i> is from 0 to <code>GL_MAX_COLOR_ATTACHMENTS_OES</code> -
   * 1), <code>GL_DEPTH_ATTACHMENT_OES</code>, or
   * <code>GL_STENCIL_ATTACHMENT_OES</code>.
   * @param renderbuffertarget the value
   * <code>GL_RENDERBUFFER_OES</code>.
   * @param renderbuffer the renderbuffer to be attached to the framebuffer.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  void glFramebufferRenderbufferOES(int target, int attachment,
				    int renderbuffertarget,
				    int renderbuffer);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Query the value of a framebuffer attachment parameter.
   *
   * <p> <code>target</code> must be <code>GL_FRAMEBUFFER_OES</code>.
   * <code>attachment</code> must be one of the attachment points of
   * the framebuffer, either <code>GL_COLOR_ATTACHMENT</code><i>n</i><code>_OES</code> (where
   * <i>n</i> is from 0 to <code>GL_MAX_COLOR_ATTACHMENTS_OES</code> -
   * 1), <code>GL_DEPTH_ATTACHMENT_OES</code>, or
   * <code>GL_STENCIL_ATTACHMENT_OES</code>.
   * <code>pname</code> must be one of the following:
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES</code>,
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES</code>,
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES</code>, or
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES</code>.
   *
   * <p> If the framebuffer currently bound to <code>target</code> is
   * zero, then <code>GL_INVALID_OPERATION</code> is generated.
   *
   * <p> Upon successful return from
   * <code>glGetFramebufferAttachmentParameterivOES</code>, if
   * <code>pname</code> is
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES</code>, then
   * <code>param</code> will contain one of <code>GL_NONE</code>,
   * <code>GL_TEXTURE</code>, or <code>GL_RENDERBUFFER_OES</code>,
   * identifying the type of object which contains the attached image.
   *
   * <p> If the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES is
   * <code>GL_RENDERBUFFER_OES</code>, then:
   * 
   * <ul> <li>If <code>pname</code> is
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES</code>,
   * <code>params</code> will contain the name of the renderbuffer
   * object which contains the attached image.</li>
   *
   * <li>Otherwise, <code>GL_INVALID_ENUM</code> is generated.</li>
   * </ul>
   *
   * <p> If the value of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES</code> is
   * <code>GL_TEXTURE</code>, then:
   *
   * <ul>
   *
   * <li>If <pname> is
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES</code>, then
   * <params> will contain the name of the texture object which
   * contains the attached image.</li>
   *
   * <li>If <pname> is
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES</code>, then
   * <params> will contain the mipmap level of the texture object
   * which contains the attached image.</li>
   *
   * <li>If <pname> is
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES</code>
   * and the texture object named
   * <code>FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES</code> is a cube map
   * texture, then <params> will contain the cube map face of the
   * cubemap texture object which contains the attached image.
   * Otherwise <params> will contain the value zero.</li>
   *
   * <li>Otherwise, <code>GL_INVALID_ENUM</code> is generated.</li>
   *
   * </ul>
   *
   * @param target the value <code>GL_FRAMEBUFFER_OES</code>.
   * @param attachment one of
   * <code>GL_COLOR_ATTACHMENT</code><i>n</i><code>_OES</code> (where
   * <i>n</i> is from 0 to <code>GL_MAX_COLOR_ATTACHMENTS_OES</code> -
   * 1), <code>GL_DEPTH_ATTACHMENT_OES</code>, or
   * <code>GL_STENCIL_ATTACHMENT_OES</code>.
   * @param pname one of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES</code>,
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES</code>,
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES</code>, or
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES</code>.
   * @param params an array to which the query results will be written.
   * @param offset the starting offset within the
   * <code>params</code> array.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glGetFramebufferAttachmentParameterivOES(int target,
						int attachment,
						int pname,
						int[] params, int offset);

  /**
   * (<code>OES_framebuffer_object</code> extension)
   * Integer <code>Buffer</code> version of
   * <code>glGetFramebufferAttachmentParameterivOES</code>.
   * 
   * @see #glGetFramebufferAttachmentParameterivOES(int target, int
   * attachment, int pname, int[] params, int offset)
   *
   * @param target the value <code>GL_FRAMEBUFFER_OES</code>.
   * @param attachment one of
   * <code>GL_COLOR_ATTACHMENT</code><i>n</i><code>_OES</code> (where
   * <i>n</i> is from 0 to <code>GL_MAX_COLOR_ATTACHMENTS_OES</code> -
   * 1), <code>GL_DEPTH_ATTACHMENT_OES</code>, or
   * <code>GL_STENCIL_ATTACHMENT_OES</code>.
   * @param pname one of
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES</code>,
   * <code>GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES</code>,
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES</code>, or
   * <code>GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES</code>.
   * @param params an <code>IntBuffer</code> to which the query
   * results will be written.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glGetFramebufferAttachmentParameterivOES(int target,
						int attachment,
						int pname,
						IntBuffer params);

  /**
   * (<code>OES_framebuffer_object</code> extension) Generate mipmaps
   * manually.
   *
   * <p> <code>target</code> is one of <code>TEXTURE_2D</code> or
   * <code>TEXTURE_CUBE_MAP</code>.  Mipmap generation affects the
   * texture image attached to <code>target</code>.  For cube map
   * textures, <code>INVALID_OPERATION</code> is generated if the
   * texture bound to <code>target</code> is not cube complete.
   *
   * @param target one of <code>TEXTURE_2D</code> or
   * <code>TEXTURE_CUBE_MAP</code>.
   *
   * @exception UnsupportedOperationException if the underlying
   * runtime engine does not support the
   * <code>OES_framebuffer_object</code> extension.
   */
  void glGenerateMipmapOES(int target);
}
