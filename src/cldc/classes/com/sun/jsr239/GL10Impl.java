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

import javax.microedition.khronos.egl.*;
import javax.microedition.khronos.opengles.*;
import java.nio.*;
import java.util.Hashtable;

public class GL10Impl implements GL10, GL10Ext {

    static final boolean debugQueue = false;

    static final boolean DEBUG_MEM = false;

    static EGL10 egl = (EGL10)EGLContext.getEGL();

    /**
     * <code>context</code> is the context the GL10
     * is created for. 
     */
    EGLContext context = null;

    int pixelStorePackAlignment = 4;
    int pixelStoreUnpackAlignment = 4;

    // Constants used to index gl*Pointer information tables.

    int VERTEX_POINTER       = 0;
    int COLOR_POINTER        = 1;
    int NORMAL_POINTER       = 2;
    int TEX_COORD_POINTER    = 3;
    int POINT_SIZE_POINTER   = 4;
    int MATRIX_INDEX_POINTER = 5;
    int WEIGHT_POINTER       = 6;

    // References to long-lived Buffers that remain part of the GL
    // state even after a qflush().  Indexed by *_POINTER constants,
    // above.

    Buffer[] pointerBuffer = new Buffer[7];

    // Parameters to gl*Pointer that affect bounds checking of
    // glDrawArrays and glDrawElements.  Indexed by *_POINTER
    // constants, above.

    boolean[] pointerEnabled = new boolean[7];
    int[] pointerSize      = new int[7];
    int[] pointerType      = new int[7];
    int[] pointerStride    = new int[7];
    int[] pointerOffset    = new int[7];
    int[] pointerRemaining = new int[7];

    public static final int CMD_ACTIVE_TEXTURE                      =   1;
    public static final int CMD_ALPHA_FUNC                          =   2;
    public static final int CMD_ALPHA_FUNCX                         =   3;
    public static final int CMD_BIND_BUFFER                         =   4;
    public static final int CMD_BIND_TEXTURE                        =   5;
    public static final int CMD_BLEND_FUNC                          =   6;
    public static final int CMD_BUFFER_DATA                         =   7;
    public static final int CMD_BUFFER_SUB_DATA                     =   8;
    public static final int CMD_CLEAR                               =   9;
    public static final int CMD_CLEAR_COLOR                         =  10;
    public static final int CMD_CLEAR_COLORX                        =  11;
    public static final int CMD_CLEAR_DEPTHF                        =  12;
    public static final int CMD_CLEAR_DEPTHX                        =  13;
    public static final int CMD_CLEAR_STENCIL                       =  14;
    public static final int CMD_CLIENT_ACTIVE_TEXTURE               =  15;
    public static final int CMD_CLIP_PLANEF                         =  16;
    public static final int CMD_CLIP_PLANEFB                        =  17;
    public static final int CMD_CLIP_PLANEX                         =  18;
    public static final int CMD_CLIP_PLANEXB                        =  19;
    public static final int CMD_COLOR4F                             =  20;
    public static final int CMD_COLOR4X                             =  21;
    public static final int CMD_COLOR4UB                            =  22;
    public static final int CMD_COLOR_MASK                          =  23;
    public static final int CMD_COLOR_POINTER                       =  24;
    public static final int CMD_COLOR_POINTER_VBO                   =  25;
    public static final int CMD_COMPRESSED_TEX_IMAGE_2D             =  26;
    public static final int CMD_COMPRESSED_TEX_SUB_IMAGE_2D         =  27;
    public static final int CMD_COPY_TEX_IMAGE_2D                   =  28;
    public static final int CMD_COPY_TEX_SUB_IMAGE_2D               =  29;
    public static final int CMD_CULL_FACE                           =  30;
    public static final int CMD_CURRENT_PALETTE_MATRIX              =  31;
    public static final int CMD_DELETE_BUFFERS                      =  32;
    public static final int CMD_DELETE_BUFFERSB                     =  33;
    public static final int CMD_DELETE_TEXTURES                     =  34;
    public static final int CMD_DELETE_TEXTURESB                    =  35;
    public static final int CMD_DEPTH_FUNC                          =  36;
    public static final int CMD_DEPTH_MASK                          =  37;
    public static final int CMD_DEPTH_RANGEF                        =  38;
    public static final int CMD_DEPTH_RANGEX                        =  39;
    public static final int CMD_DISABLE                             =  40;
    public static final int CMD_DISABLE_CLIENT_STATE                =  41;
    public static final int CMD_DRAW_ARRAYS                         =  42;
    public static final int CMD_DRAW_ELEMENTSB                      =  43;
    public static final int CMD_DRAW_ELEMENTS_VBO                   =  44;
    public static final int CMD_DRAW_TEXF                           =  45;
    public static final int CMD_DRAW_TEXFB                          =  46;
    public static final int CMD_DRAW_TEXI                           =  47;
    public static final int CMD_DRAW_TEXIB                          =  48;
    public static final int CMD_DRAW_TEXS                           =  49;
    public static final int CMD_DRAW_TEXSB                          =  50;
    public static final int CMD_DRAW_TEXX                           =  51;
    public static final int CMD_DRAW_TEXXB                          =  52;
    public static final int CMD_ENABLE                              =  53;
    public static final int CMD_ENABLE_CLIENT_STATE                 =  54;
    public static final int CMD_FOGF                                =  55;
    public static final int CMD_FOGFB                               =  56;
    public static final int CMD_FOGFV                               =  57;
    public static final int CMD_FOGX                                =  58;
    public static final int CMD_FOGXB                               =  59;
    public static final int CMD_FOGXV                               =  60;
    public static final int CMD_FRONT_FACE                          =  61;
    public static final int CMD_FRUSTUMF                            =  62;
    public static final int CMD_FRUSTUMX                            =  63;
    public static final int CMD_HINT                                =  64;
    public static final int CMD_LIGHTF                              =  65;
    public static final int CMD_LIGHTFB                             =  66;
    public static final int CMD_LIGHTFV                             =  67;
    public static final int CMD_LIGHTX                              =  68;
    public static final int CMD_LIGHTXB                             =  69;
    public static final int CMD_LIGHTXV                             =  70;
    public static final int CMD_LIGHT_MODELF                        =  71;
    public static final int CMD_LIGHT_MODELFB                       =  72;
    public static final int CMD_LIGHT_MODELFV                       =  73;
    public static final int CMD_LIGHT_MODELX                        =  74;
    public static final int CMD_LIGHT_MODELXB                       =  75;
    public static final int CMD_LIGHT_MODELXV                       =  76;
    public static final int CMD_LINE_WIDTH                          =  77;
    public static final int CMD_LINE_WIDTHX                         =  78;
    public static final int CMD_LOAD_IDENTITY                       =  79;
    public static final int CMD_LOAD_MATRIXF                        =  80;
    public static final int CMD_LOAD_MATRIXFB                       =  81;
    public static final int CMD_LOAD_MATRIXX                        =  82;
    public static final int CMD_LOAD_MATRIXXB                       =  83;
    public static final int CMD_LOAD_PALETTE_FROM_MODEL_VIEW_MATRIX =  84;
    public static final int CMD_LOGIC_OP                            =  85;
    public static final int CMD_MATERIALF                           =  86;
    public static final int CMD_MATERIALFB                          =  87;
    public static final int CMD_MATERIALFV                          =  88;
    public static final int CMD_MATERIALX                           =  89;
    public static final int CMD_MATERIALXB                          =  90;
    public static final int CMD_MATERIALXV                          =  91;
    public static final int CMD_MATRIX_INDEX_POINTER                =  92;
    public static final int CMD_MATRIX_INDEX_POINTER_VBO            =  93;
    public static final int CMD_MATRIX_MODE                         =  94;
    public static final int CMD_MULTI_TEXT_COORD4F                  =  95;
    public static final int CMD_MULTI_TEXT_COORD4X                  =  96;
    public static final int CMD_MULT_MATRIXF                        =  97;
    public static final int CMD_MULT_MATRIXFB                       =  98;
    public static final int CMD_MULT_MATRIXX                        =  99;
    public static final int CMD_MULT_MATRIXXB                       = 100;
    public static final int CMD_NORMAL3F                            = 101;
    public static final int CMD_NORMAL3X                            = 102;
    public static final int CMD_NORMAL_POINTER                      = 103;
    public static final int CMD_NORMAL_POINTER_VBO                  = 104;
    public static final int CMD_ORTHOF                              = 105;
    public static final int CMD_ORTHOX                              = 106;
    public static final int CMD_PIXEL_STOREI                        = 107;
    public static final int CMD_POINT_PARAMETERF                    = 108;
    public static final int CMD_POINT_PARAMETERFB                   = 109;
    public static final int CMD_POINT_PARAMETERFV                   = 110;
    public static final int CMD_POINT_PARAMETERX                    = 111;
    public static final int CMD_POINT_PARAMETERXB                   = 112;
    public static final int CMD_POINT_PARAMETERXV                   = 113;
    public static final int CMD_POINT_SIZE                          = 114;
    public static final int CMD_POINT_SIZEX                         = 115;
    public static final int CMD_POINT_SIZE_POINTER                  = 116;
    public static final int CMD_POINT_SIZE_POINTER_VBO              = 117;
    public static final int CMD_POLYGON_OFFSET                      = 118;
    public static final int CMD_POLYGON_OFFSETX                     = 119;
    public static final int CMD_POP_MATRIX                          = 120;
    public static final int CMD_PUSH_MATRIX                         = 121;
    public static final int CMD_ROTATEF                             = 122;
    public static final int CMD_ROTATEX                             = 123;
    public static final int CMD_SAMPLE_COVERAGE                     = 124;
    public static final int CMD_SAMPLE_COVERAGEX                    = 125;
    public static final int CMD_SCALEF                              = 126;
    public static final int CMD_SCALEX                              = 127;
    public static final int CMD_SCISSOR                             = 128;
    public static final int CMD_SHADE_MODEL                         = 129;
    public static final int CMD_STENCIL_FUNC                        = 130;
    public static final int CMD_STENCIL_MASK                        = 131;
    public static final int CMD_STENCIL_OP                          = 132;
    public static final int CMD_TEX_COORD_POINTER                   = 133;
    public static final int CMD_TEX_COORD_POINTER_VBO               = 134;
    public static final int CMD_TEX_ENVF                            = 135;
    public static final int CMD_TEX_ENVFB                           = 136;
    public static final int CMD_TEX_ENVFV                           = 137;
    public static final int CMD_TEX_ENVI                            = 138;
    public static final int CMD_TEX_ENVIB                           = 139;
    public static final int CMD_TEX_ENVIV                           = 140;
    public static final int CMD_TEX_ENVX                            = 141;
    public static final int CMD_TEX_ENVXB                           = 142;
    public static final int CMD_TEX_ENVXV                           = 143;
    public static final int CMD_TEX_IMAGE_2D                        = 144;
    public static final int CMD_TEX_PARAMETERF                      = 145;
    public static final int CMD_TEX_PARAMETERFB                     = 146;
    public static final int CMD_TEX_PARAMETERFV                     = 147;
    public static final int CMD_TEX_PARAMETERI                      = 148;
    public static final int CMD_TEX_PARAMETERIB                     = 149;
    public static final int CMD_TEX_PARAMETERIV                     = 150;
    public static final int CMD_TEX_PARAMETERX                      = 151;
    public static final int CMD_TEX_PARAMETERXB                     = 152;
    public static final int CMD_TEX_PARAMETERXV                     = 153;
    public static final int CMD_TEX_SUB_IMAGE_2D                    = 154;
    public static final int CMD_TRANSLATEF                          = 155;
    public static final int CMD_TRANSLATEX                          = 156;
    public static final int CMD_VERTEX_POINTER                      = 157;
    public static final int CMD_VERTEX_POINTER_VBO                  = 158;
    public static final int CMD_VIEWPORT                            = 159;
    public static final int CMD_WEIGHT_POINTER                      = 160;
    public static final int CMD_WEIGHT_POINTER_VBO                  = 161;
    public static final int CMD_FINISH                              = 162;
    public static final int CMD_FLUSH                               = 163;
    public static final int CMD_TEX_GENF                            = 164;
    public static final int CMD_TEX_GENI                            = 165;
    public static final int CMD_TEX_GENX                            = 166;
    public static final int CMD_TEX_GENFB                           = 167;
    public static final int CMD_TEX_GENIB                           = 168;
    public static final int CMD_TEX_GENXB                           = 169;
    public static final int CMD_TEX_GENFV                           = 170;
    public static final int CMD_TEX_GENIV                           = 171;
    public static final int CMD_TEX_GENXV                           = 172;
    public static final int CMD_BLEND_EQUATION                      = 173;
    public static final int CMD_BLEND_FUNC_SEPARATE                 = 174;
    public static final int CMD_BLEND_EQUATION_SEPARATE             = 175;
    public static final int CMD_BIND_RENDERBUFFER                   = 176;
    public static final int CMD_DELETE_RENDERBUFFERS                = 177;
    public static final int CMD_DELETE_RENDERBUFFERSB               = 178;
    public static final int CMD_GEN_RENDERBUFFERSB                  = 179;
    public static final int CMD_RENDERBUFFER_STORAGE                = 180;
    public static final int CMD_BIND_FRAMEBUFFER                    = 181;
    public static final int CMD_DELETE_FRAMEBUFFERS                 = 182;
    public static final int CMD_DELETE_FRAMEBUFFERSB                = 183;
    public static final int CMD_GEN_FRAMEBUFFERSB                   = 184;
    public static final int CMD_FRAMEBUFFER_TEXTURE2D               = 185;
    public static final int CMD_FRAMEBUFFER_RENDERBUFFER            = 186;
    public static final int CMD_GENERATE_MIPMAP                     = 187;
    public static final int CMD_GEN_BUFFERSB                        = 188;
    public static final int CMD_GEN_TEXTURESB                       = 189;

    static final String[] commandStrings = {
        null,
        "CMD_ACTIVE_TEXTURE",
        "ALPHA_FUNC",
        "ALPHA_FUNCX",
        "BIND_BUFFER",
        "BIND_TEXTURE",
        "BLEND_FUNC",
        "BUFFER_DATA",
        "BUFFER_SUB_DATA",
        "CLEAR",
        "CLEAR_COLOR",
        "CLEAR_COLORX",
        "CLEAR_DEPTHF",
        "CLEAR_DEPTHX",
        "CLEAR_STENCIL",
        "CLIENT_ACTIVE_TEXTURE",
        "CLIP_PLANEF",
        "CLIP_PLANEFB",
        "CLIP_PLANEX",
        "CLIP_PLANEXB",
        "COLOR4F",
        "COLOR4X",
        "COLOR4UB",
        "COLOR_MASK",
        "COLOR_POINTER",
        "COLOR_POINTER_VBO",
        "COMPRESSED_TEX_IMAGE_2D",
        "COMPRESSED_TEX_SUB_IMAGE_2D",
        "COPY_TEX_IMAGE_2D",
        "COPY_TEX_SUB_IMAGE_2D",
        "CULL_FACE",
        "CURRENT_PALETTE_MATRIX",
        "DELETE_BUFFERS",
        "DELETE_BUFFERSB",
        "DELETE_TEXTURES",
        "DELETE_TEXTURESB",
        "DEPTH_FUNC",
        "DEPTH_MASK",
        "DEPTH_RANGEF",
        "DEPTH_RANGEX",
        "DISABLE",
        "DISABLE_CLIENT_STATE",
        "DRAW_ARRAYS",
        "DRAW_ELEMENTSB",
        "DRAW_ELEMENTS_VBO",
        "DRAW_TEXF",
        "DRAW_TEXFB",
        "DRAW_TEXI",
        "DRAW_TEXIB",
        "DRAW_TEXS",
        "DRAW_TEXSB",
        "DRAW_TEXX",
        "DRAW_TEXXB",
        "ENABLE",
        "ENABLE_CLIENT_STATE",
        "FOGF",
        "FOGFB",
        "FOGFV",
        "FOGX",
        "FOGXB",
        "FOGXV",
        "FRONT_FACE",
        "FRUSTUMF",
        "FRUSTUMX",
        "HINT",
        "LIGHTF",
        "LIGHTFB",
        "LIGHTFV",
        "LIGHTX",
        "LIGHTXB",
        "LIGHTXV",
        "LIGHT_MODELF",
        "LIGHT_MODELFB",
        "LIGHT_MODELFV",
        "LIGHT_MODELX",
        "LIGHT_MODELXB",
        "LIGHT_MODELXV",
        "LINE_WIDTH",
        "LINE_WIDTHX",
        "LOAD_IDENTITY",
        "LOAD_MATRIXF",
        "LOAD_MATRIXFB",
        "LOAD_MATRIXX",
        "LOAD_MATRIXXB",
        "LOAD_PALETTE_FROM_MODEL_VIEW_MATRIX",
        "LOGIC_OP",
        "MATERIALF",
        "MATERIALFB",
        "MATERIALFV",
        "MATERIALX",
        "MATERIALXB",
        "MATERIALXV",
        "MATRIX_INDEX_POINTER",
        "MATRIX_INDEX_POINTER_VBO",
        "MATRIX_MODE",
        "MULTI_TEXT_COORD4F",
        "MULTI_TEXT_COORD4X",
        "MULT_MATRIXF",
        "MULT_MATRIXFB",
        "MULT_MATRIXX",
        "MULT_MATRIXXB",
        "NORMAL3F",
        "NORMAL3X",
        "NORMAL_POINTER",
        "NORMAL_POINTER_VBO",
        "ORTHOF",
        "ORTHOX",
        "PIXEL_STOREI",
        "POINT_PARAMETERF",
        "POINT_PARAMETERFB",
        "POINT_PARAMETERFV",
        "POINT_PARAMETERX",
        "POINT_PARAMETERXB",
        "POINT_PARAMETERXV",
        "POINT_SIZE",
        "POINT_SIZEX",
        "POINT_SIZE_POINTER",
        "POINT_SIZE_POINTER_VBO",
        "POLYGON_OFFSET",
        "POLYGON_OFFSETX",
        "POP_MATRIX",
        "PUSH_MATRIX",
        "ROTATEF",
        "ROTATEX",
        "SAMPLE_COVERAGE",
        "SAMPLE_COVERAGEX",
        "SCALEF",
        "SCALEX",
        "SCISSOR",
        "SHADE_MODEL",
        "STENCIL_FUNC",
        "STENCIL_MASK",
        "STENCIL_OP",
        "TEX_COORD_POINTER",
        "TEX_COORD_POINTER_VBO",
        "TEX_ENVF",
        "TEX_ENVFB",
        "TEX_ENVFV",
        "TEX_ENVI",
        "TEX_ENVIB",
        "TEX_ENVIV",
        "TEX_ENVX",
        "TEX_ENVXB",
        "TEX_ENVXV",
        "TEX_IMAGE_2D",
        "TEX_PARAMETERF",
        "TEX_PARAMETERFB",
        "TEX_PARAMETERFV",
        "TEX_PARAMETERI",
        "TEX_PARAMETERIB",
        "TEX_PARAMETERIV",
        "TEX_PARAMETERX",
        "TEX_PARAMETERXB",
        "TEX_PARAMETERXV",
        "TEX_SUB_IMAGE_2D",
        "TRANSLATEF",
        "TRANSLATEX",
        "VERTEX_POINTER",
        "VERTEX_POINTER_VBO",
        "VIEWPORT",
        "WEIGHT_POINTER",
        "WEIGHT_POINTER_VBO",
        "FINISH",
        "FLUSH",
        "TEX_GENF",
        "TEX_GENI",
        "TEX_GENX",
        "TEX_GENFB",
        "TEX_GENIB",
        "TEX_GENXB",
        "TEX_GENFV",
        "TEX_GENIV",
        "TEX_GENXV",
        "BLEND_EQUATION",
        "BLEND_FUNC_SEPARATE",
        "BLEND_EQUATION_SEPARATE",
        "BIND_RENDERBUFFER",
        "DELETE_RENDERBUFFERS",
        "DELETE_RENDERBUFFERSB",
        "GEN_RENDERBUFFERSB",
        "RENDERBUFFER_STORAGE",
        "BIND_FRAMEBUFFER",
        "DELETE_FRAMEBUFFERS",
        "DELETE_FRAMEBUFFERSB",
        "GEN_FRAMEBUFFERSB",
        "FRAMEBUFFER_TEXTURE2D",
        "FRAMEBUFFER_RENDERBUFFER",
        "GENERATE_MIPMAP",
        "GEN_BUFFERSB",
        "GEN_TEXTURESB"
    };

    native void    _glGenerateError(int error);
    
    native void    _glGenBuffers(int n, int[] buffers, int offset);
    native void    _glGenTextures(int n, int[] textures, int offset);
    native int     _glGetError();
    native void    _glGetIntegerv(int pname, int[] params,
                                  int offset, int length);
    native String  _glGetString(int name);
    native void    _glGetBooleanv(int pname, int[] params,
                                  int offset, int length);
    native void    _glGetFixedv(int pname, int[] params,
                                int offset, int length);
    native void    _glGetFloatv(int pname, float[] params,
                                int offset, int length);
    native void    _glGetLightfv(int light, int pname,
                                 float[] params, int offset,
                                 int length);
    native void    _glGetMaterialfv(int face, int pname,
                                    float[] params, int offset,
                                    int length);
    native void    _glGetMaterialxv(int face, int pname,
                                    int[] params, int offset,
                                    int length);
    native void    _glGetLightxv(int light, int pname,
                                 int[] params, int offset,
                                 int length);
    native void    _glGetTexEnvfv(int env, int pname,
                                  float[] params, int offset,
                                  int length);
    native void    _glGetTexEnviv(int env, int pname,
                                  int[] params, int offset,
                                  int length);
    native void    _glGetTexEnvxv(int env, int pname,
                                  int[] params, int offset,
                                  int length);
    native void    _glGetTexParameterfv(int target, int pname,
                                        float[] params, int offset,
                                        int length);
    native void    _glGetTexParameteriv(int target, int pname,
                                        int[] params, int offset,
                                        int length);
    native void    _glGetTexParameterxv(int target, int pname,
                                        int[] params, int offset,
                                        int length);
    native void    _glGetBufferParameteriv(int target, int pname,
                                           int[] params, int offset,
                                           int length);
    native void    _glGetClipPlanef(int pname,
                                    float[] eqn, int offset);
    native void    _glGetClipPlanex(int pname, int[] eqn, int offset);
    native int     _glIsBuffer(int buffer);
    native int     _glIsEnabled(int cap);
    native int     _glIsTexture(int texture);
    native void    _glReadPixelsPtr(int x, int y,
                                    int width, int height,
                                    int format, int type,
                                    int pointer);
    native void    _glReadPixelsByte(int x, int y,
                                     int width, int height,
                                     int format, int type,
                                     byte[] array, int offset);
    native void    _glReadPixelsInt(int x, int y,
                                    int width, int height,
                                    int format, int type,
                                    int[] array, int offset);

    // OES_query_matrix

    native int     _glQueryMatrixxOES(int[] mantissa,
                                              int mantissaOffset,
                                              int[] exponent,
                                              int exponentOffset);

    // OES_texture_cube_map


    native int     _glGetTexGenfv(int coord, int pname,
                                          float[] params,
                                          int offset, int length);
    native int     _glGetTexGeniv(int coord, int pname,
                                          int[] params,
                                          int offset, int length);
    native int     _glGetTexGenxv(int coord, int pname,
                                          int[] params,
                                          int offset, int length);

    // OES_framebuffer_object

    native int     _glIsRenderbufferOES(int renderbuffer);
    native void    _glGenRenderbuffersOES(int n, int[] renderbuffers,
                                                  int offset);
    native void    _glGetRenderbufferParameterivOES(int target,
                                                            int pname,
                                                            int[] params,
                                                            int offset,
                                                            int length);
    native int     _glIsFramebufferOES(int framebuffer);
    native void    _glGenFramebuffersOES(int n, int[] framebuffers,
                                                 int offset);
    native int     _glCheckFramebufferStatusOES(int target);
    native void
        _glGetFramebufferAttachmentParameterivOES(int target,
                                                  int attachment,
                                                  int pname,
                                                  int[] params,
                                                  int offset,
                                                  int length);
    
    // Execute a queue of GL commands
    native void _execute(int[] queue, int count);

    int[] queue = new int[GLConfiguration.COMMAND_QUEUE_SIZE];
    int index = 0;

    int commandsLeft;

    void throwIAE(String message) {
        throw new IllegalArgumentException(message);
    }

    public void qflush() {
        if (debugQueue) {
            System.out.println("Flushing the queue, index = " + index);
        }
        grabContext();
        _execute(queue, index);
        index = 0;

        // Ensure GL does not starve other threads
        Thread.yield();
    }

    void q(int cmd, int count) {
        if (index + count + 1 >= GLConfiguration.COMMAND_QUEUE_SIZE) {
            qflush();
        }
        if (debugQueue) {
            System.out.println("Queueing command " + 
                               commandStrings[cmd] + " with " +
                               count + " args from thread " +
                               Thread.currentThread());
        }
        queue[index++] = cmd;
        commandsLeft = count;
    }

    void q(int i) {
        queue[index++] = i;
    
        if (debugQueue) {
            System.out.println("Queueing integer " + i +
                               " from thread " +
                               Thread.currentThread());
        }

        if (GLConfiguration.flushQueueAfterEachCommand &&
            (--commandsLeft == 0)) {
            if (debugQueue) {
                System.out.println("Last arg for command, flushing");
            }
            qflush();
        }
    }

    int floatToIntBits(float f) {
        int i = Float.floatToIntBits(f);
        if (GLConfiguration.SWAP_FLOAT_BYTES) {
            // Start: AABBCCDD
            // End:   DDCCBBAA
            i = (((i << 24) & 0xff000000) |
                 ((i <<  8) & 0x00ff0000) |
                 ((i >>  8) & 0x0000ff00) |
                 ((i >> 24) & 0x000000ff));
        }
        return i;
    }
  
    void q(float f) {
        int i = floatToIntBits(f);
        queue[index++] = i;

        if (debugQueue) {
            System.out.println("Queueing float " + f +
                               " from thread " +
                               Thread.currentThread());
        }

        if (GLConfiguration.flushQueueAfterEachCommand &&
            (--commandsLeft == 0)) {
            if (debugQueue) {
                System.out.println("Last arg for command, flushing");
            }
            qflush();
        }
    }

    // offset will be shifted according to the buffer datatype
    // and added to the native base address
    static native int _getNativeAddress(Buffer buffer, int offset);

    int pointer(Buffer buffer) {
        int offset = buffer.position();
        int nativeAddress = _getNativeAddress(buffer, offset);

        return nativeAddress;
    }

    int offset(ByteBuffer buffer) {
        return buffer.arrayOffset() + buffer.position();
    }

    int offset(ShortBuffer buffer) {
        return buffer.arrayOffset() + buffer.position();
    }

    int offset(IntBuffer buffer) {
        return buffer.arrayOffset() + buffer.position();
    }

    int offset(FloatBuffer buffer) {
        return buffer.arrayOffset() + buffer.position();
    }

    void q(Buffer buf) {
        q(pointer(buf));

        if (debugQueue) {
            System.out.println("Queueing buffer pointer " + pointer(buf) +
                               " from thread " +
                               Thread.currentThread());
        }

        if (GLConfiguration.flushQueueAfterEachCommand &&
            (--commandsLeft == 0)) {
            if (debugQueue) {
                System.out.println("Last arg for command, flushing");
            }
            qflush();
        }
    }

    boolean isDirect(Buffer buf) {
        if (buf instanceof ByteBuffer) {
            return ((ByteBuffer)buf).isDirect();
        } else if (buf instanceof ShortBuffer) {
            return ((ShortBuffer)buf).isDirect();
        } else if (buf instanceof IntBuffer) {
            return ((IntBuffer)buf).isDirect();
        } else if (buf instanceof FloatBuffer) {
            return ((FloatBuffer)buf).isDirect();
        } else {
            throw new IllegalArgumentException(Errors.GL_UNKNOWN_BUFFER);
        }
    }

    Buffer createDirectCopy(Buffer data) {
        int length = data.remaining();
    
        ByteBuffer direct;
        if (data instanceof ByteBuffer) {
            direct = ByteBuffer.allocateDirect(length);
            direct.put((ByteBuffer)data);
            return direct;
        } else if (data instanceof ShortBuffer) {
            direct = ByteBuffer.allocateDirect(2*length);
            ShortBuffer directShort = direct.asShortBuffer();
            directShort.put((ShortBuffer)data);
            return directShort;
        } else if (data instanceof IntBuffer) {
            direct = ByteBuffer.allocateDirect(4*length);
            IntBuffer directInt = direct.asIntBuffer();
            directInt.put((IntBuffer)data);
            return directInt;
        } else if (data instanceof FloatBuffer) {
            direct = ByteBuffer.allocateDirect(4*length);
            FloatBuffer directFloat = direct.asFloatBuffer();
            directFloat.put((FloatBuffer)data);
            return directFloat;
        } else {
            throw new IllegalArgumentException(Errors.GL_UNKNOWN_BUFFER);
        }
    }

    /**
     * Utility for common error checking.
     * 
     * @exception <code>IllegalArgumentException</code> if
     * <code>param</code> is <code>null</code> or shorter than
     * <code>length</code>.
     */
    void checkLength(boolean[] params, int length, int offset) {
        if (params == null) {
            throwIAE(Errors.GL_PARAMS_NULL);
        }
        if (offset < 0) {
            throwIAE(Errors.GL_OFFSET_NEGATIVE);
        }
        if (params.length - offset < length) {
            throwIAE(Errors.GL_BAD_LENGTH);
        }
    }

    /**
     * Utility for common error checking.
     * 
     * @exception <code>IllegalArgumentException</code> if
     * <code>param</code> is <code>null</code> or shorter than
     * <code>length</code>.
     */
    void checkLength(short[] params, int length, int offset) {
        if (params == null) {
            throwIAE(Errors.GL_PARAMS_NULL);
        }
        if (offset < 0) {
            throwIAE(Errors.GL_OFFSET_NEGATIVE);
        }
        if (params.length - offset < length) {
            throwIAE(Errors.GL_BAD_LENGTH);
        }
    }

    /**
     * Utility for common error checking.
     * 
     * @exception <code>IllegalArgumentException</code> if
     * <code>param</code> is <code>null</code> or shorter than
     * <code>length</code>.
     */
    void checkLength(int[] params, int length, int offset) {
        if (params == null) {
            throwIAE(Errors.GL_PARAMS_NULL);
        }
        if (offset < 0) {
            throwIAE(Errors.GL_OFFSET_NEGATIVE);
        }
        if (params.length - offset < length) {
            throwIAE(Errors.GL_BAD_LENGTH);
        }
    }

    /**
     * Utility for common error checking.
     * 
     * @exception <code>IllegalArgumentException</code> if
     * <code>param</code> is <code>null</code> or shorter than
     * <code>length</code>.
     */
    void checkLength(float[] params, int length, int offset) {
        if (params == null) {
            throwIAE(Errors.GL_PARAMS_NULL);
        }
        if (offset < 0) {
            throwIAE(Errors.GL_OFFSET_NEGATIVE);
        }
        if (params.length - offset < length) {
            throwIAE(Errors.GL_BAD_LENGTH);
        }
    }

    /**
     * Utility for common error checking.
     * 
     * @exception <code>IllegalArgumentException</code> if
     * <code>param</code> is <code>null</code> or shorter than
     * <code>length</code>.
     */
    void checkLength(Buffer params, int length) {
        if (params == null) {
            throwIAE(Errors.GL_PARAMS_NULL);
        }
        if (params.remaining() < length) {
            throwIAE(Errors.GL_BAD_LENGTH);
        }    
    }

    void checkThread() {

        Thread boundThread = ((ContextAccess)context).getBoundThread();
        if (Thread.currentThread() != boundThread) {
            throw new IllegalStateException("GL call from improper thread");
        }
    }

    // If GLConfiguration.singleThreaded is true, the context that is
    // current on the (single) native thread, or EGL_NO_CONTEXT.  If
    // singleThreaded is false, this variable has no meaning.
    public static EGLContext currentContext = EGL10.EGL_NO_CONTEXT;

    /**
     * IMPL_NOTE: <code>contextsByThread</code> may lead to the Java memory
     * leaks (when a thread dies, an associated context will not be ever
     * collected). As the possible workaround the following solution can be
     * used: a private <code>Hashtable</code> member can be added for class
     * <code>Thread</code> to keep a reference to the context. In this case
     * the life span of a context will not be longer then that of a thread.
     */
    // Map Thread -> EGLContext
    public static Hashtable contextsByThread = new Hashtable();

    // Current cull face mode for CR 6401385 workaround
    public int cullFaceMode = GL_BACK;

    /**
     * Set the context associated with the current Java thread as the
     * native context.  This is only necessary if we are on a
     * single-threaded VM and the context is not already current.
     */
    public static void grabContext() {
        if (!GLConfiguration.singleThreaded) {
            return;
        }

        // Locate the desired context for this Java thread
        Thread currentThread = Thread.currentThread();
        EGLContext newContext =
                (EGLContext)contextsByThread.get(currentThread); 
        if (newContext == GL10Impl.currentContext) {
            return;
        }

        if (newContext != null) {
            EGLDisplay display = ((ContextAccess) newContext).getDisplay();

            EGLSurface draw = ((ContextAccess) newContext).getDrawSurface();

            EGLSurface read = ((ContextAccess) newContext).getReadSurface();

            egl.eglMakeCurrent(display, draw, read, newContext);

            GL10Impl.currentContext = newContext;
        }
    }

    // Begin GL methods

    public synchronized void glActiveTexture(int texture) {
        checkThread();
        q(CMD_ACTIVE_TEXTURE, 1);
        q(texture);
    }

    public synchronized void glAlphaFunc(int func, float ref) {
        checkThread();
        q(CMD_ALPHA_FUNC, 2);
        q(func);
        q(ref);
    }

    public synchronized void glAlphaFuncx(int func, int ref) {
        checkThread();
        q(CMD_ALPHA_FUNCX, 2);
        q(func);
        q(ref);
    }

    public synchronized void glBindTexture(int target, int texture) {
        checkThread();
        q(CMD_BIND_TEXTURE, 2);
        q(target);
        q(texture);
    }

    public synchronized void glBlendFunc(int sfactor, int dfactor) {
        checkThread();
        q(CMD_BLEND_FUNC, 2);
        q(sfactor);
        q(dfactor);
    }

    public synchronized void glClear(int mask) {
        checkThread();
        q(CMD_CLEAR, 1);
        q(mask);
    }

    public synchronized void glClearColor(float red,
                                          float green,
                                          float blue,
                                          float alpha) {
        checkThread();
        q(CMD_CLEAR_COLOR, 4);
        q(red);
        q(green);
        q(blue);
        q(alpha);
    }

    public synchronized void glClearColorx(int red,
                                           int green,
                                           int blue,
                                           int alpha) {
        checkThread();
        q(CMD_CLEAR_COLORX, 4);
        q(red);
        q(green);
        q(blue);
        q(alpha);
    }

    public synchronized void glClearDepthf(float depth) {
        checkThread();
        q(CMD_CLEAR_DEPTHF, 1);
        q(depth);
    }

    public synchronized void glClearDepthx(int depth) {
        checkThread();
        q(CMD_CLEAR_DEPTHX, 1);
        q(depth);
    }

    public synchronized void glClearStencil(int s) {
        checkThread();
        q(CMD_CLEAR_STENCIL, 1);
        q(s);
    }

    public synchronized void glClientActiveTexture(int texture) {
        checkThread();
        q(CMD_CLIENT_ACTIVE_TEXTURE, 1);
        q(texture);
    }

    void IglClipPlanef(int plane, float[] equation, int offset) {
        q(CMD_CLIP_PLANEF, 5);
        q(plane);
        q(equation[offset]);
        q(equation[offset + 1]);
        q(equation[offset + 2]);
        q(equation[offset + 3]);
    }

    void IglClipPlanex(int plane, int[] equation, int offset) {
        q(CMD_CLIP_PLANEX, 5);
        q(plane);
        q(equation[offset]);
        q(equation[offset + 1]);
        q(equation[offset + 2]);
        q(equation[offset + 3]);
    }

    public synchronized void glColor4f(float red,
                                       float green,
                                       float blue,
                                       float alpha) {
        checkThread();
        q(CMD_COLOR4F, 4);
        q(red);
        q(green);
        q(blue);
        q(alpha);
    }

    public synchronized void glColor4x(int red,
                                       int green,
                                       int blue,
                                       int alpha) {
        checkThread();
        q(CMD_COLOR4X, 4);
        q(red);
        q(green);
        q(blue);
        q(alpha);
    }

    public synchronized void glColor4ub(byte red,
                                        byte green,
                                        byte blue,
                                        byte alpha) {
        checkThread();
        q(CMD_COLOR4UB, 4);
        q((int)red);
        q((int)green);
        q((int)blue);
        q((int)alpha);
    }

    public synchronized void glColorMask(boolean red,
                                         boolean green,
                                         boolean blue,
                                         boolean alpha) {
        checkThread();
        q(CMD_COLOR_MASK, 4);
        q(red ? 1 : 0);
        q(green ? 1 : 0);
        q(blue ? 1 : 0);
        q(alpha ? 1 : 0);
    }

    public synchronized void glColorPointer(int size, int type, int stride,
                                            Buffer pointer) {
        checkThread();
        if (VBOArrayBufferBound != 0) {
            throw new IllegalStateException("glColorPointer:" +
                                            Errors.VBO_ARRAY_BUFFER_BOUND);
        }
        if (pointer == null) {
            throwIAE(Errors.GL_POINTER_NULL);
        }
        if (!isDirect(pointer)) {
            throwIAE(Errors.GL_NOT_DIRECT);
        }

        // Only record details if this is a legal operation
        if ((size == 4) && 
            (type == GL_UNSIGNED_BYTE ||
             type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[COLOR_POINTER]);
            BufferManager.useBuffer(pointer);
            
            pointerBuffer[COLOR_POINTER] = pointer;
            pointerSize[COLOR_POINTER] = size;
            pointerType[COLOR_POINTER] = type;
            pointerStride[COLOR_POINTER] = stride;
            int nbytes = bufferTypeSize(pointer);
            pointerRemaining[COLOR_POINTER] = pointer.remaining()*nbytes;
            pointerOffset[COLOR_POINTER] = 0;
        }

        q(CMD_COLOR_POINTER, 4);
        q(size);
        q(type);
        q(stride);
        q(pointer);

        qflush();
    }

    public synchronized void glCompressedTexImage2D(int target, int level,
                                                    int internalformat,
                                                    int width, int height,
                                                    int border, int imageSize,
                                                    Buffer data) {
        checkThread();
        checkLength(data, imageSize);

        boolean isReadOnly = false;
        if (!isDirect(data)) {
            data = createDirectCopy(data);
            isReadOnly = true;
        }

        // Need revisit: BufferManager stuff

        q(CMD_COMPRESSED_TEX_IMAGE_2D, 8);
        q(target);
        q(level);
        q(internalformat);
        q(width);
        q(height);
        q(border);
        q(imageSize);
        q(data);
    
        if (!isReadOnly) {
            qflush();
        }
    }
    
    public synchronized void
        glCompressedTexSubImage2D(int target, int level,
                                  int xoffset, int yoffset,
                                  int width, int height,
                                  int format, int imageSize,
                                  Buffer data) {
        checkThread();
        checkLength(data, imageSize);

        boolean isReadOnly = false;
        if (!isDirect(data)) {
            data = createDirectCopy(data);
            isReadOnly = true;
        }

        // Need revisit: BufferManager stuff

        q(CMD_COMPRESSED_TEX_SUB_IMAGE_2D, 9);
        q(target);
        q(level);
        q(xoffset);
        q(yoffset);
        q(width);
        q(height);
        q(format);
        q(imageSize);
        q(data);
    
        if (!isReadOnly) {
            qflush();
        }
    }

    public synchronized void glCopyTexImage2D(int target, int level,
                                              int internalformat,
                                              int x, int y,
                                              int width, int height,
                                              int border) {
        checkThread();
        q(CMD_COPY_TEX_IMAGE_2D, 8);
        q(target);
        q(level);
        q(internalformat);
        q(x);
        q(y);
        q(width);
        q(height);
        q(border);
    }

    public synchronized void glCopyTexSubImage2D(int target, int level,
                                                 int xoffset, int yoffset,
                                                 int x, int y,
                                                 int width, int height) {
        checkThread();
        q(CMD_COPY_TEX_SUB_IMAGE_2D, 8);
        q(target);
        q(level);
        q(xoffset);
        q(yoffset);
        q(x);
        q(y);
        q(width);
        q(height);
    }

    public synchronized void glCullFace(int mode) {
        checkThread();
        q(CMD_CULL_FACE, 1);
        q(mode);

        // Workaround for Gerbera bug, CR 6401385
        cullFaceMode = mode;
    }

    public synchronized void glDeleteTextures(int n,
                                              int[] textures, int offset) {
        checkThread();
        checkLength(textures, n, offset);

        IglDeleteTextures(n, textures, offset);
    }

    public synchronized void glDeleteTextures(int n, IntBuffer textures) {
        checkThread();
        // Need revisit: defend against race condition
        checkLength(textures, n);

        if (!textures.isDirect()) {
            IglDeleteTextures(n, textures.array(), offset(textures));
            return;
        }

        // Queue pointer
        q(CMD_DELETE_TEXTURESB, 2);
        q(n);
        q(textures);

        qflush();
    }

    void IglDeleteTextures(int n, int[] textures, int offset) {
        q(CMD_DELETE_TEXTURES, n + 1);
        q(n);
        for (int i = 0; i < n; i++) {
            q(textures[i + offset]);
        }
    }

    public synchronized void glDepthFunc(int func) {
        checkThread();
        q(CMD_DEPTH_FUNC, 1);
        q(func);
    }

    public synchronized void glDepthMask(boolean flag) {
        checkThread();
        q(CMD_DEPTH_MASK, 1);
        q(flag ? 1 : 0);
    }

    public synchronized void glDepthRangef(float zNear, float zFar) {
        checkThread();
        q(CMD_DEPTH_RANGEF, 2);
        q(zNear);
        q(zFar);
    }

    public synchronized void glDepthRangex(int zNear, int zFar) {
        checkThread();
        q(CMD_DEPTH_RANGEX, 2);
        q(zNear);
        q(zFar);
    }

    public synchronized void glDisable(int cap) {
        checkThread();
        q(CMD_DISABLE, 1);
        q(cap);
    }

    public synchronized void glDisableClientState(int array) {
        checkThread();

        switch (array) {
        case GL_VERTEX_ARRAY:
            pointerEnabled[VERTEX_POINTER] = false;
            break;
        case GL_COLOR_ARRAY:
            pointerEnabled[COLOR_POINTER] = false;
            break;
        case GL_NORMAL_ARRAY:
            pointerEnabled[NORMAL_POINTER] = false;
            break;
        case GL_TEXTURE_COORD_ARRAY:
            pointerEnabled[TEX_COORD_POINTER] = false;
            break;
        case GL11.GL_POINT_SIZE_ARRAY_OES:
            pointerEnabled[POINT_SIZE_POINTER] = false;
            break;
        case GL11Ext.GL_MATRIX_INDEX_ARRAY_OES:
            pointerEnabled[MATRIX_INDEX_POINTER] = false;
            break;
        case GL11Ext.GL_WEIGHT_ARRAY_OES:
            pointerEnabled[WEIGHT_POINTER] = false;
            break;
        }

        q(CMD_DISABLE_CLIENT_STATE, 1);
        q(array);
    }
    
    void checkBounds(int last) {
        for (int ptr = VERTEX_POINTER; ptr <= WEIGHT_POINTER; ptr++) {
            if (pointerEnabled[ptr]) {
                final int remaining = (pointerBuffer[ptr] != null)
                        ? pointerRemaining[ptr]
                        : getBufferSize(GL11.GL_ARRAY_BUFFER);
                final int size = pointerSize[ptr];
                final int type = pointerType[ptr];
                final int offset = pointerOffset[ptr];
                final int sizeOfType = GLConfiguration.sizeOfType(type);
                final int stride = pointerStride[ptr] == 0 ? sizeOfType :
                        pointerStride[ptr];
                final int elementSize = size * sizeOfType;

                final int lastByte = offset + last * stride + elementSize;

                if (lastByte > remaining) {
                    throw new ArrayIndexOutOfBoundsException(
                            String.valueOf(lastByte));
                }
            }
        }
    }
    
    public synchronized void glDrawArrays(int mode, int first, int count) {
        checkThread();

        checkBounds(first + count - 1);

        q(CMD_DRAW_ARRAYS, 3);
        q(mode);
        q(first);
        q(count);
    }

    void checkIndices(int[] indices) {
        int count = indices.length;

//         System.out.println("checkIndices:");
//         System.out.println("  indices = ");
//         for (int i = 0; i < indices.length; i++) {
//             System.out.print(indices[i] + " ");
//         }
//         System.out.println();
            
        for (int ptr = VERTEX_POINTER; ptr <= WEIGHT_POINTER; ptr++) {
//             System.out.println("ptr = " + ptr);
            if (pointerEnabled[ptr]) {
                int size, type, stride, offset, remaining;

                if (pointerBuffer[ptr] != null) {
                    remaining = pointerRemaining[ptr];
                } else {
                    remaining = getBufferSize(GL11.GL_ARRAY_BUFFER);
                }
//                 System.out.println("remaining = " + remaining);
                size = pointerSize[ptr];
                type = pointerType[ptr];
                stride = pointerStride[ptr];
                offset = pointerOffset[ptr];

//                 System.out.println("size = " + size);
//                 System.out.println("type = " + type);
//                 System.out.println("stride = " + stride);
//                 System.out.println("offset = " + offset);
                
                int elementSize = size*GLConfiguration.sizeOfType(type);
//                 System.out.println("elementSize = " + elementSize);
                
                for (int i = 0; i < count; i++) {
                    int idx = indices[i];
                    int bidx =
                        offset + idx*(elementSize + stride) + elementSize;
                    
//                     System.out.println("bidx[" + i + "] = " + bidx);
                    if (bidx > remaining) {
                        throw new ArrayIndexOutOfBoundsException("" +
                                                                 bidx);
                    }
                }
            }
        }
    }

    void checkDrawElementsBounds(byte[] indices) {
    }

    public synchronized void glDrawElements(int mode, int count, int type,
                                            Buffer indices) {
        checkThread();
        if (VBOElementArrayBufferBound != 0) {
            throw new IllegalStateException("glDrawElements:" +
                                        Errors.VBO_ELEMENT_ARRAY_BUFFER_BOUND);
        }
        if (indices == null) {
            throwIAE(Errors.GL_INDICES_NULL);
        }

        boolean isReadOnly = false;
        if (!isDirect(indices)) {
            indices = createDirectCopy(indices);
            isReadOnly = true;
        }

        // No need to bounds check if there will be a type error
        if (type == GL_UNSIGNED_BYTE ||
            type == GL_UNSIGNED_SHORT) {
            int nbytes = (type == GL_UNSIGNED_BYTE) ? 1 : 2;

            if (count > indices.remaining()) {
                throw new ArrayIndexOutOfBoundsException(
                                                        Errors.VBO_OFFSET_OOB);
            }
            
            if (DEBUG_MEM) {
                System.out.print("glDrawElements: Allocating bufferData " +
                                 count*nbytes);
            }
            byte[] bufferData = new byte[count*nbytes];
            BufferManager.getBytes(indices, 0, bufferData, 0, count*nbytes);

            if (DEBUG_MEM) {
                System.out.println(": done");
                System.out.print("glDrawElements: Allocating indexArray " +
                                 count);
            }
            int[] indexArray = new int[count];
            boolean isBigEndian = GLConfiguration.IS_BIG_ENDIAN;
            if (DEBUG_MEM) {
                System.out.println(": done");
            }

            if (type == GL_UNSIGNED_BYTE) {
                for (int i = 0; i < count; i++) {
                    indexArray[i] = bufferData[i] & 0xff;
                }
            } else if (type == GL_UNSIGNED_SHORT) {
                for (int i = 0; i < count; i++) {
                    int b0 = bufferData[2*i] & 0xff;
                    int b1 = bufferData[2*i + 1] & 0xff;
                    if (isBigEndian) {
                        indexArray[i] = (b0 << 8) | b1;
                    } else {
                        indexArray[i] = (b1 << 8) | b0;
                    }
                }
            }

            checkIndices(indexArray);
        }

        q(CMD_DRAW_ELEMENTSB, 4);
        q(mode);
        q(count);
        q(type);
        q(indices);
    
        if (!isReadOnly) {
            qflush();
        }
    }

    public synchronized void glEnable(int cap) {
        checkThread();
        q(CMD_ENABLE, 1);
        q(cap);
    }

    public synchronized void glEnableClientState(int array) {
        checkThread();

        switch (array) {
        case GL_VERTEX_ARRAY:
            pointerEnabled[VERTEX_POINTER] = true;
            break;
        case GL_COLOR_ARRAY:
            pointerEnabled[COLOR_POINTER] = true;
            break;
        case GL_NORMAL_ARRAY:
            pointerEnabled[NORMAL_POINTER] = true;
            break;
        case GL_TEXTURE_COORD_ARRAY:
            pointerEnabled[TEX_COORD_POINTER] = true;
            break;
        case GL11.GL_POINT_SIZE_ARRAY_OES:
            pointerEnabled[POINT_SIZE_POINTER] = true;
            break;
        case GL11Ext.GL_MATRIX_INDEX_ARRAY_OES:
            pointerEnabled[MATRIX_INDEX_POINTER] = true;
            break;
        case GL11Ext.GL_WEIGHT_ARRAY_OES:
            pointerEnabled[WEIGHT_POINTER] = true;
            break;
        }

        q(CMD_ENABLE_CLIENT_STATE, 1);
        q(array);
    }

    public synchronized void glFinish() {
        checkThread();
        q(CMD_FINISH, 0);
        qflush();
    }


    public synchronized void glFlush() {
        checkThread();
        q(CMD_FLUSH, 0);
        qflush();
    }

    public synchronized void glFogf(int pname, float param) {
        checkThread();
        q(CMD_FOGF, 2);
        q(pname);
        q(param);
    }

    public synchronized void glFogfv(int pname, float[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glFogNumParams(pname);
        checkLength(params, length, offset);

        IglFogfv(pname, params, offset);
    }

    public synchronized void glFogfv(int pname, FloatBuffer params) {
        checkThread();
        int length = GLConfiguration.glFogNumParams(pname);
        checkLength(params, length);

        if (!isDirect(params)) {
            IglFogfv(pname, params.array(), offset(params));
            return;
        }

        q(CMD_FOGFB, 2);
        q(pname);
        q(params);

        qflush();
    }

    void IglFogfv(int pname, float[] params, int offset) {
        int n = GLConfiguration.glFogNumParams(pname);

        q(CMD_FOGFV, n + 2);
        q(n);
        q(pname);
        for (int i = 0; i < n; i++) { 
            q(params[i + offset]);
        }
    }

    public synchronized void glFogx(int pname, int param) {
        checkThread();
        q(CMD_FOGX, 2);
        q(pname);
        q(param);
    }

    public synchronized void glFogxv(int pname, int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glFogNumParams(pname);
        checkLength(params, length, offset);

        IglFogxv(pname, params, offset);
    }

    public synchronized void glFogxv(int pname, IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glFogNumParams(pname);
        checkLength(params, length);

        if (!isDirect(params)) {
            IglFogxv(pname, params.array(), offset(params));
            return;
        }

        q(CMD_FOGXB, 2);
        q(pname);
        q(params);

        qflush();
    }
  
    void IglFogxv(int pname, int[] params, int offset) {
        int n = GLConfiguration.glFogNumParams(pname);

        q(CMD_FOGXV, n + 2);
        q(n);
        q(pname);
        for (int i = 0; i < n; i++) { 
            q(params[i + offset]);
        }
    }
  
    public synchronized void glFrontFace(int mode) {
        checkThread();
        q(CMD_FRONT_FACE, 1);
        q(mode);
    }

    public synchronized void glFrustumf(float left, float right,
                                        float bottom, float top,
                                        float zNear, float zFar) {
        checkThread();
        q(CMD_FRUSTUMF, 6);
        q(left);
        q(right);
        q(bottom);
        q(top);
        q(zNear);
        q(zFar);
    }

    public synchronized void glFrustumx(int left, int right,
                                        int bottom, int top,
                                        int zNear, int zFar) {
        checkThread();
        q(CMD_FRUSTUMX, 6);
        q(left);
        q(right);
        q(bottom);
        q(top);
        q(zNear);
        q(zFar);
    }

    public synchronized void glGenTextures(int n,
                                           int[] textures, int offset) {
        checkThread();
        checkLength(textures, n, offset);

        qflush();
        IglGenTextures(n, textures, offset);
    }

    public synchronized void glGenTextures(int n, IntBuffer textures) {
        checkThread();
        checkLength(textures, n);

        qflush();
        if (!isDirect(textures)) {
            IglGenTextures(n, textures.array(), offset(textures));
            return;
        }

        q(CMD_GEN_TEXTURESB, 2);
        q(n);
        q(textures);

        qflush();
    }

    void IglGenTextures(int n, int[] textures, int offset) {
        grabContext();
        _glGenTextures(n, textures, offset);
    }

    public synchronized void glGetBooleanv(int pname,
                                           boolean[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glGetNumParams(pname);
        checkLength(params, length, offset);

        qflush();

        int[] iparams = new int[length];
        IglGetBooleanv(pname, iparams, 0, length);
        for (int i = 0; i < length; i++) {
            params[offset + i] = (iparams[i] == 1);
        }
    }

    public synchronized void glGetBooleanv(int pname, IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glGetNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetBooleanv(pname, params.array(), offset(params), length);
        } else {
            IglGetBooleanv(pname, null, pointer(params), length);
        }
    }
  
    void IglGetBooleanv(int pname, int[] params, int offset, int length) {
        grabContext();
        _glGetBooleanv(pname, params, offset, length);
    }

    public synchronized void glGetIntegerv(int pname,
                                           int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glGetNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetIntegerv(pname, params, offset, length);

        // Workaround for Gerbera bug, CR 6401385
        if (pname == GL_CULL_FACE) {
            params[offset] = cullFaceMode;
        }
    }

    public synchronized void glGetIntegerv(int pname, IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glGetNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            int[] array = params.array();
            int offset = offset(params);

            IglGetIntegerv(pname, array, offset, length);

            // Workaround for Gerbera bug, CR 6401385
            if (pname == GL_CULL_FACE) {
                array[offset] = cullFaceMode;
            }
        } else {
            IglGetIntegerv(pname, null, pointer(params), length);

            // Workaround for Gerbera bug, CR 6401385
            if (pname == GL_CULL_FACE) {
                params.put(params.position(), cullFaceMode);
            }
        }
    }

    void IglGetIntegerv(int pname, int[] params, int offset, int length) {
        grabContext();
        _glGetIntegerv(pname, params, offset, length);
    }

    public synchronized int glGetError() {
        checkThread();
        qflush();
        
        grabContext();
        int error = _glGetError();
        return error;
    }

    public synchronized String glGetString(int name) {
        checkThread();
        qflush();

        grabContext();
        String s = _glGetString(name);
        return s;
    }

    public synchronized boolean glIsBuffer(int buffer) {
        checkThread();
        qflush();

        grabContext();
        boolean retval = GL_TRUE == _glIsBuffer(buffer);
        return retval;
    }

    public synchronized boolean glIsEnabled(int cap) {
        checkThread();
        qflush();

        grabContext();
        boolean retval = GL_TRUE == _glIsEnabled(cap);
        return retval;
    }

    public synchronized boolean glIsTexture(int texture) {
        checkThread();
        qflush();

        grabContext();
        boolean retval = false;

        // Woraround for Gerbera bug, see CR 6401677
        if (texture != 0) {
            retval = GL_TRUE == _glIsTexture(texture);
        }
        return retval;
    }

    public synchronized void glHint(int target, int mode) {
        checkThread();
        q(CMD_HINT, 2);
        q(target);
        q(mode);
    }

    public synchronized void glLightModelf(int pname, float param) {
        checkThread();
        q(CMD_LIGHT_MODELF, 2);
        q(pname);
        q(param);
    }

    public synchronized void glLightModelfv(int pname,
                                            float[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glLightModelNumParams(pname);
        checkLength(params, length, offset);

        IglLightModelfv(pname, params, offset);
    }

    public synchronized void glLightModelfv(int pname, FloatBuffer params) {
        checkThread();
        int length = GLConfiguration.glLightModelNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglLightModelfv(pname, params.array(), offset(params));
            return;
        }

        q(CMD_LIGHT_MODELFB, 2);
        q(pname);
        q(params);

        qflush();
    }

    void IglLightModelfv(int pname, float[] params, int offset) {
        int n = GLConfiguration.glLightModelNumParams(pname);

        q(CMD_LIGHT_MODELFV, n + 2);
        q(n);
        q(pname);
        for (int i = 0; i < n; i++) { 
            q(params[i + offset]);
        }
    }

    public synchronized void glLightModelx(int pname, int param) {
        checkThread();
        q(CMD_LIGHT_MODELX, 2);
        q(pname);
        q(param);
    }

    public synchronized void glLightModelxv(int pname,
                                            int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glLightModelNumParams(pname);
        checkLength(params, length, offset);

        IglLightModelxv(pname, params, offset);
    }

    public synchronized void glLightModelxv(int pname, IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glLightModelNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglLightModelxv(pname, params.array(), offset(params));
            return;
        }

        q(CMD_LIGHT_MODELXB, 2);
        q(pname);
        q(params);

        qflush();
    }

    void IglLightModelxv(int pname, int[] params, int offset) {
        int n = GLConfiguration.glLightModelNumParams(pname);

        q(CMD_LIGHT_MODELXV, n + 2);
        q(n);
        q(pname);
        for (int i = 0; i < n; i++) { 
            q(params[i + offset]);
        }
    }

    public synchronized void glLightf(int light, int pname, float param) {
        checkThread();
        q(CMD_LIGHTF, 3);
        q(light);
        q(pname);
        q(param);
    }

    public synchronized void glLightfv(int light, int pname,
                                       float[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glLightNumParams(pname);
        checkLength(params, length, offset);
    
        IglLightfv(light, pname, params, offset);
    }

    public synchronized void glLightfv(int light, int pname,
                                       FloatBuffer params) {
        checkThread();
        int length = GLConfiguration.glLightNumParams(pname);
        checkLength(params, length);
    
        if (!params.isDirect()) {
            IglLightfv(light, pname, params.array(), offset(params));
            return;
        }

        q(CMD_LIGHTFB, 3);
        q(light);
        q(pname);
        q(params);

        qflush();
    }

    void IglLightfv(int light, int pname, float[] params, int offset) {
        int n = GLConfiguration.glLightNumParams(pname);

        q(CMD_LIGHTFV, n + 3);
        q(n);
        q(light);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glLightx(int light, int pname, int param) {
        checkThread();
        q(CMD_LIGHTX, 3);
        q(light);
        q(pname);
        q(param);
    }

    public synchronized void glLightxv(int light, int pname,
                                       int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glLightNumParams(pname);
        checkLength(params, length, offset);
    
        IglLightxv(light, pname, params, offset);
    }

    public synchronized void glLightxv(int light, int pname,
                                       IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glLightNumParams(pname);
        checkLength(params, length);
    
        if (!params.isDirect()) {
            IglLightxv(light, pname, params.array(), offset(params));
            return;
        }

        q(CMD_LIGHTXB, 3);
        q(light);
        q(pname);
        q(params);

        qflush();
    }

    void IglLightxv(int light, int pname, int[] params, int offset) {
        int n = GLConfiguration.glLightNumParams(pname);

        q(CMD_LIGHTXV, n + 3);
        q(n);
        q(light);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glLineWidth(float width) {
        checkThread();
        q(CMD_LINE_WIDTH, 1);
        q(width);
    }

    public synchronized void glLineWidthx(int width) {
        checkThread();
        q(CMD_LINE_WIDTHX, 1);
        q(width);
    }

    public synchronized void glLoadIdentity() {
        checkThread();
        q(CMD_LOAD_IDENTITY, 0);
    }

    public synchronized void glLoadMatrixf(float[] m, int offset) {
        checkThread();
        checkLength(m, 16, offset);

        IglLoadMatrixf(m, offset);
    }

    public synchronized void glLoadMatrixf(FloatBuffer m) {
        checkThread();
        checkLength(m, 16);

        if (!m.isDirect()) {
            IglLoadMatrixf(m.array(), offset(m));
            return;
        }
        
        q(CMD_LOAD_MATRIXFB, 1);
        q(m);
        
        qflush();
    }

    void IglLoadMatrixf(float[] m, int offset) {
        q(CMD_LOAD_MATRIXF, 16);
        for (int i = 0; i < 16; i++) {
            q(m[i + offset]);
        }
    }

    public synchronized void glLoadMatrixx(int[] m, int offset) {
        checkThread();
        checkLength(m, 16, offset);

        IglLoadMatrixx(m, offset);
    }

    public synchronized void glLoadMatrixx(IntBuffer m) {
        checkThread();
        checkLength(m, 16);

        if (!m.isDirect()) {
            IglLoadMatrixx(m.array(), offset(m));
            return;
        }

        q(CMD_LOAD_MATRIXXB, 1);
        q(m);
        
        qflush();
    }

    void IglLoadMatrixx(int[] m, int offset) {
        q(CMD_LOAD_MATRIXX, 16);
        for (int i = 0; i < 16; i++) {
            q(m[i + offset]);
        }
    }

    public synchronized void glLogicOp(int opcode) {
        checkThread();
        q(CMD_LOGIC_OP, 1);
        q(opcode);
    }

    public synchronized void glMaterialf(int face, int pname, float param) {
        checkThread();
        q(CMD_MATERIALF, 3);
        q(face);
        q(pname);
        q(param);
    }

    public synchronized void glMaterialfv(int face, int pname,
                                          float[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glMaterialNumParams(pname);
        checkLength(params, length, offset);

        IglMaterialfv(face, pname, params, offset);
    }

    public synchronized void glMaterialfv(int face, int pname,
                                          FloatBuffer params) {
        checkThread();
        int length = GLConfiguration.glMaterialNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglMaterialfv(face, pname, params.array(), offset(params));
            return;
        }

        q(CMD_MATERIALFB, 3);
        q(face);
        q(pname);
        q(params);

        qflush();
    }

    void IglMaterialfv(int face, int pname,
                               float[] params, int offset) {
        int n = GLConfiguration.glMaterialNumParams(pname);

        q(CMD_MATERIALFV, n + 3);
        q(n);
        q(face);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glMaterialx(int face, int pname, int param) {
        checkThread();
        q(CMD_MATERIALX, 3);
        q(face);
        q(pname);
        q(param);
    }

    public synchronized void glMaterialxv(int face, int pname,
                                          int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glMaterialNumParams(pname);
        checkLength(params, length, offset);

        IglMaterialxv(face, pname, params, offset);
    }

    public synchronized void glMaterialxv(int face, int pname,
                                          IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glMaterialNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglMaterialxv(face, pname, params.array(), offset(params));
            return;
        }

        q(CMD_MATERIALXB, 3);
        q(face);
        q(pname);
        q(params);

        qflush();
    }

    void IglMaterialxv(int face, int pname, int[] params, int offset) {
        int n = GLConfiguration.glMaterialNumParams(pname);

        q(CMD_MATERIALXV, n + 3);
        q(n);
        q(face);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glMatrixMode(int mode) {
        checkThread();
        q(CMD_MATRIX_MODE, 1);
        q(mode);
    }

    public synchronized void glMultMatrixf(float[] m, int offset) {
        checkThread();
        checkLength(m, 16, offset);

        IglMultMatrixf(m, offset);
    }

    public synchronized void glMultMatrixf(FloatBuffer m) {
        checkThread();
        checkLength(m, 16);

        if (!m.isDirect()) {
            IglMultMatrixf(m.array(), offset(m));
            return;
        }

        q(CMD_MULT_MATRIXFB, 1);
        q(m);
        
        qflush();
    }

    void IglMultMatrixf(float[] m, int offset) {
        q(CMD_MULT_MATRIXF, 16);
        for (int i = 0; i < 16; i++) {
            q(m[i + offset]);
        }
    }

    public synchronized void glMultMatrixx(int[] m, int offset) {
        checkThread();
        checkLength(m, 16, offset);

        IglMultMatrixx(m, offset);
    }

    public synchronized void glMultMatrixx(IntBuffer m) {
        checkThread();
        checkLength(m, 16);

        if (!m.isDirect()) {
            IglMultMatrixx(m.array(), offset(m));
            return;
        }
        
        q(CMD_MULT_MATRIXXB, 1);
        q(m);
        
        qflush();
    }

    void IglMultMatrixx(int[] m, int offset) {
        q(CMD_MULT_MATRIXX, 16);
        for (int i = 0; i < 16; i++) {
            q(m[i + offset]);
        }
    }

    public synchronized void glMultiTexCoord4f(int target,
                                               float s, float t,
                                               float r, float q) {
        checkThread();
        q(CMD_MULTI_TEXT_COORD4F, 5);
        q(target);
        q(s);
        q(t);
        q(r);
        q(q);
    }

    public synchronized void glMultiTexCoord4x(int target,
                                               int s, int t,
                                               int r, int q) {
        checkThread();
        q(CMD_MULTI_TEXT_COORD4X, 5);
        q(target);
        q(s);
        q(t);
        q(r);
        q(q);
    }

    public synchronized void glNormal3f(float nx, float ny, float nz) {
        checkThread();
        q(CMD_NORMAL3F, 3);
        q(nx);
        q(ny);
        q(nz);
    }

    public synchronized void glNormal3x(int nx, int ny, int nz) {
        checkThread();
        q(CMD_NORMAL3X, 3);
        q(nx);
        q(ny);
        q(nz);
    }

    public synchronized void glNormalPointer(int type, int stride,
                                             Buffer pointer) {
        checkThread();
        if (VBOArrayBufferBound != 0) {
            throw new IllegalStateException("glNormalPointer:" +
                                            Errors.VBO_ARRAY_BUFFER_BOUND);
        }
        if (!isDirect(pointer)) {
            throwIAE(Errors.GL_NOT_DIRECT);
        }

        if ((type == GL_BYTE ||
             type == GL_SHORT ||
             type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[NORMAL_POINTER]);
            BufferManager.useBuffer(pointer);

            pointerBuffer[NORMAL_POINTER] = pointer;
            pointerSize[NORMAL_POINTER] = 3;
            pointerType[NORMAL_POINTER] = type;
            pointerStride[NORMAL_POINTER] = stride;
            int nbytes = bufferTypeSize(pointer);
            pointerRemaining[NORMAL_POINTER] = pointer.remaining()*nbytes;
            pointerOffset[NORMAL_POINTER] = 0;
        }

        q(CMD_NORMAL_POINTER, 3);
        q(type);
        q(stride);
        q(pointer);

        qflush();
    }

    public synchronized void glOrthof(float left, float right,
                                      float bottom, float top,
                                      float zNear, float zFar) {
        checkThread();
        q(CMD_ORTHOF, 6);
        q(left);
        q(right);
        q(bottom);
        q(top);
        q(zNear);
        q(zFar);
    }

    public synchronized void glOrthox(int left, int right,
                                      int bottom, int top,
                                      int zNear, int zFar) {
        checkThread();
        q(CMD_ORTHOX, 6);
        q(left);
        q(right);
        q(bottom);
        q(top);
        q(zNear);
        q(zFar);
    }

    public synchronized void glPixelStorei(int pname, int param) {
        checkThread();
        q(CMD_PIXEL_STOREI, 3);
        q(pname);
        q(param);

        /* Only update local copy if we know command will succeed */
        if (param == 1 || param == 2 || param == 4 || param == 8) {
            if (pname == GL_PACK_ALIGNMENT) {
                pixelStorePackAlignment = param;
            } else if (pname == GL_UNPACK_ALIGNMENT) {
                pixelStoreUnpackAlignment = param;
            }
        }
    }

    public synchronized void glPointParameterf(int pname, float param) {
        checkThread();
        q(CMD_POINT_PARAMETERF, 2);
        q(pname);
        q(param);
    }

    public synchronized void glPointParameterfv(int pname,
                                                float[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glPointParameterNumParams(pname);
        checkLength(params, length, offset);

        IglPointParameterfv(pname, params, offset);
    }

    public synchronized void glPointParameterfv(int pname,
                                                FloatBuffer params) {
        checkThread();
        int length = GLConfiguration.glPointParameterNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglPointParameterfv(pname, params.array(), offset(params));
            return;
        }

        q(CMD_POINT_PARAMETERFB, 2);
        q(pname);
        q(params);

        qflush();
    }

    void IglPointParameterfv(int pname, float[] params, int offset) {
        int n = GLConfiguration.glPointParameterNumParams(pname);

        q(CMD_POINT_PARAMETERFV, n + 2);
        q(n);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glPointParameterx(int pname, int param) {
        checkThread();
        q(CMD_POINT_PARAMETERX, 2);
        q(pname);
        q(param);
    }

    public synchronized void glPointParameterxv(int pname,
                                                int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glPointParameterNumParams(pname);
        checkLength(params, length, offset);

        IglPointParameterxv(pname, params, offset);
    }

    public synchronized void glPointParameterxv(int pname, IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glPointParameterNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglPointParameterxv(pname, params.array(), offset(params));
            return;
        }

        q(CMD_POINT_PARAMETERXB, 2);
        q(pname);
        q(params);

        qflush();
    }

    void IglPointParameterxv(int pname, int[] params, int offset) {
        int n = GLConfiguration.glPointParameterNumParams(pname);

        q(CMD_POINT_PARAMETERXV, n + 2);
        q(n);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glPointSize(float size) {
        checkThread();
        q(CMD_POINT_SIZE, 1);
        q(size);
    }

    public synchronized void glPointSizex(int size) {
        checkThread();
        q(CMD_POINT_SIZEX, 1);
        q(size);
    }

    public synchronized void glPolygonOffset(float factor, float units) {
        checkThread();
        q(CMD_POLYGON_OFFSET, 2);
        q(factor);
        q(units);
    }

    public synchronized void glPolygonOffsetx(int factor, int units) {
        checkThread();
        q(CMD_POLYGON_OFFSETX, 2);
        q(factor);
        q(units);
    }

    public synchronized void glPopMatrix() {
        checkThread();
        q(CMD_POP_MATRIX, 0);
    }

    public synchronized void glPushMatrix() {
        checkThread();
        q(CMD_PUSH_MATRIX, 0);
    }

    // Pad must be a power of 2
    private int rasterBytes(int width, int height,
                            int format, int type,
                            int pad) {
        int bytesPerPixel = GLConfiguration.formatChannels(format)*
            GLConfiguration.sizeOfType(type);
        int rowWidth = width*bytesPerPixel;
        // stride = rowWidth rounded up to a multiple of 'pad' bytes
        int stride = (rowWidth + pad - 1) & ~(pad - 1);
    
        return stride*(height - 1) + rowWidth;
    }

    public synchronized void glReadPixels(int x, int y,
                                          int width, int height,
                                          int format, int type,
                                          Buffer pixels) {
        checkThread();
        qflush();

        if (pixels == null) {
            throwIAE(Errors.GL_PIXELS_NULL);
        }
        int remaining = pixels.remaining()*bufferTypeSize(pixels);
        int needed = rasterBytes(width, height, format, type,
                                 pixelStorePackAlignment);
        if (needed > remaining) {
            throwIAE(Errors.NOT_ENOUGH_ROOM);
        }

        if (isDirect(pixels)) {
            grabContext();
            _glReadPixelsPtr(x, y, width, height, format,
                             type, pointer(pixels));
        } else {
            if (pixels instanceof ByteBuffer) {
                ByteBuffer p = (ByteBuffer)pixels;
                grabContext();
                _glReadPixelsByte(x, y, width, height, format, type,
                                  p.array(), offset(p));
            } else if (pixels instanceof IntBuffer) {
                IntBuffer p = (IntBuffer)pixels;
                grabContext();
                _glReadPixelsInt(x, y, width, height, format, type,
                                 p.array(), offset(p));
            } else {
                throwIAE(Errors.GL_UNKNOWN_BUFFER);
            }
        }
    }

    public synchronized void glRotatef(float angle,
                                       float x, float y, float z) {
        checkThread();
        q(CMD_ROTATEF, 4);
        q(angle);
        q(x);
        q(y);
        q(z);
    }

    public synchronized void glRotatex(int angle, int x, int y, int z) {
        checkThread();
        q(CMD_ROTATEX, 4);
        q(angle);
        q(x);
        q(y);
        q(z);
    }

    public synchronized void glSampleCoverage(float value, boolean invert) {
        checkThread();
        q(CMD_SAMPLE_COVERAGE, 2);
        q(value);
        q(invert ? 1 : 0);
    }

    public synchronized void glSampleCoveragex(int value, boolean invert) {
        checkThread();
        q(CMD_SAMPLE_COVERAGEX, 2);
        q(value);
        q(invert ? 1 : 0);
    }

    public synchronized void glScalef(float x, float y, float z) {
        checkThread();
        q(CMD_SCALEF, 3);
        q(x);
        q(y);
        q(z);
    }

    public synchronized void glScalex(int x, int y, int z) {
        checkThread();
        q(CMD_SCALEX, 3);
        q(x);
        q(y);
        q(z);
    }

    public synchronized void glScissor(int x, int y, int width, int height) {
        checkThread();
        q(CMD_SCISSOR, 4);
        q(x);
        q(y);
        q(width);
        q(height);
    }

    public synchronized void glShadeModel(int mode) {
        checkThread();
        q(CMD_SHADE_MODEL, 1);
        q(mode);
    }

    public synchronized void glStencilFunc(int func, int ref, int mask) {
        checkThread();
        q(CMD_STENCIL_FUNC, 3);
        q(func);
        q(ref);
        q(mask);
    }

    public synchronized void glStencilMask(int mask) {
        checkThread();
        q(CMD_STENCIL_MASK, 2);
        q(mask);
    }

    public synchronized void glStencilOp(int fail, int zfail, int zpass) {
        checkThread();
        q(CMD_STENCIL_OP, 3);
        q(fail);
        q(zfail);
        q(zpass);
    }

    public synchronized void glTexCoordPointer(int size, int type, int stride,
                                               Buffer pointer) {
        checkThread();
        if (VBOArrayBufferBound != 0) {
            throw new IllegalStateException("glTexCoordPointer:" +
                                            Errors.VBO_ARRAY_BUFFER_BOUND);
        }
        if (!isDirect(pointer)) {
            throwIAE(Errors.GL_NOT_DIRECT);
        }

        if ((size >= 2 && size <= 4) && 
            (type == GL_BYTE ||
             type == GL_SHORT ||
             type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[TEX_COORD_POINTER]);
            BufferManager.useBuffer(pointer);

            pointerBuffer[TEX_COORD_POINTER] = pointer;
            pointerSize[TEX_COORD_POINTER] = size;
            pointerType[TEX_COORD_POINTER] = type;
            pointerStride[TEX_COORD_POINTER] = stride;
            int nbytes = bufferTypeSize(pointer);
            pointerRemaining[TEX_COORD_POINTER] = pointer.remaining()*nbytes;
            pointerOffset[TEX_COORD_POINTER] = 0;
        }

        q(CMD_TEX_COORD_POINTER, 4);
        q(size);
        q(type);
        q(stride);
        q(pointer);
        
        qflush();
    }

    public synchronized void glTexEnvi(int target, int pname, int param) {
        checkThread();
        q(CMD_TEX_ENVI, 3);
        q(target);
        q(pname);
        q(param);
    }

    public synchronized void glTexEnviv(int target, int pname,
                                        int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length, offset);
    
        IglTexEnviv(target, pname, params, offset);
    }

    public synchronized void glTexEnviv(int target, int pname,
                                        IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglTexEnviv(target, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_ENVIB, 3);
        q(target);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexEnviv(int target, int pname, int[] params, int offset) {
        int n = GLConfiguration.glTexEnvNumParams(pname);

        q(CMD_TEX_ENVIV, n + 3);
        q(n);
        q(target);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glTexEnvf(int target, int pname, float param) {
        checkThread();
        q(CMD_TEX_ENVF, 3);
        q(target);
        q(pname);
        q(param);
    }

    public synchronized void glTexEnvfv(int target, int pname,
                                        float[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length, offset);

        IglTexEnvfv(target, pname, params, offset);
    }

    public synchronized void glTexEnvfv(int target, int pname,
                                        FloatBuffer params) {
        checkThread();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglTexEnvfv(target, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_ENVFB, 3);
        q(target);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexEnvfv(int target, int pname, float[] params, int offset) {
        int n = GLConfiguration.glTexEnvNumParams(pname);

        q(CMD_TEX_ENVFV, n + 3);
        q(n);
        q(target);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glTexEnvx(int target, int pname, int param) {
        checkThread();
        q(CMD_TEX_ENVX, 3);
        q(target);
        q(pname);
        q(param);
    }

    public synchronized void glTexEnvxv(int target, int pname,
                                        int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length, offset);

        IglTexEnvxv(target, pname, params, offset);
    }

    public synchronized void glTexEnvxv(int target, int pname,
                                        IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglTexEnvxv(target, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_ENVXB, 3);
        q(target);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexEnvxv(int target, int pname, int[] params, int offset) {
        int n = GLConfiguration.glTexEnvNumParams(pname);

        q(CMD_TEX_ENVXV, n + 3);
        q(n);
        q(target);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glTexImage2D(int target, int level,
                                          int internalformat,
                                          int width, int height,
                                          int border, int format, int type,
                                          Buffer pixels) {
        checkThread();
        boolean isReadOnly = false;
        if (!isDirect(pixels)) {
            pixels = createDirectCopy(pixels);
            isReadOnly = true;
        }

        int remaining = pixels.remaining()*bufferTypeSize(pixels);
        int needed = rasterBytes(width, height, format, type,
                                 pixelStoreUnpackAlignment);
        if (needed > remaining) {
            throwIAE(Errors.NOT_ENOUGH_ROOM);
        }

        q(CMD_TEX_IMAGE_2D, 9);
        q(target);
        q(level);
        q(internalformat);
        q(width);
        q(height);
        q(border);
        q(format);
        q(type);
        q(pixels);
    
        if (!isReadOnly) {
            qflush();
        }
    }
  
    public synchronized void glTexParameterf(int target, int pname,
                                             float param) {
        checkThread();
        q(CMD_TEX_PARAMETERF, 3);
        q(target);
        q(pname);
        q(param);
    }

    public synchronized void glTexParameterx(int target, int pname,
                                             int param) {
        checkThread();
        q(CMD_TEX_PARAMETERX, 3);
        q(target);
        q(pname);
        q(param);
    }

    public synchronized void glTexParameterfv(int target, int pname,
                                              float[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length, offset);

        IglTexParameterfv(target, pname, params, offset);
    }

    public synchronized void glTexParameterfv(int target, int pname,
                                              FloatBuffer params) {
        checkThread();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglTexParameterfv(target, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_PARAMETERFB, 3);
        q(target);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexParameterfv(int target, int pname, float[] params, int offset) {
        int n = GLConfiguration.glTexParameterNumParams(pname);

        q(CMD_TEX_PARAMETERFV, n + 3);
        q(n);
        q(target);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glTexParameteri(int target, int pname,
                                             int param) {
        checkThread();
        q(CMD_TEX_PARAMETERI, 3);
        q(target);
        q(pname);
        q(param);
    }

    public synchronized void glTexParameteriv(int target, int pname,
                                              int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length, offset);

        IglTexParameteriv(target, pname, params, offset);
    }

    public synchronized void glTexParameteriv(int target, int pname,
                                              IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglTexParameteriv(target, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_PARAMETERIB, 3);
        q(target);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexParameteriv(int target, int pname, int[] params, int offset) {
        int n = GLConfiguration.glTexParameterNumParams(pname);

        q(CMD_TEX_PARAMETERIV, n + 3);
        q(n);
        q(target);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glTexParameterxv(int target, int pname,
                                              int[] params, int offset) {
        checkThread();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length, offset);

        IglTexParameterxv(target, pname, params, offset);
    }

    public synchronized void glTexParameterxv(int target, int pname,
                                              IntBuffer params) {
        checkThread();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            IglTexParameterxv(target, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_PARAMETERXB, 3);
        q(target);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexParameterxv(int target, int pname, int[] params, int offset) {
        int n = GLConfiguration.glTexParameterNumParams(pname);

        q(CMD_TEX_PARAMETERXV, n + 3);
        q(n);
        q(target);
        q(pname);
        for (int i = 0; i < n; i++) {
            q(params[i + offset]);
        }
    }

    public synchronized void glTexSubImage2D(int target, int level,
                                             int xoffset, int yoffset,
                                             int width, int height,
                                             int format, int type,
                                             Buffer pixels) {
        checkThread();
        if ((pixels instanceof ShortBuffer) ||
            (pixels instanceof IntBuffer)) {
            throwIAE(Errors.GL_PIXELS_NOT_SHORT_OR_INT);
        }

        boolean isReadOnly = false;
        if (!isDirect(pixels)) {
            pixels = createDirectCopy(pixels);
            isReadOnly = true;
        }

        int remaining = pixels.remaining()*bufferTypeSize(pixels);
        int needed = rasterBytes(width, height, format, type,
                                 pixelStoreUnpackAlignment);
        if (needed > remaining) {
            throwIAE(Errors.NOT_ENOUGH_ROOM);
        }

        q(CMD_TEX_SUB_IMAGE_2D, 9);
        q(target);
        q(level);
        q(xoffset);
        q(yoffset);
        q(width);
        q(height);
        q(format);
        q(type);
        q(pixels);
    
        if (!isReadOnly) {
            qflush();
        }
    }

    public synchronized void glTranslatef(float x, float y, float z) {
        checkThread();
        q(CMD_TRANSLATEF, 3);
        q(x);
        q(y);
        q(z);
    }

    public synchronized void glTranslatex(int x, int y, int z) {
        checkThread();
        q(CMD_TRANSLATEX, 3);
        q(x);
        q(y);
        q(z);
    }

    // Utility to return the number of bytes per Buffer element
    int bufferTypeSize(Buffer pointer) {
        if (pointer instanceof ByteBuffer) {
            return 1;
        } else if (pointer instanceof ShortBuffer) {
            return 2;
        } else if (pointer instanceof IntBuffer ||
                   pointer instanceof FloatBuffer) {
            return 4;
        } else {
            throw new IllegalArgumentException("Unknown Buffer subclass!");
        }
    }

    public synchronized void glVertexPointer(int size, int type, int stride,
                                             Buffer pointer) {
        checkThread();
        if (VBOArrayBufferBound != 0) {
            throw new IllegalStateException("glVertexPointer:" +
                                            Errors.VBO_ARRAY_BUFFER_BOUND);
        }
        if (!isDirect(pointer)) {
            throwIAE(Errors.GL_NOT_DIRECT);
        }

        // Only record details if this is a legal operation
        if ((size >= 2 && size <= 4) && 
            (type == GL_BYTE ||
             type == GL_SHORT ||
             type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[VERTEX_POINTER]);
            BufferManager.useBuffer(pointer);

            pointerBuffer[VERTEX_POINTER] = pointer;
            pointerSize[VERTEX_POINTER] = size;
            pointerType[VERTEX_POINTER] = type;
            pointerStride[VERTEX_POINTER] = stride;
            int nbytes = bufferTypeSize(pointer);
            pointerRemaining[VERTEX_POINTER] = pointer.remaining()*nbytes;
            pointerOffset[VERTEX_POINTER] = 0;
        }

        q(CMD_VERTEX_POINTER, 4);
        q(size);
        q(type);
        q(stride);
        q(pointer);

        qflush();
    }

    public synchronized void glViewport(int x, int y, int width, int height) {
        checkThread();
        q(CMD_VIEWPORT, 4);
        q(x);
        q(y);
        q(width);
        q(height);
    }

    // OES_query_matrix

    public synchronized int glQueryMatrixxOES(int[] mantissa,
                                              int mantissaOffset,
                                              int[] exponent,
                                              int exponentOffset) {
        checkThread();
        if (!GLConfiguration.supports_OES_query_matrix) {
            throw new UnsupportedOperationException(
                                           Errors.GL_QUERY_MATRIX_UNSUPPORTED);
        }
        checkLength(mantissa, 16, mantissaOffset);
        checkLength(exponent, 16, exponentOffset);

        qflush();

        grabContext();
        int retval = _glQueryMatrixxOES(mantissa, mantissaOffset,
                                        exponent, exponentOffset);
        return retval;
    }

    // VBO Support

    int VBOArrayBufferBound = 0;
    int VBOElementArrayBufferBound = 0;

    // buffer ID (Integer) -> size (Integer)
    Hashtable bufferSize = new Hashtable();

    // buffer ID (Integer) -> indices (int[])
    Hashtable bufferIndices = new Hashtable();

    // Set of currently existing buffers
    // Entries have the form (Integer -> Object)
    // There is an entry for each currently valid buffer ID
    // The value is not used, only the key set
    Hashtable VBOBuffersTable = new Hashtable();

    // Sizes of currently existing buffers
    // Entries have the form (Integer -> Integer)
    // There is an entry for each currently valid buffer ID
    Hashtable VBOBufferSizeTable = new Hashtable();

    // Copy of currently existing index buffers
    // Entries have the form (Integer -> int[])
    // There is an entry for each currently valid buffer ID
    Hashtable VBOBufferIndicesTable = new Hashtable();

    // Adds an entry for a buffer, initially with size 0
    void addBuffer(int buffer) {
        if (DEBUG_MEM) {
            System.out.println("addBuffer " + buffer);
        }
        if (buffer != 0) {
            Integer key = new Integer(buffer);
            VBOBuffersTable.put(key, new Object());
            VBOBufferSizeTable.put(key, new Integer(0));
            VBOBufferIndicesTable.put(key, new byte[0]);
        }
    }

    // Deletes an entry for a buffer
    void removeBuffer(int buffer) {
        if (DEBUG_MEM) {
            System.out.println("removeBuffer " + buffer);
        }
        if (buffer != 0) {
            Integer key = new Integer(buffer);
            VBOBuffersTable.remove(key);
            VBOBufferSizeTable.remove(key);
            VBOBufferIndicesTable.remove(key);
        }
    }

    // Returns true if there is a buffer with the given ID
    boolean bufferExists(int buffer) {
        return (buffer == 0) ||
            VBOBuffersTable.containsKey(new Integer(buffer));
    }

    private Integer key(int target) {
        if (target == GL11.GL_ARRAY_BUFFER) {
            return new Integer(VBOArrayBufferBound);
        } else if (target == GL11.GL_ELEMENT_ARRAY_BUFFER) {
            return new Integer(VBOElementArrayBufferBound);
        } else {
            throw new IllegalArgumentException("target = " + target);
        }
    }

    // Sets the size of the current buffer for the given target
    void setBufferSize(int target, int size) {
        if (DEBUG_MEM) {
            System.out.println("setBufferSize " + target + " " + size);
        }

        Integer key = key(target);
        if (target == GL11.GL_ELEMENT_ARRAY_BUFFER) {
            if (DEBUG_MEM) {
                System.out.print("setBufferSize: allocating " + size);
            }            
            VBOBufferIndicesTable.put(key, new byte[size]);
            if (DEBUG_MEM) {
                System.out.println(": done");
            }            
        }
        VBOBufferSizeTable.put(key, new Integer(size));
    }

    int getBufferSize(int target) {
        Integer key = key(target);

        Object o = VBOBufferSizeTable.get(key);
        if (o != null) {
            return ((Integer)o).intValue();
        } else {
            throw new IllegalStateException("No VBO active for target!");
        }
    }

    byte[] getBufferIndices() {
        Integer key = key(GL11.GL_ELEMENT_ARRAY_BUFFER);

        Object o = VBOBufferIndicesTable.get(key);
        if (o != null) {
            return (byte[])o;
        } else {
            throw new IllegalStateException("No VBO active for target!");
        }
    }
    
    // Records the index data where it can be used for bounds
    // checking in glDrawElements.
    void bufferIndexData(Buffer data, int offset, int size,
                         boolean removeOldData) {
        Integer key = new Integer(VBOElementArrayBufferBound);
        byte[] array;

        Object o = VBOBufferIndicesTable.get(key);
        if (o == null || removeOldData) {
            array = new byte[offset + size];
        } else {
            array = (byte[])o;
            if (array.length < offset + size) {
                byte[] narray = new byte[offset + size];
                System.arraycopy(narray, 0, array, 0, offset + size);
                array = narray;
            }
        }
        
        // Copy bytes from 'data' into 'array'
        BufferManager.getBytes(data, offset, array, offset, size);
        // (Re-)store the index data
        VBOBufferIndicesTable.put(key, array);
    }

// End VBO Support

    public synchronized void dispose() {
        glFinish();
        this.context = null;
    }

    public GL10Impl(EGLContext context) {
        this.context = context;
    }
}
