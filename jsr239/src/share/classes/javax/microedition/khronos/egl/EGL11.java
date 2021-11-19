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

package javax.microedition.khronos.egl;


/**
 * The EGL11 interface contains the Java(TM) programming language
 * bindings for EGL 1.1. It extends the EGL10 interface.
 *
 * <p> The documentation in this interface is normative with respect
 * to instance variable names and values, method names and signatures,
 * and exception behavior.  The remaining documentation is placed here
 * for convenience and does not replace the normative documentation
 * found in the EGL specification and relevant
 * extension specifications.  EGL documentation is available at the <a
 * href="http://www.khronos.org/opengles/spec">Khronos</a> web site.
 */
public interface EGL11 extends EGL10 {
  
  /** EGL 1.1 constant. */
  public static final int EGL_CONTEXT_LOST            = 0x300E;
  
  /** EGL 1.1 constant. */
  public static final int EGL_BIND_TO_TEXTURE_RGB     = 0x3039;
  /** EGL 1.1 constant. */
  public static final int EGL_BIND_TO_TEXTURE_RGBA    = 0x303A;
  /** EGL 1.1 constant. */
  public static final int EGL_MIN_SWAP_INTERVAL       = 0x303B;
  /** EGL 1.1 constant. */
  public static final int EGL_MAX_SWAP_INTERVAL       = 0x303C;
  
  /** EGL 1.1 constant. */
  public static final int EGL_NO_TEXTURE              = 0x305C;
  /** EGL 1.1 constant. */
  public static final int EGL_TEXTURE_RGB             = 0x305D;
  /** EGL 1.1 constant. */
  public static final int EGL_TEXTURE_RGBA            = 0x305E;
  /** EGL 1.1 constant. */
  public static final int EGL_TEXTURE_2D              = 0x305F;
  
  // Surface attributes

  /** EGL 1.1 constant. */
  public static final int EGL_TEXTURE_FORMAT          = 0x3080;
  /** EGL 1.1 constant. */
  public static final int EGL_TEXTURE_TARGET          = 0x3081;
  /** EGL 1.1 constant. */
  public static final int EGL_MIPMAP_TEXTURE          = 0x3082;
  /** EGL 1.1 constant. */
  public static final int EGL_MIPMAP_LEVEL            = 0x3083;
  
  // BindTexImage / ReleaseTexImage buffer target
  /** EGL 1.1 constant. */
  public static final int EGL_BACK_BUFFER             = 0x3084;
  
  /**
   * (EGL 1.1 only) Set an EGL surface attribute.
   * 
   * <p><code>eglSurfaceAttrib</code> sets the value of
   * <code>attribute</code> for <code>surface</code> to
   * <code>value</code>. <code>attribute</code> can be one of the
   * following:
   * 
   * <ul>
   * 
   * <li><code>EGL_MIPMAP_LEVEL</code></li>
   * 
   * <p>For mipmap textures, the <code>EGL_MIPMAP_LEVEL</code>
   * attribute indicates which level of the mipmap should be
   * rendered. If the value of this attribute is outside the range of
   * supported mipmap levels, the closest valid mipmap level is
   * selected for rendering. The default value is 0.
   * 
   * </ul>
   * 
   * <h4>Notes</h4>
   * 
   * <p>If the value of pbuffer attribute
   * <code>EGL_TEXTURE_FORMAT</code> is <code>EGL_NO_TEXTURE</code>,
   * if the value of attribute <code>EGL_TEXTURE_TYPE</code> is
   * <code>EGL_NO_TEXTURE</code>, or if surface is not a pbuffer, then
   * attribute <code>EGL_MIPMAP_LEVEL</code> may be set, but has no
   * effect.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>false</code> is returned on failure, <code>true</code>
   * otherwise.
   * 
   * <p><code>EGL_BAD_DISPLAY</code> is generated if
   * <code>display</code> is not an EGL display connection.
   * 
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   * 
   * <p><code>EGL_BAD_SURFACE</code> is generated if
   * <code>surface</code> is not an EGL surface.
   * 
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attribute</code> is not a valid surface attribute.
   * 
   * @param display Specifies the EGL display connection.
   * @param surface Specifies the EGL surface.
   * @param attribute Specifies the EGL surface attribute to set.
   * @param value Specifies the attributes required value.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>surface</code> is
   * <code>null</code>.
   * 
   * @return <code>true</code> if the operation succeeds.
   */
  boolean eglSurfaceAttrib(EGLDisplay display,
			   EGLSurface surface,
			   int attribute,
			   int value);
  
  /**
   * (EGL 1.1 only) Defines a two-dimensional texture image.
   * 
   * <p>The texture image consists of the image data in
   * <code>buffer</code> for the specified <code>surface</code>, and
   * need not be copied.
   * 
   * <p>The texture target, the texture format and the size of the
   * texture components are derived from attributes of the specified
   * surface, which must be a pbuffer supporting one of the
   * <code>EGL_BIND_TO_TEXTURE_RGB</code> or
   * <code>EGL_BIND_TO_TEXTURE_RGBA</code> attributes.
   * 
   * <p>The pbuffer attribute <code>EGL_TEXTURE_FORMAT</code>
   * determines the base internal format of the texture.
   * 
   * <p>The texture target is derived from the
   * <code>EGL_TEXTURE_TARGET</code> attribute of
   * <code>surface</code>. If the attribute value is
   * <code>EGL_TEXTURE_2D</code>, then buffer defines a texture for
   * the two-dimensional texture object which is bound to the current
   * context (hereafter referred to as the current texture object).
   * 
   * <p>If <code>display</code> and <code>surface</code> are the
   * display and surface for the calling thread’s current context,
   * <code>eglBindTexImage</code> performs an implicit
   * <code>glFlush</code>. For other surfaces,
   * <code>eglBindTexImage</code> waits for all effects from
   * previously issued OpenGL ES commands drawing to the surface to
   * complete before defining the texture image, as though
   * <code>glFinish</code> were called on the last context to which
   * that surface were bound.
   * 
   * <p>After <code>eglBindTexImage</code> is called, the specified
   * surface is no longer available for reading or writing. Any read
   * operation, such as <code>glReadPixels</code> or
   * <code>eglCopyBuffers</code>, which reads values from any of the
   * surface’s color buffers or ancillary buffers will produce
   * indeterminate results. In addition, draw operations that are done
   * to the surface before its color buffer is released from the
   * texture produce indeterminate results. Specifically, if the
   * surface is current to a context and thread then rendering
   * commands will be processed and the context state will be updated,
   * but the surface may or may not be written.
   * 
   * <p>Texture mipmap levels are automatically generated when all of
   * the following conditions are met while calling
   * <code>eglBindTexImage</code>:
   * 
   * <p>The <code>EGL_MIPMAP_TEXTURE</code> attribute of the pbuffer
   * being bound is <code>EGL_TRUE</code>.
   * 
   * <p>The OpenGL ES texture parameter
   * <code>GL_GENERATE_MIPMAP</code> is <code>GL_TRUE</code> for the
   * currently bound texture.
   * 
   * <p>The value of the <code>EGL_MIPMAP_LEVEL</code> attribute of
   * the pbuffer being bound is equal to the value of the texture
   * parameter <code>GL_TEXTURE_BASE_LEVEL</code>. In this case,
   * additional mipmap levels are generated as described in section
   * 3.8 of the OpenGL ES 1.1 Specification.
   * 
   * <h4>Notes</h4>
   * 
   * <p><code>eglSwapBuffers</code> has no effect if it is called on
   * a bound surface.
   * 
   * <p>Any existing images associated with the different mipmap
   * levels of the texture object are freed (it is as if
   * <code>glTexImage</code> was called with an image of zero width).
   * 
   * <p>The color buffer is bound to a texture object. If the texture
   * object is shared between contexts, then the color buffer is also
   * shared. If a texture object is deleted before
   * <code>eglReleaseTexImage</code> is called, then the color buffer
   * is released and the surface is made available for reading and
   * writing.
   * 
   * <p>It is not an error to call <code>glTexImage2D</code> or
   * <code>glCopyTexImage2D</code> to replace an image of a texture
   * object that has a color buffer bound to it. However, these calls
   * will cause the color buffer to be released back to the surface
   * and new memory will be allocated for the texture. Note that the
   * color buffer is released even if the image that is being defined
   * is a mipmap level that was not defined by the color buffer.
   * 
   * <p><code>eglBindTexImage</code> is ignored if there is no
   * current rendering context.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>EGL_BAD_MATCH</code> is generated if the surface
   * attribute <code>EGL_TEXTURE_FORMAT</code> is set to
   * <code>EGL_NO_TEXTURE</code>.
   * 
   * <p><code>EGL_BAD_ACCESS</code> is generated if
   * <code>buffer</code> is already bound to a texture.
   * 
   * <p><code>EGL_BAD_VALUE</code> is generated if
   * <code>buffer</code> is not a valid buffer (currently only
   * <code>EGL_BACK_BUFFER</code> may be specified).
   * 
   * <p><code>EGL_BAD_SURFACE</code> is generated if surface is not
   * an EGL surface, or is not a pbuffer surface supporting texture
   * binding.
   * 
   * @param display Specifies the EGL display connection.
   * @param surface Specifies the EGL surface.
   * @param buffer Specifies the texture image data.
   * 
   * @return <code>true</code> if the operation succeeds.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>surface</code> is
   * <code>null</code>.
   */
  boolean eglBindTexImage(EGLDisplay display,
			  EGLSurface surface,
			  int buffer);
  
  /**
   * (EGL 1.1 only) Releases a color buffer that is being used as a
   * texture.
   * 
   * <p>The specified color buffer is released back to the
   * surface. The surface is made available for reading and writing
   * when it no longer has any color buffers bound as textures.
   * 
   * <h4>Notes</h4>
   * 
   * <p>If the specified color buffer is no longer bound to a texture
   * (e.g., because the texture object was deleted) then
   * <code>eglReleaseTexImage</code> has no effect. No error is
   * generated.
   * 
   * <p>The contents of the color buffer are undefined when it is
   * first released. In particular, there is no guarantee that the
   * texture image is still present. However, the contents of other
   * color buffers are unaffected by this call. Also, the contents of
   * the depth and stencil buffers are not affected by
   * <code>eglBindTexImage</code> and <code>eglReleaseTexImage</code>.
   * 
   * <p>After a color buffer is released from a texture (either
   * explicitly by calling <code>eglReleaseTexImage</code> or
   * implicitly by calling a routine such as
   * <code>glTexImage2D</code>), all texture images that were defined
   * by the color buffer become NULL (it is as if
   * <code>glTexImage</code> was called with an image of zero width).
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>EGL_BAD_MATCH</code> is generated if the surface
   * attribute <code>EGL_TEXTURE_FORMAT</code> is set to
   * <code>EGL_NO_TEXTURE</code>.
   * 
   * <p><code>EGL_BAD_VALUE</code> is generated if <code>buffer</code>
   * is not a valid buffer (currently only
   * <code>EGL_BACK_BUFFER</code> may be specified).
   * 
   * <p><code>EGL_BAD_SURFACE</code> is generated if
   * <code>surface</code> is not an EGL surface, or is not a bound
   * pbuffer surface.
   * 
   * @param display Specifies the EGL display connection.
   * @param surface Specifies the EGL surface.
   * @param buffer Specifies the texture image data.
   * 
   * @return <code>true</code> if the operation succeeds.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>surface</code> is
   * <code>null</code>.
   */
  boolean eglReleaseTexImage(EGLDisplay display,
			     EGLSurface surface,
			     int buffer);
  
  /**
   * (EGL 1.1 only) Specifies the minimum number of video frame
   * periods per buffer swap for the window associated with the
   * current context.
   * 
   * <p>The interval takes effect when <code>eglSwapBuffers</code> is
   * first called subsequent to the <code>eglSwapInterval</code> call.
   * 
   * <p>The interval specified by the function applies to the draw
   * surface bound to the context that is current on the calling
   * thread.
   * 
   * <p>If <code>interval</code> is set to a value of 0, buffer swaps
   * are not synchronized to a video frame, and the swap happens as
   * soon as the render is complete. <code>interval</code> is silently
   * clamped to minimum and maximum implementation dependent
   * values before being stored; these values are defined by
   * <code>EGLConfig</code> attributes
   * <code>EGL_MIN_SWAP_INTERVAL</code> and
   * <code>EGL_MAX_SWAP_INTERVAL</code> respectively.
   * 
   * <h4>Notes</h4>
   * 
   * <p>The swap interval has no effect on
   * <code>eglCopyBuffers</code>.
   * 
   * <p>The default swap interval is 1.
   * 
   * <h4>Errors</h4>
   * 
   * <p><code>false</code> is returned on failure, <code>true</code>
   * otherwise.
   * 
   * <p><code>EGL_BAD_CONTEXT</code> is generated if there is no
   * current context on the calling thread.
   * 
   * <p><code>EGL_BAD_SURFACE</code> is generated if there is no
   * surface bound to the current context.
   * 
   * @param display Specifies the EGL display connection.
   * @param interval Specifies the minimum number of video frames that
   * are displayed before a buffer swap will occur.
   *   
   * @return <code>true</code> if the operation succeeds. 
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   */
  boolean eglSwapInterval(EGLDisplay display, int interval);
}
