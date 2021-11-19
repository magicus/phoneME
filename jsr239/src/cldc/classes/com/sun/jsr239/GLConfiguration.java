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

package com.sun.jsr239;

import javax.microedition.khronos.opengles.GL11;
import javax.microedition.khronos.opengles.GL11Ext;
import javax.microedition.khronos.opengles.GL11ExtensionPack;

import com.sun.midp.main.Configuration;

/**
 * A class containing constants that describe the OpenGL ES version
 * supported by the runtime platform and the set of supported
 * extensions.  This class also contains methods that return the
 * number of values that will be returned by various 'getters' as well
 * as tunable implementation parameters.
 */
public final class GLConfiguration {

    /**
     * If true, each GL command will be submitted to the native OpenGL
     * ES engine immediately.
     */
    public static boolean flushQueueAfterEachCommand = false;

    /**
     * If true, the underlying VM uses a single native thread for all
     * Java threads, false if the VM uses a 1-1 mapping of Java
     * threads to native thread.  If the VM uses some other sort of
     * mapping, this implementation is not guaranteed to work.
     *
     * The value should be 'true' for Sun's CLDC implementations,
     * and 'false' for Sun's CDC implementation.
     */
    public static final boolean singleThreaded = true;

    /**
     * True if the platform requires the bytes returned by
     * <code>Float.floatToIntBits</code> to be swapped prior to
     * placement on the command queue.
     */
    public static final boolean SWAP_FLOAT_BYTES = false;

    /**
     * True on big-endian platforms.
     */
    public static boolean IS_BIG_ENDIAN = false;

    /**
     * The maximum number of commands and arguments that will be
     * enqueued prior to submittal to the native OpenGL ES engine.
     */
    public static final int COMMAND_QUEUE_SIZE = 4096;

    /**
     * True if the native OpenGL ES engine supports EGL 1.1
     */
    public static boolean supportsEGL11 = true;

    /**
     * True if the native OpenGL ES engine supports OpenGL ES 1.1
     */
    public static boolean supportsGL11 = true;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_query_matrix</code> extension.
     */
    public static boolean supports_OES_query_matrix = true;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_query_matrix</code> extension.
     */
    public static boolean supports_OES_draw_texture = true;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_query_matrix</code> extension.
     */
    public static boolean supports_OES_matrix_palette = true;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_texture_cube_map</code> extension.
     */
    public static boolean supports_OES_texture_cube_map = false;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_texture_env_crossbar</code> extension.
     */
    public static boolean supports_OES_texture_env_crossbar = false;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_texture_mirrored_repeate</code> extension.
     */
    public static boolean supports_OES_texture_mirrored_repeat = false;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_blend_subtract</code> extension.
     */
    public static boolean supports_OES_blend_subtract = false;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_blend_func_separate</code> extension.
     */
    public static boolean supports_OES_blend_func_separate = false;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_blend_equation_separate</code> extension.
     */
    public static boolean supports_OES_blend_equation_separate = false;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_stencil_wrap</code> extension.
     */
    public static boolean supports_OES_stencil_wrap = false;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_extended_matrix_palette</code> extension.
     */
    public static boolean supports_OES_extended_matrix_palette = false;

    /**
     * True if the native OpenGL ES engine supports
     * the <code>OES_framebuffer_object</code> extension.
     */
    public static boolean supports_OES_framebuffer_object = false;

    /**
     * The number of compressed texture formats, as returned by
     * 'glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS)'
     */
    public static final int NUM_COMPRESSED_TEXTURE_FORMATS = 10;

    /**
     * The value of GL_MAX_VERTEX_UNITS_OES.
     */
    public static final int MAX_VERTEX_UNITS = 4;

    private static boolean getBooleanProp(String p, boolean defaultVal) {
        String propString = Configuration.getProperty(p);
	boolean returnVal = defaultVal;
	if (propString != null) {
            if (propString.equals("true") || 
		propString.equals("t") ||
                propString.equals("True") || 
		propString.equals("T") ||
		propString.equals("1"))
	    {
                returnVal = true;
            } else if (propString.equals("false") ||
                       propString.equals("f") ||
                       propString.equals("False") ||
                       propString.equals("F") || 
		       propString.equals("0"))
            {
                returnVal = false;
            }
        }
        return returnVal;
    }

    static {
        flushQueueAfterEachCommand = getBooleanProp("jsr239.noqueue", false);
        
        supportsEGL11 = getBooleanProp("jsr239.supportsEGL11", true);
        
        supportsGL11 = getBooleanProp("jsr239.supportsGL11", true);
        
        supports_OES_query_matrix =
            getBooleanProp("jsr239.supports_OES_query_matrix", true);
        
        supports_OES_draw_texture =
            getBooleanProp("jsr239.supports_OES_draw_texture", true);
        
        supports_OES_matrix_palette =
            getBooleanProp("jsr239.supports_OES_matrix_palette", true);
        
        supports_OES_texture_cube_map =
            getBooleanProp("jsr239.supports_OES_texture_cube_map", false);
        
        supports_OES_texture_env_crossbar =
            getBooleanProp("jsr239.supports_OES_texture_env_crossbar", false);
        
        supports_OES_texture_mirrored_repeat =
            getBooleanProp("jsr239.supports_OES_texture_mirrored_repeat",
                           false);
        
        supports_OES_blend_subtract =
            getBooleanProp("jsr239.supports_OES_blend_subtract", false);
        
        supports_OES_blend_func_separate =
            getBooleanProp("jsr239.supports_OES_blend_func_separate", false);
        
        supports_OES_blend_equation_separate =
            getBooleanProp("jsr239.supports_OES_blend_equation_separate",
                           false);
        
        supports_OES_stencil_wrap =
            getBooleanProp("jsr239.supports_OES_stencil_wrap", false);
        
        supports_OES_extended_matrix_palette =
            getBooleanProp("jsr239.supports_OES_extended_matrix_palette",
                           false);
        
        supports_OES_framebuffer_object =
            getBooleanProp("jsr239.supports_OES_framebuffer_object", false);
    }

    /**
     * Returns the number of values required by the given
     * parameter <code>pname</code> by <code>glFog*v</code>.
     */
    public static int glFogNumParams(int pname) {
	return (pname == GL11.GL_FOG_COLOR) ? 4 : 1;
    }
    
    /**
     * Returns the number of values required by the given
     * parameter <code>pname</code> by <code>glGetFixedv</code>,
     * <code>glGetFloatv</code>, and <code>glGetIntegerv</code>.
     */
    public static int glGetNumParams(int pname) {
	switch (pname) {
	case GL11.GL_ALPHA_BITS:
	case GL11.GL_ALPHA_TEST_FUNC:
	case GL11.GL_ALPHA_TEST_REF:
	case GL11.GL_BLEND_DST:
	case GL11ExtensionPack.GL_BLEND_DST_ALPHA:
	case GL11ExtensionPack.GL_BLEND_DST_RGB:
	case GL11ExtensionPack.GL_BLEND_EQUATION:	
            /* synonym: GL11ExtensionPack.GL_BLEND_EQUATION_RGB */
        case GL11ExtensionPack.GL_BLEND_EQUATION_ALPHA:
	case GL11.GL_BLEND_SRC:
	case GL11ExtensionPack.GL_BLEND_SRC_ALPHA:
	case GL11ExtensionPack.GL_BLEND_SRC_RGB:
	case GL11.GL_BLUE_BITS:
	case GL11.GL_COLOR_ARRAY_BUFFER_BINDING:
	case GL11.GL_COLOR_ARRAY_SIZE:
	case GL11.GL_COLOR_ARRAY_STRIDE:
	case GL11.GL_COLOR_ARRAY_TYPE:
        case GL11ExtensionPack.GL_MAX_CUBE_MAP_TEXTURE_SIZE:
        case GL11.GL_CULL_FACE:
        case GL11.GL_DEPTH_BITS:
        case GL11.GL_DEPTH_CLEAR_VALUE:
        case GL11.GL_DEPTH_FUNC:
        case GL11.GL_DEPTH_WRITEMASK:
	case GL11.GL_FOG_DENSITY:
	case GL11.GL_FOG_END:
	case GL11.GL_FOG_HINT:
	case GL11.GL_FOG_MODE:
	case GL11.GL_FOG_START:
	case GL11.GL_FRONT_FACE:
	case GL11.GL_GREEN_BITS:
	case GL11.GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
	case GL11.GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
        case GL11.GL_LIGHT_MODEL_TWO_SIDE:
        case GL11.GL_LINE_SMOOTH_HINT:
        case GL11.GL_LINE_WIDTH:
        case GL11.GL_LOGIC_OP_MODE:
        case GL11Ext.GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES:
        case GL11Ext.GL_MATRIX_INDEX_ARRAY_SIZE_OES:
        case GL11Ext.GL_MATRIX_INDEX_ARRAY_STRIDE_OES:
	case GL11.GL_MATRIX_MODE:
	case GL11.GL_MAX_CLIP_PLANES:
	case GL11.GL_MAX_ELEMENTS_INDICES:
	case GL11.GL_MAX_ELEMENTS_VERTICES:
	case GL11.GL_MAX_LIGHTS:
	case GL11.GL_MAX_MODELVIEW_STACK_DEPTH:
	case GL11Ext.GL_MAX_PALETTE_MATRICES_OES:
	case GL11.GL_MAX_PROJECTION_STACK_DEPTH:
	case GL11.GL_MAX_TEXTURE_SIZE:
	case GL11.GL_MAX_TEXTURE_STACK_DEPTH:
	case GL11.GL_MAX_TEXTURE_UNITS:
	case GL11Ext.GL_MAX_VERTEX_UNITS_OES:
	case GL11.GL_MODELVIEW_STACK_DEPTH:
	case GL11.GL_NORMAL_ARRAY_BUFFER_BINDING:
	case GL11.GL_NORMAL_ARRAY_STRIDE:
	case GL11.GL_NORMAL_ARRAY_TYPE:
	case GL11.GL_NUM_COMPRESSED_TEXTURE_FORMATS:
	case GL11.GL_PACK_ALIGNMENT:
	case GL11.GL_PERSPECTIVE_CORRECTION_HINT:
	case GL11.GL_POINT_SIZE:
	case GL11.GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
	case GL11.GL_POINT_SIZE_ARRAY_STRIDE_OES:
	case GL11.GL_POINT_SIZE_ARRAY_TYPE_OES:
	case GL11.GL_POINT_SMOOTH_HINT:
	case GL11.GL_POLYGON_OFFSET_FACTOR:
	case GL11.GL_POLYGON_OFFSET_UNITS:
	case GL11.GL_PROJECTION_STACK_DEPTH:
	case GL11.GL_RED_BITS:
	case GL11.GL_SHADE_MODEL:
	case GL11.GL_STENCIL_BITS:
	case GL11.GL_STENCIL_CLEAR_VALUE:
	case GL11.GL_STENCIL_FAIL:
	case GL11.GL_STENCIL_FUNC:
	case GL11.GL_STENCIL_PASS_DEPTH_FAIL:
	case GL11.GL_STENCIL_PASS_DEPTH_PASS:
	case GL11.GL_STENCIL_REF:
	case GL11.GL_STENCIL_VALUE_MASK:
	case GL11.GL_STENCIL_WRITEMASK:
	case GL11.GL_SUBPIXEL_BITS:
	case GL11.GL_TEXTURE_BINDING_2D:
	case GL11.GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
	case GL11.GL_TEXTURE_COORD_ARRAY_SIZE:
	case GL11.GL_TEXTURE_COORD_ARRAY_STRIDE:
	case GL11.GL_TEXTURE_COORD_ARRAY_TYPE:
	case GL11.GL_TEXTURE_STACK_DEPTH:
	case GL11.GL_UNPACK_ALIGNMENT:
	case GL11.GL_VERTEX_ARRAY_BUFFER_BINDING:
	case GL11.GL_VERTEX_ARRAY_SIZE:
	case GL11.GL_VERTEX_ARRAY_STRIDE:
	case GL11.GL_VERTEX_ARRAY_TYPE:
	case GL11Ext.GL_WEIGHT_ARRAY_BUFFER_BINDING_OES:
	case GL11Ext.GL_WEIGHT_ARRAY_SIZE_OES:
	case GL11Ext.GL_WEIGHT_ARRAY_STRIDE_OES:
	case GL11Ext.GL_WEIGHT_ARRAY_TYPE_OES:
	    return 1;
	    
	case GL11.GL_ALIASED_POINT_SIZE_RANGE:
	case GL11.GL_ALIASED_LINE_WIDTH_RANGE:
        case GL11.GL_DEPTH_RANGE:
	case GL11.GL_MAX_VIEWPORT_DIMS:
	case GL11.GL_SMOOTH_LINE_WIDTH_RANGE:
	case GL11.GL_SMOOTH_POINT_SIZE_RANGE:
	    return 2;

	case GL11.GL_COLOR_CLEAR_VALUE:
        case GL11.GL_COLOR_WRITEMASK:
        case GL11.GL_FOG_COLOR:
        case GL11.GL_LIGHT_MODEL_AMBIENT:
        case GL11.GL_SCISSOR_BOX:
        case GL11.GL_VIEWPORT:
            return 4;
	    
	case GL11.GL_COMPRESSED_TEXTURE_FORMATS:
	    return NUM_COMPRESSED_TEXTURE_FORMATS;

	case GL11.GL_MODELVIEW_MATRIX:
	case GL11.GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES:
	case GL11.GL_PROJECTION_MATRIX:
	case GL11.GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES:
	case GL11.GL_TEXTURE_MATRIX:
	case GL11.GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES:
            return 16;
	    
	default:
	    System.out.println("GLConfiguration: glGetNumParams called " +
                               "with pname=" + pname);
	    return 0;
	}
    }

    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glGetBufferParameteriv</code>.
     */
    public static int glGetBufferParametervNumParams(int pname) {
	return 1;
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glLight*v</code>.
     */
    public static int glLightNumParams(int pname) {
	switch (pname) {
	case GL11.GL_SPOT_EXPONENT:
	case GL11.GL_SPOT_CUTOFF:
	case GL11.GL_CONSTANT_ATTENUATION:
	case GL11.GL_LINEAR_ATTENUATION:
	case GL11.GL_QUADRATIC_ATTENUATION:
	    return 1;
	    
	case GL11.GL_SPOT_DIRECTION:
	    return 3;
	    
	case GL11.GL_POSITION:
	case GL11.GL_AMBIENT:
	case GL11.GL_DIFFUSE:
	case GL11.GL_SPECULAR:
	case GL11.GL_EMISSION:
	    return 4;
	    
	default:
	    System.out.println("GLConfiguration: glLightNumParams called " +
                               "with pname=" + pname);
	    return 0;
	}
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glLightModel*v</code>.
     */
    public static int glLightModelNumParams(int pname) {
	switch (pname) {
	case GL11.GL_LIGHT_MODEL_AMBIENT:
	    return 4;
	case GL11.GL_LIGHT_MODEL_TWO_SIDE:
	    return 1;
	default:
	    System.out.println("GLConfiguration: glLightModelNumParams " +
                               "called with pname=" + pname);
	    return 0;
	}
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glMaterial*v</code>.
     */
    public static int glMaterialNumParams(int pname) {
	switch (pname) {
	case GL11.GL_AMBIENT:
	case GL11.GL_DIFFUSE:
	case GL11.GL_SPECULAR:
	case GL11.GL_EMISSION:
	    return 4;
	    
	case GL11.GL_SHININESS:
	    return 1;
	    
	default:
	    System.out.println("GLConfiguration: glMaterialNumParams " +
                               "called with pname=" + pname);
	    return 0;
	}
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glPointParameter*v</code>.
     */
    public static int glPointParameterNumParams(int pname) {
	return (pname == GL11.GL_POINT_DISTANCE_ATTENUATION) ? 3 : 1;
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glTexEnv*v</code>.
     */
    public static int glTexEnvNumParams(int pname) {
	switch (pname) {
	case GL11.GL_TEXTURE_ENV_MODE:
	case GL11.GL_COMBINE_RGB:
	case GL11.GL_COMBINE_ALPHA:
	case GL11.GL_COORD_REPLACE_OES:
	    return 1;
	case GL11.GL_TEXTURE_ENV_COLOR:
	    return 4;
	default:
	    System.out.println("GLConfiguration: glTexEnvNumParams called " +
                               "with pname=" + pname);
	    return 0;
	}
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glTexGen*v</code>.
     */
    public static int glTexGenNumParams(int pname) {
	return 1;
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glTexParameter*v</code>.
     */
    public static int glTexParameterNumParams(int pname) {
	switch (pname) {
	case GL11.GL_TEXTURE_MIN_FILTER:
	case GL11.GL_TEXTURE_MAG_FILTER:
	case GL11.GL_TEXTURE_WRAP_S:
	case GL11.GL_TEXTURE_WRAP_T:
	case GL11.GL_GENERATE_MIPMAP:
	    return 1;
	case GL11Ext.GL_TEXTURE_CROP_RECT_OES:
	    return 4;
	default:
	    System.out.println("GLConfiguration: glTexParameterNumParams " +
                               "called with pname=" + pname);
	    return 0;
	}
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by <code>glRenderbufferParameterivOES</code>
     * and <code>glGetRenderbufferParameterivOES</code>.
     */
    public static int glRenderbufferParameterNumParams(int pname) {
	return 1;
    }
    
    /**
     * Returns the number of values required by the given parameter
     * <code>pname</code> by
     * <code>glFramebufferAttachmentParameterivOES</code> and
     * <code>glGetFramebufferAttachmentParameterivOES</code>.
     */
    public static int glFramebufferAttachmentParameterNumParams(int pname) {
	return 1;
    }

    /**
     * Returns the number of bytes associated with a given GL data type.
     */
    public static int sizeOfType(int type) {
        switch (type) {
        case GL11.GL_BYTE: case GL11.GL_UNSIGNED_BYTE:
            return 1;

        case GL11.GL_SHORT:
        case GL11.GL_UNSIGNED_SHORT_4_4_4_4:
        case GL11.GL_UNSIGNED_SHORT_5_5_5_1:
        case GL11.GL_UNSIGNED_SHORT_5_6_5:
            return 2;

        case GL11.GL_FIXED:
        case GL11.GL_FLOAT:
            return 4;
        }

        throw new IllegalArgumentException("Unknown type: " + type);
    }

    /**
     * Returns the number of channels associated with a given
     * GL pixel format.
     */
    public static int formatChannels(int format) {
        switch (format) {
        case GL11.GL_LUMINANCE:
        case GL11.GL_ALPHA:
            return 1;

        case GL11.GL_LUMINANCE_ALPHA:
            return 2;

        case GL11.GL_RGB:
            return 3;

        case GL11.GL_RGBA:
            return 4;
        }

        throw new IllegalArgumentException("Unknown format: " + format);
    }
}
