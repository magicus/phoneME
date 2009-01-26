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

package com.sun.jsr239;

public final class Errors {

    // EGL Errors

    public static final String EGL_DISPLAY_NOT_EGL_DEFAULT_DISPLAY =
        "display != EGL.EGL_DEFAULT_DISPLAY";

    public static final String EGL_DISPLAY_NULL =
        "display == null";

    public static final String EGL_PIXMAP_OUT_OF_MEMORY =
        "operation failed due to out of memory error";    

    public static final String EGL_MAJOR_MINOR_SHORT =
        "major_minor != null && major_minor.length < 2";

    public static final String EGL_CONFIG_SHORT =
        "configs != null && configs.length < config_size";

    public static final String EGL_NUM_CONFIG_SHORT =
        "num_config != null && num_config.length < 1";

    public static final String EGL_ATTRIBS_NOT_TERMINATED =
        "attrib_list not terminated with EGL.EGL_NONE";

    public static final String EGL_VALUE_SHORT =
        "value == null || value.length < 1";

    public static final String EGL_SHARE_CONTEXT_NULL =
        "share_context == null";

    public static final String EGL_BAD_WINDOW_SURFACE =
        "win is not a Canvas, GameCanvas, EGLCanvas, or Graphics";

    public static final String EGL_PIXMAP_NULL =
        "pixmap == null";

    public static final String EGL_CONFIG_NULL =
        "config == null";

    public static final String EGL_CONTEXT_NULL =
        "context == null (perhaps you meant EGL.EGL_NO_CONTEXT)";

    public static final String EGL_SURFACE_NULL =
        "surface == null (perhaps you meant EGL.EGL_NO_SURFACE)";

    public static final String EGL_DRAW_NULL =
        "draw == null (perhaps you meant EGL.EGL_NO_SURFACE)";

    public static final String EGL_READ_NULL =
        "read == null (perhaps you meant EGL.EGL_NO_SURFACE)";

    public static final String EGL_BAD_PIXMAP =
        "pixmap is not a Graphics instance";

    public static final String EGL_IMAGE_IMMUTABLE =
        "pixmap is not mutable";

    public static final String EGL_READDRAW_BAD =
        "readdraw not one of EGL.EGL_READ or EGL.EGL_DRAW";

    public static final String EGL_NATIVE_PIXMAP_NULL =
        "native_pixmap == null";

    public static final String EGL_EGL11_UNSUPPORTED =
        "EGL 1.1 not supported";

    public static final String EGL_CANT_HAPPEN =
        "This can't happen!";

    // GL Errors

    public static final String GL_UNKNOWN_BUFFER =
        "Unknown Buffer class";

    public static final String GL_GL11_UNSUPPORTED =
        "OpenGL ES 1.1 not supported";

    public static final String GL_PARAMS_NULL =
        "params == null";

    public static final String GL_OFFSET_NEGATIVE =
        "offset < 0";

    public static final String GL_BAD_LENGTH =
        "not enough remaining entries";

    public static final String GL_NOT_DIRECT =
        "Buffer must be direct";

    public static final String GL_POINTER_NULL =
        "pointer == null";

    public static final String GL_INDICES_NULL =
        "indices == null";

    public static final String GL_PIXELS_NULL =
        "pixels == null";

    public static final String GL_PIXELS_NOT_SHORT_OR_INT =
        "pixels must be a ShortBuffer or an IntBuffer";

    public static final String GL_DRAW_TEXTURE_UNSUPPORTED =
        "OES_draw_texture extension not available";

    public static final String GL_MATRIX_PALETTE_UNSUPPORTED =
        "OES_matrix_palette extension not available";

    public static final String GL_QUERY_MATRIX_UNSUPPORTED =
        "OES_query_matrix extension not available";

    public static final String GL_TEXTURE_CUBE_MAP_UNSUPPORTED =
        "OES_texture_cube_map extension not available";

    public static final String GL_BLEND_SUBTRACT_UNSUPPORTED =
        "OES_blend_subtract extension not available";

    public static final String GL_BLEND_FUNC_SEPARATE_UNSUPPORTED =
        "OES_blend_func_separate extension not available";

    public static final String GL_BLEND_EQUATION_SEPARATE_UNSUPPORTED =
        "OES_blend_equations_separate extension not available";

    public static final String GL_FRAMEBUFFER_OBJECT_UNSUPPORTED =
        "OES_framebuffer_object extension not available";
 
    public static final String VBO_ARRAY_BUFFER_BOUND =
        "Buffer version called while VBO array buffer is bound";

    public static final String VBO_ARRAY_BUFFER_UNBOUND =
        "Integer version called while VBO array buffer is not bound";

    public static final String VBO_ELEMENT_ARRAY_BUFFER_BOUND =
        "Buffer version called while VBO element array buffer is bound";

    public static final String VBO_ELEMENT_ARRAY_BUFFER_UNBOUND =
        "Integer version called while VBO element array buffer is not bound";

    public static final String VBO_OFFSET_OOB =
        "Argument 'offset' is outside the bound VBO buffer";

    public static final String NOT_ENOUGH_ROOM =
        "Not enough room for pixel data";
}
