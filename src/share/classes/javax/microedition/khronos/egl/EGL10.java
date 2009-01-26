/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
 * The EGL10 interface contains the Java(TM) programming language
 * bindings for EGL 1.0.
 *
 * <p> The documentation in this interface is normative with respect
 * to instance variable names and values, method names and signatures,
 * and exception behavior.  The remaining documentation is placed here
 * for convenience and does not replace the normative documentation
 * found in the EGL specification and relevant
 * extension specifications.  EGL documentation is available at the <a
 * href="http://www.khronos.org/opengles/spec">Khronos</a> web site.
 *
 * <p>Extensions may return values or allow arguments to take on
 * values other than those listed in this specification.
 * Implementations that provide a given extension must pass such values
 * to and from the underlying engine.
 *
 * <p>If a method throws an exception, the state of the underlying EGL
 * engine is left intact.
 *
 * <p> All OpenGL ES drawing (except to Pbuffer surfaces) must be
 * preceded by a call to
 * <code>eglWaitNative(EGL10.EGL_CORE_NATIVE_ENGINE, target)</code>
 * where <code>target</code> is a platform-specific object describing
 * the rendering target,
 * and followed by a call to <code>eglWaitGL()</code>. Between these
 * calls, the results of calls or calls to any other drawing API, such
 * as MIDP, JSR 184, <code>java.awt</code>, etc., are undefined.  When
 * drawing to an image, the results are not guaranteed to appear in
 * the image pixels until <code>eglWaitGL</code> has returned.
 *
 * <p> It is not required that calls to methods of this interface be
 * mapped one-to-one onto calls to functions in the underlying EGL
 * implementation.  Implementations may contain logic to manage
 * configurations, drawing surface access, threading issues, etc., as
 * required in order to integrate EGL-based APIs with other platform
 * drawing APIs. For example, an implementation that makes use of a
 * software back buffer may translate a call to
 * <code>EGL10.eglCreateWindowSurface</code> into a call to the native
 * function <code>eglCreatePixmapSurface</code> targetting the
 * buffer. Naturally, hardware-accelerated implementations should
 * endeavor to avoid such workarounds.
 *
 * <h4>Drawing to a Pbuffer</h4>
 *
 * <p> A Pbuffer is an invisible, possibly hardware-accelerated
 * buffer. Pbuffer surfaces are created using the
 * <code>EGL10.eglCreatePbufferSurface</code> method.  Pbuffers are
 * accessible only from EGL-based APIs, and follow the rules set out
 * in the EGL specification.  Pbuffers may be used in the same manner
 * on any Java platform.
 *
 * <p>The integration between EGL and specific Java ME platforms is as
 * follows.
 *
 * <h3>CLDC/MIDP</h3>
 *
 * <p>On the CLDC/MIDP platform, drawing can be performed to four
 * types of targets: <code>javax.microedition.lcdui.Canvas</code>,
 * <code>javax.microedition.lcdui.game.GameCanvas</code>, (mutable)
 * <code>javax.microedition.lcdui.Image</code>,
 * <!-- <code>javax.microedition.khronos.egl.EGLCanvas</code>,--> or to a
 * Pbuffer.
 *
 * <p>The <code>EGL_DEFAULT_DISPLAY</code> token is used to
 * specify a display.
 *
 * <h4>Drawing to a <code>Canvas</code> or <code>GameCanvas</code></h4>
 *
 * <p> A <code>Canvas</code> or <code>GameCanvas</code> is specified
 * as a drawing target using the <code>eglCreateWindowSurface</code>
 * method.  The <code>native_window</code> argument must be an
 * instance of <code>javax.microedition.lcdui.Graphics</code> that was
 * obtained directly from the argument to the
 * <code>Canvas.paint()</code> method or from the
 * <code>GameCanvas.getGraphics()</code> method.  A
 * <code>Graphics</code> instance obtained from the argument to
 * <code>Canvas.paint</code> may be reused in subsequent
 * <code>paint</code> calls targeting the same <code>Canvas</code>.
 *
 * <p> Drawing to a <code>Canvas</code> or <code>GameCanvas</code>
 * allows for mixing of different drawing APIs.
 *
 * <p> When drawing to a <code>Canvas</code> (that is not a
 * <code>GameCanvas</code>), drawing must take place entirely within
 * the scope of a system-generated call to the <code>Canvas</code>'s
 * <code>paint(Graphics)</code> method.  The results of drawing to a
 * <code>Canvas</code> outside the scope of such a call are undefined.
 *
 * <p> Calling <code>eglSwapBuffers</code> is equivalent to calling
 * <code>glFinish</code>, and does not affect the screen output.  The
 * normal <code>GameCanvas.flushGraphics</code> method is used to
 * control screen output.
 *
 * <p> The initial contents of the back buffer for a
 * <code>GameCanvas</code> are initialized to white, and calls to
 * <code>flushGraphics</code> do not alter the buffer contents.
 *
 * <pre>
 * import javax.microedition.lcdui.Display;
 * import javax.microedition.lcdui.Graphics;
 * import javax.microedition.lcdui.game.GameCanvas;
 * import javax.microedition.midlet.MIDlet;
 * import javax.microedition.khronos.egl.*;
 * import javax.microedition.khronos.opengles.*;
 * 
 * class MyGameCanvas extends GameCanvas {
 *   EGL11 egl;
 *   GL11 gl;
 *   Graphics midpGraphics;
 *   javax.microedition.m3g.Graphics3D m3gContext;
 * 
 *   MyGameCanvas(MIDlet thisMIDlet) {
 *     // This example doesn't require key events
 *     super(true);
 *
 *     // Get a Graphics instance for MIDP rendering
 *     // and to use in createWindowSurface
 *     this.midpGraphics = getGraphics();
 *
 *     // Show this GameCanvas on the display
 *     Display display = Display.getDisplay(thisMIDlet);
 *     display.setCurrent(this);
 *
 *     // Create an EGL instance
 *     this.egl = (EGL11)EGLContext.getEGL();
 *
 *     // Get the EGL display object and initialize EGL
 *     EGLDisplay eglDisplay = egl.eglGetDisplay(EGL11.EGL_DEFAULT_DISPLAY);
 *     int[] major_minor = new int[2];
 *     egl.eglInitialize(eglDisplay, major_minor);
 *     System.out.println("EGL revision: major = " + major_minor[0]);
 *     System.out.println("EGL revision: minor = " + major_minor[1]);
 *
 *     // Determine the number of available configurations
 *     int[] num_config = new int[1];
 *     egl.eglGetConfigs(eglDisplay, null, 0, num_config);
 *     System.out.println("There are " + num_config[0] + " configurations");
 *
 *     // Locate an 8/8/8 RGB configuration
 *     int[] configAttrs = { EGL11.EGL_RED_SIZE, 8,
 *                           EGL11.EGL_GREEN_SIZE, 8,
 *                           EGL11.EGL_BLUE_SIZE, 8,
 *                           EGL11.EGL_ALPHA_SIZE, EGL11.EGL_DONT_CARE,
 *                           EGL11.EGL_DEPTH_SIZE, EGL11.EGL_DONT_CARE,
 *                           EGL11.EGL_STENCIL_SIZE, EGL11.EGL_DONT_CARE,
 *                           EGL11.EGL_NONE
 *     };
 *
 *     // Grab the first matching config
 *     EGLConfig[] eglConfigs = new EGLConfig[1];
 *     egl.eglChooseConfig(eglDisplay, configAttrs,
 *                         eglConfigs, 1, num_config);
 *     EGLConfig eglConfig = eglConfigs[0];
 * 
 *     // Get a context for EGL rendering
 *     EGLContext eglContext =
 *       egl.eglCreateContext(eglDisplay, eglConfig,
 *                            EGL11.EGL_NO_CONTEXT, null);
 *
 *     // Get a GL object for rendering
 *     this.gl = (GL11)eglContext.getGL();
 *
 *     // Bind a window surface to the context
 *     EGLSurface eglSurface =
 *       egl.eglCreateWindowSurface(eglDisplay, eglConfig, midpGraphics, null);

 *     // Make the context current for future GL calls
 *     egl.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
 *
 *     // Get a context for M3G rendering
 *     this.m3g = ...;
 *   }
 * 
 *   // The update loop
 *   public void run() {
 *     while (true) {
 *       // Do some MIDP 2D rendering
 *       // Use of OpenGL ES or M3G is not allowed here
 *       // MIDP primitives drawn here will appear underneath the
 *       // OpenGL ES drawing
 *       midpGraphics.drawLine(...);
 *
 *       // Wait for MIDP drawing to complete
 *       egl.eglWaitNative(EGL11.EGL_CORE_NATIVE_ENGINE, midpGraphics);
 *       // Now it is O.K. to use OpenGL ES drawing APIs
 *
 *       // Do some OpenGL ES rendering
 *       // Use of MIDP, JSR 184, or other drawing APIs is undefined here
 *       gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
 *       gl.glClear(GL11.GL_COLOR_BUFFER_BIT | GL11.GL_DEPTH_BUFFER_BIT);
 *       gl.glFrustumf(...);
 *       // etc.
 *
 *       // Wait for OpenGL ES rendering to complete
 *       egl.eglWaitGL();
 *       // Now it is O.K. to use MIDP drawing APIs
 * 
 *       // MIDP primitives drawn here will appear on top of the
 *       // OpenGL ES drawing
 *       midpGraphics.drawRect(...);
 *
 *       // Do some M3G (JSR 184) rendering
 *       m3g.bindTarget(midpGraphics, ...);
 *       // Now it is O.K. to use M3G, but not MIDP or OpenGL ES
 *
 *       // M3G commands go here
 *
 *       // Wait for M3G drawing to complete
 *       m3g.releaseTarget();
 *       // Now it is O.K. to use MIDP drawing APIs
 * 
 *       // Do some more MIDP 2D rendering
 *       // MIDP primitives drawn here will appear on top of the
 *       // OpenGL ES and M3G drawings
 *       midp_graphics.drawArc(...);
 * 
 *       // Flush the back buffer to the screen
 *       flushGraphics();
 *     }
 *   }
 * }
 * </pre>
 *
 * <h4>Drawing to an <code>Image</code></h4>
 *
 * <p> The <code>eglCreatePixmapSurface</code> method allows drawing
 * to an <code>Image</code>.  The <code>Image</code> must be mutable.
 * The <code>native_pixmap</code> argument must be an instance of
 * <code>javax.microedition.lcdui.Graphics</code> that was obtained
 * from the <code>Image</code>'s <code>getGraphics()</code> method.
 *
 * <p> OpenGL ES drawing must be bracketed between calls to
 * <code>eglWaitNative</code> and <code>eglWaitGL</code> in the same
 * manner as for <code>Canvas</code> and <code>GameCanvas</code>.
 * Calling <code>eglSwapBuffers</code> is equivalent to calling
 * <code>glFinish</code>, since there is no back buffer.
 *
 * <!--
 * <h4>Drawing to an <code>EGLCanvas</code></h4>
 *
 * <p> <code>EGLCanvas</code> maximizes the potential for efficiency
 * by removing the requirement to allow non-OpenGL ES drawing.  The
 * results of drawing to an <code>EGLCanvas</code> using other non-EGL
 * based APIs such as MIDP and JSR 184 is undefined.
 *
 * <p> An <code>EGLCanvas</code> is specified as a drawing target
 * using the <code>eglCreateWindowSurface</code> method.  The
 * <code>native_window</code> argument must be an instance of
 * <code>javax.microedition.lcdui.Graphics</code> that was obtained
 * directly from the <code>EGLCanvas.getGraphics()</code> method.
 *
 * <p> When an <code>EGLCanvas</code> is set using the
 * <code>Display.setCurrent</code> method, it occupies as much of the
 * screen as is allowed by the platform (leaving room for a status bar
 * if the platform requires one).  OpenGL ES drawing is performed to a
 * back buffer, the contents of which are initially undefined. When
 * drawing is complete, a call to <code>eglSwapBuffers</code> causes
 * the contents of the back buffer to be copied to the
 * screen. Following the call, the back buffer contents are once again
 * undefined.
 * -->
 *
 * <h3>Other platforms</h3>
 *
 * <p> The current specification does not address AWT-based platforms
 * such as CDC/Personal Basis Profile, CDC/Personal Profile,
 * and Java Standard Edition.  The details of how to bind to displays
 * and surfaces on these platforms will be provided in a future release
 * of the specification.
 */
public interface EGL10 extends EGL {

  /**
   * An object that is used as an argument to
   * <code>eglGetDisplay</code> to indicate that the defalt display of
   * the device is to be used.
   */
  public static final Object EGL_DEFAULT_DISPLAY =
      new Object();

  /**
   * An <code>EGLContext</code> object used to indicate a null context.
   */
  public static final EGLContext EGL_NO_CONTEXT =
      EGLContextImpl.getInstance(0);

  /**
   * An <code>EGLContext</code> object used to indicate a null display.
   */
  public static final EGLDisplay EGL_NO_DISPLAY =
      EGLDisplayImpl.getInstance(0);

  /**
   * An <code>EGLContext</code> object used to indicate a null surface.
   */
  public static final EGLSurface EGL_NO_SURFACE =
      EGLSurfaceImpl.getInstance(0); // C #define of EGL_NO_SURFACE
  
  // Boolean

  /**
   * A value corresponding to the 'EGLBoolean' false value.
   */
  public static final int EGL_FALSE                   = 0;

  /**
   * A value corresponding to the 'EGLBoolean' true value.
   */
  public static final int EGL_TRUE                    = 1;
  
  // Errors

  /**
   * EGL error code indicating success.
   */
  public static final int EGL_SUCCESS                 = 0x3000;

  /**
   * EGL error code indicating 'not initialized'.
   */
  public static final int EGL_NOT_INITIALIZED         = 0x3001;

  /**
   * EGL error code indicating 'bad access'.
   */
  public static final int EGL_BAD_ACCESS              = 0x3002;

  /**
   * EGL error code indicating 'bad alloc'.
   */
  public static final int EGL_BAD_ALLOC               = 0x3003;

  /**
   * EGL error code indicating 'bad attribute'.
   */
  public static final int EGL_BAD_ATTRIBUTE           = 0x3004;

  /**
   * EGL error code indicating 'bad config'.
   */
  public static final int EGL_BAD_CONFIG              = 0x3005;

  /**
   * EGL error code indicating 'bad context'.
   */
  public static final int EGL_BAD_CONTEXT             = 0x3006;

  /**
   * EGL error code indicating 'bad current surface'.
   */
  public static final int EGL_BAD_CURRENT_SURFACE     = 0x3007;

  /**
   * EGL error code indicating 'bad display'.
   */
  public static final int EGL_BAD_DISPLAY             = 0x3008;

  /**
   * EGL error code indicating 'bad match'.
   */
  public static final int EGL_BAD_MATCH               = 0x3009;

  /**
   * EGL error code indicating 'bad native pixmap'.
   */
  public static final int EGL_BAD_NATIVE_PIXMAP       = 0x300A;

  /**
   * EGL error code indicating 'bad native window'.
   */
  public static final int EGL_BAD_NATIVE_WINDOW       = 0x300B;

  /**
   * EGL error code indicating 'bad parameter'.
   */
  public static final int EGL_BAD_PARAMETER           = 0x300C;

  /**
   * EGL error code indicating 'bad surface'.
   */
  public static final int EGL_BAD_SURFACE             = 0x300D;
  
  // Config attributes

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_BUFFER_SIZE             = 0x3020;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_ALPHA_SIZE              = 0x3021;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_BLUE_SIZE               = 0x3022;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_GREEN_SIZE              = 0x3023;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_RED_SIZE                = 0x3024;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_DEPTH_SIZE              = 0x3025;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_STENCIL_SIZE            = 0x3026;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_CONFIG_CAVEAT           = 0x3027;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_CONFIG_ID               = 0x3028;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_LEVEL                   = 0x3029;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_MAX_PBUFFER_HEIGHT      = 0x302A;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_MAX_PBUFFER_PIXELS      = 0x302B;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_MAX_PBUFFER_WIDTH       = 0x302C;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_NATIVE_RENDERABLE       = 0x302D;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_NATIVE_VISUAL_ID        = 0x302E;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_NATIVE_VISUAL_TYPE      = 0x302F;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_PRESERVED_RESOURCES  = 0x3030;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_SAMPLES                 = 0x3031;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_SAMPLE_BUFFERS          = 0x3032;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_SURFACE_TYPE            = 0x3033;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_TRANSPARENT_TYPE        = 0x3034;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_TRANSPARENT_BLUE_VALUE  = 0x3035;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_TRANSPARENT_GREEN_VALUE = 0x3036;

  /**
   * <code>EGLConfig</code> attribute name.
   */
  public static final int EGL_TRANSPARENT_RED_VALUE   = 0x3037;

  // Config atribute and value

  /**
   * <code>EGLConfig</code> attribute name and value.
   */
  public static final int EGL_NONE                    = 0x3038;

  // Config values

  /**
   * <code>EGLConfig</code> attribute value.
   */
  public static final int EGL_DONT_CARE               = -1;

  /**
   * <code>EGLConfig</code> attribute value.
   */
  public static final int EGL_PBUFFER_BIT             = 0x01;

  /**
   * <code>EGLConfig</code> attribute value.
   */
  public static final int EGL_PIXMAP_BIT              = 0x02;

  /**
   * <code>EGLConfig</code> attribute value.
   */
  public static final int EGL_WINDOW_BIT              = 0x04;

  /**
   * <code>EGLConfig</code> attribute value.
   */
  public static final int EGL_SLOW_CONFIG             = 0x3050;

  /**
   * <code>EGLConfig</code> attribute value.
   */
  public static final int EGL_NON_CONFORMANT_CONFIG   = 0x3051;

  /**
   * <code>EGLConfig</code> attribute value.
   */
  public static final int EGL_TRANSPARENT_RGB         = 0x3052;

  // String names

  /**
   * Constant for use in <code>eglQueryString</code>.
   */
  public static final int EGL_VENDOR                  = 0x3053;

  /**
   * Constant for use in <code>eglQueryString</code>.
   */
  public static final int EGL_VERSION                 = 0x3054;

  /**
   * Constant for use in <code>eglQueryString</code>.
   */
  public static final int EGL_EXTENSIONS              = 0x3055;
    
  // Surface attributes

  /**
   * <code>EGLSurface</code> attribute name.
   */
  public static final int EGL_HEIGHT                  = 0x3056;

  /**
   * <code>EGLSurface</code> attribute name.
   */
  public static final int EGL_WIDTH                   = 0x3057;

  /**
   * <code>EGLSurface</code> attribute name.
   */
  public static final int EGL_LARGEST_PBUFFER         = 0x3058;

  // Current surfaces

  /**
   * Constant for use in the <code>readdraw</code> argument of
   * <code>getCurrentSurface</code>.
   */
  public static final int EGL_DRAW                    = 0x3059;

  /**
   * Constant for use as the <code>readdraw</code> argument of
   * <code>getCurrentSurface</code>.
   */
  public static final int EGL_READ                    = 0x305A;

  // Engines

  /**
   * Constant for use as the <code>engine</code> argument of
   * <code>eglWaitNative</code>, indicating the core native engine of
   * the platform.
   *
   * <p>On a JME CLDC/MIDP platform, this specifies the MIDP/LCDUI
   * drawing engine.
   *
   * <!-- <p>On other JME or JSE platforms, this specifies the AWT/Java2D
   * drawing engine. -->
   */
  public static final int EGL_CORE_NATIVE_ENGINE      = 0x305B;

  // Functions
  
  /**
   * Return error information.
   *
   * <p><code>eglGetError</code> returns the error of the last called EGL
   * function in the current thread. Initially, the error is set to
   * <code>EGL_SUCCESS</code>.
   *
   * <p>The following errors are currently defined:
   *
   * <ul>
   *
   * <li><code>EGL_SUCCESS</code></li>
   *
   * <p>The last function succeeded without error.
   *
   * <li><code>EGL_NOT_INITIALIZED</code></li>
   *
   * <p>EGL is not initialized, or could not be initialized, for the
   * specified EGL display connection.
   *
   * <li><code>EGL_BAD_ACCESS</code></li>
   *
   * <p>EGL cannot access a requested resource (for example a context
   * is bound in another thread).
   *
   * <li><code>EGL_BAD_ALLOC</code></li>
   *
   * <p>EGL failed to allocate resources for the requested operation.
   *
   * <li><code>EGL_BAD_ATTRIBUTE</code></li>
   *
   * <p>An unrecognized attribute or attribute value was passed in
   * the attribute list.
   *
   * <li><code>EGL_BAD_CONTEXT</code></li>
   *
   * <p>An <code>EGLContext</code> argument does not name a valid EGL
   * rendering context.
   *
   * <li><code>EGL_BAD_CONFIG</code></li>
   *
   * <p>An <code>EGLConfig</code> argument does not name a valid EGL
   * frame buffer configuration.
   *
   * <li><code>EGL_BAD_CURRENT_SURFACE</code></li>
   *
   * <p>The current surface of the calling thread is a window, pixel
   * buffer or pixmap that is no longer valid.
   *
   * <li><code>EGL_BAD_DISPLAY</code></li>
   *
   * <p>An <code>EGLDisplay</code> argument does not name a valid EGL
   * display connection.
   *
   * <li><code>EGL_BAD_SURFACE</code></li>
   *
   * <p>An <code>EGLSurface</code> argument does not name a valid
   * surface (window, pixel buffer or pixmap) configured for GL
   * rendering.
   *
   * <li><code>EGL_BAD_MATCH</code></li>
   *
   * <p>Arguments are inconsistent (for example, a valid context
   * requires buffers not supplied by a valid surface).
   *
   * <li><code>EGL_BAD_PARAMETER</code></li>
   *
   * <p>One or more argument values are invalid.
   *
   * <li><code>EGL_BAD_NATIVE_PIXMAP</code></li>
   *
   * <p>A <code>native_pixmap</code> argument does not refer to a
   * valid native pixmap.
   *
   * <li><code>EGL_BAD_NATIVE_WINDOW</code></li>
   *
   * <p>A <code>native_window</code> argument does not refer to a
   * valid native window.
   *
   * <li><code>EGL_CONTEXT_LOST</code> (1.1 only)</li>
   *
   * <p>A power management event has occurred. The application must
   * destroy all contexts and reinitialise OpenGL ES state and objects
   * to continue rendering.
   *
   * </ul>
   *
   * <h4>Errors</h4>
   *
   * <p>A call to <code>eglGetError</code> sets the error to
   * <code>EGL_SUCCESS</code>.
   *
   * @return One of <code>EGL_SUCCESS</code>,
   * <code>EGL_NOT_INITIALIZED</code>, <code>EGL_BAD_ACCESS</code>,
   * <code>EGL_BAD_ALLOC</code>, <code>EGL_BAD_ATTRIBUTE</code>,
   * <code>EGL_BAD_CONTEXT</code>, <code>EGL_BAD_CONFIG</code>,
   * <code>EGL_BAD_CURRENT_SURFACE</code>,
   * <code>EGL_BAD_DISPLAY</code>, <code>EGL_BAD_MATCH</code>,
   * <code>EGL_BAD_NATIVE_PIXMAP</code>,
   * <code>EGL_BAD_NATIVE_WINDOW</code>,
   * <code>EGL_BAD_PARAMETER</code>, <code>EGL_BAD_SURFACE</code>, or
   * other error code defined by an extension.
   */
  int eglGetError();

  /**
   * Return an EGL display connection.
   *
   * <p><code>eglGetDisplay</code> obtains the EGL display connection
   * for the native display <code>native_display</code>.
   *
   * <p>If <code>display_id</code> is
   * <code>EGL_DEFAULT_DISPLAY</code>, a default display connection is
   * returned.
   *
   * <p>If no display connection matching <code>native_display</code>
   * is available, <code>EGL_NO_DISPLAY</code> is returned. No error
   * is generated.
   *
   * <p>Use <code>eglInitialize</code> to initialize the display
   * connection.
   *
   * @param native_display Specifies the display to connect
   * to. <code>EGL_DEFAULT_DISPLAY</code> indicates the default
   * display.
   *
   * @return An <code>EGLDisplay</code> object.
   *
   * @exception IllegalArgumentException if <code>native_display</code> is
   * <code>null</code> or is not of a suitable type for the platform.
   *
   */
  EGLDisplay eglGetDisplay(Object native_display);

  /**
   * Initialize an EGL display connection.
   *
   * <p><code>eglInitialize</code> initializes the EGL display
   * connection obtained with <code>eglGetDisplay</code>. Initializing
   * an already initialized EGL display connection has no effect
   * besides returning the version numbers and returing
   * <code>true</code>.
   *
   * <p>No value is returned in <code>major_minor</code> if it is specified as
   * <code>null</code>.
   *
   * <p>Use <code>eglTerminate</code> to release resources associated
   * with an EGL display connection.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned if <code>eglInitialize</code>
   * fails, <code>true</code> otherwise. <code>major_minor</code> is
   * not modified when <code>false</code> is returned.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if
   * <code>display</code> is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> cannot be initialized.
   *
   * @param display Specifies the EGL display connection to initialize.
   * @param major_minor an <code>int</code> array of length at least
   * 2, in which the major and minor version number of the EGL
   * implementation will be returned as elements 0 and 1. May be
   * <code>null</code>.
   *
   * @return <code>true</code> if the operation succeeds.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>major_minor</code> is
   * non-<code>null</code> but has length less than 2.
   */
  boolean eglInitialize(EGLDisplay display, int[] major_minor);

  /**
   * Terminate an EGL display connection.
   *
   * <p><code>eglTerminate</code> releases resources associated with
   * an EGL display connection. Termination marks all EGL resources
   * associated with the EGL display connection for deletion. If
   * contexts or surfaces associated with <code>display</code> is
   * current to any thread, they are not released until they are no
   * longer current as a result of <code>eglMakeCurrent</code>.
   *
   * <p>Terminating an already terminated EGL display connection has
   * no effect other than returning <code>true</code>. A terminated
   * display may be re-initialized by calling
   * <code>eglInitialize</code> again.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned if <code>eglTerminate</code>
   * fails, <code>true</code> otherwise.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if display is not an
   * EGL display connection.
   *
   * @param display Specifies the EGL display connection to terminate.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @return <code>true</code> if the operation succeeds.
   */
  boolean eglTerminate(EGLDisplay display);

  /**
   * Return a string describing an EGL display connection.
   *
   * <p><code>eglQueryString</code> returns a <code>String</code>
   * describing an EGL display connection. <code>name</code> can be
   * one of the following:
   *
   * <ul>
   *
   * <li><code>EGL_VENDOR</code></li>
   *
   * <p>Returns the company responsible for this EGL implementation. This
   * name does not change from release to release.
   *
   * <li><code>EGL_VERSION</code></li>
   *
   * <p>Returns a version or release number. The <code>EGL_VERSION</code>
   * string is laid out as follows:
   *
   * <p><code>major_version.minor_version space vendor_specific_info</code>
   *
   * <li><code>EGL_EXTENSIONS</code></li>
   *
   * <p>Returns a space-separated list of supported extensions to EGL.
   *
   * </ul>
   *
   * <p>The string data returned from the native EGL implementation is
   * converted into UTF8 format and returned as a Java
   * <code>String</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>null</code> is returned on failure.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if
   * <code>display</code> is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_PARAMETER</code> is generated if
   * <code>name</code> is not an accepted value.
   *
   * @param display Specifies the EGL display connection.
   * @param name Specifies a symbolic constant, one of
   * <code>EGL_VENDOR</code>, <code>EGL_VERSION</code>, or
   * <code>EGL_EXTENSIONS</code>.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @return A <code>String</code> containing the query result.
   */
  String eglQueryString(EGLDisplay display, int name);
 
  /**
   * Return a list of all EGL frame buffer configurations for a display.
   *
   * <p><code>eglGetConfigs</code> returns a list of all EGL frame
   * buffer configurations that are available for the specified
   * display. The items in the list can be used in any EGL function
   * that requires an EGL frame buffer configuration. No more than
   * <code>config_size</code> <code>EGLConfig</code>s will be returned
   * even if more are available on the specified display.
   *
   * <p><code>configs</code> does not return values, if it is specified
   * as <code>null</code>. This is useful for querying just the number
   * of all frame buffer configurations.
   *
   * <p>Use <code>eglGetConfigAttrib</code> to retrieve individual
   * attribute values of a frame buffer configuration.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned on failure, <code>true</code>
   * otherwise. <code>configs</code> and <code>num_config</code> are
   * not modified when <code>false</code> is returned.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if
   * <code>display</code> is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_PARAMETER</code> is generated if
   * <code>num_config</code> is <code>null</code>.
   *
   * @param display Specifies the EGL display connection.
   * @param configs An array of <code>EGLConfig</code> objects into
   * which a list of configs will be stored, or <code>null</code>.
   * @param config_size Specifies the size of the list of configs.
   * @param num_config An <code>int</code> array, in which the number
   * of configs will be returned in element 0.
   *
   * @return <code>true</code> if the operation succeeds.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>configs</code> is
   * non-<code>null</code> and <code>configs.length</code> is smaller
   * than <code>config_size</code>.
   * @exception IllegalArgumentException if <code>num_config</code> is
   * non-<code>null</code> but has length less than 1.
   */
  boolean eglGetConfigs(EGLDisplay display,
			EGLConfig[] configs,
    			int config_size,
    			int[] num_config);
    
  /**
   * Return a list of EGL frame buffer configurations that match
   * specified attributes.
   *
   * <p><code>eglChooseConfig</code> returns a list of all EGL frame
   * buffer configurations that match the attributes specified in
   * <code>attrib_list</code>. The items in the list can be used in
   * any EGL function that requires an EGL frame buffer configuration.
   *
   * <p><code>configs</code> does not return values, if it is specified
   * as <code>null</code>. This is useful for querying just the number
   * of matching frame buffer configurations.
   *
   * <p>All attributes in <code>attrib_list</code>, including boolean
   * attributes, are immediately followed by the corresponding desired
   * value. The list is terminated with <code>EGL_NONE</code>. If an
   * attribute is not specified in <code>attrib_list</code> then the
   * default value (see below) is used (and the attribute is said to
   * be specified implicitly). For example, if
   * <code>EGL_DEPTH_SIZE</code> is not specified then it is assumed
   * to be 0. For some attributes, the default is
   * <code>EGL_DONT_CARE</code> meaning that any value is OK for this
   * attribute, so the attribute will not be checked.
   *
   * <p> If <code>attrib_list</code> is <code>null</code> or empty
   * (first attribute is <code>EGL_NONE</code>), then selection and
   * sorting of <code>EGLConfig</code>s is done according to the
   * default criteria described below.
   *
   * <p> Attributes are matched in an attribute-specific manner. Some of
   * the attributes, such as <code>EGL_LEVEL</code>, must match the
   * specified value exactly. Others, such as,
   * <code>EGL_RED_SIZE</code> must meet or exceed the specified
   * minimum values. If more than one EGL frame buffer configuration
   * is found, then a list of configurations, sorted according to the
   * "best" match criteria, is returned. The match criteria for each
   * attribute and the exact sorting order is defined below.
   *
   * <p>The interpretations of the various EGL frame buffer configuration
   * attributes are as follows:
   *
   * <ul>
   *
   * <li><code>EGL_BUFFER_SIZE</code></li>
   *
   * <p>Must be followed by a nonnegative integer that indicates the
   * desired color buffer size. The smallest color buffer of at least
   * the specified size is preferred. The default value is 0.
   *
   * <li><code>EGL_RED_SIZE</code></li>
   *
   * <p>Must be followed by a nonnegative minimum size specification. If
   * this value is zero, the smallest available red buffer is
   * preferred. Otherwise, the largest available red buffer of at
   * least the minimum size is preferred. The default value is 0.
   *
   * <li><code>EGL_GREEN_SIZE</code></li>
   *
   * <p>Must be followed by a nonnegative minimum size specification. If
   * this value is zero, the smallest available green buffer is
   * preferred. Otherwise, the largest available green buffer of at
   * least the minimum size is preferred. The default value is 0.
   *
   * <li><code>EGL_BLUE_SIZE</code></li>
   *
   * <p>Must be followed by a nonnegative minimum size specification. If
   * this value is zero, the smallest available blue buffer is
   * preferred. Otherwise, the largest available blue buffer of at
   * least the minimum size is preferred. The default value is 0.
   *
   * <li><code>EGL_ALPHA_SIZE</code></li>
   *
   * <p>Must be followed by a nonnegative minimum size specification. If
   * this value is zero, the smallest available alpha buffer is
   * preferred. Otherwise, the largest available alpha buffer of at
   * least the minimum size is preferred. The default value is 0.
   *
   * <li><code>EGL_CONFIG_CAVEAT</code></li>
   *
   * <p>Must be followed by one of <code>EGL_DONT_CARE</code>,
   * <code>EGL_NONE</code>, <code>EGL_SLOW_CONFIG</code>,
   * <code>EGL_NON_CONFORMANT_CONFIG</code>. If <code>EGL_NONE</code>
   * is specified, then only frame buffer configurations with no
   * caveats will be considered. If <code>EGL_SLOW_CONFIG</code> is
   * specified, then only slow frame buffer configurations will be
   * considered. If <code>EGL_NON_CONFORMANT_CONFIG</code> is
   * specified, then only non-conformant frame buffer configurations
   * will be considered. The default value is
   * <code>EGL_DONT_CARE</code>.
   *
   * <li><code>EGL_CONFIG_ID</code></li>
   *
   * <p>Must be followed by a valid ID that indicates the desired EGL
   * frame buffer configuration. When a <code>EGL_CONFIG_ID</code> is
   * specified, all attributes are ignored. The default value is
   * <code>EGL_DONT_CARE</code>.
   *
   * <li><code>EGL_DEPTH_SIZE</code></li>
   *
   * <p>Must be followed by a nonnegative integer that indicates the
   * desired depth buffer size. The smallest available depth buffer of
   * at least the minimum size is preferred. If the desired value is
   * zero, frame buffer configurations with no depth buffer are
   * preferred. The default value is 0.
   *
   * <li><code>EGL_LEVEL</code></li>
   *
   * <p>Must be followed by an integer buffer-level specification. This
   * specification is honored exactly. Buffer level 0 corresponds to
   * the default frame buffer of the display. Buffer level 1 is the
   * first overlay frame buffer, level two the second overlay frame
   * buffer, and so on. Negative buffer levels correspond to underlay
   * frame buffers. The default value is 0.
   *
   * <li><code>EGL_NATIVE_RENDERABLE</code></li>
   *
   * <p>Must be followed by <code>EGL_DONT_CARE</code>,
   * <code>EGL_TRUE</code>, or <code>EGL_FALSE</code>. If
   * <code>EGL_TRUE</code> is specified, then only frame buffer
   * configurations that allow native rendering into the surface will
   * be considered. The default value is <code>EGL_DONT_CARE</code>.
   *
   * <li><code>EGL_NATIVE_VISUAL_TYPE</code> (1.0 only)</li>
   *
   * <p>Must be followed by a platform dependent value or
   * <code>EGL_DONT_CARE</code>. The default value is
   * <code>EGL_DONT_CARE</code>.
   *
   * <li><code>EGL_SAMPLE_BUFFERS</code></li>
   *
   * <p>Must be followed by the minimum acceptable number of multisample
   * buffers. Configurations with the smallest number of multisample
   * buffers that meet or exceed this minimum number are
   * preferred. Currently operation with more than one multisample
   * buffer is undefined, so only values of zero or one will produce a
   * match. The default value is 0.
   *
   * <li><code>EGL_SAMPLES</code></li>
   *
   * <p>Must be followed by the minimum number of samples required in
   * multisample buffers. Configurations with the smallest number of
   * samples that meet or exceed the specified minimum number are
   * preferred. Note that it is possible for color samples in the
   * multisample buffer to have fewer bits than colors in the main
   * color buffers. However, multisampled colors maintain at least as
   * much color resolution in aggregate as the main color buffers.
   *
   * <li><code>EGL_STENCIL_SIZE</code></li>
   *
   * <p>Must be followed by a nonnegative integer that indicates the
   * desired number of stencil bitplanes. The smallest stencil buffer
   * of at least the specified size is preferred. If the desired value
   * is zero, frame buffer configurations with no stencil buffer are
   * preferred. The default value is 0.
   *
   * <li><code>EGL_SURFACE_TYPE</code></li>
   *
   * <p>Must be followed by a mask indicating which EGL surface types
   * the frame buffer configuration must support. Valid bits are
   * <code>EGL_WINDOW_BIT</code>, <code>EGL_PBUFFER_BIT</code>, and
   * <code>EGL_PIXMAP_BIT</code>. For example, if <code>mask</code> is
   * set to <code>EGL_WINDOW_BIT | EGL_PIXMAP_BIT</code>,
   * only frame buffer configurations that support both windows and
   * pixmaps will be considered. The default value is
   * <code>EGL_WINDOW_BIT</code>.
   *
   * <li><code>EGL_TRANSPARENT_TYPE</code></li>
   *
   * <p>Must be followed by one of <code>EGL_NONE</code> or
   * <code>EGL_TRANSPARENT_RGB</code>. If <code>EGL_NONE</code> is
   * specified, then only opaque frame buffer configurations will be
   * considered. If <code>EGL_TRANSPARENT_RGB</code> is specified,
   * then only transparent frame buffer configurations will be
   * considered. The default value is <code>EGL_NONE</code>.
   *
   * <li><code>EGL_TRANSPARENT_RED_VALUE</code></li>
   *
   * <p>Must be followed by an integer value indicating the transparent
   * red value. The value must be between 0 and the maximum color
   * buffer value for red. Only frame buffer configurations that use
   * the specified transparent red value will be considered. The
   * default value is <code>EGL_DONT_CARE</code>.
   *
   * <p>This attribute is ignored unless
   * <code>EGL_TRANSPARENT_TYPE</code> is included in
   * <code>attrib_list</code> and specified as
   * <code>EGL_TRANSPARENT_RGB</code>.
   *
   * <li><code>EGL_TRANSPARENT_GREEN_VALUE</code></li>
   *
   * <p>Must be followed by an integer value indicating the transparent
   * green value. The value must be between 0 and the maximum color
   * buffer value for red. Only frame buffer configurations that use
   * the specified transparent green value will be considered. The
   * default value is <code>EGL_DONT_CARE</code>.
   *
   * <p>This attribute is ignored unless
   * <code>EGL_TRANSPARENT_TYPE</code> is included in
   * <code>attrib_list</code> and specified as
   * <code>EGL_TRANSPARENT_RGB</code>.
   *
   * <li><code>EGL_TRANSPARENT_BLUE_VALUE</code></li>
   *
   * <p>Must be followed by an integer value indicating the transparent
   * blue value. The value must be between 0 and the maximum color
   * buffer value for red. Only frame buffer configurations that use
   * the specified transparent blue value will be considered. The
   * default value is <code>EGL_DONT_CARE</code>.
   *
   * <p>This attribute is ignored unless
   * <code>EGL_TRANSPARENT_TYPE</code> is included in
   * <code>attrib_list</code> and specified as
   * <code>EGL_TRANSPARENT_RGB</code>.
   *
   * <li><code>EGL_BIND_TO_TEXTURE_RGB</code> (1.1 only)</li>
   *
   * <p>Must be followed by <code>EGL_DONT_CARE</code>,
   * <code>EGL_TRUE</code>, or <code>EGL_FALSE</code>. If
   * <code>EGL_TRUE</code> is specified, then only frame buffer
   * configurations that support binding of color buffers to an RGB
   * texture will be considered. Currently only frame buffer
   * configurations that support pbuffers allow this. The default
   * value is <code>EGL_DONT_CARE</code>.
   *
   * <li><code>EGL_BIND_TO_TEXTURE_RGBA</code> (1.1 only)></li>
   *
   * <p>Must be followed by <code>EGL_DONT_CARE</code>,
   * <code>EGL_TRUE</code>, or <code>EGL_FALSE</code>. If
   * <code>EGL_TRUE</code> is specified, then only frame buffer
   * configurations that support binding of color buffers to an RGBA
   * texture will be considered. Currently only frame buffer
   * configurations that support pbuffers allow this. The default
   * value is <code>EGL_DONT_CARE</code>.
   *
   * <li><code>EGL_MAX_SWAP_INTERVAL</code> (1.1 only)</li>
   *
   * <p>Must be followed by a integer that indicates the maximum value
   * that can be passed to <code>eglSwapInterval</code>. The default
   * value is <code>EGL_DONT_CARE</code>.
   *
   * <li><code>EGL_MIN_SWAP_INTERVAL</code> (1.1 only)</li>
   *
   * <p>Must be followed by a integer that indicates the minimum value
   * that can be passed to <code>eglSwapInterval</code>. The default
   * value is <code>EGL_DONT_CARE</code>.
   *
   * </ul>
   *
   * <p>When more than one EGL frame buffer configuration matches the
   * specified attributes, a list of matching configurations is
   * returned. The list is sorted according to the following
   * precedence rules, which are applied in ascending order (i.e.,
   * configurations that are considered equal by a lower numbered rule
   * are sorted by the higher numbered rule):
   *
   * <ol>
   *
   * <li>By <code>EGL_CONFIG_CAVEAT</code>, where the precedence is
   * <code>EGL_NONE</code>, <code>EGL_SLOW_CONFIG</code>, and
   * <code>EGL_NON_CONFORMANT_CONFIG</code>.</li>
   *
   * <li>Larger total number of color components
   * (<code>EGL_RED_SIZE</code>, <code>EGL_GREEN_SIZE</code>,
   * <code>EGL_BLUE_SIZE</code>, and <code>EGL_ALPHA_SIZE</code>) that
   * have higher number of bits. If the requested number of bits in
   * <code>attrib_list</code> is zero or <code>EGL_DONT_CARE</code>
   * for a particular color component, then the number of bits for
   * that component is not considered.</li>
   *
   * <li>Smaller <code>EGL_BUFFER_SIZE</code>.</li>
   *
   * <li>Smaller <code>EGL_SAMPLE_BUFFERS</code>.</li>
   *
   * <li>Smaller <code>EGL_SAMPLES</code>.</li>
   *
   * <li>Smaller <code>EGL_DEPTH_SIZE</code>.</li>
   *
   * <li>Smaller <code>EGL_STENCIL_SIZE</code>.</li>
   *
   * <li>By <code>EGL_NATIVE_VISUAL_TYPE</code>, where the precedence
   * order is platform dependent.</li>
   *
   * <li>Smaller <code>EGL_CONFIG_ID</code>.</li>
   *
   * </ol>
   *
   * <p>(1.1) <code>EGLConfigs</code> are not sorted with respect to the
   * parameters <code>EGL_BIND_TO_TEXTURE_RGB</code>,
   * <code>EGL_BIND_TO_TEXTURE_RGBA</code>, <code>EGL_LEVEL</code>,
   * <code>EGL_NATIVE_RENDERABLE</code>,
   * <code>EGL_MAX_SWAP_INTERVAL</code>,
   * <code>EGL_MIN_SWAP_INTERVAL</code>,
   * <code>EGL_SURFACE_TYPE</code>, <code>EGL_TRANSPARENT_TYPE</code>,
   * <code>EGL_TRANSPARENT_RED_VALUE</code>,
   * <code>EGL_TRANSPARENT_GREEN_VALUE</code>, and
   * <code>EGL_TRANSPARENT_BLUE_VALUE</code>.
   *
   * <h4>Examples</h4>
   *
   * <p>The following example specifies a frame buffer configuration in
   * the normal frame buffer (not an overlay or underlay). The
   * returned frame buffer configuration supports at least 4 bits each
   * of red, green and blue and possible no alpha bits. The code shown
   * in the example may or may not have a depth buffer, or a stencil
   * buffer.
   *
   * <pre>
   * int attrib_list[] = {
   *     EGL10.EGL_RED_SIZE, 4,
   *     EGL10.EGL_GREEN_SIZE, 4,
   *     EGL10.EGL_BLUE_SIZE, 4,
   *     EGL10.EGL_NONE
   * };
   * </pre>
   *
   * <h4>Notes</h4>
   *
   * <p><code>eglGetConfigs</code> and <code>eglGetConfigAttrib</code> can be
   * used to implement selection algorithms other than the generic one
   * implemented by <code>eglChooseConfig</code>. Call
   * <code>eglGetConfigs</code> to retrieve all the frame buffer
   * configurations, or alternatively, all the frame buffer configurations
   * with a particular set of attributes. Next call
   * <code>eglGetConfigAttrib</code> to retrieve additional attributes for
   * the frame buffer configurations and then select between them.
   *
   * <p>EGL implementors are strongly discouraged, but not proscribed, from
   * changing the selection algorithm used by
   * <code>eglChooseConfig</code>. Therefore, selections may change from
   * release to release of the client-side library.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned on failure, <code>true</code>
   * otherwise. <code>configs</code> and <code>num_config</code> are
   * not modified when <code>false</code> is returned.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if
   * <code>display</code> is not an EGL display connection.
   *
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attribute_list</code> contains an invalid frame buffer
   * configuration attribute or an attribute value that is
   * unrecognized or out of range.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_PARAMETER</code> is generated if
   * <code>num_config</code> is <code>null</code>.
   *
   * @param display Specifies the EGL display connection.
   * @param attrib_list Specifies attributes required to match by configs.
   * @param configs Returns an array of frame buffer configurations.
   * @param config_size Specifies the size of the array of frame
   * buffer configurations.
   * @param num_config An <code>int</code> array in which the number of frame
   * buffer configurations will be returned in element 0.
   * @return <code>true</code> if the operation succeeds.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>attrib_list</code> is
   * non-<code>null</code> but is not terminated with
   * <code>EGL_NONE</code>.
   * @exception IllegalArgumentException if <code>configs</code> is
   * non-<code>null</code> and <code>configs.length</code> is smaller
   * than <code>config_size</code>.
   * @exception IllegalArgumentException if <code>num_config</code> is
   * non-<code>null</code> but has length less than 1.
   */
  boolean eglChooseConfig(EGLDisplay display,
    			  int[] attrib_list,
    			  EGLConfig[] configs,
    			  int config_size,
    			  int[] num_config);
   
  /**
   * Return information about an EGL frame buffer configuration.
   *
   * <p><code>eglGetConfigAttrib</code> returns in <code>value[0]</code> the
   * value of <code>attribute</code> for
   * <code>config</code>. <code>attribute</code> can be one of the
   * following:
   *
   * <ul>
   *
   * <li><code>EGL_BUFFER_SIZE</code></li>
   *
   * <p>Returns the depth of the color buffer. It is the sum of
   * <code>EGL_RED_SIZE</code>, <code>EGL_GREEN_SIZE</code>,
   * <code>EGL_BLUE_SIZE</code>, and <code>EGL_ALPHA_SIZE</code>.
   *
   * <li><code>EGL_RED_SIZE</code></li>
   *
   * <p>Returns the number of bits of red stored in the color buffer.
   *
   * <li><code>EGL_GREEN_SIZE</code></li>
   *
   * <p>Returns the number of bits of green stored in the color buffer.
   *
   * <li><code>EGL_BLUE_SIZE</code></li>
   *
   * <p>Returns the number of bits of blue stored in the color buffer.
   *
   * <li><code>EGL_ALPHA_SIZE</code></li>
   *
   * <p>Returns the number of bits of alpha stored in the color buffer.
   *
   * <li><code>EGL_CONFIG_CAVEAT</code></li>
   *
   * <p>Returns the caveats for the frame buffer configuration. Possible
   * caveat values are <code>EGL_NONE</code>,
   * <code>EGL_SLOW_CONFIG</code>, and
   * <code>EGL_NON_CONFORMANT</code>.
   *
   * <li><code>EGL_CONFIG_ID</code></li>
   *
   * <p>Returns the ID of the frame buffer configuration.
   *
   * <li><code>EGL_DEPTH_SIZE</code></li>
   *
   * <p>Returns the number of bits in the depth buffer.
   *
   * <li><code>EGL_LEVEL</code></li>
   *
   * <p>Returns the frame buffer level. Level zero is the default frame
   * buffer. Positive levels correspond to frame buffers that overlay
   * the default buffer and negative levels correspond to frame
   * buffers that underlay the default buffer.
   *
   * <li><code>EGL_MAX_PBUFFER_WIDTH</code></li>
   *
   * <p>Returns the maximum width of a pixel buffer surface in pixels.
   *
   * <li><code>EGL_MAX_PBUFFER_HEIGHT</code></li>
   *
   * <p>Returns the maximum height of a pixel buffer surface in pixels.
   *
   * <li><code>EGL_MAX_PBUFFER_PIXELS</code></li>
   *
   * <p>Returns the maximum size of a pixel buffer surface in pixels.
   *
   * <li><code>EGL_NATIVE_RENDERABLE</code></li>
   *
   * <p>Returns <code>EGL_TRUE</code> if native rendering APIs can render
   * into the surface, <code>EGL_FALSE</code> otherwise.
   *
   * <li><code>EGL_NATIVE_VISUAL_ID</code></li>
   *
   * <p>Returns the ID of the associated native visual.
   *
   * <li><code>EGL_NATIVE_VISUAL_TYPE</code></li>
   *
   * <p>Returns the type of the associated native visual.
   *
   * <li><code>EGL_PRESERVED_RESOURCES</code> (1.0 only)</li>
   *
   * Returns <code>EGL_TRUE</code> if resources are preserved across
   * power management events, <code>EGL_FALSE</code> otherwise.
   *
   * <li><code>EGL_SAMPLE_BUFFERS</code></li>
   *
   * <p>Returns the number of multisample buffers.
   *
   * <li><code>EGL_SAMPLES</code></li>
   *
   * <p>Returns the number of samples per pixel.
   *
   * <li><code>EGL_STENCIL_SIZE</code></li>
   *
   * <p>Returns the number of bits in the stencil buffer.
   *
   * <li><code>EGL_SURFACE_TYPE</code></li>
   *
   * <p>Returns the types of supported EGL surfaces.
   *
   * <li><code>EGL_TRANSPARENT_TYPE</code></li>
   *
   * <p>Returns the type of supported transparency. Possible transparency
   * values are: <code>EGL_NONE</code>, and
   * <code>EGL_TRANSPARENT_RGB</code>.
   *
   * <li><code>EGL_TRANSPARENT_RED_VALUE</code></li>
   *
   * <p>Returns the transparent red value.
   *
   * <li><code>EGL_TRANSPARENT_GREEN_VALUE</code></li>
   *
   * <p>Returns the transparent green value.
   *
   * <li><code>EGL_TRANSPARENT_BLUE_VALUE</code></li>
   *
   * <p>Returns the transparent blue value.
   *
   * <li><code>EGL_BIND_TO_TEXTURE_RGB</code> (1.1 only)</li>
   *
   * <p>Returns <code>EGL_TRUE</code> if color buffers can be bound to an
   * RGB texture, <code>EGL_FALSE</code> otherwise.
   *
   * <li><code>EGL_BIND_TO_TEXTURE_RGBA</code> (1.1 only)</li>
   *
   * <p>Returns <code>EGL_TRUE</code> if color buffers can be bound to an
   * RGBA texture, <code>EGL_FALSE</code> otherwise.
   *
   * <li><code>EGL_MAX_SWAP_INTERVAL</code> (1.1 only)</li>
   *
   * <p>Returns the maximum value that can be passed to
   * <code>eglSwapInterval</code>.
   *
   * <li><code>EGL_MIN_SWAP_INTERVAL</code> (1.1 only)</li>
   *
   * <p>Returns the minimum value that can be passed to
   * <code>eglSwapInterval</code>.
   *
   * </ul>
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned on failure, <code>true</code>
   * otherwise. <code>value</code> is not modified when
   * <code>false</code> is returned.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_CONFIG</code> is generated if <code>config</code>
   * is not an EGL frame buffer configuration.
   *
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attribute</code> is not a valid frame buffer configuration
   * attribute.
   *
   * @param display Specifies the EGL display connection.
   * @param config Specifies the EGL frame buffer configuration to be queried.
   * @param attribute Specifies the EGL rendering context attribute to
   * be returned.
   * @param value An <code>int</code> array of length at least 1 in
   * which the requested value will be returned as element 0.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>config</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>value</code> is
   * <code>null</code> or has length less than 1.
   *
   * @return <code>true</code> if the query succeeds.
   */
  boolean eglGetConfigAttrib(EGLDisplay display,
    			     EGLConfig config,
    			     int attribute,
    			     int[] value);
  
  /**
   * Create a new EGL window surface.
   *
   * <p><code>eglCreateWindowSurface</code> creates an EGL window surface
   * and returns its handle. If <code>eglCreateWindowSurface</code>
   * fails to create a window surface, <code>EGL_NO_SURFACE</code> is
   * returned.
   *
   * <p>Any EGL rendering context that was created with respect to
   * <code>config</code> can be used to render into the surface. Use
   * <code>eglMakeCurrent</code> to attach an EGL rendering context to
   * the surface.
   *
   * <p>Use <code>eglQuerySurface</code> to retrieve the ID of
   * <code>config</code>.
   *
   * <p>Use <code>eglDestroySurface</code> to destroy the surface.
   *
   * <h4>Notes</h4>
   *
   * <p>On JME CLDC/MIDP platforms, <code>native_window</code> must be
   * an instance of <code>javax.microedition.lcdui.Graphics</code>
   * object created directly from an instance of
   * <code>GameCanvas</code> or from the argument supplied to the
   * <code>Canvas.paint</code> method. <!-- <code>EGLCanvas</code> -->
   *
   * <!--
   * <p>On JME Personal Basis Profile platforms,
   * <code>native_window</code> must be an instance of
   * <code>java.awt.Container</code>.

   * <p>On JME Personal Profile platforms,
   * <code>native_window</code> must be an instance of
   * <code>java.awt.Canvas</code>.
   * -->
   *
   * <h4>Errors</h4>
   *
   * <p><code>EGL_NO_SURFACE</code> is returned if creation of the
   * context fails.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_CONFIG</code> is generated if <code>config</code>
   * is not an EGL frame buffer configuration.
   *
   * <p><code>EGL_BAD_NATIVE_WINDOW</code> may be generated if
   * <code>native_window</code> is not a valid native window.
   *
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attrib_list</code> contains an invalid window attribute or
   * if an attribute value is not recognized or is out of range.
   *
   * <p><code>EGL_BAD_ALLOC</code> is generated if there are not enough
   * resources to allocate the new surface.
   *
   * <p><code>EGL_BAD_MATCH</code> is generated if the attributes of
   * <code>native_window</code> do not correspond to
   * <code>config</code> or if <code>config</code> does not support
   * rendering to windows (the <code>EGL_SURFACE_TYPE</code> attribute
   * does not contain <code>EGL_WINDOW_BIT</code>).
   *
   * @param display Specifies the EGL display connection.
   * @param config Specifies the EGL frame buffer configuration that
   * defines the frame buffer resource available to the surface.
   * @param native_window Specifies the native window.
   * @param attrib_list Specifies window surface attributes. Must be
   * <code>null</code> or empty (first attribute is <code>EGL_NONE</code>).
   *
   * @return an <code>EGLSurface</code> for rendering to the window.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>config</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>native_window</code>
   * is <code>null</code> or is not of a suitable type for the
   * underlying platform (e.g., not a properly-created
   * <code>javax.microedition.lcdui.Graphics</code> instance for
   * CLDC/MIDP).
   * @exception IllegalArgumentException if <code>attrib_list</code>
   * is non-<code>null</code> but is not terminated with
   * <code>EGL_NONE</code>.
   */
  EGLSurface eglCreateWindowSurface(EGLDisplay display,
    				    EGLConfig config,
    				    Object native_window,
    				    int[] attrib_list);     

  /**
   * Create a new EGL pixmap surface.
   *
   * <p><code>eglCreatePixmapSurface</code> creates an off-screen EGL
   * pixmap surface and returns its handle. If
   * <code>eglCreatePixmapSurface</code> fails to create a pixmap
   * surface, <code>EGL_NO_SURFACE</code> is returned.
   *
   * <p>Any EGL rendering context that was created with respect to
   * <code>config</code> can be used to render into the surface. Use
   * <code>eglMakeCurrent</code> to attach an EGL rendering context to
   * the surface.
   *
   * <p>Use <code>eglQuerySurface</code> to retrieve the ID of
   * <code>config</code>.
   *
   * <p>Use <code>eglDestroySurface</code> to destroy the surface.
   *
   * <h4>Notes</h4>
   *
   * <p>On JME CLDC/MIDP platforms, <code>native_pixmap</code> must be
   * an instance of <code>javax.microedition.lcdui.Graphics</code>
   * that was created directly from a mutable instance of
   * <code>javax.microedition.lcdui.Image</code>.
   *
   * <!--
   * <p>On JME Personal Basis Profile and Personal Profile platforms,
   * <code>native_pixmap</code> must be an instance of
   * <code>java.awt.Graphics</code> that was created using the
   * <code>BufferedImage.getGraphics</code> or
   * <code>createGraphics</code> methods.
   * -->
   *
   * <h4>Errors</h4>
   *
   * <p><code>EGL_NO_SURFACE</code> is returned if creation of the
   * context fails.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_CONFIG</code> is generated if <code>config</code>
   * is not an EGL config.
   *
   * <p><code>EGL_BAD_NATIVE_PIXMAP</code> may be generated if
   * <code>native_pixmap</code> is not a valid native pixmap.
   *
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attrib_list</code> contains an invalid pixmap attribute or
   * if an attribute value is not recognized or out of range.
   *
   * <p><code>EGL_BAD_ALLOC</code> is generated if there are not enough
   * resources to allocate the new surface.
   *
   * <p><code>EGL_BAD_MATCH</code> is generated if the attributes of
   * <code>native_pixmap</code> do not correspond to
   * <code>config</code> or if <code>config</code> does not support
   * rendering to pixmaps (the <code>EGL_SURFACE_TYPE</code> attribute
   * does not contain <code>EGL_PIXMAP_BIT</code>).
   *
   * @param display Specifies the EGL display connection.
   * @param config Specifies the EGL frame buffer configuration that
   * defines the frame buffer resource available to the surface.
   * @param native_pixmap Specifies the native pixmap.
   * @param attrib_list Specifies pixmap surface attributes. Must be
   * <code>null</code> or empty (first attribute is <code>EGL_NONE</code>).
   *
   * @return An <code>EGLSurface</code> for offscreen rendering.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>config</code> is
   * <code>null</code>
   * @exception IllegalArgumentException if <code>native_pixmap</code>
   * is <code>null</code> or is not of a suitable type for the
   * underlying platform (e.g., not a mutable <code>Image</code> for
   * CLDC/MIDP).
   * @exception IllegalArgumentException if <code>attrib_list</code>
   * is non-<code>null</code> but is not terminated with
   * <code>EGL_NONE</code>.
   */
  EGLSurface eglCreatePixmapSurface(EGLDisplay display,
    				    EGLConfig config,
    				    Object native_pixmap,
    				    int[] attrib_list);     
 
  /**
   * Create a new EGL pixel buffer surface.
   *
   * <p><code>eglCreatePbufferSurface</code> creates an off-screen pixel
   * buffer surface and returns its handle. If
   * <code>eglCreatePbufferSurface</code> fails to create a pixel
   * buffer surface, <code>EGL_NO_SURFACE</code> is returned.
   *
   * <p>Any EGL rendering context that was created with respect to
   * <code>config</code> can be used to render into the surface. Use
   * <code>eglMakeCurrent</code> to attach an EGL rendering context to
   * the surface.
   *
   * <p>Use <code>eglQuerySurface</code> to retrieve the dimensions of
   * the allocated pixel buffer surface or the ID of
   * <code>config</code>.
   *
   * <p>Use <code>eglDestroySurface</code> to destroy the surface.
   *
   * <p>The pixel buffer surface attributes are specified in
   * <code>attrib_list</code> as a list of attribute value pairs,
   * terminated with <code>EGL_NONE</code>. A <code>null</code> value
   * for <code>attrib_list</code> is treated as an empty list. The
   * accepted attributes for an EGL pixel buffer surface are:
   *
   * <ul>
   *
   * <li><code>EGL_WIDTH</code></li>
   *
   * <p>Specifies the requested width of the pixel buffer surface. The
   * default value is 0.
   *
   * <li><code>EGL_HEIGHT</code></li>
   *
   * <p>Specifies the requests height of the pixel buffer surface. The
   * default value is 0.
   *
   * <li><code>EGL_LARGEST_PBUFFER</code></li>
   *
   * <p>Requests the largest available pixel buffer surface when the
   * allocation would otherwise fail. Use <code>eglQuerySurface</code>
   * to retrieve the dimensions of the allocated pixel buffer. The
   * default value is <code>EGL_FALSE</code>.
   *
   * <li><code>EGL_TEXTURE_FORMAT</code> (1.1 only)</li>
   *
   * <p>Specifies the format of the texture that will be created when
   * a pbuffer is bound to a texture map. Possible values are
   * <code>EGL_NO_TEXTURE</code>, <code>EGL_TEXTURE_RGB</code>, and
   * <code>EGL_TEXTURE_RGBA</code>. The default value is
   * <code>EGL_NO_TEXTURE</code>.
   *
   * <li><code>EGL_TEXTURE_TARGET</code> (1.1 only)</li>
   *
   * <p>Specifies the target for the texture that will be created when
   * the pbuffer is created with a texture format of
   * <code>EGL_TEXTURE_RGB</code> or
   * <code>EGL_TEXTURE_RGBA</code>. Possible values are
   * <code>EGL_NO_TEXTURE</code>, or <code>EGL_TEXTURE_2D</code>. The
   * default value is <code>EGL_NO_TEXTURE</code>.
   *
   * <li><code>EGL_MIPMAP_TEXTURE</code> (1.1 only)</li>
   *
   * <p>Specifies whether storage for mipmaps should be allocated. Space
   * for mipmaps will be set aside if the attribute value is
   * <code>EGL_TRUE</code> and <code>EGL_TEXTURE_FORMAT</code> is not
   * <code>EGL_NO_TEXTURE</code>. The default value is
   * <code>EGL_FALSE</code>.
   *
   * </ul>
   *
   * <h4>1.1 Notes</h4>
   *
   * <p>If the value of config attribute <code>EGL_TEXTURE_FORMAT</code>
   * is not <code>EGL_NO_TEXTURE</code>, then the pbuffer width and
   * height specify the size of the level zero texture image
   *
   * <p>If <code>EGL_LARGEST_PBUFFER</code> is specified and if the
   * pbuffer will be used as a texture (i.e. the value of
   * <code>EGL_TEXTURE_TARGET</code> is <code>EGL_TEXTURE_2D</code>,
   * and the value of EGL_TEXTURE <code>FORMAT</code> is
   * <code>EGL_TEXTURE_RGB</code> or <code>EGL_TEXTURE_RGBA</code>),
   * then the aspect ratio will be preserved and the new width and
   * height will be valid sizes for the texture target (e.g. if the
   * underlying OpenGL ES implementation does not support
   * non-power-of-two textures, both the width and height will be a
   * power of 2).
   *
   * <p>The contents of the depth and stencil buffers may not be
   * preserved when rendering a texture to the pbuffer and switching
   * which image of the texture is rendered to (e.g., switching from
   * rendering one mipmap level to rendering another).
   *
   * <h4>Errors</h4>
   *
   * <p><code>EGL_NO_SURFACE</code> is returned if creation of the
   * context fails.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_CONFIG</code> is generated if <code>config</code>
   * is not an EGL frame buffer configuration.
   *
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attrib_list</code> contains an invalid pixel buffer
   * attribute or if an attribute value is not recognized or out of
   * range.
   *
   * <p><code>EGL_BAD_ALLOC</code> is generated if there are not enough
   * resources to allocate the new surface.
   *
   * <p><code>EGL_BAD_MATCH</code> is generated if <code>config</code>
   * does not support rendering to pixel buffers (the
   * <code>EGL_SURFACE_TYPE</code> attribute does not contain
   * <code>EGL_PBUFFER_BIT</code>).
   *
   * <p>(1.1 only) <code>EGL_BAD_VALUE</code> is generated if the
   * <code>EGL_TEXTURE_FORMAT</code> attribute is not
   * <code>EGL_NO_TEXTURE</code>, and <code>EGL_WIDTH</code> and/or
   * <code>EGL_HEIGHT</code> specify an invalid size (e.g., the
   * texture size is not a power of 2, and the underlying OpenGL ES
   * implementation does not support non-power-of-two textures).
   *
   * <p>(1.1 only) <code>EGL_BAD_VALUE</code> can also be generated if
   * The <code>EGL_TEXTURE_FORMAT</code> attribute is
   * <code>EGL_NO_TEXTURE</code>, and <code>EGL_TEXTURE_TARGET</code>
   * is something other than <code>EGL_NO_TEXTURE</code>; or,
   * <code>EGL_TEXTURE_FORMAT</code> is something other than
   * <code>EGL_NO_TEXTURE</code>, and <code>EGL_TEXTURE_TARGET</code>
   * is <code>EGL_NO_TEXTURE</code>.
   *
   * @param display Specifies the EGL display connection.
   * @param config Specifies the EGL frame buffer configuration that
   * defines the frame buffer resource available to the surface.
   * @param attrib_list Specifies the pixel buffer surface
   * attributes. May be <code>null</code> or empty (first attribute is
   * <code>EGL_NONE</code>).
   *
   * @return An <code>EGLSurface</code> for offscreen rendering.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>config</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>attrib_list</code>
   * is non-<code>null</code> but is not terminated with
   * <code>EGL_NONE</code>.
   */
  EGLSurface eglCreatePbufferSurface(EGLDisplay display,
    				     EGLConfig config,
    				     int[] attrib_list);

  /**
   * Destroy an EGL surface.
   *
   * <p>If the EGL surface <code>surface</code> is not current to any
   * thread, <code>eglDestroySurface</code> destroys it
   * immediately. Otherwise, <code>surface</code> is destroyed when it
   * becomes not current to any thread.
   *
   * <p>(1.1) In OpenGL ES 1.1, resources associated with a pbuffer surface
   * are not released until all color buffers of that pbuffer bound to
   * a texture object have been released.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned if destruction of the surface
   * fails, <code>true</code> otherwise.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_SURFACE</code> is generated if <code>surface</code>
   * is not an EGL surface.
   *
   * @param display Specifies the EGL display connection.
   * @param surface Specifies the EGL surface to be destroyed.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>surface</code> is
   * <code>null</code>.
   *
   * @return <code>true</code> if the operation succeeds.
   */
  boolean eglDestroySurface(EGLDisplay display, EGLSurface surface);

  /**
   * Return EGL surface information.
   *
   * <p><code>eglQuerySurface</code> returns in <code>value[0]</code> the
   * <code>value</code> of <code>attribute</code> for
   * <code>surface</code>. <code>attribute</code> can be one of the
   * following:
   *
   * <ul>
   *
   * <li><code>EGL_CONFIG_ID</code></li>
   *
   * <p>Returns the ID of the EGL frame buffer configuration with respect
   * to which the surface was created.
   *
   * <li><code>EGL_WIDTH</code></li>
   *
   * <p>Returns the width of the surface in pixels.
   *
   * <li><code>EGL_HEIGHT</code></li>
   *
   * <p>Returns the height of the surface in pixels.
   *
   * <li><code>EGL_LARGEST_PBUFFER</code></li>
   *
   * <p>Returns the same attribute value specified when the surface was
   * created with <code>eglCreatePbufferSurface</code>. For a window
   * or pixmap surface, value is not modified.
   *
   * <li><code>EGL_TEXTURE_FORMAT</code> (1.1 only)</li>
   *
   * <p>Returns format of texture. Possible values are
   * <code>EGL_NO_TEXTURE</code>, <code>EGL_TEXTURE_RGB</code>, and
   * <code>EGL_TEXTURE_RGBA</code>.
   *
   * <li><code>EGL_TEXTURE_TARGET</code> (1.1 only)</li>
   *
   * <p>Returns type of texture. Possible values are
   * <code>EGL_NO_TEXTURE</code>, or <code>EGL_TEXTURE_2D</code>.
   *
   * <li><code>EGL_MIPMAP_TEXTURE</code> (1.1 only)</li>
   *
   * <p>Returns <code>EGL_TRUE</code> if texture has mipmaps,
   * <code>EGL_FALSE</code> otherwise.
   *
   * <li><code>EGL_MIPMAP_LEVEL</code> (1.1 only)</li>
   *
   * <p>Returns which level of the mipmap to render to, if texture has mipmaps.
   *
   * </ul>
   *
   * <h4>1.1 Notes</h4>
   *
   * Querying <code>EGL_TEXTURE_FORMAT</code>,
   * <code>EGL_TEXTURE_TARGET</code>, <code>EGL_MIPMAP_TEXTURE</code>,
   * or <code>EGL_MIPMAP_LEVEL</code> for a non-pbuffer surface is not
   * an error, but <code>value</code> is not modified.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned on failure, <code>true</code>
   * otherwise. <code>value</code> is not modified when
   * <code>false</code> is returned.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_SURFACE</code> is generated if <code>surface</code>
   * is not an EGL surface.
   *
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attribute</code> is not a valid surface attribute.
   *
   * @param display Specifies the EGL display connection.
   * @param surface Specifies the EGL surface to query.
   * @param attribute Specifies the EGL surface attribute to be returned.
   * @param value Returns the requested value.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>surface</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>value</code> is
   * <code>null</code> or has length less than 1.
   *
   * @return <code>true</code> if the query succeeds.
   */
  boolean eglQuerySurface(EGLDisplay display,
    			  EGLSurface surface,
    			  int attribute,
    			  int[] value);

  /**
   * Create a new EGL rendering context.
   *
   * <p><code>eglCreateContext</code> creates an EGL rendering context
   * and returns its handle. This context can be used to render into
   * an EGL drawing surface. If <code>eglCreateContext</code> fails to
   * create a rendering context, <code>EGL_NO_CONTEXT</code> is
   * returned.
   *
   * <p>If <code>share_context</code> is not <code>EGL_NO_CONTEXT</code>,
   * then all texture objects except object 0, are shared by context
   * <code>share_context</code> and by the newly created context. An
   * arbitrary number of rendering contexts can share a single texture
   * object space. However, all rendering contexts that share a single
   * texture object space must themselves exist in the same address
   * space. Two rendering contexts share an address space if both are
   * owned by a single process.
   *
   * <p> Currently no attributes are recognized, so
   * <code>attrib_list</code> will normally be <code>null</code> or
   * empty (first attribute is <code>EGL_NONE</code>). However, it is
   * possible that some platforms will define attributes specific to
   * those environments, as an EGL extension.  A non-<code>null</code>
   * attribute list that is terminated with <code>EGL_NONE</code> will
   * be passed to the underlying EGL implementation.
   *
   * <!--
   * <h4>Notes</h4>
   *
   * <p>A process is a single execution environment, implemented in a
   * single address space, consisting of one or more threads.
   *
   * <p>A thread is one of a set of subprocesses that share a single
   * address space, but maintain separate program counters, stack
   * spaces, and other related global data. A thread is the only
   * member of its subprocess group is equivalent to a process.
   * -->
   *
   * <h4>Errors</h4>
   *
   * <p><code>EGL_NO_CONTEXT</code> is returned if creation of the
   * context fails.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_CONFIG</code> is generated if <code>config</code>
   * is not an EGL frame buffer configuration.
   *
   * <p><code>EGL_BAD_CONTEXT</code> is generated if
   * <code>share_context</code> is not an EGL rendering context and is
   * not <code>EGL_NO_CONTEXT</code>.
   *
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attrib_list</code> contains an invalid context attribute or
   * if an attribute is not recognized or out of range.
   *
   * <p><code>EGL_BAD_ALLOC</code> is generated if there are not enough
   * resources to allocate the new context.
   *
   * @param display Specifies the EGL display connection.
   * @param config Specifies the EGL frame buffer configuration that
   * defines the frame buffer resource available to the rendering
   * context.
   * @param share_context Specifies the EGL rendering context with
   * which to share texture objects. <code>EGL_NO_CONTEXT</code>
   * indicates that no sharing is to take place.
   * @param attrib_list Specifies attributes.
   *
   * @return A new <code>EGLContext</code>, or <code>null</code>
   * if context creation was unsuccessful.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>config</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>share_context</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>attrib_list</code> is
   * non-<code>null</code> but is not terminated with
   * <code>EGL_NONE</code>.
   */
  EGLContext eglCreateContext(EGLDisplay display,
    			      EGLConfig config,
    			      EGLContext share_context,
    			      int[] attrib_list);

  /**
   * Destroy an EGL rendering context.
   *
   * <p>If the EGL rendering context context is not current to any
   * thread, <code>eglDestroyContext</code> destroys it
   * immediately. Otherwise, <code>context</code> is destroyed when it
   * becomes not current to any thread.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned if destruction of the context
   * fails, <code>true</code> otherwise.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_CONTEXT</code> is generated if <code>context</code>
   * is not an EGL rendering context.
   *
   * @param display Specifies the EGL display connection.
   * @param context Specifies the EGL rendering context to be destroyed.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>context</code> is
   * <code>null</code>.
   *
   * @return <code>true</code> if the operation succeeds.
   */
  boolean eglDestroyContext(EGLDisplay display, EGLContext context);

  /**
   * Attach an EGL rendering context to EGL surfaces.
   *
   * <p><code>eglMakeCurrent</code> binds <code>context</code> to the
   * current rendering thread and to the <code>draw</code> and
   * <code>read</code> surfaces. <code>draw</code> is used for all GL
   * operations except for any pixel data read back
   * (<code>glReadPixels</code>, <code>glCopyTexImage2D</code>, and
   * <code>glCopyTexSubImage2D</code>), which is taken from the frame
   * buffer values of <code>read</code>.
   *
   * <p>If the calling thread has already a current rendering context,
   * that context is flushed and marked as no longer current.
   *
   * <p>The first time that context is made current, the viewport and
   * scissor dimensions are set to the size of the <code>draw</code>
   * surface. The viewport and scissor are not modified when
   * <code>context</code> is subsequently made current.
   *
   * <p>To release the current context without assigning a new one, call
   * eglMakeCurrent with <code>draw</code> and <code>read</code> set
   * to <code>EGL_NO_SURFACE</code> and <code>context</code> set to
   * <code>EGL_NO_CONTEXT</code>.
   *
   * <p>Use <code>eglGetCurrentContext</code>,
   * <code>eglGetCurrentDisplay</code>, and
   * <code>eglGetCurrentSurface</code> to query the current rendering
   * context and associated display connection and surfaces.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned on failure,
   * <code>true</code> otherwise. If <code>false</code> is
   * returned, the previously current rendering context and surfaces
   * (if any) remain unchanged.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_SURFACE</code> is generated if <code>draw</code> or
   * <code>read</code> is not an EGL surface.
   *
   * <p><code>EGL_BAD_CONTEXT</code> is generated if <code>context</code>
   * is not an EGL rendering context.
   *
   * <p><code>EGL_BAD_MATCH</code> is generated if <code>draw</code> or
   * <code>read</code> are not compatible with context, or if
   * <code>context</code> is set to <code>EGL_NO_CONTEXT</code> and
   * <code>draw</code> or <code>read</code> are not set to
   * <code>EGL_NO_SURFACE</code>, or if <code>draw</code> or
   * <code>read</code> are set to <code>EGL_NO_SURFACE</code> and
   * <code>context</code> is not set to <code>EGL_NO_CONTEXT</code>.
   *
   * <p><code>EGL_BAD_ACCESS</code> is generated if <code>context</code>
   * is current to some other thread.
   *
   * <p><code>EGL_BAD_NATIVE_PIXMAP</code> may be generated if a native
   * pixmap underlying either <code>draw</code> or <code>read</code>
   * is no longer valid.
   *
   * <p><code>EGL_BAD_NATIVE_WINDOW</code> may be generated if a native
   * window underlying either <code>draw</code> or <code>read</code>
   * is no longer valid.
   *
   * <p><code>EGL_BAD_CURRENT_SURFACE</code> is generated if the previous
   * context has unflushed commands and the previous surface is no
   * longer valid.
   *
   * <p><code>EGL_BAD_ALLOC</code> may be generated if allocation of
   * ancillary buffers for draw or read were delayed until
   * <code>eglMakeCurrent</code> is called, and there are not enough
   * resources to allocate them.
   *
   * <p><code>EGL_CONTEXT_LOST</code> (1.1 only)
   *
   * <p>A power management event has occurred. The application must
   * destroy all contexts and reinitialise OpenGL ES state and objects
   * to continue rendering.
   *
   * @param display Specifies the EGL display connection.
   * @param draw Specifies the EGL draw surface.
   * @param read Specifies the EGL read surface.
   * @param context Specifies the EGL rendering context to be attached
   * to the surfaces.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>draw</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>read</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>context</code> is
   * <code>null</code>.
   *
   * @return <code>true</code> if the operation succeeds.
   */
  boolean eglMakeCurrent(EGLDisplay display,
    			 EGLSurface draw,
    			 EGLSurface read,
    			 EGLContext context);
   
  /**
   * Return the current EGL rendering context.
   *
   * <p><code>eglGetCurrentContext</code> returns the current EGL
   * rendering context, as specified by
   * <code>eglMakeCurrent</code>. If no context is current,
   * <code>EGL_NO_CONTEXT</code> is returned.
   *
   * @return The current <code>EGLContext</code>, or
   * <code>EGL_NO_CONTEXT</code>.
   */
  EGLContext eglGetCurrentContext();

  /**
   * Return the read or draw surface for the current EGL rendering
   * context.
   *
   * <p><code>eglGetCurrentSurface</code> returns the read or draw
   * surface attached to the current EGL rendering context, as
   * specified by <code>eglMakeCurrent</code>. If no context is
   * current, <code>EGL_NO_SURFACE</code> is returned.
   *
   * @param readdraw Specifies whether the EGL read or draw surface is
   * to be returned, one of <code>EGL_READ</code> or
   * <code>EGL_DRAW</code>.
   * @return The current <code>EGLSurface</code> used for reading (if
   * <code>readdraw</code> equals <code>EGL_READ</code>, or drawing
   * (if <code>readdraw</code> equals <code>EGL_DRAW</code>).
   *
   * @exception IllegalArgumentException if <code>readdraw</code> is
   * not one of the specified constants.
   */
  EGLSurface eglGetCurrentSurface(int readdraw);

  /**
   * Return the display for the current EGL rendering context.
   *
   * <p><code>eglGetCurrentDisplay</code> returns the current EGL display
   * connection for the current EGL rendering context, as specified by
   * <code>eglMakeCurrent</code>. If no context is current,
   * <code>EGL_NO_DISPLAY</code> is returned.
   *
   * @return The current <code>EGLDisplay</code>
   */
  EGLDisplay eglGetCurrentDisplay();

  /**
   * Return EGL rendering context information.
   *
   * <p><code>eglQueryContext</code> returns in <code>value[0]</code> the
   * value of <code>attribute</code> for
   * <code>context</code>. <code>attribute</code> can be one of the
   * following:
   *
   * <ul>
   *
   * <li><code>EGL_CONFIG_ID</code></li>
   *
   * <p>Returns the ID of the EGL frame buffer configuration with respect
   * to which the context was created.
   *
   * </ul>
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned on failure, <code>true</code>
   * otherwise. <code>value</code> is not modified when
   * <code>false</code> is returned.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_CONTEXT</code> is generated if <code>context</code>
   * is not an EGL rendering context.
   *
   * <p><code>EGL_BAD_ATTRIBUTE</code> is generated if
   * <code>attribute</code> is not a valid context attribute.
   *
   * @param display Specifies the EGL display connection.
   * @param context Specifies the EGL rendering context to query.
   * @param attribute Specifies the EGL rendering context attribute to
   * be returned.
   * @param value Returns the requested value.
   * 
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>context</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>value</code> is
   * <code>null</code> or has length less than 1.
   *
   * @return <code>true</code> if the query succeeds.
   */
  boolean eglQueryContext(EGLDisplay display,
    			  EGLContext context,
    			  int attribute,
    			  int[] value);

  /**
   * Complete GL execution prior to subsequent native rendering calls.
   *
   * <p>GL rendering calls made prior to <code>eglWaitGL</code> are
   * guaranteed to be executed before native rendering calls made
   * after <code>eglWaitGL</code>. The same result can be achieved
   * using <code>glFinish</code>.
   *
   * <p><code>eglWaitGL</code> is ignored if there is no current EGL
   * rendering context.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned if <code>eglWaitGL</code> fails,
   * <code>true</code> otherwise.
   *
   * <p><code>EGL_BAD_NATIVE_SURFACE</code> is generated if the surface
   * associated with the current context has a native window or
   * pixmap, and that window or pixmap is no longer valid.
   *
   * @return <code>true</code> if the operation succeeds.
   */
  boolean eglWaitGL();

  /**
   * Complete native execution prior to subsequent GL rendering calls.
   *
   * <p>Native rendering calls made prior to <code>eglWaitNative</code>
   * are guaranteed to be executed before GL rendering calls made
   * after <code>eglWaitNative</code>.
   *
   * <p><code>eglWaitNative</code> is ignored if there is no current EGL
   * rendering context.
   *
   * <h4>Notes</h4>
   *
   * <p>In a MIDP environment, OpenGL ES drawing to an <code>Image</code>,
   * <code>Canvas</code>, or <code>GameCanvas</code> must be preceded
   * by a call to
   * <code>eglWaitNative(<code>EGL10.EGL_CORE_NATIVE_ENGINE</code>,
   * graphics)</code> and followed by a call to
   * <code>eglWaitGL()</code>.  The <code>bindTarget</code> parameter
   * must be the instance of
   * <code>javax.microedition.lcdui.Graphics</code> obtained from the
   * <code>GameCanvas.getGraphics</code> method (if drawing to a
   * <code>GameCanvas</code>), from the <code>Image.getGraphics()</code>
   * method (if drawing to a mutable <code>Image</code>),
   * or the instance obtained as the
   * parameter to the <code>Canvas.paint</code> method (if drawing to
   * a <code>Canvas</code>).
   *
   * <p>The results of passing improper values for <code>engine</code>
   * and/or <code>bindTarget</code> are undefined.
   * 
   * <p> The results of MIDP drawing calls are undefined from the time
   * of the call to <code>eglWaitNative</code> until
   * <code>eglWaitGL</code> returns.
   * 
   * <h4>Errors</h4>
   *
   * <p><code>EGL_BAD_PARAMETER</code> is generated if
   * <code>engine</code> is not a recognized marking engine.
   *
   * <p><code>EGL_BAD_NATIVE_SURFACE</code> is generated if the surface
   * associated with the current context has a native window or
   * pixmap, and that window or pixmap is no longer valid.
   *
   * @param engine Specifies a particular marking engine to be waited
   * on. Must be <code>EGL_CORE_NATIVE_ENGINE</code>.
   * @param bindTarget the rendering target shared by the native
   * engine and the EGL drawing surface.  For MIDP, must be the
   * instance of <code>javax.microedition.lcdui.Graphics</code>
   * representing the targetted <code>Canvas</code>, <code>Image</code>, or
   * <code>GameCanvas</code>.
   *
   * @return <code>true</code> if the operation succeeds.
   */
  boolean eglWaitNative(int engine, Object bindTarget);

  /**
   * Post EGL surface color buffer to a native window.
   *
   * <p>If surface is a window surface, <code>eglSwapBuffers</code> posts
   * its color buffer to the associated native window.
   *
   * <p><code>eglSwapBuffers</code> performs an implicit
   * <code>glFlush</code> before it returns. Subsequent GL commands
   * may be issued immediately after calling
   * <code>eglSwapBuffers</code>, but are not executed until the
   * buffer exchange is completed.
   *
   * <p>If surface is a pixel buffer or a pixmap,
   * <code>eglSwapBuffers</code> has no effect, and no error is
   * generated.
   *
   * <h4>Notes</h4>
   *
   * <p>The color buffer of surface is left undefined after calling
   * <code>eglSwapBuffers</code>.
   *
   * <p>On the CLDC/MIDP plaform, calling <code>eglSwapBuffers</code>
   * is equivalent to calling <code>glFinish</code> when drawing to a
   * <code>GameCanvas</code>.  The
   * <code>GameCanvas.flushGraphics</code> method is used to copy the
   * contents of the back buffer to the screen.
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned if swapping of the surface
   * buffers fails, <code>true</code> otherwise.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_SURFACE</code> is generated if <code>surface</code>
   * is not an EGL drawing surface.
   *
   * <p><code>EGL_CONTEXT_LOST</code> (1.1 only)
   *
   * <p>A power management event has occurred. The application must
   * destroy all contexts and reinitialise OpenGL ES state and objects
   * to continue rendering.
   *
   * @param display Specifies the EGL display connection.
   * @param surface Specifies the EGL drawing surface whose buffers
   * are to be swapped.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>surface</code> is
   * <code>null</code>.
   *
   * @return <code>true</code> if the operation succeeds.
   */
  boolean eglSwapBuffers(EGLDisplay display, EGLSurface surface);

  /**
   * Copy EGL surface color buffer to a native pixmap.
   *
   * <p><code>eglCopyBuffers</code> copies the color buffer of
   * <code>surface</code> to <code>native_pixmap</code>.
   *
   * <p><code>eglCopyBuffers</code> performs an implicit
   * <code>glFlush</code> before it returns. Subsequent GL commands
   * may be issued immediately after calling
   * <code>eglCopyBuffers</code>, but are not executed until copying
   * of the color buffer is completed.
   *
   * <h4>Notes</h4>
   *
   * <p>The color buffer of surface is left unchanged after calling
   * <code>eglCopyBuffers</code>.
   *
   * <p>On JME CLDC/MIDP platforms, <code>native_pixmap</code> must be
   * an instance of <code>javax.microedition.lcdui.Image</code> that
   * is mutable, or an instance of
   * <code>javax.microedition.lcdui.Graphics</code> 
   * that was obtained directly from such
   * an image by means of a call to <code>createGraphics()</code> or
   * <code>getGraphics()</code>.
   *
   * <!--
   * <p>On JME Personal Basis Profile and Personal Profile platforms,
   * <code>native_pixmap</code> must be an instance of
   * <code>java.awt.image.BufferedImage</code>, or an instance of
   * <code>java.awt.Graphics</code> that was obtained directly from
   * such an image by means of a call to <code>createGraphics()</code>
   * or <code>getGraphics()</code>.
   * -->
   *
   * <h4>Errors</h4>
   *
   * <p><code>false</code> is returned if swapping of the surface
   * buffers fails, <code>true</code> otherwise.
   *
   * <p><code>EGL_BAD_DISPLAY</code> is generated if <code>display</code>
   * is not an EGL display connection.
   *
   * <p><code>EGL_NOT_INITIALIZED</code> is generated if
   * <code>display</code> has not been initialized.
   *
   * <p><code>EGL_BAD_SURFACE</code> is generated if <code>surface</code>
   * is not an EGL drawing surface.
   *
   * <p><code>EGL_BAD_NATIVE_PIXMAP</code> is generated if the
   * implementation does not support native pixmaps.
   *
   * <p><code>EGL_BAD_NATIVE_PIXMAP</code> may be generated if
   * <code>native_pixmap</code> is not a valid native pixmap.
   *
   * <p><code>EGL_BAD_MATCH</code> is generated if the format of
   * <code>native_pixmap</code> is not compatible with the color
   * buffer of surface.
   * 
   * <p>(1.1 only) <code>EGL_CONTEXT_LOST</code> is generated if a
   * power management event has occurred. The application must destroy
   * all contexts and reinitialize OpenGL ES state and objects to
   * continue rendering.
   *
   * @param display Specifies the EGL display connection.
   * @param surface Specifies the EGL surface whose color buffer is to
   * be copied.
   * @param native_pixmap Specifies the native pixmap as target of the
   * copy.
   *
   * @exception IllegalArgumentException if <code>display</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>surface</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>native_pixmap</code>
   * is <code>null</code> or is not of a suitable type for the
   * underlying platform (e.g., a mutable <code>Image</code> for CLDC/MIDP).
   *
   * @return <code>true</code> if the copy succeeds.
   */
  boolean eglCopyBuffers(EGLDisplay display,
    			 EGLSurface surface,
			 Object native_pixmap);
}

