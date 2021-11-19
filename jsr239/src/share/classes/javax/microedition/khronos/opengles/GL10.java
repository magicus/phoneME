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
 * The <code>GL10</code> interface contains the Java(TM) programming
 * language bindings for OpenGL(R) ES 1.0 core functionality.
 *
 * <p> The <code>OES_byte_coordinates</code>,
 * <code>OES_single_precision</code>, <code>OES_fixed_point</code>,
 * <code>OES_read_format</code>, and
 * <code>OES_compressed_paletted_texture</code> extensions are
 * included as part of this interface.
 *
 * <p> Methods with an <code>x</code> or <code>xv</code> suffix belong
 * to the <code>OES_fixed_point</code> extension and take one or more
 * fixed-point arguments. Fixed-point arguments may be derived from
 * floating-point values by multiplying by 65536 (2^16) and rounding
 * to the nearest integer.
 *
 * <p> For ease of use, this interface documents method behavior for
 * all versions of OpenGL ES including version 1.1 and extensions.
 * When running on an OpenGL ES 1.0 implementation, sections marked
 * "(1.1 only)" should be disregarded.
 *
 * <p> See the <code>GL</code> interface for a description of how to
 * obtain an instance of this interface.
 */
public interface GL10 extends GL {

  // ClearBufferMask
  /** OpenGL ES 1.0 constant. */
  int GL_DEPTH_BUFFER_BIT                     = 0x00000100;
  /** OpenGL ES 1.0 constant. */
  int GL_STENCIL_BUFFER_BIT                   = 0x00000400;
  /** OpenGL ES 1.0 constant. */
  int GL_COLOR_BUFFER_BIT                     = 0x00004000;

  // Boolean
  /** OpenGL ES 1.0 constant. */
  int GL_FALSE                                = 0;
  /** OpenGL ES 1.0 constant. */
  int GL_TRUE                                 = 1;

  // BeginMode
  /** OpenGL ES 1.0 constant. */
  int GL_POINTS                               = 0x0000;
  /** OpenGL ES 1.0 constant. */
  int GL_LINES                                = 0x0001;
  /** OpenGL ES 1.0 constant. */
  int GL_LINE_LOOP                            = 0x0002;
  /** OpenGL ES 1.0 constant. */
  int GL_LINE_STRIP                           = 0x0003;
  /** OpenGL ES 1.0 constant. */
  int GL_TRIANGLES                            = 0x0004;
  /** OpenGL ES 1.0 constant. */
  int GL_TRIANGLE_STRIP                       = 0x0005;
  /** OpenGL ES 1.0 constant. */
  int GL_TRIANGLE_FAN                         = 0x0006;

  // AlphaFunction
  /** OpenGL ES 1.0 constant. */
  int GL_NEVER                                = 0x0200;
  /** OpenGL ES 1.0 constant. */
  int GL_LESS                                 = 0x0201;
  /** OpenGL ES 1.0 constant. */
  int GL_EQUAL                                = 0x0202;
  /** OpenGL ES 1.0 constant. */
  int GL_LEQUAL                               = 0x0203;
  /** OpenGL ES 1.0 constant. */
  int GL_GREATER                              = 0x0204;
  /** OpenGL ES 1.0 constant. */
  int GL_NOTEQUAL                             = 0x0205;
  /** OpenGL ES 1.0 constant. */
  int GL_GEQUAL                               = 0x0206;
  /** OpenGL ES 1.0 constant. */
  int GL_ALWAYS                               = 0x0207;

  // BlendingFactorDest
  /** OpenGL ES 1.0 constant. */
  int GL_ZERO                                 = 0;
  /** OpenGL ES 1.0 constant. */
  int GL_ONE                                  = 1;
  /** OpenGL ES 1.0 constant. */
  int GL_SRC_COLOR                            = 0x0300;
  /** OpenGL ES 1.0 constant. */
  int GL_ONE_MINUS_SRC_COLOR                  = 0x0301;
  /** OpenGL ES 1.0 constant. */
  int GL_SRC_ALPHA                            = 0x0302;
  /** OpenGL ES 1.0 constant. */
  int GL_ONE_MINUS_SRC_ALPHA                  = 0x0303;
  /** OpenGL ES 1.0 constant. */
  int GL_DST_ALPHA                            = 0x0304;
  /** OpenGL ES 1.0 constant. */
  int GL_ONE_MINUS_DST_ALPHA                  = 0x0305;

  // BlendingFactorSrc
  /** OpenGL ES 1.0 constant. */
  int GL_DST_COLOR                            = 0x0306;
  /** OpenGL ES 1.0 constant. */
  int GL_ONE_MINUS_DST_COLOR                  = 0x0307;
  /** OpenGL ES 1.0 constant. */
  int GL_SRC_ALPHA_SATURATE                   = 0x0308;

  // CullFaceMode
  /** OpenGL ES 1.0 constant. */
  int GL_FRONT                                = 0x0404;
  /** OpenGL ES 1.0 constant. */
  int GL_BACK                                 = 0x0405;
  /** OpenGL ES 1.0 constant. */
  int GL_FRONT_AND_BACK                       = 0x0408;

  // EnableCap
  /** OpenGL ES 1.0 constant. */
  int GL_FOG                                  = 0x0B60;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHTING                             = 0x0B50;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_2D                           = 0x0DE1;
  /** OpenGL ES 1.0 constant. */
  int GL_CULL_FACE                            = 0x0B44;
  /** OpenGL ES 1.0 constant. */
  int GL_ALPHA_TEST                           = 0x0BC0;
  /** OpenGL ES 1.0 constant. */
  int GL_BLEND                                = 0x0BE2;
  /** OpenGL ES 1.0 constant. */
  int GL_COLOR_LOGIC_OP                       = 0x0BF2;
  /** OpenGL ES 1.0 constant. */
  int GL_DITHER                               = 0x0BD0;
  /** OpenGL ES 1.0 constant. */
  int GL_STENCIL_TEST                         = 0x0B90;
  /** OpenGL ES 1.0 constant. */
  int GL_DEPTH_TEST                           = 0x0B71;
  /** OpenGL ES 1.0 constant. */
  int GL_POINT_SMOOTH                         = 0x0B10;
  /** OpenGL ES 1.0 constant. */
  int GL_LINE_SMOOTH                          = 0x0B20;
  /** OpenGL ES 1.0 constant. */
  int GL_SCISSOR_TEST                         = 0x0C11;
  /** OpenGL ES 1.0 constant. */
  int GL_COLOR_MATERIAL                       = 0x0B57;
  /** OpenGL ES 1.0 constant. */
  int GL_NORMALIZE                            = 0x0BA1;
  /** OpenGL ES 1.0 constant. */
  int GL_RESCALE_NORMAL                       = 0x803A;
  /** OpenGL ES 1.0 constant. */
  int GL_POLYGON_OFFSET_FILL                  = 0x8037;
  /** OpenGL ES 1.0 constant. */
  int GL_VERTEX_ARRAY                         = 0x8074;
  /** OpenGL ES 1.0 constant. */
  int GL_NORMAL_ARRAY                         = 0x8075;
  /** OpenGL ES 1.0 constant. */
  int GL_COLOR_ARRAY                          = 0x8076;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_COORD_ARRAY                  = 0x8078;
  /** OpenGL ES 1.0 constant. */
  int GL_MULTISAMPLE                          = 0x809D;
  /** OpenGL ES 1.0 constant. */
  int GL_SAMPLE_ALPHA_TO_COVERAGE             = 0x809E;
  /** OpenGL ES 1.0 constant. */
  int GL_SAMPLE_ALPHA_TO_ONE                  = 0x809F;
  /** OpenGL ES 1.0 constant. */
  int GL_SAMPLE_COVERAGE                      = 0x80A0;

  // ErrorCode
  /** OpenGL ES 1.0 constant. */
  int GL_NO_ERROR                             = 0;
  /** OpenGL ES 1.0 constant. */
  int GL_INVALID_ENUM                         = 0x0500;
  /** OpenGL ES 1.0 constant. */
  int GL_INVALID_VALUE                        = 0x0501;
  /** OpenGL ES 1.0 constant. */
  int GL_INVALID_OPERATION                    = 0x0502;
  /** OpenGL ES 1.0 constant. */
  int GL_STACK_OVERFLOW                       = 0x0503;
  /** OpenGL ES 1.0 constant. */
  int GL_STACK_UNDERFLOW                      = 0x0504;
  /** OpenGL ES 1.0 constant. */
  int GL_OUT_OF_MEMORY                        = 0x0505;

  // FogMode
  /** OpenGL ES 1.0 constant. */
  int GL_EXP                                  = 0x0800;
  /** OpenGL ES 1.0 constant. */
  int GL_EXP2                                 = 0x0801;

  // FogParameter
  /** OpenGL ES 1.0 constant. */
  int GL_FOG_DENSITY                          = 0x0B62;
  /** OpenGL ES 1.0 constant. */
  int GL_FOG_START                            = 0x0B63;
  /** OpenGL ES 1.0 constant. */
  int GL_FOG_END                              = 0x0B64;
  /** OpenGL ES 1.0 constant. */
  int GL_FOG_MODE                             = 0x0B65;
  /** OpenGL ES 1.0 constant. */
  int GL_FOG_COLOR                            = 0x0B66;

  // FrontFaceDirection
  /** OpenGL ES 1.0 constant. */
  int GL_CW                                   = 0x0900;
  /** OpenGL ES 1.0 constant. */
  int GL_CCW                                  = 0x0901;

  // GetPName
  /** OpenGL ES 1.0 constant. */
  int GL_SMOOTH_POINT_SIZE_RANGE              = 0x0B12;
  /** OpenGL ES 1.0 constant. */
  int GL_SMOOTH_LINE_WIDTH_RANGE              = 0x0B22;
  /** OpenGL ES 1.0 constant. */
  int GL_ALIASED_POINT_SIZE_RANGE             = 0x846D;
  /** OpenGL ES 1.0 constant. */
  int GL_ALIASED_LINE_WIDTH_RANGE             = 0x846E;
  /** OpenGL ES 1.0 constant. */
  int GL_IMPLEMENTATION_COLOR_READ_TYPE_OES   = 0x8B9A;
  /** OpenGL ES 1.0 constant. */
  int GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES = 0x8B9B;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_LIGHTS                           = 0x0D31;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_TEXTURE_SIZE                     = 0x0D33;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_MODELVIEW_STACK_DEPTH            = 0x0D36;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_PROJECTION_STACK_DEPTH           = 0x0D38;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_TEXTURE_STACK_DEPTH              = 0x0D39;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_VIEWPORT_DIMS                    = 0x0D3A;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_ELEMENTS_VERTICES                = 0x80E8;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_ELEMENTS_INDICES                 = 0x80E9;
  /** OpenGL ES 1.0 constant. */
  int GL_MAX_TEXTURE_UNITS                    = 0x84E2;
  /** OpenGL ES 1.0 constant. */
  int GL_NUM_COMPRESSED_TEXTURE_FORMATS       = 0x86A2;
  /** OpenGL ES 1.0 constant. */
  int GL_COMPRESSED_TEXTURE_FORMATS           = 0x86A3;
  /** OpenGL ES 1.0 constant. */
  int GL_SUBPIXEL_BITS                        = 0x0D50;
  /** OpenGL ES 1.0 constant. */
  int GL_RED_BITS                             = 0x0D52;
  /** OpenGL ES 1.0 constant. */
  int GL_GREEN_BITS                           = 0x0D53;
  /** OpenGL ES 1.0 constant. */
  int GL_BLUE_BITS                            = 0x0D54;
  /** OpenGL ES 1.0 constant. */
  int GL_ALPHA_BITS                           = 0x0D55;
  /** OpenGL ES 1.0 constant. */
  int GL_DEPTH_BITS                           = 0x0D56;
  /** OpenGL ES 1.0 constant. */
  int GL_STENCIL_BITS                         = 0x0D57;

  // HintMode
  /** OpenGL ES 1.0 constant. */
  int GL_DONT_CARE                            = 0x1100;
  /** OpenGL ES 1.0 constant. */
  int GL_FASTEST                              = 0x1101;
  /** OpenGL ES 1.0 constant. */
  int GL_NICEST                               = 0x1102;

  // HintTarget
  /** OpenGL ES 1.0 constant. */
  int GL_PERSPECTIVE_CORRECTION_HINT          = 0x0C50;
  /** OpenGL ES 1.0 constant. */
  int GL_POINT_SMOOTH_HINT                    = 0x0C51;
  /** OpenGL ES 1.0 constant. */
  int GL_LINE_SMOOTH_HINT                     = 0x0C52;
  /** OpenGL ES 1.0 constant. */
  int GL_POLYGON_SMOOTH_HINT                  = 0x0C53;
  /** OpenGL ES 1.0 constant. */
  int GL_FOG_HINT                             = 0x0C54;

  // LightModelParameter
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT_MODEL_AMBIENT                  = 0x0B53;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT_MODEL_TWO_SIDE                 = 0x0B52;
  /** OpenGL ES 1.0 constant. */

  // LightParameter
  /** OpenGL ES 1.0 constant. */
  int GL_AMBIENT                              = 0x1200;
  /** OpenGL ES 1.0 constant. */
  int GL_DIFFUSE                              = 0x1201;
  /** OpenGL ES 1.0 constant. */
  int GL_SPECULAR                             = 0x1202;
  /** OpenGL ES 1.0 constant. */
  int GL_POSITION                             = 0x1203;
  /** OpenGL ES 1.0 constant. */
  int GL_SPOT_DIRECTION                       = 0x1204;
  /** OpenGL ES 1.0 constant. */
  int GL_SPOT_EXPONENT                        = 0x1205;
  /** OpenGL ES 1.0 constant. */
  int GL_SPOT_CUTOFF                          = 0x1206;
  /** OpenGL ES 1.0 constant. */
  int GL_CONSTANT_ATTENUATION                 = 0x1207;
  /** OpenGL ES 1.0 constant. */
  int GL_LINEAR_ATTENUATION                   = 0x1208;
  /** OpenGL ES 1.0 constant. */
  int GL_QUADRATIC_ATTENUATION                = 0x1209;

  // DataType
  /** OpenGL ES 1.0 constant. */
  int GL_BYTE                                 = 0x1400;
  /** OpenGL ES 1.0 constant. */
  int GL_UNSIGNED_BYTE                        = 0x1401;
  /** OpenGL ES 1.0 constant. */
  int GL_SHORT                                = 0x1402;
  /** OpenGL ES 1.0 constant. */
  int GL_UNSIGNED_SHORT                       = 0x1403;
  /** OpenGL ES 1.0 constant. */
  int GL_FLOAT                                = 0x1406;
  /** OpenGL ES 1.0 constant. */
  int GL_FIXED                                = 0x140C;

  // LogicOp
  /** OpenGL ES 1.0 constant. */
  int GL_CLEAR                                = 0x1500;
  /** OpenGL ES 1.0 constant. */
  int GL_AND                                  = 0x1501;
  /** OpenGL ES 1.0 constant. */
  int GL_AND_REVERSE                          = 0x1502;
  /** OpenGL ES 1.0 constant. */
  int GL_COPY                                 = 0x1503;
  /** OpenGL ES 1.0 constant. */
  int GL_AND_INVERTED                         = 0x1504;
  /** OpenGL ES 1.0 constant. */
  int GL_NOOP                                 = 0x1505;
  /** OpenGL ES 1.0 constant. */
  int GL_XOR                                  = 0x1506;
  /** OpenGL ES 1.0 constant. */
  int GL_OR                                   = 0x1507;
  /** OpenGL ES 1.0 constant. */
  int GL_NOR                                  = 0x1508;
  /** OpenGL ES 1.0 constant. */
  int GL_EQUIV                                = 0x1509;
  /** OpenGL ES 1.0 constant. */
  int GL_INVERT                               = 0x150A;
  /** OpenGL ES 1.0 constant. */
  int GL_OR_REVERSE                           = 0x150B;
  /** OpenGL ES 1.0 constant. */
  int GL_COPY_INVERTED                        = 0x150C;
  /** OpenGL ES 1.0 constant. */
  int GL_OR_INVERTED                          = 0x150D;
  /** OpenGL ES 1.0 constant. */
  int GL_NAND                                 = 0x150E;
  /** OpenGL ES 1.0 constant. */
  int GL_SET                                  = 0x150F;

  // MaterialParameters
  /** OpenGL ES 1.0 constant. */
  int GL_EMISSION                             = 0x1600;
  /** OpenGL ES 1.0 constant. */
  int GL_SHININESS                            = 0x1601;
  /** OpenGL ES 1.0 constant. */
  int GL_AMBIENT_AND_DIFFUSE                  = 0x1602;

  // MatrixMode
  /** OpenGL ES 1.0 constant. */
  int GL_MODELVIEW                            = 0x1700;
  /** OpenGL ES 1.0 constant. */
  int GL_PROJECTION                           = 0x1701;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE                              = 0x1702;

  // PixelFormat
  /** OpenGL ES 1.0 constant. */
  int GL_ALPHA                                = 0x1906;
  /** OpenGL ES 1.0 constant. */
  int GL_RGB                                  = 0x1907;
  /** OpenGL ES 1.0 constant. */
  int GL_RGBA                                 = 0x1908;
  /** OpenGL ES 1.0 constant. */
  int GL_LUMINANCE                            = 0x1909;
  /** OpenGL ES 1.0 constant. */
  int GL_LUMINANCE_ALPHA                      = 0x190A;

  // PixelStoreParameter
  /** OpenGL ES 1.0 constant. */
  int GL_UNPACK_ALIGNMENT                     = 0x0CF5;
  /** OpenGL ES 1.0 constant. */
  int GL_PACK_ALIGNMENT                       = 0x0D05;

  // PixelType
  /** OpenGL ES 1.0 constant. */
  int GL_UNSIGNED_SHORT_4_4_4_4               = 0x8033;
  /** OpenGL ES 1.0 constant. */
  int GL_UNSIGNED_SHORT_5_5_5_1               = 0x8034;
  /** OpenGL ES 1.0 constant. */
  int GL_UNSIGNED_SHORT_5_6_5                 = 0x8363;

  // ShadingModel
  /** OpenGL ES 1.0 constant. */
  int GL_FLAT                                 = 0x1D00;
  /** OpenGL ES 1.0 constant. */
  int GL_SMOOTH                               = 0x1D01;

  // StencilOp
  /** OpenGL ES 1.0 constant. */
  int GL_KEEP                                 = 0x1E00;
  /** OpenGL ES 1.0 constant. */
  int GL_REPLACE                              = 0x1E01;
  /** OpenGL ES 1.0 constant. */
  int GL_INCR                                 = 0x1E02;
  /** OpenGL ES 1.0 constant. */
  int GL_DECR                                 = 0x1E03;

  // StringName
  /** OpenGL ES 1.0 constant. */
  int GL_VENDOR                               = 0x1F00;
  /** OpenGL ES 1.0 constant. */
  int GL_RENDERER                             = 0x1F01;
  /** OpenGL ES 1.0 constant. */
  int GL_VERSION                              = 0x1F02;
  /** OpenGL ES 1.0 constant. */
  int GL_EXTENSIONS                           = 0x1F03;

  // TextureEnvMode
  /** OpenGL ES 1.0 constant. */
  int GL_MODULATE                             = 0x2100;
  /** OpenGL ES 1.0 constant. */
  int GL_DECAL                                = 0x2101;
  /** OpenGL ES 1.0 constant. */
  int GL_ADD                                  = 0x0104;

  // TextureEnvParameter
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_ENV_MODE                     = 0x2200;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_ENV_COLOR                    = 0x2201;

  // TextureEnvTarget
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_ENV                          = 0x2300;

  // TextureMagFilter
  /** OpenGL ES 1.0 constant. */
  int GL_NEAREST                              = 0x2600;
  /** OpenGL ES 1.0 constant. */
  int GL_LINEAR                               = 0x2601;

  // TextureMinFilter
  /** OpenGL ES 1.0 constant. */
  int GL_NEAREST_MIPMAP_NEAREST               = 0x2700;
  /** OpenGL ES 1.0 constant. */
  int GL_LINEAR_MIPMAP_NEAREST                = 0x2701;
  /** OpenGL ES 1.0 constant. */
  int GL_NEAREST_MIPMAP_LINEAR                = 0x2702;
  /** OpenGL ES 1.0 constant. */
  int GL_LINEAR_MIPMAP_LINEAR                 = 0x2703;

  // TextureParameterName
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_MAG_FILTER                   = 0x2800;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_MIN_FILTER                   = 0x2801;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_WRAP_S                       = 0x2802;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE_WRAP_T                       = 0x2803;

  // TextureUnit
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE0                             = 0x84C0;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE1                             = 0x84C1;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE2                             = 0x84C2;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE3                             = 0x84C3;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE4                             = 0x84C4;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE5                             = 0x84C5;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE6                             = 0x84C6;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE7                             = 0x84C7;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE8                             = 0x84C8;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE9                             = 0x84C9;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE10                            = 0x84CA;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE11                            = 0x84CB;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE12                            = 0x84CC;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE13                            = 0x84CD;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE14                            = 0x84CE;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE15                            = 0x84CF;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE16                            = 0x84D0;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE17                            = 0x84D1;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE18                            = 0x84D2;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE19                            = 0x84D3;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE20                            = 0x84D4;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE21                            = 0x84D5;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE22                            = 0x84D6;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE23                            = 0x84D7;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE24                            = 0x84D8;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE25                            = 0x84D9;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE26                            = 0x84DA;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE27                            = 0x84DB;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE28                            = 0x84DC;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE29                            = 0x84DD;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE30                            = 0x84DE;
  /** OpenGL ES 1.0 constant. */
  int GL_TEXTURE31                            = 0x84DF;

  // TextureWrapMode
  /** OpenGL ES 1.0 constant. */
  int GL_REPEAT                               = 0x2901;
  /** OpenGL ES 1.0 constant. */
  int GL_CLAMP_TO_EDGE                        = 0x812F;

  // PixelInternalFormat
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE4_RGB8_OES                    = 0x8B90;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE4_RGBA8_OES                   = 0x8B91;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE4_R5_G6_B5_OES                = 0x8B92;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE4_RGBA4_OES                   = 0x8B93;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE4_RGB5_A1_OES                 = 0x8B94;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE8_RGB8_OES                    = 0x8B95;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE8_RGBA8_OES                   = 0x8B96;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE8_R5_G6_B5_OES                = 0x8B97;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE8_RGBA4_OES                   = 0x8B98;
  /** OpenGL ES 1.0 constant. */
  int GL_PALETTE8_RGB5_A1_OES                 = 0x8B99;

  // LightName
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT0                               = 0x4000;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT1                               = 0x4001;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT2                               = 0x4002;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT3                               = 0x4003;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT4                               = 0x4004;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT5                               = 0x4005;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT6                               = 0x4006;
  /** OpenGL ES 1.0 constant. */
  int GL_LIGHT7                               = 0x4007;

  /** 
   * Select server-side active texture unit.
   *
   * <p><code>glActiveTexture</code> selects which texture unit
   * subsequent texture state calls will affect. The number of texture
   * units an implementation supports is implementation dependent, it
   * must be at least 1 for OpenGL ES 1.0, or 2 for OpenGL ES 1.1.
   *
   * <h4>Notes</h4>
   *
   * <p>It is always the case that <code>GL_TEXTURE</code><i>i</i> =
   * <code>GL_TEXTURE0</code> + <i>i</i>.
   *
   * <p>A texture unit consists of the texture enable state, texture
   * matrix stack, texture environment and currently bound
   * texture. Modifying any of these states has an effect only on the
   * active texture unit.
   *
   * <p>Vertex arrays are client-side GL resources, which are selected by
   * the <code>glClientActiveTexture</code> routine.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>texture</code> is not one of
   * <code>GL_TEXTURE</code><i>i</i>, where <code>0 <= <i>i</i> <
   * GL_MAX_TEXTURE_UNITS</code>.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_UNITS</code>.
   *
   * @param texture Specifies which texture unit to make active. The
   * number of texture units is implementation dependent, but must be
   * at least one (for 1.0) or two (for 1.1). <code>texture</code>
   * must be one of <code>GL_TEXTURE</code><i>i</i>, where <code>0 <=
   * <i>i</i> < GL_MAX_TEXTURE_UNITS</code>, which is an
   * implementation-dependent value. The intial value is
   * <code>GL_TEXTURE0</code>.
   *
   */
  void glActiveTexture(int texture);

  /**
   * Specify the alpha test function.
   *
   * <p>The alpha test discards fragments depending on the outcome of a
   * comparison between an incoming fragment's alpha value and a
   * constant reference value. <code>glAlphaFunc</code> specifies the
   * reference value and the comparison function. The comparison is
   * performed only if alpha testing is enabled. To enable and disable
   * alpha testing, call <code>glEnable</code> and
   * <code>glDisable</code> with argument
   * <code>GL_ALPHA_TEST</code>. Alpha testing is initially disabled.
   *
   * <p><code>func</code> and <code>ref</code> specify the conditions
   * under which the pixel is drawn. The incoming alpha value is
   * compared to <code>ref</code> using the function specified by
   * <code>func</code>. If the value passes the comparison, the
   * incoming fragment is drawn if it also passes subsequent stencil
   * and depth buffer tests. If the value fails the comparison, no
   * change is made to the frame buffer at that pixel location. The
   * comparison functions are as follows:
   * 
   * <ul>
   * <li><code>GL_NEVER</code></li>
   * Never passes.
   * <li><code>GL_LESS</code></li>
   * Passes if the incoming alpha value is less than the reference value.
   * <li><code>GL_EQUAL</code></li>
   * Passes if the incoming alpha value is equal to the reference value.
   * <li><code>GL_LEQUAL</code></li>
   * Passes if the incoming alpha value is less than or equal to the
   * reference value.
   * <li><code>GL_GREATER</code></li>
   * Passes if the incoming alpha value is greater than the reference value.
   * <li><code>GL_NOTEQUAL</code></li>
   * Passes if the incoming alpha value is not equal to the reference value.
   * <li><code>GL_GEQUAL</code></li>
   * Passes if the incoming alpha value is greater than or equal to
   * the reference value.
   * <li><code>GL_ALWAYS</code></li>
   * Always passes (initial value).
   * </ul>
   *
   * <p><code>glAlphaFunc</code> operates on all pixel write
   * operations, including those resulting from the scan conversion of
   * points, lines, and polygons. <code>glAlphaFunc</code> does not
   * affect <code>glClear</code>.
   *
   * <h4>Errors</h4>
   * 
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>func</code>
   * is not an accepted value.
   *
   * @param func Specifies the alpha comparison function. Symbolic
   * constants <code>GL_NEVER</code>, <code>GL_LESS</code>,
   * <code>GL_EQUAL</code>, <code>GL_LEQUAL</code>,
   * <code>GL_GREATER</code>, <code>GL_NOTEQUAL</code>,
   * <code>GL_GEQUAL</code>, and <code>GL_ALWAYS</code> are accepted. The
   * initial value is <code>GL_ALWAYS</code>.
   * @param ref Specifies the reference value that incoming alpha
   * values are compared to. This value is clamped to the range
   * <code>[0, 1]</code>, where 0 represents the lowest possible alpha
   * value and 1 the highest possible value. The initial reference
   * value is 0.
   */
  void glAlphaFunc(int func, float ref);

  /**
   * Fixed-point version of <code>glAlphaFunc</code>.
   *
   * @see #glAlphaFunc
   */ 
  void glAlphaFuncx(int func, int ref);

  /**
   * Bind a named texture to a texturing target.
   *
   * <p><code>glBindTexture</code> lets you create or use a named
   * texture. Calling <code>glBindTexture</code> with
   * <code>target</code> set to <code>GL_TEXTURE_2D</code>, and
   * <code>texture</code> set to the name of the new texture binds the
   * texture name to the target. When a texture is bound to a target,
   * the previous binding for that target is automatically broken.
   *
   * <p>Texture names are unsigned integers. The value 0 is reserved
   * to represent the default texture for each texture target. Texture
   * names and the corresponding texture contents are local to the
   * shared texture-object space (see <code>eglCreateContext</code>)
   * of the current GL rendering context.
   *
   * <p>You may use <code>glGenTextures</code> to generate a set of new
   * texture names.
   *
   * <p>While a texture is bound, GL operations on the target to which it
   * is bound affect the bound texture. If texture mapping of the
   * dimensionality of the target to which a texture is bound is
   * active, the bound texture is used. In effect, the texture targets
   * become aliases for the textures currently bound to them, and the
   * texture name 0 refers to the default textures that were bound to
   * them at initialization.
   *
   * <p>A texture binding created with <code>glBindTexture</code> remains
   * active until a different texture is bound to the same target, or
   * until the bound texture is deleted with
   * <code>glDeleteTextures</code>.
   *
   * <p>Once created, a named texture may be re-bound to the target of
   * the matching dimensionality as often as needed. It is usually
   * much faster to use <code>glBindTexture</code> to bind an existing named
   * texture to one of the texture targets than it is to reload the
   * texture image using <code>glTexImage2D</code>.
   * 
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> is not one of the allowable values.
   *
   * @param target Specifies the target to which the texture is
   * bound. Must be <code>GL_TEXTURE_2D</code>.
   * @param texture Specifies the name of a texture.
   */
  void glBindTexture(int target, int texture);

  /**
   * Specify pixel arithmetic.
   *
   * <p>Pixels can be drawn using a function that blends the incoming
   * (source) values with the values that are already in the color
   * buffer (the destination values). Use <code>glEnable</code> and
   * <code>glDisable</code> with argument <code>GL_BLEND</code> to
   * enable and disable blending. Blending is initially disabled.
   *
   * <p><code>glBlendFunc</code> defines the operation of blending
   * when it is enabled. <code>sfactor</code> specifies which of eleven
   * methods is used to scale the source color
   * components. <code>dfactor</code> specifies which of ten methods
   * is used to scale the destination color components. The eleven
   * possible methods are described in the following table. Each
   * method defines four scale factors, one each for red, green, blue,
   * and alpha.
   *
   * <p>In the table and in subsequent equations, source and
   * destination color components are referred to as <code>(Rs, Gs,
   * Bs, As)</code> and <code>(Rd, Gd, Bd, Ad)</code>. They are
   * understood to have integer values between 0 and <code>(kR, kG,
   * kB, kA)</code>, where
   *
   * <pre>
   * kc = 2mc - 1
   * </pre>
   *
   * and <code>(mR, mG, mB, mA)</code> is the number of red, green,
   * blue, and alpha bitplanes.
   *
   * <p>Source and destination scale factors are referred to as
   * <code>(sR, sG, sB, sA)</code> and <code>(dR, dG, dB,
   * dA)</code>. The scale factors described in the table, denoted
   * <code>(fR, fG, fB, fA)</code>, represent either source or
   * destination factors. All scale factors have range [0, 1].
   *
   * <pre>
   * Parameter               (fR, fG, fB, fA)
   *
   * GL_ZERO                 (0, 0, 0, 0)
   * GL_ONE                  (1, 1, 1, 1)
   * GL_SRC_COLOR            (Rs/kR, Gs/kG, Bs/kB, As/kA )
   * GL_ONE_MINUS_SRC_COLOR  (1, 1, 1, 1) - (Rs/kR, Gs/kG, Bs/kB, As/kA)
   * GL_DST_COLOR            (Rd/kR, Gd/kG, Bd/kB, Ad/kA )
   * GL_ONE_MINUS_DST_COLOR  (1, 1, 1, 1) - (Rd/kR, Gd/kG, Bd/kB, Ad/kA)
   * GL_SRC_ALPHA            (As/kA, As/kA, As/kA, As/kA )
   * GL_ONE_MINUS_SRC_ALPHA  (1, 1, 1, 1) - (As/kA, As/kA, As/kA, As/kA)
   * GL_DST_ALPHA            (Ad/kA, Ad/kA, Ad/kA, Ad/kA )
   * GL_ONE_MINUS_DST_ALPHA  (1, 1, 1, 1) - (Ad/kA, Ad/kA, Ad/kA, Ad/kA)
   * GL_SRC_ALPHA_SATURATE   (i, i, i, 1)
   * </pre>
   *
   * <p>In the table,
   *
   * <pre>
   * i = min(As, kA - Ad) / kA
   * </pre>
   *
   * <p>To determine the blended values of a pixel, the system uses the
   * following equations:
   *
   * <pre>
   * Rd = min( kR, Rs sR + Rd dR )
   * Gd = min( kG, Gs sG + Gd dG )
   * Bd = min( kB, Bs sB + Bd dB )
   * Ad = min( kA, As sA + Ad dA )
   * </pre>
   *
   * <p>Despite the apparent precision of the above equations,
   * blending arithmetic is not exactly specified, because blending
   * operates with imprecise integer color values. However, a blend
   * factor that should be equal to 1 is guaranteed not to modify its
   * multiplicand, and a blend factor equal to 0 reduces its
   * multiplicand to 0. For example, when <code>sfactor</code> is
   * <code>GL_SRC_ALPHA</code>, <code>dfactor</code> is
   * <code>GL_ONE_MINUS_SRC_ALPHA</code>, and <code>As</code> is equal
   * to <code>kA</code>, the equations reduce to simple replacement:
   *
   * <pre>
   * Rd = Rs
   * Gd = Gs
   * Bd = Bs
   * Ad = As
   * </pre>
   *
   * <p><code>glBlendFunc</code> operates on all pixel write
   * operations, including the scan conversion of points, lines, and
   * polygons. <code>glBlendFunc</code> does not affect
   * <code>glClear</code>.
   *
   * <h4>Examples</h4>
   *
   * <p>Transparency is best implemented using
   * <code>glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)</code> with
   * primitives sorted from farthest to nearest. Note that this
   * transparency calculation does not require the presence of alpha
   * bitplanes in the color buffer.
   *
   * <p><code>glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)</code> is
   * also useful for rendering antialiased points and lines.
   *
   * <h4>Notes</h4> Incoming (source) alpha is correctly thought of as
   * a material opacity, ranging from 1.0 (kA), representing complete
   * opacity, to 0.0 (0), representing complete transparency.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if either
   * <code>sfactor</code> or <code>dfactor</code> is not an accepted
   * value.
   *
   * @param sfactor Specifies how the red, green, blue, and alpha
   * source blending factors are computed. The following symbolic
   * constants are accepted: <code>GL_ZERO</code>,
   * <code>GL_ONE</code>, <code>GL_SRC_COLOR</code> (1.1 only),
   * <code>GL_ONE_MINUS_SRC_COLOR</code> (1.1 only),
   * <code>GL_DST_COLOR</code>, <code>GL_ONE_MINUS_DST_COLOR</code>,
   * <code>GL_SRC_ALPHA</code>, <code>GL_ONE_MINUS_SRC_ALPHA</code>,
   * <code>GL_DST_ALPHA</code>, <code>GL_ONE_MINUS_DST_ALPHA</code>,
   * and <code>GL_SRC_ALPHA_SATURATE</code>. The initial value is
   * <code>GL_ONE</code>.
   * @param dfactor Specifies how the red, green, blue, and alpha
   * destination blending factors are computed. The following symbolic
   * constants are accepted: <code>GL_ZERO</code>,
   * <code>GL_ONE</code>, <code>GL_SRC_COLOR</code>,
   * <code>GL_ONE_MINUS_SRC_COLOR</code>, <code>GL_DST_COLOR</code>
   * (1.1 only), <code>GL_ONE_MINUS_DST_COLOR</code> (1.1 only),
   * <code>GL_SRC_ALPHA</code>, <code>GL_ONE_MINUS_SRC_ALPHA</code>,
   * <code>GL_DST_ALPHA</code>, and
   * <code>GL_ONE_MINUS_DST_ALPHA</code>. The initial value is
   * <code>GL_ZERO</code>.
   */
  void glBlendFunc(int sfactor, int dfactor);

  /**
   * Clear buffers to preset values.
   *
   * <p><code>glClear</code> sets the bitplane area of the window to
   * values previously selected by <code>glClearColor</code>,
   * <code>glClearDepth</code>, and <code>glClearStencil</code>.
   *
   * <p>The pixel ownership test, the scissor test, dithering, and the
   * color buffer masks affect the operation of
   * <code>glClear</code>. The scissor box bounds the cleared
   * region. Alpha function, blend function, logical operation,
   * stenciling, texture mapping, and depth-buffering are ignored by
   * <code>glClear</code>.
   *
   * <p><code>glClear</code> takes a single argument that is the
   * bitwise OR of several values indicating which buffer is to be
   * cleared.
   *
   * <p>The values are as follows:
   *
   * <ul>
   * <li><code>GL_COLOR_BUFFER_BIT</code></li>
   * Indicates the color buffer.
   * <li><code>GL_DEPTH_BUFFER_BIT</code></li>
   * Indicates the depth buffer.
   * <li><code>GL_STENCIL_BUFFER_BIT</code></li>
   * Indicates the stencil buffer.
   * </ul>
   * 
   * <p>The value to which each buffer is cleared depends on the
   * setting of the clear value for that buffer.
   *
   * <h4>Notes</h4>
   *
   * <p>If a buffer is not present, then a <code>glClear</code>
   * directed at that buffer has no effect.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if any bit other
   * than the defined bits is set in <code>mask</code>.
   *
   * @param mask Bitwise OR of masks that indicate the buffers to be
   * cleared. Valid masks are <code>GL_COLOR_BUFFER_BIT</code>,
   * <code>GL_DEPTH_BUFFER_BIT</code>, and <code>GL_STENCIL_BUFFER_BIT</code>.
   */
  void glClear(int mask);

  /**
   * Specify clear values for the color buffer.
   *
   * <p><code>glClearColor</code> specifies the red, green, blue, and
   * alpha values used by <code>glClear</code> to clear the color
   * buffer. Values specified by <code>glClearColor</code> are clamped
   * to the range <code>[0, 1]</code>.
   *
   * @param red Specifies the red value used when the color buffer is
   * cleared.  The initial value is 0.
   * @param green Specifies the green value used when the color buffer
   * is cleared.  The initial value is 0.
   * @param blue Specifies the blue value used when the color buffer is
   * cleared.  The initial value is 0.
   * @param alpha Specifies the alpha value used when the color buffer
   * is cleared. The initial value is 0.
   */
  void glClearColor(float red, float green, float blue, float alpha);

  /**
   * Fixed-point version of <code>glClearColor</code>.
   *
   * @see #glClearColor
   */ 
  void glClearColorx(int red, int green, int blue, int alpha);

  /**
   * Specify the clear value for the depth buffer.
   *
   * <p><code>glClearDepth</code> specifies the depth value used by
   * <code>glClear</code> to clear the depth buffer. Values specified
   * by <code>glClearDepth</code> are clamped to the range <code>[0, 1]</code>.
   *
   * @param depth Specifies the depth value used when the depth buffer
   * is cleared. The initial value is 1.
   */
  void glClearDepthf(float depth);

  /**
   * Fixed-point version of <code>glClearDepth</code>.
   *
   * @see #glClearDepthf
   */
  void glClearDepthx(int depth);

  /**
   * Specify the clear value for the stencil buffer.
   *
   * <p><code>glClearStencil</code> specifies the index used by
   * <code>glClear</code> to clear the stencil buffer. <code>s</code>
   * is masked with 2^<i>m</i> - 1, where <i>m</i> is the number of
   * bits in the stencil buffer.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_STENCIL_BITS</code>
   *
   * @param s Specifies the index used when the stencil buffer is
   * cleared. The initial value is 0.
   */
  void glClearStencil(int s);

  /**
   * Select client-side active texture unit.
   *
   * <p><code>glClientActiveTexture</code> selects the vertex array
   * client state parameters to be modified by
   * <code>glTexCoordPointer</code>, and enabled or disabled with
   * <code>glEnableClientState</code> or
   * <code>glDisableClientState</code>, respectively, when called with
   * a parameter of <code>GL_TEXTURE_COORD_ARRAY</code>.
   *
   * <h4>Notes</h4>
   *
   * <p>It is always the case that <code>GL_TEXTURE</code><i>i</i> =
   * <code>GL_TEXTURE0</code> + <i>i</i>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>texture</code> is not one of
   * <code>GL_TEXTURE</code><i>i</i>, where <code>0 <= <i>i</i> <
   * GL_MAX_TEXTURE_UNITS</code>.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_UNITS</code>
   *
   * @param texture Specifies which texture unit to make active. The
   * number of texture units is implementation dependent, but must be
   * at least one (for 1.0), or two (for 1.1). <code>texture</code>
   * must be one of <code>GL_TEXTURE</code><i>i</i>, <code>0 <=
   * <i>i</i> < GL_MAX_TEXTURE_UNITS</code>, which is an
   * implementation-dependent value. The initial value is
   * <code>GL_TEXTURE0</code>.
   */
  void glClientActiveTexture(int texture);

  /**
   * Set the current color.
   *
   * <p>The GL stores a current four-valued RGBA
   * color. <code>glColor</code> sets a new four-valued RGBA color.
   *
   * <p>Current color values are stored in fixed-point or
   * floating-point. In case the values are stored in floating-point,
   * the mantissa and exponent sizes are unspecified.
   *
   * <p>Neither fixed-point nor floating-point values are clamped to
   * the range <code>[0, 1]</code> before the current color is
   * updated. However, color components are clamped to this range
   * before they are interpolated or written into the color buffer.
   *
   * @param red Specifies a new red value for the current color.  The
   * initial value is 1.
   * @param green Specifies a new green value for the current color.  The
   * initial value is 1.
   * @param blue Specifies a new blue value for the current color.  The
   * initial value is 1.
   * @param alpha Specifies a new alpha value for the current color.  The
   * initial value is 1.
   */
  void glColor4f(float red, float green, float blue, float alpha);

  /**
   * Fixed-point version of <code>glColor</code>.
   *
   * @see #glColor4f
   */
  void glColor4x(int red, int green, int blue, int alpha);

  /**
   * Enable and disable writing of color buffer components.
   *
   * <p><code>glColorMask</code> specifies whether the individual
   * components in the color buffer can or cannot be written. If
   * <code>red</code> is <code>false</code>, for example, no change
   * is made to the red component of any pixel in the color buffer,
   * regardless of the drawing operation attempted, including
   * <code>glClear</code>.
   *
   * <p>Changes to individual bits of components cannot be
   * controlled. Rather, changes are either enabled or disabled for
   * entire color components.
   *
   * @param red Specifies whether red can or cannot be written into
   * the color buffer. The initial value is <code>true</code>, indicating
   * that the color component can be written.
   * @param green Specifies whether green can or cannot be written into
   * the color buffer. The initial value is <code>true</code>, indicating
   * that the color component can be written.
   * @param blue Specifies whether blue can or cannot be written into
   * the color buffer. The initial value is <code>true</code>, indicating
   * that the color component can be written.
   * @param alpha Specifies whether alpha can or cannot be written
   * into the color buffer. The initial value is <code>true</code>,
   * indicating that the color component can be written.
   */
  void glColorMask(boolean red, boolean green, boolean blue, boolean alpha);

  /**
   * Define an array of colors.
   *
   * <p><code>glColorPointer</code> specifies an array of color
   * components to use when rendering. <code>size</code> specifies the
   * number of components per color, and must be 4. <code>type</code>
   * specifies the data type of each color component, and
   * <code>stride</code> specifies the byte stride from one color to
   * the next allowing vertices and attributes to be packed into a
   * single array or stored in separate arrays. (Single-array storage
   * may be more efficient on some implementations.)
   *
   * <p>When a color array is specified, <code>size</code>,
   * <code>type</code>, <code>stride</code>, and <code>pointer</code>
   * are saved as client-side state.
   *
   * <p>If the color array is enabled, it is used when
   * <code>glDrawArrays</code>, or <code>glDrawElements</code> is
   * called. To enable and disable the color array, call
   * <code>glEnableClientState</code> and
   * <code>glDisableClientState</code> with the argument
   * <code>GL_COLOR_ARRAY</code>. The color array is initially
   * disabled and isn't accessed when <code>glDrawArrays</code> or
   * <code>glDrawElements</code> is called.
   *
   * <p>Use <code>glDrawArrays</code> to construct a sequence of
   * primitives (all of the same type) from prespecified vertex and
   * vertex attribute arrays. Use <code>glDrawElements</code> to
   * construct a sequence of primitives by indexing vertices and
   * vertex attributes.
   *
   * <p>Setting <code>pointer</code> to <code>null</code> releases any
   * previously set <code>Buffer</code>.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glColorPointer</code> is typically implemented on the
   * client side.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>size</code> is not 4.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>type</code>
   * is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>stride</code> is negative.
   *
   * <p> The <code>pointer</code> argument must be a direct buffer
   * with a type matching that specified by the <code>type</code>
   * argument.
   *
   * @param size Specifies the number of components per color. Must be
   * 4. The initial value is 4.
   * @param type Specifies the data type of each color component in
   * the array. Symbolic constants <code>GL_UNSIGNED_BYTE</code>,
   * <code>GL_FIXED</code>, and <code>GL_FLOAT</code> are
   * accepted. The initial value is <code>GL_FLOAT</code>.
   * @param stride Specifies the byte offset between consecutive
   * colors. If <code>stride</code> is 0, the colors are understood to
   * be tightly packed in the array. The initial value is 0.
   * @param pointer Specifies a <code>Buffer</code> containing the
   * colors.
   *
   * @exception IllegalStateException if OpenGL ES 1.1 is being used and
   * VBOs are enabled.
   * @exception IllegalArgumentException if <code>pointer</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>pointer</code> is
   * not direct.
   */
  void glColorPointer(int size, int type, int stride, Buffer pointer);

  /**
   * Specify a two-dimensional compressed texture image.
   *
   * <p><code>glCompressedTexImage2D</code> defines a two-dimensional
   * texture image in compressed format.
   *
   * <p>The supported compressed formats are paletted textures. The
   * layout of the compressed image is a palette followed by multiple
   * mip-levels of texture indices used for lookup into the
   * palette. The palette format can be one of <code>R5_G6_B5</code>,
   * <code>RGBA4</code>, <code>RGB5_A1</code>, <code>RGB8</code>, or
   * <code>RGBA8</code>. The texture indices can have a resolution of
   * 4 or 8 bits. As a result, the number of palette entries is either
   * 16 or 256. If level is 0, only one mip-level of texture indices
   * is described in data. Otherwise, the negative value of level
   * specifies up to which mip-level the texture indices are
   * described. A possibly remaining pad nibble (half byte) for the
   * lowest resolution mip-level is ignored.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glPixelStore</code> has no effect on compressed texture
   * images.
   *
   * <p><code>glCompressedTexImage2D</code> specifies the
   * two-dimensional texture for the currently bound texture,
   * specified with <code>glBindTexture</code>, and the current
   * texture unit, specified with <code>glActiveTexture</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> is not <code>GL_TEXTURE_2D</code>.
   *
   * <p><code>GL_INVALID_VALUE</code> may be generated if
   * <code>level</code> is greater than 0 or the absolute value of
   * level is greater than log_2(<i>max</i>), where <i>max</i> is the
   * returned value of <code>GL_MAX_TEXTURE_SIZE</code>.
   *
   * <p>(1.0) <code>GL_INVALID_VALUE</code> is generated if
   * <code>internalformat</code> is not one of the accepted symbolic
   * constants.
   *
   * <p>(1.1) <code>GL_INVALID_ENUM</code> is generated if
   * <code>internalformat</code> is not one of the accepted symbolic
   * constants.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>width</code> or <code>height</code> is less than 0 or
   * greater than 2 + <code>GL_MAX_TEXTURE_SIZE</code>, or if either
   * cannot be represented as 2^<i>k</i> + 2*<code>border</code> for
   * some integer <i>k</i>.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>border</code> is not 0.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>imageSize</code> is not consistent with format, dimentions,
   * and contents of the compressed image.
   *
   * @param target Specifies the target texture. Must be
   * <code>GL_TEXTURE_2D</code>.
   * @param level Specifies the level-of-detail number. Must be less
   * than or equal to 0. Level 0 indicates a single
   * mip-level. Negative values indicate how many mip-levels are
   * described by data.
   * @param internalformat Specifies the color components in the
   * texture. The following symbolic constants are accepted:
   * <code>GL_PALETTE4_RGB8_OES</code>,
   * <code>GL_PALETTE4_RGBA8_OES</code>,
   * <code>GL_PALETTE4_R5_G6_B5_OES</code>,
   * <code>GL_PALETTE4_RGBA4_OES</code>,
   * <code>GL_PALETTE4_RGB5_A1_OES</code>,
   * <code>GL_PALETTE8_RGB8_OES</code>,
   * <code>GL_PALETTE8_RGBA8_OES</code>,
   * <code>GL_PALETTE8_R5_G6_B5_OES</code>,
   * <code>GL_PALETTE8_RGBA4_OES</code>, and
   * <code>GL_PALETTE8_RGB5_A1_OES</code>.
   * @param width Specifies the width of the texture image. Must be
   * 2^<i>n</i> + 2*<code>border</code> for some integer <i>n</i>. All
   * implementations support texture images that are at least 64
   * texels wide.
   * @param height Specifies the height of the texture image. Must be
   * 2^<i>m</i> + 2*<code>border</code> for some integer <i>m</i>. All
   * implementations support texture images that are at least 64
   * texels high.
   * @param border Specifies the width of the border. Must be 0.
   * @param imageSize Specifies the size of the compressed image data
   * in bytes.
   * @param data Specifies a <code>Buffer</code> containing the
   * compressed image data.
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
   * Specify a two-dimensional compressed texture subimage.
   *
   * <p><code>glCompressedTexSubImage2D</code> redefines a contiguous
   * subregion of an existing two-dimensional compressed texture
   * image. The texels referenced by <code>pixels</code> replace the
   * portion of the existing texture array with x indices
   * <code>xoffset</code> and <code>xoffset</code> +
   * <code>width</code> - 1, inclusive, and y indices
   * <code>yoffset</code> and <code>yoffset</code> +
   * <code>height</code> - 1, inclusive. This region may not include
   * any texels outside the range of the texture array as it was
   * originally specified. It is not an error to specify a subtexture
   * with zero width or height, but such a specification has no
   * effect.
   *
   * <p>Currently, there is no supported compressed format for this
   * function.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glPixelStore</code> has no effect on compressed texture
   * images.
   *
   * <p><code>glCompressedTexSubImage2D</code> specifies the
   * two-dimensional sub texture for the currently bound texture,
   * specified with <code>glBindTexture</code>, and the current
   * texture unit, specified with <code>glActiveTexture</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> is not <code>GL_TEXTURE_2D</code>.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if the texture
   * array has not been defined by a previous
   * <code>glCompressedTexImage2D</code> operation.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>level</code> is less than 0.
   *
   * <p><code>GL_INVALID_VALUE</code> may be generated if
   * <code>level</code> is greater than log2(<i>max</i>), where
   * <i>max</i> is the returned value of
   * <code>GL_MAX_TEXTURE_SIZE</code>.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>xoffset</code> < <i>-b</i>, <code>xoffset</code> + width >
   * (<i>w - b</i>), <code>yoffset</code> < <i>-b</i>, or
   * <code>yoffset</code> + <code>height</code> > (<i>h</i> - <i>b</i>)
   * , where <i>w</i> is the texture width, <i>h</i> is the texture
   * height, and <i>b</i> is the border of the texture image being
   * modified. Note that <i>w</i> and <i>h</i> include twice the
   * border width.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>width</code> or <code>height</code> is less than 0.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>type</code>
   * is not a type constant.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if type
   * <code>is</code> <code>GL_UNSIGNED_SHORT_5_6_5</code> and
   * <code>format</code> is not <code>GL_RGB</code>.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if
   * <code>type</code> is one of
   * <code>GL_UNSIGNED_SHORT_4_4_4_4</code>, or
   * <code>GL_UNSIGNED_SHORT_5_5_5_1</code> and <code>format</code> is
   * not <code>GL_RGBA</code>.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if none of the
   * above error conditions apply.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_SIZE</code>
   *
   * @param target Specifies the target texture. Must be
   * <code>GL_TEXTURE_2D</code>.
   * @param level Specifies the level-of-detail number.
   * @param xoffset Specifies a texel offset in the x direction within
   * the texture array.
   * @param yoffset Specifies a texel offset in the y direction within
   * the texture array.
   * @param width Specifies the width of the texture subimage.
   * @param height Specifies the height of the texture subimage.
   * @param format Specifies the format of the pixel data. Currently,
   * there is no supported format.
   * @param imageSize Specifies the size of the compressed pixel data in bytes.
   * @param data Specifies a <code>Buffer</code> containing the
   * compressed image data.
   *
   * @exception IllegalArgumentException if <code>data</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>data.remaining()</code> is less than
   * <code>imageSize</code>.
   */
  void glCompressedTexSubImage2D(int target, int level,
                                 int xoffset, int yoffset,
                                 int width, int height,
                                 int format, int imageSize,
                                 Buffer data);

  /** 
   * Specify a two-dimensional texture image with pixels from the
   * color buffer.
   *
   * <p><code>glCopyTexImage2D</code> defines a two-dimensional
   * texture image with pixels from the color buffer.
   *
   * <p>The screen-aligned pixel rectangle with lower left corner at
   * (<code>x</code>, <code>y</code>) and with a width of
   * <code>width</code> + 2*<code>border</code> and a height of
   * <code>height</code> + 2*<code>border</code> defines the texture
   * array at the mipmap level specified by
   * <code>level</code>. <code>internalformat</code> specifies the
   * color components of the texture.
   *
   * <p>The red, green, blue, and alpha components of each pixel that
   * is read are converted to an internal fixed-point or
   * floating-point format with unspecified precision. The conversion
   * maps the largest representable component value to 1.0, and
   * component value 0 to 0.0. The values are then converted to the
   * texture's internal format for storage in the texel array.
   *
   * <p><code>internalformat</code> must be chosen such that color
   * buffer components can be dropped during conversion to the
   * internal format, but new components cannot be added. For example,
   * an RGB color buffer can be used to create <code>LUMINANCE</code>
   * or <code>RGB</code> textures, but not <code>ALPHA</code>,
   * <code>LUMINANCE_ALPHA</code> or <code>RGBA</code> textures.
   *
   * <p>Pixel ordering is such that lower x and y screen coordinates
   * correspond to lower s and t texture coordinates.
   *
   * <p>If any of the pixels within the specified rectangle of the
   * color buffer are outside the window associated with the current
   * rendering context, then the values obtained for those pixels are
   * undefined.
   *
   * <h4>Notes</h4>
   *
   * <p>An image with <code>height</code> or <code>width</code> of 0
   * indicates a null-texture.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> is not <code>GL_TEXTURE_2D</code>. <!-- If the
   * <code>OES_texture_cube_map</code> extension is present, then
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Y</code>,
   * <code>GL_TEXTURE_CUBE_MAP_POSITIVE_Z</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_X</code>,
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Y</code>, and
   * <code>GL_TEXTURE_CUBE_MAP_NEGATIVE_Z</code> are also accepted. -->
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if
   * <code>internalformat</code> is not compatible with the color
   * buffer format.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>level</code> is less than 0.
   *
   * <p><code>GL_INVALID_VALUE</code> may be generated if level is
   * greater than log_2(<i>max</i>), where <i>max</i> is the returned
   * value of <code>GL_MAX_TEXTURE_SIZE</code>.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>width</code> or <code>height</code> is less than 0, greater
   * than <code>GL_MAX_TEXTURE_SIZE</code>, or if <code>width</code>
   * or <code>height</code> cannot be represented as 2^<i>k</i> +
   * 2*<code>border</code> for some integer <i>k</i>.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>border</code> is not 0.
   *
   * <p>(1.0) <code>GL_INVALID_VALUE</code> is generated if
   * <code>internalformat</code> is not an accepted constant.
   *
   * <p>(1.1) <code>GL_INVALID_ENUM</code> is generated if
   * <code>internalformat</code> is not an accepted constant.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_SIZE</code>
   * 
   * @param target Specifies the target texture. Must be
   * <code>GL_TEXTURE_2D</code>.
   * @param level Specifies the level-of-detail number. Level 0 is the
   * base image level. Level n is the nth mipmap reduction image.
   * @param internalformat Specifies the color components of the
   * texture. Must be one of the following symbolic constants:
   * <code>GL_ALPHA</code>, <code>GL_LUMINANCE</code>,
   * <code>GL_LUMINANCE_ALPHA</code>, <code>GL_RGB</code>, or
   * <code>GL_RGBA</code>.
   * @param x Specifies the window x coordinate of the lower left
   * corner of the rectangular region of pixels to be copied.
   * @param y Specifies the window y coordinate of the lower left
   * corner of the rectangular region of pixels to be copied.
   * @param width Specifies the width of the texture image. Must be 0
   * or 2^<i>n</i? + 2*<code>border</code> for some integer <i>n</i>.
   * @param height Specifies the height of the texture image. Must be
   * 0 or 2^<i>m</i> + 2*<code>border</code> for some integer <i>m</i>.
   * @param border Specifies the width of the border. Must be 0.
   */
  void glCopyTexImage2D(int target, int level,
                        int internalformat,
                        int x, int y,
                        int width, int height,
                        int border);

  /**
   * Specify a two-dimensional texture subimage with pixels from the
   * color buffer.
   *
   * <p><code>glCopyTexSubImage2D</code> replaces a rectangular portion of a
   * two-dimensional texture image with pixels from the color buffer.
   *
   * <p>The screen-aligned pixel rectangle with lower left corner at
   * (<code>x</code>, <code>y</code>) and with width width and height
   * height replaces the portion of the texture array with x indices
   * <code>xoffset</code> through <code>xoffset</code> +
   * <code>width</code> - 1, inclusive, and y indices
   * <code>yoffset</code> through <code>yoffset</code> +
   * <code>height</code> - 1, inclusive, at the mipmap level specified
   * by level.
   *
   * <p>The pixels in the rectangle are processed the same way as with
   * <code>glCopyTexImage2D</code>.
   *
   * <p><code>glCopyTexSubImage2D</code> requires that the internal
   * format of the currently bound texture is such that color buffer
   * components can be dropped during conversion to the internal
   * format, but new components cannot be added. For example, an RGB
   * color buffer can be used to create LUMINANCE or RGB textures, but
   * not ALPHA, LUMINANCE_ALPHA or RGBA textures.
   *
   * <p>The destination rectangle in the texture array may not include
   * any texels outside the texture array as it was originally
   * specified. It is not an error to specify a subtexture with zero
   * width or height, but such a specification has no effect.
   *
   * <p>If any of the pixels within the specified rectangle of the
   * current color buffer are outside the read window associated with
   * the current rendering context, then the values obtained for those
   * pixels are undefined.
   *
   * <p>No change is made to the <code>internalformat</code>,
   * <code>width</code>, <code>height</code>, or <code>border</code>
   * parameters of the specified texture array or to texel values
   * outside the specified subregion.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>target</code> is not <code>GL_TEXTURE_2D</code>.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if the texture
   * array has not been defined by a previous
   * <code>glTexImage2D</code> or <code>glCopyTexImage2D</code>
   * operation or if the internal format of the currently bound
   * texture is not compatible with the color buffer format.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>level</code> is less than 0.
   *
   * <p><code>GL_INVALID_VALUE</code> may be generated if
   * <code>level</code> is greater than log_2(<i>max</i>), where
   * <i>max</i> is the returned value of
   * <code>GL_MAX_TEXTURE_SIZE</code>.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if x < <i>-b</i>,
   * or y < <i>-b</i>, where <i>b</i> is the border of the texture
   * being modified.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>xoffset</code> < <i>-b</i>, <code>xoffset</code> +
   * <code>width</code> > (<i>w - b</i>) , <code>yoffset</code> <
   * <i>-b</i>, or <code>yoffset</code> + <code>height</code> > (<i>h
   * - b</i>) , where <i>w</i> is the texture width, <i>h</i> is the
   * texture height, and <i>b</i> is the border of the texture image
   * being modified. Note that <i>w</i> and <i>h</i> include twice the
   * border width.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_SIZE</code>
   *
   * @param target Specifies the target texture. Must be
   * <code>GL_TEXTURE_2D</code>.
   * @param level Specifies the level-of-detail number. Level 0 is the
   * base image level. Level n is the nth mipmap reduction image.
   * @param xoffset Specifies a texel offset in the x direction within
   * the texture array.
   * @param yoffset Specifies a texel offset in the y direction within
   * the texture array.
   * @param x Specifies the window x coordinate of the lower left
   * corner of the rectangular region of pixels to be copied.
   * @param y Specifies the window y coordinate of the lower left
   * corner of the rectangular region of pixels to be copied.
   * @param width Specifies the width of the texture subimage.
   * @param height Specifies the height of the texture subimage.
   */
  void glCopyTexSubImage2D(int target, int level,
                           int xoffset, int yoffset,
                           int x, int y,
                           int width, int height);

  /**
   * Specify whether front- or back-facing polygons are culled.
   *
   * <p><code>glCullFace</code> specifies whether front- or
   * back-facing polygons are culled (as specified by
   * <code>mode</code>) when culling is enabled. To enable and disable
   * culling, call <code>glEnable</code> and <code>glDisable</code>
   * with argument <code>GL_CULL_FACE</code>. Culling is initially
   * disabled.
   *
   * <p><code>glFrontFace</code> specifies which of the clockwise and
   * counterclockwise polygons are front-facing and back-facing.
   *
   * <h4>Notes</h4>
   *
   * <p>If mode is <code>GL_FRONT_AND_BACK</code>, no polygons are
   * drawn, but other primitives such as points and lines are drawn.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>mode</code>
   * is not an accepted value.
   *
   * @param mode Specifies whether front- or back-facing polygons are
   * culled. Symbolic constants <code>GL_FRONT</code>,
   * <code>GL_BACK</code>, and <code>GL_FRONT_AND_BACK</code> are
   * accepted. The initial value is <code>GL_BACK</code>.
   */
  void glCullFace(int mode);

  /**
   * Delete named textures.
   *
   * <p><code>glDeleteTextures</code> deletes <code>n</code> textures
   * named by the elements of the array <code>textures</code>. After a
   * texture is deleted, it has no contents or dimensionality, and its
   * name is free for reuse (for example by
   * <code>glGenTextures</code>). If a texture that is currently bound
   * is deleted, the binding reverts to 0 (the default texture).
   *
   * <p><code>glDeleteTextures</code> silently ignores 0's and names
   * that do not correspond to existing textures.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if <code>n</code>
   * is negative.
   *
   * @param n Specifies the number of textures to be deleted.
   * @param textures Specifies an array of textures to be deleted.
   * @param offset the starting offset within the
   * <code>textures</code> array.
   *
   * @exception IllegalArgumentException if <code>textures</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>textures.length -
   * offset</code> is less than <code>n</code>.
   */
  void glDeleteTextures(int n, int[] textures, int offset);

  /**
   * Integer <code>Buffer</code> version of <code>glDeleteTextures</code>.
   *
   * @param textures an <code>IntBuffer</code>.
   *
   * @exception IllegalArgumentException if <code>textures</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>textures.remaining()</code> is less than <code>n</code>.
   *
   * @see #glDeleteTextures(int n, int[] textures, int offset)
   */
  void glDeleteTextures(int n, IntBuffer textures);

  /**
   * Specify the value used for depth buffer comparisons.
   *
   * <p><code>glDepthFunc</code> specifies the function used to compare
   * each incoming pixel depth value with the depth value present in
   * the depth buffer. The comparison is performed only if depth
   * testing is enabled. To enable and disable depth testing, call
   * <code>glEnable</code> and <code>glDisable</code> with argument
   * <code>GL_DEPTH_TEST</code>. Depth testing is initially disabled.
   *
   * <p>func specifies the conditions under which the pixel will be
   * drawn. The comparison functions are as follows:
   *
   * <ul>
   * <li><code>GL_NEVER</code></li>
   * Never passes.
   * <li><code>GL_LESS</code></li>
   * Passes if the incoming depth value is less than the stored depth value.
   * <li><code>GL_EQUAL</code></li>
   * Passes if the incoming depth value is equal to the stored depth value.
   * <li><code>GL_LEQUAL</code></li>
   * Passes if the incoming depth value is less than or equal to the
   * stored depth value.
   * <li><code>GL_GREATER</code></li>
   * Passes if the incoming depth value is greater than the stored
   * depth value.
   * <li><code>GL_NOTEQUAL</code></li>
   * Passes if the incoming depth value is not equal to the stored
   * depth value.
   * <li><code>GL_GEQUAL</code></li>
   * Passes if the incoming depth value is greater than or equal to
   * the stored depth value.
   * <li><code>GL_ALWAYS</code></li>
   * Always passes.
   * </ul>
   *
   * <p>The initial value of func is <code>GL_LESS</code>. Initially,
   * depth testing is disabled. Even if the depth buffer exists and
   * the depth mask is non-zero, the depth buffer is not updated if
   * the depth test is disabled.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>func</code>
   * is not an accepted value.
   *
   * @param func Specifies the depth comparison function. Symbolic
   * constants <code>GL_NEVER</code>, <code>GL_LESS</code>,
   * <code>GL_EQUAL</code>, <code>GL_LEQUAL</code>,
   * <code>GL_GREATER</code>, <code>GL_NOTEQUAL</code>,
   * <code>GL_GEQUAL</code>, and <code>GL_ALWAYS</code> are
   * accepted. The initial value is <code>GL_LESS</code>.
   */
  void glDepthFunc(int func);

  /**
   * Enable or disable writing into the depth buffer.
   *
   * <p><code>glDepthMask</code> specifies whether the depth buffer is
   * enabled for writing. If <code>flag</code> is <code>false</code>,
   * depth buffer writing is disabled. Otherwise, it is
   * enabled. Initially, depth buffer writing is enabled.
   *
   * <h4>1.0 Notes</h4>
   *
   * <p><code>glDepthMask</code> does not affect <code>glClear</code>.
   *
   * @param flag Specifies whether the depth buffer is enabled for
   * writing. If flag is <code>GL_FALSE</code>, depth buffer writing
   * is disabled, otherwise it is enabled. The initial value is
   * <code>true</code>.
   */
  void glDepthMask(boolean flag);

  /**
   * Specify mapping of depth values from normalized device
   * coordinates to window coordinates.
   *
   * <p>After clipping and division by w, depth coordinates range from
   * -1 to 1, corresponding to the near and far clipping
   * planes. <code>glDepthRange</code> specifies a linear mapping of
   * the normalized depth coordinates in this range to window depth
   * coordinates. Regardless of the actual depth buffer
   * implementation, window coordinate depth values are treated as
   * though they range from 0 through 1 (like color components). Thus,
   * the values accepted by <code>glDepthRange</code> are both clamped
   * to this range before they are accepted.
   *
   * <p>The setting of (0, 1) maps the near plane to 0 and the far
   * plane to 1. With this mapping, the depth buffer range is fully
   * utilized.
   *
   * <h4>Notes</h4>
   *
   * <p>It is not necessary that <code>near</code> be less than
   * <code>far</code>. Reverse mappings such as <code>near</code> = 1,
   * and <code>far</code> = 0 are acceptable.
   *
   * @param zNear Specifies the mapping of the near clipping plane to
   * window coordinates. The initial value is 0.
   * @param zFar Specifies the mapping of the far clipping plane to
   * window coordinates. The initial value is 1.
   */
  void glDepthRangef(float zNear, float zFar);

  /**
   * Fixed-point version of <code>glDepthRange</code>.
   *
   * @see #glDepthRangef(float zNear, float zFar)
   */
  void glDepthRangex(int zNear, int zFar);

  /**
   * Disable server-side GL capabilities.
   *
   * @see #glEnable
   */
  void glDisable(int cap);

  /**
   * Disable client-side capability.
   *
   * @see #glEnableClientState
   */
  void glDisableClientState(int array);

  /**
   * Render primitives from array data.
   *
   * <p><code>glDrawArrays</code> specifies multiple geometric
   * primitives with very few subroutine calls. You can prespecify
   * separate arrays of vertices, normals, colors, and texture
   * coordinates and use them to construct a sequence of primitives
   * with a single call to <code>glDrawArrays</code>.
   *
   * <p>When <code>glDrawArrays</code> is called, it uses
   * <code>count</code> sequential elements from each enabled array to
   * construct a sequence of geometric primitives, beginning with
   * element <code>first</code>. <code>mode</code> specifies what kind
   * of primitives are constructed, and how the array elements
   * construct those primitives. If <code>GL_VERTEX_ARRAY</code> is
   * not enabled, no geometric primitives are generated.
   *
   * <p>Vertex attributes that are modified by
   * <code>glDrawArrays</code> have an unspecified value after
   * <code>glDrawArrays</code> returns. For example, if
   * <code>GL_COLOR_ARRAY</code> is enabled, the value of the current
   * color is undefined after <code>glDrawArrays</code>
   * executes. Attributes that aren't modified remain well defined.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>mode</code>
   * is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>count</code> is negative.
   *
   * @exception ArrayIndexOutOfBoundsException if any index in the
   * sequence <code>first, ..., first + count - 1</code> will result
   * in a reference to an entry outside of the current vertex, color,
   * normal, texture coordinate, point size, matrix index, or weight
   * array.
   *
   * @param mode Specifies what kind of primitives to render. Symbolic
   * constants <code>GL_POINTS</code>, <code>GL_LINE_STRIP</code>,
   * <code>GL_LINE_LOOP</code>, <code>GL_LINES</code>,
   * <code>GL_TRIANGLE_STRIP</code>, <code>GL_TRIANGLE_FAN</code>, and
   * <code>GL_TRIANGLES</code> are accepted.
   * @param first Specifies the starting index in the enabled arrays.
   * @param count Specifies the number of indices to be rendered.
   */
  void glDrawArrays(int mode, int first, int count);

  /**
   * Render primitives from array data.
   *
   * <p><code>glDrawElements</code> specifies multiple geometric
   * primitives with very few subroutine calls. You can prespecify
   * separate arrays of vertices, normals, colors, and texture
   * coordinates and use them to construct a sequence of primitives
   * with a single call to <code>glDrawElements</code>.
   *
   * <p>When <code>glDrawElements</code> is called, it uses
   * <code>count</code> sequential indices from <code>indices</code>
   * to lookup elements in enabled arrays to construct a sequence of
   * geometric primitives. <code>mode</code> specifies what kind of
   * primitives are constructed, and how the array elements construct
   * these primitives. If <code>GL_VERTEX_ARRAY</code> is not enabled,
   * no geometric primitives are constructed.
   *
   * <p>Vertex attributes that are modified by
   * <code>glDrawElements</code> have an unspecified value after
   * <code>glDrawElements</code> returns. For example, if
   * <code>GL_COLOR_ARRAY</code> is enabled, the value of the current
   * color is undefined after <code>glDrawElements</code>
   * executes. Attributes that aren't modified maintain their previous
   * values.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>mode</code>
   * is not an accepted value.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>type</code>
   * is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>count</code> is negative.
   *
   * @param mode Specifies what kind of primitives to render. Symbolic
   * constants <code>GL_POINTS</code>, <code>GL_LINE_STRIP</code>,
   * <code>GL_LINE_LOOP</code>, <code>GL_LINES</code>,
   * <code>GL_TRIANGLE_STRIP</code>, <code>GL_TRIANGLE_FAN</code>, and
   * <code>GL_TRIANGLES</code> are accepted.
   * @param count Specifies the number of elements to be rendered.
   * @param type Specifies the type of the values in indices. Must be
   * either <code>GL_UNSIGNED_BYTE</code> or <code>GL_UNSIGNED_SHORT</code>.
   * @param indices Specifies a pointer to the location where the
   * indices are stored.
   *
   * @exception IllegalStateException if the most recent call to
   * <code>glBindBuffer</code> for the
   * <code>GL_ELEMENT_ARRAY_BUFFER</code> target had a non-zero
   * <code>buffer</code> parameter (i.e., an index buffer is bound).
   * @exception IllegalArgumentException if <code>indices</code> is
   * <code>null</code>.
   * @exception ArrayIndexOutOfBoundsException if any index in the
   * sequence of indices from <code>0</code> to <code>count - 1</code>
   * would result in a reference to an entry outside of the currently
   * bound index or data (vertex, color, normal, texture coordinate
   * array, weight, matrix index, or point size) array.
   */
  void glDrawElements(int mode, int count, int type, Buffer indices);

  /**
   * Enable server-side GL capabilities.
   *
   * <p><code>glEnable</code> and <code>glDisable</code> enable and
   * disable various capabilities. The initial value for each
   * capability with the exception of <code>GL_DITHER</code> and
   * <code>GL_MULTISAMPLE</code> is <code>GL_FALSE</code>. The initial
   * value for <code>GL_DITHER</code> and <code>GL_MULTISAMPLE</code>
   * is <code>GL_TRUE</code>.
   *
   * <p>Both <code>glEnable</code> and <code>glDisable</code> take a
   * single argument, <code>cap</code>, which can assume one of the
   * following values:
   *
   * <ul>
   * <li><code>GL_ALPHA_TEST</code></li>
   *
   * <p>If enabled, do alpha testing. See <code>glAlphaFunc</code>.
   *
   * <li><code>GL_BLEND</code></li>
   *
   * <p>If enabled, blend the incoming color values with the values in
   * the color buffers. See <code>glBlendFunc</code>.
   *
   * <li><code>GL_COLOR_LOGIC_OP</code></li>
   *
   * <p>If enabled, apply the currently selected logical operation to the
   * incoming color and color buffer values. See <code>glLogicOp</code>.
   *
   * <li><code>GL_COLOR_MATERIAL</code></li>
   *
   * <p>If enabled, have ambient and diffuse material parameters track
   * the current color.
   *
   * <li><code>GL_CULL_FACE</code></li>
   *
   * <p>If enabled, cull polygons based on their winding in window
   * coordinates. See <code>glCullFace</code>.
   *
   * <li><code>GL_DEPTH_TEST</code></li>
   *
   * <p>If enabled, do depth comparisons and update the depth
   * buffer. Note that even if the depth buffer exists and the depth
   * mask is non-zero, the depth buffer is not updated if the depth
   * test is disabled. See <code>glDepthFunc</code>,
   * <code>glDepthMask</code>, and <code>glDepthRange</code>.
   *
   * <li><code>GL_DITHER</code></li>
   *
   * <p>If enabled, dither color components or indices before they are
   * written to the color buffer.
   *
   * <li><code>GL_FOG</code></li>
   *
   * <p>If enabled, blend a fog color into the posttexturing color. See
   * <code>glFog</code>.
   *
   * <li><code>GL_LIGHT</code><i>i</i></li>
   *
   * <p>If enabled, include light <i>i</i> in the evaluation of the
   * lighting equation. See <code>glLightModel</code> and
   * <code>glLight</code>.
   *
   * <li><code>GL_LIGHTING</code></li>
   *
   * <p>If enabled, use the current lighting parameters to compute the
   * vertex color. Otherwise, simply associate the current color with
   * each vertex. See <code>glMaterial</code>,
   * <code>glLightModel</code>, and <code>glLight</code>.
   *
   * <li><code>GL_LINE_SMOOTH</code></li>
   *
   * <p>If enabled, draw lines with correct filtering. Otherwise, draw
   * aliased lines. See <code>glLineWidth</code>.
   *
   * <li><code>GL_MULTISAMPLE</code></li>
   *
   * <p>If enabled, perform multisampling of fragments for single-pass
   * antialiasing and other effects. See <code>glSampleCoverage</code>.
   *
   * <li><code>GL_NORMALIZE</code></li>
   *
   * <p>If enabled, normal vectors are scaled to unit length after
   * transformation. See <code>glNormal</code> and
   * <code>glNormalPointer</code>.
   *
   * <li><code>GL_POINT_SMOOTH</code></li>
   *
   * <p>If enabled, draw points with proper filtering. Otherwise, draw
   * aliased points. See <code>glPointSize</code>.
   *
   * <li><code>GL_POLYGON_OFFSET_FILL</code></li>
   *
   * <p>If enabled, an offset is added to depth values of a polygon's
   * fragments before the depth comparison is performed. See
   * <code>glPolygonOffset</code>.
   *
   * <li><code>GL_RESCALE_NORMAL</code></li>
   *
   * <p>If enabled, normal vectors are scaled by a factor derived from
   * the modelview matrix. See <code>glNormal</code> and
   * <code>glNormalPointer</code>.
   *
   * <li><code>GL_SAMPLE_ALPHA_TO_MASK</code> (1.0 only)</li>
   *
   * <p>If enabled, convert fragment alpha values to multisample coverage
   * modification masks. See <code>glSampleCoverage</code>.
   *
   * <li><code>GL_SAMPLE_ALPHA_TO_COVERAGE</code> (1.1 only)</li>
   *
   * <p>If enabled, a temporary coverage value is generated where each
   * bit is determined by the alpha value at the corresponding sample
   * location. The temporary coverage value is then ANDed with the
   * fragment coverage value. Otherwise the fragment coverage value is
   * unchanged at this point. See <code>glSampleCoverage</code>.
   *
   * <li><code>GL_SAMPLE_ALPHA_TO_ONE</code></li>
   *
   * <p>If enabled, set fragment alpha to the maximum permissible value
   * after computing multisample coverage modification masks. See
   * <code>glSampleCoverage</code>.
   *
   * <li><code>GL_SAMPLE_MASK</code> (1.0 only)</li>
   *
   * <p>If enabled, apply a mask to modify fragment coverage during
   * multisampling. See <code>glSampleCoverage</code>.
   *
   * <li><code>GL_SAMPLE_COVERAGE</code> (1.1 only)</li>
   *
   * <p>If enabled, the fragment coverage is ANDed with another temporary
   * coverage. This temporary coverage is generated in the same manner
   * as for <code>GL_SAMPLE_ALPHA_TO_COVERAGE</code> described above,
   * but as a function of the value of
   * <code>GL_SAMPLE_COVERAGE_VALUE</code>. If
   * <code>GL_SAMPLE_COVERAGE_INVERT</code> is <code>GL_TRUE</code>,
   * the temporary coverage is inverted (all bit values are inverted)
   * before it is ANDed with the fragment coverage. See
   * <code>glSampleCoverage</code>.
   *
   * <li><code>GL_SCISSOR_TEST</code></li>
   *
   * <p>If enabled, discard fragments that are outside the scissor
   * rectangle. See <code>glScissor</code>.
   *
   * <li><code>GL_STENCIL_TEST</code></li>
   *
   * <p>If enabled, do stencil testing and update the stencil buffer. See
   * <code>glStencilFunc</code>, <code>glStencilMask</code>, and
   * <code>glStencilOp</code>.
   *
   * <li><code>GL_TEXTURE_2D</code></li>
   *
   * <p>If enabled, two-dimensional texturing is performed for the active
   * texture unit. See <code>glActiveTexture</code>,
   * <code>glTexImage2D</code>, <code>glCompressedTexImage2D</code>,
   * and <code>glCopyTexImage2D</code>.
   *
   * <li><code>GL_CLIP_PLANE</code><i>i</i> (1.1 only)</li>
   *
   * <p>If enabled, clipping plane <i>i</i> is enabled.  See
   * <code>glClipPlane</code>.
   *
   * <li><code>GL_POINT_SPRITE_OES</code>
   * (1.1 + <code>OES_point_sprite</code> extension)</li>
   *
   * <p>If enabled, point sprites are enabled. See
   * <code>glPointSize</code> and <code>glTexEnv</code>.
   *
   * </ul>
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if cap is not one of
   * the values listed previously.
   *
   * @param cap Specifies a symbolic constant indicating a GL capability.
   */
  void glEnable(int cap);

  /**
   * Enable client-side capability.
   *
   * <p><code>glEnableClientState</code> and
   * <code>glDisableClientState</code> enable or disable individual
   * client-side capabilities. By default, all client-side
   * capabilities are disabled. Both <code>glEnableClientState</code>
   * and <code>glDisableClientState</code> take a single argument,
   * <code>array</code>, which can assume one of the following values:
   *
   * <ul>
   *
   * <li><code>GL_COLOR_ARRAY</code></li>
   *
   * <p>If enabled, the color array is enabled for writing and used
   * during rendering when <code>glDrawArrays</code>, or
   * <code>glDrawElements</code> is called. See
   * <code>glColorPointer</code>.
   *
   * <li><code>GL_NORMAL_ARRAY</code></li>
   *
   * <p>If enabled, the normal array is enabled for writing and used
   * during rendering when <code>glDrawArrays</code>, or
   * <code>glDrawElements</code> is called. See
   * <code>glNormalPointer</code>.
   *
   * <li><code>GL_TEXTURE_COORD_ARRAY</code></li>
   *
   * <p>If enabled, the texture coordinate array is enabled for writing
   * and used during rendering when <code>glDrawArrays</code>, or
   * <code>glDrawElements</code> is called. See
   * <code>glTexCoordPointer</code>.
   *
   * <li><code>GL_VERTEX_ARRAY</code></li>
   *
   * <p>If enabled, the vertex array is enabled for writing and used
   * during rendering when <code>glDrawArrays</code>, or
   * <code>glDrawElements</code> is called. See
   * <code>glVertexPointer</code>.
   *
   * <li><code>GL_POINT_SIZE_ARRAY_OES</code>
   * (<code>OES_point_size_array</code> extension)</li>
   *
   * <p>If enabled, the point size array controls the sizes used to
   * render points and point sprites. In this case the point size
   * defined by <code>glPointSize</code> is ignored. The point sizes
   * supplied in the point size arrays will be the sizes used to
   * render both points and point sprites. See
   * <code>glPointSize</code>.
   *
   * </ul>
   * 
   * <h4>Notes</h4>
   *
   * <p>Enabling and disabling <code>GL_TEXTURE_COORD_ARRAY</code>
   * affects the active client texture unit. The active client texture
   * unit is controlled with <code>glClientActiveTexture</code>.
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>array</code> is not an accepted value.
   *
   * @param array Specifies the capability to enable or
   * disable. Symbolic constants <code>GL_COLOR_ARRAY</code>,
   * <code>GL_NORMAL_ARRAY</code>,
   * <code>GL_TEXTURE_COORD_ARRAY</code>,
   * <code>GL_VERTEX_ARRAY</code>, and
   * <code>GL_POINT_SIZE_ARRAY_OES</code>
   * (<code>OES_point_size_array</code> extension) are accepted. <!-- If the
   * <code>OES_matrix_palette</code> extension is present, symbolic
   * constants <code>GL_MATRIX_INDEX_ARRAY_OES</code>,
   * <code>GL_WEIGHT_ARRAY_OES</code> are additionally accepted. -->
   */
  void glEnableClientState(int array);

  /**
   * Block until all GL execution is complete.
   *
   * <p><code>glFinish</code> does not return until the effects of all
   * previously called GL commands are complete. Such effects include
   * all changes to GL state, all changes to connection state, and all
   * changes to the frame buffer contents.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glFinish</code> requires a round trip to the server.
   */
  void glFinish();

  /**
   * Force execution of GL commands in finite time.
   *
   * <p>Different GL implementations buffer commands in several
   * different locations, including network buffers and the graphics
   * accelerator itself. <code>glFlush</code> empties all of these
   * buffers, causing all issued commands to be executed as quickly as
   * they are accepted by the actual rendering engine. Though this
   * execution may not be completed in any particular time period, it
   * does complete in finite time.
   *
   * <p>Because any GL program might be executed over a network, or on an
   * accelerator that buffers commands, all programs should call
   * <code>glFlush</code> whenever they count on having all of their previously
   * issued commands completed. For example, call <code>glFlush</code> before
   * waiting for user input that depends on the generated image.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glFlush</code> can return at any time. It does not wait until the
   * execution of all previously issued GL commands is complete.
   */
  void glFlush();

  /**
   * Specify fog parameters.
   *
   * <p>If fog is enabled, fog affects rasterized geometry, bitmaps,
   * and pixel blocks, but not buffer clear operations. To enable and
   * disable fog, call <code>glEnable</code> and
   * <code>glDisable</code> with argument <code>GL_FOG</code>. Fog is
   * initially disabled.
   *
   * <p><code>glFog</code> assigns the value in <code>param</code> to
   * the fog parameter specified by <code>pname</code>. The following
   * values are accepted for <code>pname</code>:
   *
   * <ul>
   *
   * <li><code>GL_FOG_MODE</code></li>
   *
   * <p><code>param</code> is a single value that specifies the equation
   * to be used to compute the fog blend factor f. Three symbolic
   * constants are accepted: <code>GL_LINEAR</code>,
   * <code>GL_EXP</code>, and <code>GL_EXP2</code>. The equations
   * corresponding to these symbolic constants are defined below. The
   * initial fog mode is <code>GL_EXP</code>.
   *
   * <li><code>GL_FOG_DENSITY</code></li>
   *
   * <p><code>param</code> is a single value that specifies density, the
   * fog density used in both exponential fog equations. Only
   * nonnegative densities are accepted. The initial fog density is 1.
   *
   * <li><code>GL_FOG_START</code></li>
   *
   * <p><code>param</code> is a single value that specifies start, the
   * near distance used in the linear fog equation. The initial near
   * distance is 0.
   *
   * <li><code>GL_FOG_END</code></li>
   *
   * <p><code>param</code> is a single value that specifies end, the far
   * distance used in the linear fog equation. The initial far
   * distance is 1.
   *
   * </ul>
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>pname</code> is not an accepted value, or if
   * <code>pname</code> is <code>GL_FOG_MODE</code> and
   * <code>param</code> is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>pname</code> is <code>GL_FOG_DENSITY</code>, and
   * <code>param</code> is negative.
   *
   * @param pname Specifies a single-valued fog
   * parameter. <code>GL_FOG_MODE</code>, <code>GL_FOG_DENSITY</code>,
   * <code>GL_FOG_START</code>, and <code>GL_FOG_END</code> are
   * accepted.
   * @param param Specifies the value that <code>pname</code> will be
   * set to.
   */
  void glFogf(int pname, float param);

  /**
   * Specify fog parameters (array version).
   *
   * <p>If fog is enabled, fog affects rasterized geometry, bitmaps,
   * and pixel blocks, but not buffer clear operations. To enable and
   * disable fog, call <code>glEnable</code> and
   * <code>glDisable</code> with argument <code>GL_FOG</code>. Fog is
   * initially disabled.
   *
   * <p><code>glFog</code> assigns the value or values in
   * <code>params</code> to the fog parameter specified by
   * <code>pname</code>. The following values are accepted for
   * <code>pname</code>:
   *
   * <ul>
   *
   * <li><code>GL_FOG_MODE</code></li>
   *
   * <p><code>params</code> contains a single value that specifies the
   * equation to be used to compute the fog blend factor f. Three
   * symbolic constants are accepted: <code>GL_LINEAR</code>,
   * <code>GL_EXP</code>, and <code>GL_EXP2</code>. The equations
   * corresponding to these symbolic constants are defined below. The
   * initial fog mode is <code>GL_EXP</code>.
   *
   * <li><code>GL_FOG_DENSITY</code></li>
   *
   * <p><code>params</code> contains a single value that specifies
   * density, the fog density used in both exponential fog
   * equations. Only nonnegative densities are accepted. The initial
   * fog density is 1.
   *
   * <li><code>GL_FOG_START</code></li>
   *
   * <p><code>params</code> contains a single value that specifies start,
   * the near distance used in the linear fog equation. The initial
   * near distance is 0.
   *
   * <li><code>GL_FOG_END</code></li>
   *
   * <p><code>params</code> contains a single value that specifies end,
   * the far distance used in the linear fog equation. The initial far
   * distance is 1.
   *
   * <li><code>GL_FOG_COLOR</code></li>
   *
   * <p><code>params</code> contains four values that specify
   * <i>Cf</i>, the fog color. Both fixed-point and floating-point
   * values are mapped directly. After conversion, all color
   * components are clamped to the range <code>[0, 1]</code>. The
   * initial fog color is <code>(0, 0, 0, 0)</code>.
   *
   * </ul>
   *
   * <p>Fog blends a fog color with each rasterized pixel fragment's
   * posttexturing color using a blending factor f. Factor f is computed in
   * one of three ways, depending on the fog mode. Let z be the distance in
   * eye coordinates from the origin to the fragment being fogged. The
   * equation for <code>GL_LINEAR</code> fog is
   *
   * <pre>
   * f = (end - z)/(end - start)
   * </pre>
   *
   * <p>The equation for <code>GL_EXP</code> fog is
   *
   * <pre>
   * f = e -(density - z)
   * </pre>
   *
   * <p>The equation for <code>GL_EXP2</code> fog is
   *
   * <pre>
   * f = e -(density - z)2
   * </pre>
   *
   * <p>Regardless of the fog mode, <i>f</i> is clamped to the range
   * <code>[0, 1]</code> after it is computed. Then, the fragment's
   * red, green, and blue colors, represented by <i>Cr</i>, are
   * replaced by:
   *
   * <pre>
   * C'r = f Cr + (1 - f) Cf
   * </pre>
   *
   * <p>Fog does not affect a fragment's alpha component.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>pname</code> is not an accepted value, or if
   * <code>pname</code> is <code>GL_FOG_MODE</code> and
   * <code>params</code> is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>pname</code> is <code>GL_FOG_DENSITY</code>, and the first
   * value in <code>params</code> is negative.
   *
   * @param pname Specifies a fog parameter. <code>GL_FOG_MODE</code>,
   * <code>GL_FOG_DENSITY</code>, <code>GL_FOG_START</code>,
   * <code>GL_FOG_END</code>, and <code>GL_FOG_COLOR</code> are
   * accepted.
   * @param params Specifies the value or values to be assigned to
   * <code>pname</code>. <code>GL_FOG_COLOR</code> requires an array
   * of four values. All other parameters accept an array containing
   * only a single value.
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
  void glFogfv(int pname, float[] params, int offset);

  /**
   * Floating-point <code>Buffer</code> version of <code>glFog</code>.
   *
   * @see #glFogfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glFogfv(int pname, FloatBuffer params);

  /**
   * Fixed-point version of <code>glFog</code>.
   *
   * @see #glFogf(int pname, float param)
   */
  void glFogx(int pname, int param);

  /**
   * Fixed-point array version of <code>glFog</code>.
   *
   * @see #glFogfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glFogxv(int pname, int[] params, int offset);

  /**
   * Fixed-point <code>Buffer</code> version of <code>glFog</code>.
   *
   * @see #glFogfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glFogxv(int pname, IntBuffer params);

  /**
   * Define front- and back-facing polygons.
   *
   * <p>In a scene composed entirely of opaque closed surfaces,
   * back-facing polygons are never visible. Eliminating (culling)
   * these invisible polygons has the obvious benefit of speeding up
   * the rendering of the image. To enable and disable culling, call
   * <code>glEnable</code> and <code>glDisable</code> with argument
   * <code>GL_CULL_FACE</code>. Culling is initially disabled.
   *
   * <p>The projection of a polygon to window coordinates is said to
   * have clockwise winding if an imaginary object following the path
   * from its first vertex, its second vertex, and so on, to its last
   * vertex, and finally back to its first vertex, moves in a
   * clockwise direction about the interior of the polygon. The
   * polygon's winding is said to be counterclockwise if the imaginary
   * object following the same path moves in a counterclockwise
   * direction about the interior of the polygon. glFrontFace
   * specifies whether polygons with clockwise winding in window
   * coordinates, or counterclockwise winding in window coordinates,
   * are taken to be front-facing. Passing <code>GL_CCW</code> to
   * <code>mode</code> selects counterclockwise polygons as
   * front-facing. <code>GL_CW</code> selects clockwise polygons as
   * front-facing. By default, counterclockwise polygons are taken to
   * be front-facing.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>mode</code>
   * is not an accepted value.
   *
   * @param mode Specifies the orientation of front-facing
   * polygons. <code>GL_CW</code> and <code>GL_CCW</code> are
   * accepted. The initial value is <code>GL_CCW</code>.
   */
  void glFrontFace(int mode);

  // Need revisit - definition wrong?
  /**
   * Multiply the current matrix by a perspective matrix.
   *
   * <p><code>glFrustum</code> describes a perspective matrix that
   * produces a perspective projection. The current matrix (see
   * <code>glMatrixMode</code>) is multiplied by this matrix and the
   * result replaces the current matrix, as if
   * <code>glMultMatrix</code> were called with the following matrix
   * as its argument:
   *
   * <pre>
   * ( 2/(right - left)        0            A        0 )
   * ( 0                2/(top - bottom)    B        0 )
   * ( 0                       0            C        D )
   * ( 0                       0           -1        0 )
   * </pre>
   *
   * <p>where
   *
   * <pre>
   * A = - (right + left)/(right - left)
   * B = - (top + bottom)/(top - bottom)
   * C = - (far + near)/(far - near)
   * D = - 2farnear/(far - near)
   * </pre>
   *
   * <p>Typically, the matrix mode is <code>GL_PROJECTION</code>, and
   * (<code>left</code>, <code>bottom</code>, -<code>near</code>) and
   * (<code>right</code>, <code>top</code>, -<code>near</code>)
   * specify the points on the near clipping plane that are mapped to
   * the lower left and upper right corners of the window, assuming
   * that the eye is located at (0, 0, 0). -<code>far</code> specifies
   * the location of the far clipping plane. Both <code>near</code>
   * and <code>far</code> must be positive.
   *
   * <p>Use <code>glPushMatrix</code> and <code>glPopMatrix</code> to
   * save and restore the current matrix stack.
   *
   * <h4>Notes</h4>
   *
   * <p>Depth buffer precision is affected by the values specified for
   * near and far. The greater the ratio of <code>far</code> to
   * <code>near</code> is, the less effective the depth buffer will be
   * at distinguishing between surfaces that are near each other. If
   *
   * <pre>
   * r = far/near
   * </pre>
   *
   * roughly log_2(<i>r</i>) bits of depth buffer precision are
   * lost. Because <i>r</i> approaches infinity as <code>near</code>
   * approaches 0, <code>near</code> must never be set to 0.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if
   * <code>near</code> or <code>far</code> is not positive, or if
   * <code>left</code> = <code>right</code>, or <code>bottom</code> =
   * <code>top</code>.
   *
   * @param left Specifies the coordinate for the left vertical
   * clipping plane.
   * @param right Specifies the coordinate for the right vertical
   * clipping plane.
   * @param bottom Specifies the coordinate for the bottom horizontal
   * clipping plane.
   * @param top Specifies the coordinate for the top horizontal
   * clipping plane.
   * @param near Specifies the distances to the near depth clipping
   * plane. The distance must be positive.
   * @param far Specifies the distances to the near depth clipping
   * plane. The distance must be positive.
   */
  void glFrustumf(float left, float right,
		  float bottom, float top,
		  float near, float far);

  /**
   * Fixed-point version of <code>glFrustum</code>.
   *
   * @see #glFrustumf
   */
  void glFrustumx(int left, int right,
                  int bottom, int top,
                  int near, int far);

  /**
   * Generate texture names.
   *
   * <p><code>glGenTextures</code> returns <code>n</code> texture
   * names in <code>textures</code>. There is no guarantee that the
   * names form a contiguous set of integers. However, it is
   * guaranteed that none of the returned names was in use immediately
   * before the call to <code>glGenTextures</code>.
   *
   * <p>The generated textures have no dimensionality; they assume the
   * dimensionality of the texture target to which they are first
   * bound (see glBindTexture).
   *
   * <p>Texture names returned by a call to <code>glGenTextures</code> are not
   * returned by subsequent calls, unless they are first deleted with
   * <code>glDeleteTextures</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if <code>n</code>
   * is negative.
   *
   * @param n Specifies the number of texture names to be generated.
   * @param textures Specifies an array in which the generated texture
   * names are stored.
   * @param offset the starting offset within the
   * <code>textures</code> array.
   *
   * @exception IllegalArgumentException if <code>textures</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>textures.length -
   * offset</code> is less than <code>n</code>.
   */
  void glGenTextures(int n, int[] textures, int offset);

  /**
   * Integer <code>Buffer</code> version of <code>glGenTextures</code>.
   *
   * @see #glGenTextures(int n, int[] textures, int offset)
   *
   * @exception IllegalArgumentException if <code>textures</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>textures.remaining()</code> is less than <code>n</code>.
   */
  void glGenTextures(int n, IntBuffer textures);

  /**
   * Return error information.
   *
   * <p><code>glGetError</code> returns the value of the error
   * flag. Each detectable error is assigned a numeric code and
   * symbolic name. When an error occurs, the error flag is set to the
   * appropriate error code value. No other errors are recorded until
   * <code>glGetError</code> is called, the error code is returned,
   * and the flag is reset to <code>GL_NO_ERROR</code>. If a call to
   * <code>glGetError</code> returns <code>GL_NO_ERROR</code>, there
   * has been no detectable error since the last call to
   * <code>glGetError</code>, or since the GL was initialized.
   *
   * <p>To allow for distributed implementations, there may be several
   * error flags. If any single error flag has recorded an error, the
   * value of that flag is returned and that flag is reset to
   * <code>GL_NO_ERROR</code> when <code>glGetError</code> is
   * called. If more than one flag has recorded an error,
   * <code>glGetError</code> returns and clears an arbitrary error
   * flag value. Thus, <code>glGetError</code> should always be called
   * in a loop, until it returns <code>GL_NO_ERROR</code>, if all
   * error flags are to be reset.
   *
   * <p>Initially, all error flags are set to <code>GL_NO_ERROR</code>.
   *
   * <p>The following errors are currently defined:
   *
   * <ul>
   *
   * <li><code>GL_NO_ERROR</code></li>
   *
   * <p>No error has been recorded. The value of this symbolic constant
   * is guaranteed to be 0.
   *
   * <li><code>GL_INVALID_ENUM</code></li>
   *
   * <p>An unacceptable value is specified for an enumerated
   * argument. The offending command is ignored, and has no other side
   * effect than to set the error flag.
   *
   * <li><code>GL_INVALID_VALUE</code></li>
   *
   * <p>A numeric argument is out of range. The offending command is
   * ignored, and has no other side effect than to set the error flag.
   *
   * <li><code>GL_INVALID_OPERATION</code></li>
   *
   * <p>The specified operation is not allowed in the current state. The
   * offending command is ignored, and has no other side effect than
   * to set the error flag.
   *
   * <li><code>GL_STACK_OVERFLOW</code></li>
   *
   * <p>This command would cause a stack overflow. The offending command
   * is ignored, and has no other side effect than to set the error
   * flag.
   *
   * <li><code>GL_STACK_UNDERFLOW</code></li>
   *
   * <p>This command would cause a stack underflow. The offending command
   * is ignored, and has no other side effect than to set the error
   * flag.
   *
   * <li><code>GL_OUT_OF_MEMORY</code></li>
   *
   * <p>There is not enough memory left to execute the command. The state
   * of the GL is undefined, except for the state of the error flags,
   * after this error is recorded.
   *
   * </ul>
   *
   * <p>When an error flag is set, results of a GL operation are
   * undefined only if <code>GL_OUT_OF_MEMORY</code> has occurred. In
   * all other cases, the command generating the error is ignored and
   * has no effect on the GL state or frame buffer contents. If the
   * generating command returns a value, it returns 0. If
   * <code>glGetError</code> itself generates an error, it returns 0.
   *
   * @return One of the error codes listed above.
   */
  int glGetError();

  /**
   * Return the value or values of a selected parameter.
   *
   * <p><code>glGet</code> returns values for static state
   * variables in GL. <code>pname</code> is a symbolic constant
   * indicating the static state variable to be returned, and
   * <code>params</code> is an array of integers in which to place the
   * returned data.
   *
   * <p> A boolean value is interpreted as either 1 or 0, and a
   * floating-point value is rounded to the nearest integer, unless
   * the value is an RGBA color component, a <code>DepthRange</code>
   * value, a depth buffer clear value, or a normal coordinate. In
   * these cases, the <code>glGet</code> command does a linear mapping
   * that maps 1.0 to the most positive representable integer value,
   * and -1.0 to the most negative representable integer value.
   *
   * <p> In OpenGL ES 1.0, on <code>glGetIntegerv</code> is provided.
   * OpenGL ES 1.1 additionally provides <code>glGetBooleanv</code>,
   * <code>glGetFixedv</code>, and <code>glGetFloatv</code>.
   *
   * <p>The following symbolic constants are accepted by
   * <code>pname</code>:
   *
   * <ul>
   * <li><code>GL_ALIASED_POINT_SIZE_RANGE</code></li>
   *
   * <p><code>params</code> returns two values, the smallest and largest
   * supported sizes for aliased points. The range must include 1. See
   * <code>glPointSize</code>.
   *
   * <li><code>GL_ALIASED_LINE_WIDTH_RANGE</code></li>
   *
   * <p><code>params</code> returns two values, the smallest and largest
   * supported widths for aliased lines. The range must include 1. See
   * <code>glLineWidth</code>.
   *
   * <li><code>GL_ALPHA_BITS</code></li>
   *
   * <p><code>params</code> returns one value, the number of alpha
   * bitplanes in the color buffer.
   *
   * <li><code>GL_ALPHA_TEST_FUNC</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the symbolic name of
   * the alpha test function. See <code>glAlphaFunc</code>.
   *
   * <li><code>GL_ALPHA_TEST_REF</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the reference value for
   * the alpha test. An integer value, if requested, is linearly
   * mapped from the internal floating-point representation such that
   * 1.0 returns the most positive representable integer value, and
   * -1.0 returns the most negative representable integer value. See
   * <code>glAlphaFunc</code>.
   *
   * <li><code>GL_BLEND_DST</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the symbolic constant
   * identifying the destination blend function set by
   * <code>glBlendFunc</code>, or the destination RGB blend function
   * set by <code>glBlendFuncSeparate</code>. See
   * <code>glBlendFunc</code> and <code>glBlendFuncSeparate</code>.
   *
   * <li><code>GL_BLUE_BITS</code></li>
   *
   * <p><code>params</code> returns one value, the number of blue
   * bitplanes in the color buffer.
   *
   * <li><code>GL_COLOR_ARRAY_BUFFER_BINDING</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the color array buffer
   * binding. See <code>glColorPointer</code>.
   *
   * <li><code>GL_COLOR_ARRAY_SIZE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the number of
   * components per color in the color array. See
   * <code>glColorPointer</code>.
   *
   * <li><code>GL_COLOR_ARRAY_STRIDE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the byte offset between
   * consecutive colors in the color array. See
   * <code>glColorPointer</code>.
   *
   * <li><code>GL_COLOR_ARRAY_TYPE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, returns the data type
   * of each component in the color array. See
   * <code>glColorPointer</code>.
   *
   * <li><code>GL_COLOR_CLEAR_VALUE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns four values: the red, green, blue,
   * and alpha values used to clear the color buffers. See
   * <code>glClearColor</code>
   *
   * <li><code>GL_COLOR_WRITEMASK</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns four boolean values: the red,
   * green, blue, and alpha write enables for the color buffers. See
   * <code>glColorMask</code>.
   *
   * <li><code>GL_COMPRESSED_TEXTURE_FORMATS</code></li>
   *
   * <p><code>params</code> returns
   * <code>GL_NUM_COMPRESSED_TEXTURE_FORMATS</code> values, the
   * supported compressed texture formats. See glCompressedTexImage2D
   * and <code>glCompressedTexSubImage2D</code>.
   *
   * <li><code>GL_CULL_FACE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating which polygon faces are to be culled. See
   * <code>glCullFace</code>.
   *
   * <li><code>GL_DEPTH_BITS</code></li>
   *
   * <p><code>params</code> returns one value, the number of bitplanes in
   * the depth buffer.
   *
   * <li><code>GL_DEPTH_CLEAR_VALUE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the value that is used
   * to clear the depth buffer. See <code>glClearDepth</code>.
   *
   * <li><code>GL_DEPTH_FUNC</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the symbolic name of
   * the depth comparision function. See <code>glDepthFunc</code>.
   *
   * <li><code>GL_DEPTH_RANGE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns two values: the near and far
   * mapping limits for the depth buffer. See
   * <code>glDepthRange</code>.
   *
   * <li><code>GL_DEPTH_WRITEMASK</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns a single boolean value indicating
   * if the depth buffer is enabled for writing. See
   * <code>glDepthMask</code>.
   *
   * <li><code>GL_FOG_COLOR</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns four values: the red, green, blue,
   * and alpha components of the fog color. See <code>glFog</code>.
   *
   * <li><code>GL_FOG_DENSITY</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the fog density
   * parameter. See <code>glFog</code>.
   *
   * <li><code>GL_FOG_END</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the end factor for the
   * linear fog equation. See <code>glFog</code>.
   *
   * <li><code>GL_FOG_HINT</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating the mode of the fog hint. See <code>glHint</code>.
   *
   * <li><code>GL_FOG_MODE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating which fog equation is selected. See <code>glFog</code>.
   *
   * <li><code>GL_FOG_START</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the start factor for
   * the linear fog equation. See <code>glFog</code>.
   *
   * <li><code>GL_FRONT_FACE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating whether clockwise or counterclockwise polygon winding
   * is treated as front-facing. See <code>glFrontFace</code>.
   *
   * <li><code>GL_GREEN_BITS</code></li>
   *
   * <p><code>params</code> returns one value, the number of green
   * bitplanes in the color buffer.
   *
   * <li><code>GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES</code>
   * (<code>OES_read_format</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the preferred format for
   * pixel read back. See <code>glReadPixels</code>.
   *
   * <li><code>GL_IMPLEMENTATION_COLOR_READ_TYPE_OES</code> (
   * (<code>OES_read_format</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the preferred type for
   * pixel read back. See <code>glReadPixels</code>.
   *
   * <li><code>GL_LIGHT_MODEL_AMBIENT</code> (1.1 only)</li>
   * 
   * <p><code>params</code> returns four values: the red, green, blue,
   * and alpha components of the ambient intensity of the entire
   * scene. See <code>glLightModel</code>.
   * 
   * <li><code>GL_LIGHT_MODEL_TWO_SIDE</code> (1.1 only)</li>
   * 
   * <p><code>params</code> returns a single boolean value indicating
   * whether separate materials are used to compute lighting for front
   * and back facing polygons. See <code>glLightModel</code>.
   * 
   * <li><code>GL_LINE_SMOOTH_HINT</code> (1.1 only)</li>
   * 
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating the mode of the line antialiasing hint. See
   * <code>glHint</code>.
   * 
   * <li><code>GL_LINE_WIDTH</code> (1.1 only)</li>
   * 
   * <p><code>params</code> returns one value, the line width as
   * specified with <code>glLineWidth</code>.
   * 
   * <li><code>GL_LOGIC_OP_MODE</code> (1.1 only)</li>
   * 
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating the selected logic operation mode. See
   * <code>glLogicOp</code>.
   * 
   * <li><code>GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   * 
   * <p><code>params</code> returns one value, the matrix index array
   * buffer binding. See <code>glMatrixIndexPointer</code>.
   * 
   * <li><code>GL_MATRIX_INDEX_ARRAY_SIZE_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   * 
   * <p><code>params</code> returns one value, the number of matrix
   * indices per vertex. See <code>glMatrixIndexPointer</code>.
   * 
   * <li><code>GL_MATRIX_INDEX_ARRAY_STRIDE_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   * 
   * <p><code>params</code> returns one value, the byte offset between
   * matrix indices. See <code>glMatrixIndexPointer</code>.
   * 
   * <li><code>GL_MATRIX_INDEX_ARRAY_TYPE_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   * 
   * <p><code>params</code> returns one value, the data type of each
   * matrix index in the matrix indices array. See
   * <code>glMatrixIndexPointer</code>.
   * 
   * <li><code>GL_MATRIX_MODE</code> (1.1 only)</li>
   * 
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating which matrix stack is currently the target of all
   * matrix operations. See <code>glMatrixMode</code>.
   * 
   * <li><code>GL_MAX_CLIP_PLANES</code> (1.1 only)</li>
   * 
   * <p><code>params</code> returns one value, the maximum number of
   * application defined clipping planes. The value must be at least
   * 6. See <code>glClipPlane</code>.
   * 
   * <li><code>GL_MAX_ELEMENTS_INDICES</code></li>
   *
   * <p><code>params</code> returns one value, the recommended maximum
   * number of vertex array indices. See <code>glDrawElements</code>.
   *
   * <li><code>GL_MAX_ELEMENTS_VERTICES</code></li>
   *
   * <p><code>params</code> returns one value, the recommended maximum
   * number of vertex array vertices. See <code>glDrawArrays</code>
   * and <code>glDrawElements</code>.
   *
   * <li><code>GL_MAX_LIGHTS</code></li>
   *
   * <p><code>params</code> returns one value, the maximum number of
   * lights. The value must be at least 8. See <code>glLight</code>.
   *
   * <li><code>GL_MAX_MODELVIEW_STACK_DEPTH</code></li>
   *
   * <p><code>params</code> returns one value, the maximum supported
   * depth of the modelview matrix stack. The value must be at least
   * 16. See <code>glPushMatrix</code>.
   *
   * <li><code>GL_MAX_PALETTE_MATRICES_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   *
   * <p><code>params</code> returns the size of the matrix
   * palette. The initial value is 9.
   *
   * <li><code>GL_MAX_PROJECTION_STACK_DEPTH</code></li>
   *
   * <p><code>params</code> returns one value, the maximum supported
   * depth of the projection matrix stack. The value must be at least
   * 2. See <code>glPushMatrix</code>.
   *
   * <li><code>GL_MAX_TEXTURE_SIZE</code></li>
   *
   * <p><code>params</code> returns one value. The value gives a rough
   * estimate of the largest texture that the GL can handle. The value
   * must be at least 64. See <code>glTexImage2D</code>,
   * <code>glCompressedTexImage2D</code>, and
   * <code>glCopyTexImage2D</code>.
   *
   * <li><code>GL_MAX_TEXTURE_STACK_DEPTH</code></li>
   *
   * <p><code>params</code> returns one value, the maximum supported
   * depth of the texture matrix stack. The value must be at least
   * 2. See <code>glPushMatrix</code>.
   *
   * <li><code>GL_MAX_TEXTURE_UNITS</code></li>
   *
   * <p><code>params</code> returns a single value indicating the number
   * of texture units supported. The value must be at least 1. See
   * <code>glActiveTexture</code>, <code>glClientActiveTexture</code>
   * and <code>glMultiTexCoord</code>.
   *
   * <li><code>GL_MAX_VERTEX_UNITS_OES</code>
   *
   * <p><code>params</code> returns the number of matrices per
   * vertex. The initial value is 3.
   *
   * <li><code>GL_MAX_VIEWPORT_DIMS</code></li>
   *
   * <p><code>params</code> returns two values: the maximum supported
   * width and height of the viewport. These must be at least as large
   * as the visible dimensions of the display being rendered to. See
   * <code>glViewport</code>.
   *
   * <li><code>GL_MODELVIEW_MATRIX</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns sixteen values: the modelview
   * matrix on the top of the modelview matrix stack. See
   * <code>glPushMatrix</code>.
   *
   * <li><code>GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES</code>
   * (<code>OES_matrix_get</code> extension)</li>
   *
   * <p><code>params</code> returns a representation of the floating
   * point Model View matrix elements as as an array of integers,
   * according to the IEEE 754 floating point "single format" bit
   * layout. See <code>glMatrixMode</code>.
   *
   * <li><code>GL_MODELVIEW_STACK_DEPTH</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the number of matrices
   * on the modelview matrix stack. See <code>glPushMatrix</code>.
   *
   * <li><code>GL_NORMAL_ARRAY_BUFFER_BINDING</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the normal array buffer
   * binding. See <code>glNormalPointer</code>.
   *
   * <li><code>GL_NORMAL_ARRAY_STRIDE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the byte offset between
   * consective normals in the normal array. See
   * <code>glNormalPointer</code>.
   *
   * <li><code>GL_NORMAL_ARRAY_TYPE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the data type of each
   * normal in the normal array. See <code>glNormalPointer</code>.
   *
   * <li><code>GL_NUM_COMPRESSED_TEXTURE_FORMATS</code></li>
   *
   * <p><code>params</code> returns one value, the number of supported
   * compressed texture formats. The value must be at least 10. See
   * <code>glCompressedTexImage2D</code> and
   * <code>glCompressedTexSubImage2D</code>.
   *
   * <li><code>GL_PACK_ALIGNMENT</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the byte alignment used
   * for writing pixel data to memory. See <code>glPixelStore</code>.
   *
   * <li><code>GL_PERSPECTIVE_CORRECTION_HINT</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating the mode of the perspective correction hint. See
   * <code>glHint</code>.
   *
   * <li><code>GL_POINT_SIZE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the point size as
   * specified by <code>glPointSize</code>.
   *
   * <li><code>GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES</code>
   * (<code>OES_point_size_array</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the point size array
   * buffer binding. See <code>glPointSizePointer</code>.
   *
   * <li><code>GL_POINT_SIZE_ARRAY_STRIDE_OES</code>
   * (<code>OES_point_size_array</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the byte offset between
   * consecutive point sizes in the point size array. See
   * <code>glPointSizePointer</code>.
   *
   * <li><code>GL_POINT_SIZE_ARRAY_TYPE_OES</code>
   * (<code>OES_point_size_array</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the data type of each
   * point size in the point array. See
   * <code>glPointSizePointer</code>.
   *
   * <li><code>GL_POINT_SMOOTH_HINT</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating the mode of the point antialiasing hint. See
   * <code>glHint</code>.
   *
   * <li><code>GL_POLYGON_OFFSET_FACTOR</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the scaling factor used
   * to determine the variable offset that is added to the depth value
   * of each fragment generated when a polygon is rasterized. See
   * <code>glPolygonOffset</code>.
   *
   * <li><code>GL_POLYGON_OFFSET_UNITS</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value. This value is
   * multiplied by an implementation-specific value and then added to
   * the depth value of each fragment generated when a polygon is
   * rasterized. See <code>glPolygonOffset</code>.
   *
   * <li><code>GL_PROJECTION_MATRIX</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns sixteen values: the projection
   * matrix on the top of the projection matrix stack. See
   * <code>glPushMatrix</code>.
   *
   * <li><code>GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES</code>
   * (<code>OES_matrix_get</code> extension)</li>
   *
   * <p><code>params</code> returns a representation of the floating
   * point Projection matrix elements as as an array of integers,
   * according to the IEEE 754 floating point "single format" bit
   * layout. See <code>glMatrixMode</code>.
   *
   * <li><code>GL_PROJECTION_STACK_DEPTH</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the number of matrices
   * on the projection matrix stack. See <code>glPushMatrix</code>.
   *
   * <li><code>GL_RED_BITS</code></li>
   *
   * <p><code>params</code> returns one value, the number of red
   * bitplanes in each color buffer.
   *
   * <li><code>GL_SCISSOR_BOX</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns four values: the x and y window
   * coordinates of the scissor box, followed by its width and
   * height. See <code>glScissor</code>.
   *
   * <li><code>GL_SHADE_MODEL</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating whether the shading mode is flat or smooth. See
   * <code>glShadeModel</code>.
   *
   * <li><code>GL_SMOOTH_LINE_WIDTH_RANGE</code></li>
   *
   * <p><code>params</code> returns two values, the smallest and largest
   * supported widths for antialiased lines. The range must include
   * 1. See <code>glLineWidth</code>.
   *
   * <li><code>GL_SMOOTH_POINT_SIZE_RANGE</code></li>
   *
   * <p><code>params</code> returns two values, the smallest and largest
   * supported widths for antialiased points. The range must include
   * 1. See <code>glPointSize</code>.
   *
   * <li><code>GL_STENCIL_BITS</code></li>
   *
   * <p><code>params</code> returns one value, the number of bitplanes in
   * the stencil buffer.
   *
   * <li><code>GL_STENCIL_CLEAR_VALUE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the index to which the
   * stencil bitplanes are cleared. See <code>glClearStencil</code>.
   *
   * <li><code>GL_STENCIL_FAIL</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating what action is taken when the stencil test fails. See
   * <code>glStencilOp</code>.
   *
   * <li><code>GL_STENCIL_FUNC</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating what function is used to compare the stencil reference
   * value with the stencil buffer value. See
   * <code>glStencilFunc</code>.
   *
   * <li><code>GL_STENCIL_PASS_DEPTH_FAIL</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating what action is taken when the stencil test passes, but
   * the depth test fails. See <code>glStencilOp</code>.
   *
   * <li><code>GL_STENCIL_PASS_DEPTH_PASS</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, a symbolic constant
   * indicating what action is taken when the stencil test passes, and
   * the depth test passes. See <code>glStencilOp</code>.
   *
   * <li><code>GL_STENCIL_REF</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the reference value
   * that is compared with the contents of the stencil buffer. See
   * <code>glStencilFunc</code>.
   *
   * <li><code>GL_STENCIL_VALUE_MASK</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the mask that is used
   * to mask both the stencil reference value and the stencil buffer
   * value before they are compared. See <code>glStencilFunc</code>.
   *
   * <li><code>GL_STENCIL_WRITEMASK</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the mask that controls
   * writing of the stencil bitplanes. See <code>glStencilMask</code>.
   *
   * <li><code>GL_SUBPIXEL_BITS</code></li>
   *
   * <p><code>params</code> returns one value, an estimate of the number
   * of bits of subpixel resolution that are used to position
   * rasterized geometry in window coordinates. The value must be at
   * least 4.
   *
   * <li><code>GL_TEXTURE_BINDING_2D</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the name of the texture
   * currently bound to the target <code>GL_TEXTURE_2D</code>. See
   * <code>glBindTexture</code>.
   *
   * <li><code>GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the texture coordinate
   * array buffer binding. See <code>glTexCoordPointer</code>.
   *
   * <li><code>GL_TEXTURE_COORD_ARRAY_SIZE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the number of
   * coordinates per element in the texture coordinate array. See
   * <code>glTexCoordPointer</code>.
   *
   * <li><code>GL_TEXTURE_COORD_ARRAY_STRIDE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the byte offset between
   * consecutive elements in the texture coordinate array. See
   * <code>glTexCoordPointer</code>.
   *
   * <li><code>GL_TEXTURE_COORD_ARRAY_TYPE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, returns the data type
   * of each coordinate in the texture coordinate array. See
   * <code>glTexCoordPointer</code>.
   *
   * <li><code>GL_TEXTURE_MATRIX</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns sixteen values: the texture matrix
   * on the top of the texture matrix stack. See
   * <code>glPushMatrix</code>.
   *
   * <li><code>GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES</code>
   * (<code>OES_matrix_get</code> extension)</li>
   *
   * <p><code>params</code> returns a representation of the floating
   * point Texture matrix elements as as an array of integers,
   * according to the IEEE 754 floating point "single format" bit
   * layout. See <code>glMatrixMode</code>.
   *
   * <li><code>GL_TEXTURE_STACK_DEPTH</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the number of matrices
   * on the texture matrix stack. See <code>glBindTexture</code>.
   *
   * <li><code>GL_UNPACK_ALIGNMENT</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the byte alignment used
   * for reading pixel data from memory. See
   * <code>glPixelStore</code>.
   *
   * <li><code>GL_VIEWPORT</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns four values:, the x and y window
   * coordinates of the viewport, followed by its width and
   * height. See <code>glViewport</code>.
   *
   * <li><code>GL_VERTEX_ARRAY_BUFFER_BINDING</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the vertex array buffer
   * binding. See <code>glVertexPointer</code>.
   *
   * <li><code>GL_VERTEX_ARRAY_SIZE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, number of coordinates
   * per vertex in the vertex array. See <code>glVertexPointer</code>.
   *
   * <li><code>GL_VERTEX_ARRAY_STRIDE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, the byte offset between
   * consecutive vertexes in the vertex array. See
   * <code>glVertexPointer</code>.
   *
   * <li><code>GL_VERTEX_ARRAY_TYPE</code> (1.1 only)</li>
   *
   * <p><code>params</code> returns one value, returns the data type
   * of each coordinate in the vertex array. See
   * <code>glVertexPointer</code>.
   *
   * <li><code>GL_WEIGHT_ARRAY_BUFFER_BINDING_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the weight array buffer
   * binding. See <code>glWeightPointer</code>.
   *
   * <li><code>GL_WEIGHT_ARRAY_SIZE_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the number of weights
   * per vertex. See <code>glWeightPointer</code>.
   *
   * <li><code>GL_WEIGHT_ARRAY_STRIDE_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the byte offset between
   * weights per vertex. See <code>glWeightPointer</code>.
   *
   * <li><code>GL_WEIGHT_ARRAY_TYPE_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   *
   * <p><code>params</code> returns one value, the data type of each
   * weight in the weight array. See <code>glWeightPointer</code>.
   *
   * </ul>
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>pname</code>
   * is not an accepted value.
   *
   * @param pname Specifies the parameter value to be returned. The
   * symbolic constants in the list above are accepted.
   * @param params Returns the value or values of the specified
   * parameter.
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
  void glGetIntegerv(int pname, int[] params, int offset);

  /**
   * Integer <code>Buffer</code> version of <code>glGetIntegerv</code>.
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
   * Return a string describing the underlying GL implementation. The
   * GL string is converted to UTF8 format to produce a standard Java
   * <code>String</code> object.
   *
   * <p><code>glGetString</code> returns a <code>String</code>
   * describing some aspect of the current GL
   * implementation. <code>name</code> can be one of the following:
   *
   * <ul>
   *
   * <li><code>GL_VENDOR</code></li>
   *
   * <p>Returns the company responsible for this GL implementation. This
   * name does not change from release to release.
   *
   * <li><code>GL_RENDERER</code></li>
   *
   * <p>Returns the name of the renderer. This name is typically specific
   * to a particular configuration of a hardware platform. It does not
   * change from release to release.
   *
   * <li><code>GL_VERSION</code></li>
   *
   * <p>Returns the particular OpenGL ES profile as well as the
   * version of that profile.
   *
   * <li><code>GL_EXTENSIONS</code></li>
   *
   * <p>Returns a space-separated list of supported extensions to GL.
   *
   * </ul>
   *
   * <p>Because the GL does not include queries for the performance
   * characteristics of an implementation, some applications are
   * written to recognize known platforms and modify their GL usage
   * based on known performance characteristics of these
   * platforms. Strings <code>GL_VENDOR</code> and
   * <code>GL_RENDERER</code> together uniquely specify a
   * platform. They do not change from release to release and should
   * be used by platform-recognition algorithms.
   *
   * <p>Some applications want to make use of features that are not part
   * of the standard GL. These features may be implemented as
   * extensions to the standard GL. The <code>GL_EXTENSIONS</code>
   * string is a space-separated list of supported GL
   * extensions. (Extension names never contain a space character.)
   *
   * <p>The <code>GL_VERSION</code> string begins with a version
   * number. The version number uses one of these forms:
   *
   * <p><i>major_number.minor_number</i> (1.0 only)
   * <p><i>major_number.minor_number.release_number</i> (1.0 only)
   * <p><i>OpenGL ES-CM</i> followed by
   * <i>major_number.minor_number</i> for the common profile (1.1 only).
   * <p><i>OpenGL ES-CL</i> followed by <i>major_number.minor_number</i> for
   * the common-lite profile (1.1 only).
   *
   * <p>On 1.0 implementations, vendor-specific information may
   * follow the version number. A space always separates the version
   * number and the vendor-specific information.
   *
   * <h4>Notes</h4>
   *
   * <p>If an error is generated, <code>glGetString</code> returns NULL.
   *
   * <p>The client and server may support different versions or
   * extensions. <code>glGetString</code> always returns a compatible
   * version number or list of extensions. The release number always
   * describes the server.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if <code>name</code>
   * is not an accepted value.
   *
   * @param name Specifies a symbolic constant, one of
   * <code>GL_VENDOR</code>, <code>GL_RENDERER</code>,
   * <code>GL_VERSION</code>, or <code>GL_EXTENSIONS</code>.
   *
   * @return A <code>String</code> formatted as described above.
   */
  String glGetString(int name);

  /**
   * Specify implementation-specific hints.
   *
   * <p>Certain aspects of GL behavior, when there is room for
   * interpretation, can be controlled with hints. A hint is specified
   * with two arguments. <code>target</code> is a symbolic constant
   * indicating the behavior to be controlled, and <code>mode</code>
   * is another symbolic constant indicating the desired behavior. The
   * initial value for each <code>target</code> is
   * <code>GL_DONT_CARE</code>. <code>mode</code> can be one of the
   * following:
   *
   * <ul>
   *
   * <li><code>GL_FASTEST</code></li>
   *
   * <p>The most efficient option should be chosen.
   *
   * <li><code>GL_NICEST</code></li>
   *
   * <p>The most correct, or highest quality, option should be chosen.
   *
   * <li><code>GL_DONT_CARE</code></li>
   *
   * <p>No preference.
   *
   * </ul>
   *
   * <p>Though the implementation aspects that can be hinted are well
   * defined, the interpretation of the hints depends on the
   * implementation. The hint aspects that can be specified with
   * <code>target</code>, along with suggested semantics, are as follows:
   *
   * <ul>
   *
   * <li><code>GL_FOG_HINT</code></li>
   *
   * <p>Indicates the accuracy of fog calculation. If per-pixel fog
   * calculation is not efficiently supported by the GL
   * implementation, hinting <code>GL_DONT_CARE</code> or
   * <code>GL_FASTEST</code> can result in per-vertex calculation of
   * fog effects.
   *
   * <li><code>GL_LINE_SMOOTH_HINT</code></li>
   *
   * <p>Indicates the sampling quality of antialiased lines. If a larger
   * filter function is applied, hinting <code>GL_NICEST</code> can
   * result in more pixel fragments being generated during
   * rasterization,
   *
   * <li><code>GL_PERSPECTIVE_CORRECTION_HINT</code></li>
   *
   * <p>Indicates the quality of color and texture coordinate
   * interpolation. If perspective-corrected parameter interpolation
   * is not efficiently supported by the GL implementation, hinting
   * <code>GL_DONT_CARE</code> or <code>GL_FASTEST</code> can result
   * in simple linear interpolation of colors and/or texture
   * coordinates.
   *
   * <li><code>GL_POINT_SMOOTH_HINT</code></li>
   *
   * <p>Indicates the sampling quality of antialiased points. If a larger
   * filter function is applied, hinting <code>GL_NICEST</code> can
   * result in more pixel fragments being generated during
   * rasterization.
   *
   * <li><code>GL_GENERATE_MIPMAP_HINT</code> (1.1 only)</li>
   *
   * <p>Indicates the desired quality and performance of automatic
   * mipmap level generation.
   *
   * </ul>
   * 
   * <h4>Notes</h4>
   *
   * <p>The interpretation of hints depends on the implementation. Some
   * implementations ignore <code>glHint</code> settings.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if either
   * <code>target</code> or <code>mode</code> is not an accepted
   * value.
   *
   * @param target Specifies a symbolic constant indicating the
   * behavior to be controlled. <code>GL_FOG_HINT</code> ,
   * <code>GL_LINE_SMOOTH_HINT</code> ,
   * <code>GL_PERSPECTIVE_CORRECTION_HINT</code>, and
   * <code>GL_POINT_SMOOTH_HINT</code> are accepted. On 1.1,
   * <code>GL_GENERATE_MIPMAP_HINT</code> is additionally accepted.
   * @param mode Specifies a symbolic constant indicating the desired
   * behavior. <code>GL_FASTEST</code>, <code>GL_NICEST</code>, and
   * <code>GL_DONT_CARE</code> are accepted.
   */
  void glHint(int target, int mode);

  /**
   * Set the lighting model parameters.
   *
   * <p><code>glLightModel</code> sets the lighting model
   * parameter. <code>pname</code> names a parameter and param gives
   * the new value. There is one single-valued lighting model parameter:
   *
   * <ul>
   *
   * <li><code>GL_LIGHT_MODEL_TWO_SIDE</code></li>
   *
   * <p><code>param</code> specifies whether one- or two-sided lighting
   * calculations are done for polygons. It has no effect on the
   * lighting calculations for points, lines, or bitmaps. If
   * <code>param</code> is 0, one-sided lighting is specified, and
   * only the front material parameters are used in the lighting
   * equation. Otherwise, two-sided lighting is specified. In this
   * case, vertices of back-facing polygons are lighted using the back
   * material parameters, and have their normals reversed before the
   * lighting equation is evaluated. Vertices of front-facing polygons
   * are always lighted using the front material parameters, with no
   * change to their normals. The initial value is 0.
   *
   * </ul>
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>pname</code> is not an accepted value.
   *
   * @param pname Specifies a single-valued lighting model
   * parameter. Must be <code>GL_LIGHT_MODEL_TWO_SIDE</code>.
   * @param param Specifies the value that <code>param</code> will be
   * set to.
   */
  void glLightModelf(int pname, float param);

  /**
   * Set the lighting model parameters (array version).
   *
   * <p><code>glLightModel</code> sets the lighting model
   * parameter. <code>pname</code> names a parameter and
   * <code>params</code> gives the new value. There are two lighting
   * model parameters:
   *
   * <ul>
   *
   * <li><code>GL_LIGHT_MODEL_AMBIENT</code></li>
   *
   * <p><code>params</code> contains four values that specify the ambient
   * intensity of the entire scene. The values are not clamped. The
   * initial value is (0.2, 0.2, 0.2, 1.0).
   *
   * <li><code>GL_LIGHT_MODEL_TWO_SIDE</code></li>
   *
   * <p><code>params</code> contains a single value that specifies
   * whether one- or two-sided lighting calculations are done for
   * polygons. It has no effect on the lighting calculations for
   * points, lines, or bitmaps. If <code>params</code> contains 0,
   * one-sided lighting is specified, and only the front material
   * parameters are used in the lighting equation. Otherwise,
   * two-sided lighting is specified. In this case, vertices of
   * back-facing polygons are lighted using the back material
   * parameters, and have their normals reversed before the lighting
   * equation is evaluated. Vertices of front-facing polygons are
   * always lighted using the front material parameters, with no
   * change to their normals. The initial value is 0.
   *
   * </ul>
   *
   * <p>The lighted color of a vertex is the sum of the material
   * emission intensity, the product of the material ambient
   * reflectance and the lighting model full-scene ambient intensity,
   * and the contribution of each enabled light source. Each light
   * source contributes the sum of three terms: ambient, diffuse, and
   * specular. The ambient light source contribution is the product of
   * the material ambient reflectance and the light's ambient
   * intensity. The diffuse light source contribution is the product
   * of the material diffuse reflectance, the light's diffuse
   * intensity, and the dot product of the vertex's normal with the
   * normalized vector from the vertex to the light source. The
   * specular light source contribution is the product of the material
   * specular reflectance, the light's specular intensity, and the dot
   * product of the normalized vertex-to-eye and vertex-to-light
   * vectors, raised to the power of the shininess of the
   * material. All three light source contributions are attenuated
   * equally based on the distance from the vertex to the light source
   * and on light source direction, spread exponent, and spread cutoff
   * angle. All dot products are replaced with 0 if they evaluate to a
   * negative value.
   *
   * <p>The alpha component of the resulting lighted color is set to the
   * alpha value of the material diffuse reflectance.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>pname</code> is not an accepted value.
   *
   * @param pname Specifies a lighting model
   * parameter. <code>GL_LIGHT_MODEL_AMBIENT</code> and
   * <code>GL_LIGHT_MODEL_TWO_SIDE</code> are accepted.
   * @param params Specifies an array containing values that parameter
   * <code>pname</code> will be set to.
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
  void glLightModelfv(int pname, float[] params, int offset);

  /**
   * Floating-point <code>Buffer</code> version of <code>glLightModel</code>.
   *
   * @see #glLightModelfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glLightModelfv(int pname, FloatBuffer params);

  /** 
   * Fixed-point version of <code>glLightModel</code>.
   *
   * @see #glLightModelf(int pname, float param)
   */
  void glLightModelx(int pname, int param);

  /**
   * Fixed-point array version of <code>glLightModel</code>.
   *
   * @see #glLightModelfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glLightModelxv(int pname, int[] params, int offset);

  /**
   * Fixed-point <code>Buffer</code> version of <code>glLightModel</code>. 
   *
   * @see #glLightModelfv(int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glLightModelxv(int pname, IntBuffer params);

  /**
   * Set light source parameters.
   *
   * <p><code>glLight</code> sets the values of individual light
   * source parameters. <code>light</code> names the light and is a
   * symbolic name of the form <code>GL_LIGHT</code><i>i</i>, where
   * <code>0 <= <i>i</i> < GL_MAX_LIGHTS</code>. <code>pname</code>
   * specifies one of ten light source parameters, again by symbolic
   * name. <code>param</code> contains the new value.
   *
   * <p>To enable and disable lighting calculation, call
   * <code>glEnable</code> and <code>glDisable</code> with argument
   * <code>GL_LIGHTING</code>. Lighting is initially disabled. When it
   * is enabled, light sources that are enabled contribute to the
   * lighting calculation. Light source <i>i</i> is enabled and disabled
   * using <code>glEnable</code> and <code>glDisable</code> with
   * argument <code>GL_LIGHT</code><i>i</i>.
   *
   * <p>The light parameters are as follows:
   *
   * <ul>
   *
   * <li><code>GL_SPOT_EXPONENT</code></li>
   *
   * <p><code>param</code> is a single value that specifies the
   * intensity distribution of the light. Fixed-point and
   * floating-point values are mapped directly. Only values in the
   * range [0, 128] are accepted.
   *
   * <p>Effective light intensity is attenuated by the cosine of the
   * angle between the direction of the light and the direction from
   * the light to the vertex being lighted, raised to the power of the
   * spot exponent. Thus, higher spot exponents result in a more
   * focused light source, regardless of the spot cutoff angle (see
   * <code>GL_SPOT_CUTOFF</code>, next paragraph). The initial spot
   * exponent is 0, resulting in uniform light distribution.
   *
   * <li><code>GL_SPOT_CUTOFF</code></li>
   *
   * <p><code>param</code> is a single value that specifies the maximum
   * spread angle of a light source. Fixed-point and floating-point
   * values are mapped directly. Only values in the range [0, 90] and
   * the special value 180 are accepted. If the angle between the
   * direction of the light and the direction from the light to the
   * vertex being lighted is greater than the spot cutoff angle, the
   * light is completely masked. Otherwise, its intensity is
   * controlled by the spot exponent and the attenuation factors. The
   * initial spot cutoff is 180, resulting in uniform light
   * distribution.
   *
   * <li><code>GL_CONSTANT_ATTENUATION</code>,
   * <code>GL_LINEAR_ATTENUATION</code>,
   * <code>GL_QUADRATIC_ATTENUATION</code></li>
   *
   * <p><code>param</code> is a single value that specifies one of the
   * three light attenuation factors. Fixed-point and floating-point
   * values are mapped directly. Only nonnegative values are
   * accepted. If the light is positional, rather than directional,
   * its intensity is attenuated by the reciprocal of the sum of the
   * constant factor, the linear factor times the distance between the
   * light and the vertex being lighted, and the quadratic factor
   * times the square of the same distance. The initial attenuation
   * factors are (1, 0, 0), resulting in no attenuation.
   *
   * </ul>
   * 
   * <h4>Notes</h4>
   *
   * <p>It is always the case that <code>GL_LIGHT</code><i>i</i> =
   * <code>GL_LIGHT0</code> + <i>i</i>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if either
   * <code>light</code> or <code>pname</code> is not an accepted
   * value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if a spot exponent
   * value is specified outside the range [0, 128], or if spot cutoff
   * is specified outside the range [0, 90] (except for the special
   * value 180), or if a negative attenuation factor is specified.
   *
   * @param light Specifies a light. The number of lights depends on
   * the implementation, but at least eight lights are supported. They
   * are identified by symbolic names of the form
   * <code>GL_LIGHT</code><i>i</i> where <code>0 <= <i>i</i> <
   * GL_MAX_LIGHTS</code>.
   * @param pname Specifies a single-valued light source parameter for
   * light. <code>GL_SPOT_EXPONENT</code>,
   * <code>GL_SPOT_CUTOFF</code>,
   * <code>GL_CONSTANT_ATTENUATION</code>,
   * <code>GL_LINEAR_ATTENUATION</code>, and
   * <code>GL_QUADRATIC_ATTENUATION</code> are accepted.
   * @param param Specifies the value that parameter
   * <code>pname</code> of light source light will be set to.
   */
  void glLightf(int light, int pname, float param);

  /**
   * Set light source parameters (array version).
   *
   * <p><code>glLight</code> sets the values of individual light
   * source parameters. <code>light</code> names the light and is a
   * symbolic name of the form <code>GL_LIGHT</code><i>i</i>, where
   * <code>0 <= <i>i</i> < GL_MAX_LIGHTS</code>. <code>pname</code>
   * specifies one of ten light source parameters, again by symbolic
   * name. <code>params</code> is an array that contains the new
   * values.
   *
   * <p>To enable and disable lighting calculation, call
   * <code>glEnable</code> and <code>glDisable</code> with argument
   * <code>GL_LIGHTING</code>. Lighting is initially disabled. When it
   * is enabled, light sources that are enabled contribute to the
   * lighting calculation. Light source <i>i</i> is enabled and disabled
   * using <code>glEnable</code> and <code>glDisable</code> with
   * argument <code>GL_LIGHT</code><i>i</i>.
   *
   * <p>The light parameters are as follows:
   *
   * <ul>
   *
   * <li><code>GL_AMBIENT</code></li>
   *
   * <p><code>params</code> contains four values that specify the ambient
   * RGBA intensity of the light. Both fixed-point and floating-point
   * values are mapped directly. Neither fixed-point nor
   * floating-point values are clamped. The initial ambient light
   * intensity is (0, 0, 0, 1).
   *
   * <li><code>GL_DIFFUSE</code></li>
   *
   * <p><code>params</code> contains four values that specify the diffuse
   * RGBA intensity of the light. Both fixed-point and floating-point
   * values are mapped directly. Neither fixed-point nor
   * floating-point values are clamped. The initial value for
   * <code>GL_LIGHT0</code> is (1, 1, 1, 1). For other lights, the
   * initial value is (0, 0, 0, 0).
   *
   * <li><code>GL_SPECULAR</code></li>
   *
   * <p><code>params</code> contains four values that specify the
   * specular RGBA intensity of the light. Both fixed-point and
   * floating-point values are mapped directly. Neither fixed-point
   * nor floating-point values are clamped. The initial value for
   * <code>GL_LIGHT0</code> is (1, 1, 1, 1). For other lights, the
   * initial value is (0, 0, 0, 0).
   *
   * <li><code>GL_POSITION</code> (1.0 only)</li>
   *
   * <p><code>params</code> contains four values that specify the
   * position of the light in homogeneous object coordinates. Both
   * fixed-point and floating-point values are mapped
   * directly. Neither fixed-point nor floating-point values are
   * clamped.
   *
   * <p>The position is transformed by the modelview matrix when
   * <code>glLight</code> is called (just as if it were a point), and
   * it is stored in eye coordinates. If the w component of the
   * position is 0, the light is treated as a directional
   * source. Diffuse and specular lighting calculations take the
   * light's direction, but not its actual position, into account, and
   * attenuation is disabled. Otherwise, diffuse and specular lighting
   * calculations are based on the actual location of the light in eye
   * coordinates, and attenuation is enabled. The initial position is
   * (0, 0, 1, 0). Thus, the initial light source is directional,
   * parallel to, and in the direction of the -z axis.
   *
   * <li><code>GL_SPOT_DIRECTION</code></li>
   *
   * <p><code>params</code> contains three values that specify the
   * direction of the light in homogeneous object coordinates. Both
   * fixed-point and floating-point values are mapped
   * directly. Neither fixed-point nor floating-point values are
   * clamped.
   *
   * <p>The spot direction is transformed by the inverse of the modelview
   * matrix when <code>glLight</code> is called (just as if it were a
   * normal), and it is stored in eye coordinates. It is significant
   * only when <code>GL_SPOT_CUTOFF</code> is not 180, which it is
   * initially. The initial direction is (0, 0, -1).
   *
   * <li><code>GL_SPOT_EXPONENT</code></li>
   *
   * <p><code>params</code> is a single value that specifies the
   * intensity distribution of the light. Fixed-point and
   * floating-point values are mapped directly. Only values in the
   * range [0, 128] are accepted.
   *
   * <p>Effective light intensity is attenuated by the cosine of the
   * angle between the direction of the light and the direction from
   * the light to the vertex being lighted, raised to the power of the
   * spot exponent. Thus, higher spot exponents result in a more
   * focused light source, regardless of the spot cutoff angle (see
   * <code>GL_SPOT_CUTOFF</code>, next paragraph). The initial spot
   * exponent is 0, resulting in uniform light distribution.
   *
   * <li><code>GL_SPOT_CUTOFF</code></li>
   *
   * <p><code>params</code> is a single value that specifies the maximum
   * spread angle of a light source. Fixed-point and floating-point
   * values are mapped directly. Only values in the range [0, 90] and
   * the special value 180 are accepted. If the angle between the
   * direction of the light and the direction from the light to the
   * vertex being lighted is greater than the spot cutoff angle, the
   * light is completely masked. Otherwise, its intensity is
   * controlled by the spot exponent and the attenuation factors. The
   * initial spot cutoff is 180, resulting in uniform light
   * distribution.
   *
   * <li><code>GL_CONSTANT_ATTENUATION</code>,
   * <code>GL_LINEAR_ATTENUATION</code>,
   * <code>GL_QUADRATIC_ATTENUATION</code></li>
   *
   * <p><code>params</code> is a single value that specifies one of the
   * three light attenuation factors. Fixed-point and floating-point
   * values are mapped directly. Only nonnegative values are
   * accepted. If the light is positional, rather than directional,
   * its intensity is attenuated by the reciprocal of the sum of the
   * constant factor, the linear factor times the distance between the
   * light and the vertex being lighted, and the quadratic factor
   * times the square of the same distance. The initial attenuation
   * factors are (1, 0, 0), resulting in no attenuation.
   *
   * </ul>
   * 
   * <h4>Notes</h4>
   *
   * <p>It is always the case that <code>GL_LIGHT</code><i>i</i> =
   * <code>GL_LIGHT0</code> + <i>i</i>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if either
   * <code>light</code> or <code>pname</code> is not an accepted
   * value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if a spot exponent
   * value is specified outside the range [0, 128], or if spot cutoff
   * is specified outside the range [0, 90] (except for the special
   * value 180), or if a negative attenuation factor is specified.
   *
   * @param light Specifies a light. The number of lights depends on
   * the implementation, but at least eight lights are supported. They
   * are identified by symbolic names of the form
   * <code>GL_LIGHT</code><i>i</i> where <code>0 <= <i>i</i> <
   * GL_MAX_LIGHTS</code>.
   * @param pname Specifies a light source parameter for
   * light. <code>GL_AMBIENT</code>, <code>GL_DIFFUSE</code>,
   * <code>GL_SPECULAR</code>, <code>GL_POSITION</code> (1.0 only),
   * <code>GL_SPOT_CUTOFF</code>, <code>GL_SPOT_DIRECTION</code>,
   * <code>GL_SPOT_EXPONENT</code>,
   * <code>GL_CONSTANT_ATTENUATION</code>,
   * <code>GL_LINEAR_ATTENUATION</code>, and
   * <code>GL_QUADRATIC_ATTENUATION</code> are accepted.
   * @param params Specifies an array containing values that parameter
   * <code>pname</code> of light source light will be set to.
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
  void glLightfv(int light, int pname, float[] params, int offset);

  /**
   * Floating-point <code>Buffer</code> version of <code>glLight</code>.
   *
   * @see #glLightfv(int light, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glLightfv(int light, int pname, FloatBuffer params);

  /**
   * Fixed-point version of <code>glLight</code>.
   *
   * @see #glLightf(int light, int pname, float param)
   */
  void glLightx(int light, int pname, int param);

  /**
   * Fixed-point array version of <code>glLight</code>.
   *
   * @see #glLightfv(int light, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glLightxv(int light, int pname, int[] params, int offset);

  /**
   * Fixed-point <code>Buffer</code> version of <code>glLight</code>.
   *
   * @see #glLightfv(int light, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glLightxv(int light, int pname, IntBuffer params);

  /**
   * Specify the width of rasterized lines.
   *   
   * <p><code>glLineWidth</code> specifies the rasterized width of
   * both aliased and antialiased lines. Using a line width other than
   * 1 has different effects, depending on whether line antialiasing
   * is enabled. To enable and disable line antialiasing, call
   * <code>glEnable</code> and <code>glDisable</code> with argument
   * <code>GL_LINE_SMOOTH</code>. Line antialiasing is initially
   * disabled.
   *
   * <p>If line antialiasing is disabled, the actual width is
   * determined by rounding the supplied width to the nearest
   * integer. (If the rounding results in the value 0, it is as if the
   * line width were 1.) If <code>|delta x| >= |delta y|</code>,
   * <i>i</i> pixels are filled in each column that is rasterized,
   * where <i>i</i> is the rounded value of
   * <code>width</code>. Otherwise, <i>i</i> pixels are filled in each
   * row that is rasterized.
   *
   * <p>If antialiasing is enabled, line rasterization produces a
   * fragment for each pixel square that intersects the region lying
   * within the rectangle having width equal to the current line
   * width, length equal to the actual length of the line, and
   * centered on the mathematical line segment. The coverage value for
   * each fragment is the window coordinate area of the intersection
   * of the rectangular region with the corresponding pixel
   * square. This value is saved and used in the final rasterization
   * step.
   *
   * <p>Not all widths can be supported when line antialiasing is
   * enabled. If an unsupported width is requested, the nearest
   * supported width is used. Only width 1 is guaranteed to be
   * supported; others depend on the implementation. Likewise, there
   * is a range for aliased line widths as well. To query the range of
   * supported widths and the size difference between supported widths
   * within the range, call <code>glGetIntegerv</code> with arguments
   * <code>GL_ALIASED_LINE_WIDTH_RANGE</code>,
   * <code>GL_SMOOTH_LINE_WIDTH_RANGE</code>,
   * <code>GL_SMOOTH_LINE_WIDTH_GRANULARITY</code>.
   *
   * <h4>Notes</h4>
   *
   * <p>Nonantialiased line width may be clamped to an
   * implementation-dependent maximum. Call <code>glGetIntegerv</code>
   * with <code>GL_ALIASED_LINE_WIDTH_RANGE</code> to determine the
   * maximum width.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if width is less
   * than or equal to 0.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_ALIASED_LINE_WIDTH_RANGE</code>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_SMOOTH_LINE_WIDTH_RANGE</code>
   *
   * @param width Specifies the width of rasterized lines. The initial
   * value is 1.
   */
  void glLineWidth(float width);

  /**
   * Fixed-point version of <code>glLineWidth</code>.
   *
   * @see #glLineWidth
   */
  void glLineWidthx(int width);

  /**
   * Replace the current matrix with the identity matrix.
   *
   * <p><code>glLoadIdentity</code> replaces the current matrix with
   * the identity matrix. It is semantically equivalent to calling
   * glLoadMatrix with the identity matrix
   *
   * <pre>
   * ( 1       0       0       0 )
   * ( 0       1       0       0 )
   * ( 0       0       1       0 )
   * ( 0       0       0       1 )
   * </pre>
   *
   * but in some cases it is more efficient.
   */
  void glLoadIdentity();

  /**
   * Replace the current matrix with the specified matrix.
   *
   * <p><code>glLoadMatrix</code> replaces the current matrix with the
   * one whose elements are specified by <code>m</code>. The current
   * matrix is the projection matrix, modelview matrix, or texture
   * matrix, depending on the current matrix mode (see glMatrixMode).
   *
   * <p>The current matrix, <code>M</code>, defines a transformation
   * of coordinates. For instance, assume <code>M</code> refers to the
   * modelview matrix. If <code>v = (v[0], v[1], v[2], v[3])</code> is
   * the set of object coordinates of a vertex, and <code>m</code> is
   * an array of 16 fixed-point or single-precision floating-point
   * values <code>m[0]</code>, <code>m[1]</code>, ...,
   * <code>m[15]</code>, then the modelview transformation
   * <code>M(v)</code> does the following:
   *
   * <pre>
   *        ( m[0] m[4] m[8]  m[12] )   ( v[0] )
   * M(v) = ( m[1] m[5] m[9]  m[13] ) x ( v[1] )
   *        ( m[2] m[6] m[10] m[14] )   ( v[2] )
   *        ( m[3] m[7] m[11] m[15] )   ( v[3] )
   * </pre>
   *
   * where "x" denotes matrix multiplication.
   *
   * <p>Projection and texture transformations are similarly defined.
   *
   * <h4>Notes</h4>
   *
   * <p>While the elements of the matrix may be specified with single or
   * double precision, the GL implementation may store or operate on
   * these values in less than single precision.
   *
   * @param m Specifies an array of at least 16 consecutive values,
   * the first 16 of which are used as the elements of a 4 × 4
   * column-major matrix.
   * @param offset the starting offset within the
   * <code>m</code> array.
   *
   * @exception IllegalArgumentException if <code>m</code> is
   * <code>null</code>. 
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>m.length -
   * offset</code> is less than 16.
  */
  void glLoadMatrixf(float[] m, int offset);

  /**
   * Floating-point <code>Buffer</code> version of <code>glLoadMatrix</code>.
   *
   * @see #glLoadMatrixf(float[] m, int offset)
   *
   * @exception IllegalArgumentException if <code>m</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>m.remaining()</code>
   * is less than 16.
   */
  void glLoadMatrixf(FloatBuffer m);

  /**
   * Fixed-point array version of <code>glLoadMatrix</code>.
   *
   * @see #glLoadMatrixf(float[] m, int offset)
   *
   * @exception IllegalArgumentException if <code>m</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>m.length -
   * offset</code> is less than 16.
   */
  void glLoadMatrixx(int[] m, int offset);

  /**
   * Fixed-point <code>Buffer</code> version of <code>glLoadMatrix</code>.
   *
   * @see #glLoadMatrixf(float[] m, int offset)
   *
   * @exception IllegalArgumentException if <code>m</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>m.remaining()</code>
   * is less than 16.
   */
  void glLoadMatrixx(IntBuffer m);

  /**
   * Specify a logical pixel operation.
   *
   * <p><code>glLogicOp</code> specifies a logical operation that,
   * when enabled, is applied between the incoming color and the color
   * at the corresponding location in the frame buffer. To enable or
   * disable the logical operation, call <code>glEnable</code> and
   * <code>glDisable</code> with argument
   * <code>GL_COLOR_LOGIC_OP</code>. Logical operation is initially
   * disabled.
   *
   * <pre>
   * Opcode            Resulting Operation
   *
   * GL_CLEAR                     0
   * GL_SET                       1
   * GL_COPY                      s
   * GL_COPY_INVERTED           ~ s
   * GL_NOOP                      d
   * GL_INVERT                  ~ d
   * GL_AND                 s &   d
   * GL_NAND             ~ (s &   d)
   * GL_OR                  s |   d
   * GL_NOR              ~ (s |   d)
   * GL_XOR                 s ^   d
   * GL_EQUIV            ~ (s ^   d)
   * GL_AND_REVERSE         s & ~ d
   * GL_AND_INVERTED      ~ s &   d
   * GL_OR_REVERSE          s | ~ d
   * GL_OR_INVERTED       ~ s |   d
   * </pre>
   *
   * <p><code>opcode</code> is a symbolic constant chosen from the
   * list above. In the explanation of the logical operations,
   * <i>s</i> represents the incoming color and <i>d</i> represents
   * the color in the frame buffer. As in the Java language, "~"
   * represents bitwise negation, "&" represents bitwise AND, "|"
   * represents bitwise OR, and "^" represents bitwise XOR.  As these
   * bitwise operators suggest, the logical operation is applied
   * independently to each bit pair of the source and destination
   * indices or colors.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if opcode is not an
   * accepted value.
   *
   * @param opcode Specifies a symbolic constant that selects a
   * logical operation. The following symbols are accepted:
   * <code>GL_CLEAR</code>, <code>GL_SET</code>, <code>GL_COPY</code>,
   * <code>GL_COPY_INVERTED</code>, <code>GL_NOOP</code>,
   * <code>GL_INVERT</code>, <code>GL_AND</code>,
   * <code>GL_NAND</code>, <code>GL_OR</code>, <code>GL_NOR</code>,
   * <code>GL_XOR</code>, <code>GL_EQUIV</code>,
   * <code>GL_AND_REVERSE</code>, <code>GL_AND_INVERTED</code>,
   * <code>GL_OR_REVERSE</code>, and <code>GL_OR_INVERTED</code>. The
   * initial value is <code>GL_COPY</code>.
   */
  void glLogicOp(int opcode);

  /**
   * Specify material parameters for the lighting model.
   *
   * <p><code>glMaterial</code> assigns values to material
   * parameters. There are two matched sets of material
   * parameters. One, the front-facing set, is used to shade points,
   * lines, and all polygons (when two-sided lighting is disabled), or
   * just front-facing polygons (when two-sided lighting is
   * enabled). The other set, back-facing, is used to shade
   * back-facing polygons only when two-sided lighting is
   * enabled. Refer to the <code>glLightModel</code> reference page
   * for details concerning one- and two-sided lighting calculations.
   *
   * <p><code>glMaterial</code> takes three arguments. The first,
   * face, must be <code>GL_FRONT_AND_BACK</code> and specifies that
   * both front and back materials will be modified. The second,
   * pname, specifies which of several parameters in one or both sets
   * will be modified. The third, params, specifies what value or
   * values will be assigned to the specified parameter.
   *
   * <p>Material parameters are used in the lighting equation that is
   * optionally applied to each vertex. The equation is discussed in
   * the <code>glLightModel</code> reference page. The parameters that
   * can be specified using <code>glMaterial</code>, and their
   * interpretations by the lighting equation, are as follows:
   *
   * <ul>
   *
   * <li><code>GL_AMBIENT</code></li>
   *
   * <p><code>params</code> contains four fixed-point or
   * floating-point values that specify the ambient RGBA reflectance
   * of the material. The values are not clamped. The initial ambient
   * reflectance is (0.2, 0.2, 0.2, 1.0).
   *
   * <li><code>GL_DIFFUSE</code></li>
   *
   * <p><code>params</code> contains four fixed-point or
   * floating-point values that specify the diffuse RGBA reflectance
   * of the material. The values are not clamped. The initial diffuse
   * reflectance is (0.8, 0.8, 0.8, 1.0).
   *
   * <li><code>GL_SPECULAR</code></li>
   *
   * <p><code>params</code> contains four fixed-point or
   * floating-point values that specify the specular RGBA reflectance
   * of the material. The values are not clamped. The initial specular
   * reflectance is (0, 0, 0, 1).
   *
   * <li><code>GL_EMISSION</code></li>
   *
   * <p><code>params</code> contains four fixed-point or
   * floating-point values that specify the RGBA emitted light
   * intensity of the material. The values are not clamped. The
   * initial emission intensity is (0, 0, 0, 1).
   *
   * <li><code>GL_SHININESS</code></li>
   *
   * <p><code>params</code> is a single fixed-point or floating-point
   * value that specifies the RGBA specular exponent of the
   * material. Only values in the range [0, 128] are accepted. The
   * initial specular exponent is 0.
   *
   * <li><code>GL_AMBIENT_AND_DIFFUSE</code></li>
   *
   * <p>Equivalent to calling <code>glMaterial</code> twice with the same
   * parameter values, once with <code>GL_AMBIENT</code> and once with
   * <code>GL_DIFFUSE</code>.
   *
   * </ul>
   *
   * <h4>Notes</h4>
   *
   * <p>To change the diffuse and ambient material per vertex, color
   * material can be used. To enable and disable
   * <code>GL_COLOR_MATERIAL</code>, call <code>glEnable</code> and
   * <code>glDisable</code> with argument
   * <code>GL_COLOR_MATERIAL</code>. Color material is initially
   * disabled.
   *
   * <p>While the ambient, diffuse, specular and emission material
   * parameters all have alpha components, only the diffuse alpha
   * component is used in the lighting computation.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if either face or
   * <code>pname</code> is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if a specular
   * exponent outside the range [0, 128] is specified.
   *
   * @param face Specifies which face or faces are being updated. Must
   * be <code>GL_FRONT_AND_BACK</code>.
   * @param pname Specifies the single-valued material parameter of
   * the face or faces that is being updated. Must be
   * <code>GL_SHININESS</code>.
   * @param param Specifies the value that parameter
   * <code>GL_SHININESS</code> will be set to.
   */
  void glMaterialf(int face, int pname, float param);

  /**
   * Specify material parameters for the lighting model (array
   * version).
   *
   * <p><code>glMaterial</code> assigns values to material
   * parameters. There are two matched sets of material
   * parameters. One, the front-facing set, is used to shade points,
   * lines, and all polygons (when two-sided lighting is disabled), or
   * just front-facing polygons (when two-sided lighting is
   * enabled). The other set, back-facing, is used to shade
   * back-facing polygons only when two-sided lighting is
   * enabled. Refer to the <code>glLightModel</code> reference page
   * for details concerning one- and two-sided lighting calculations.
   *
   * <p><code>glMaterial</code> takes three arguments. The first,
   * face, must be <code>GL_FRONT_AND_BACK</code> and specifies that
   * both front and back materials will be modified. The second,
   * pname, specifies which of several parameters in one or both sets
   * will be modified. The third, params, specifies what value or
   * values will be assigned to the specified parameter.
   *
   * <p>Material parameters are used in the lighting equation that is
   * optionally applied to each vertex. The equation is discussed in
   * the <code>glLightModel</code> reference page. The parameters that
   * can be specified using <code>glMaterial</code>, and their
   * interpretations by the lighting equation, are as follows:
   *
   * <ul>
   *
   * <li><code>GL_AMBIENT</code></li>
   *
   * <p><code>params</code> contains four fixed-point or
   * floating-point values that specify the ambient RGBA reflectance
   * of the material. The values are not clamped. The initial ambient
   * reflectance is (0.2, 0.2, 0.2, 1.0).
   *
   * <li><code>GL_DIFFUSE</code></li>
   *
   * <p><code>params</code> contains four fixed-point or
   * floating-point values that specify the diffuse RGBA reflectance
   * of the material. The values are not clamped. The initial diffuse
   * reflectance is (0.8, 0.8, 0.8, 1.0).
   *
   * <li><code>GL_SPECULAR</code></li>
   *
   * <p><code>params</code> contains four fixed-point or
   * floating-point values that specify the specular RGBA reflectance
   * of the material. The values are not clamped. The initial specular
   * reflectance is (0, 0, 0, 1).
   *
   * <li><code>GL_EMISSION</code></li>
   *
   * <p><code>params</code> contains four fixed-point or
   * floating-point values that specify the RGBA emitted light
   * intensity of the material. The values are not clamped. The
   * initial emission intensity is (0, 0, 0, 1).
   *
   * <li><code>GL_SHININESS</code></li>
   *
   * <p><code>params</code> is a single fixed-point or floating-point
   * value that specifies the RGBA specular exponent of the
   * material. Only values in the range [0, 128] are accepted. The
   * initial specular exponent is 0.
   *
   * <li><code>GL_AMBIENT_AND_DIFFUSE</code></li>
   *
   * <p>Equivalent to calling <code>glMaterial</code> twice with the same
   * parameter values, once with <code>GL_AMBIENT</code> and once with
   * <code>GL_DIFFUSE</code>.
   *
   * </ul>
   *
   * <h4>Notes</h4>
   *
   * <p>To change the diffuse and ambient material per vertex, color
   * material can be used. To enable and disable
   * <code>GL_COLOR_MATERIAL</code>, call <code>glEnable</code> and
   * <code>glDisable</code> with argument
   * <code>GL_COLOR_MATERIAL</code>. Color material is initially
   * disabled.
   *
   * <p>While the ambient, diffuse, specular and emission material
   * parameters all have alpha components, only the diffuse alpha
   * component is used in the lighting computation.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if either face or
   * <code>pname</code> is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if a specular
   * exponent outside the range [0, 128] is specified.
   *
   * @param face Specifies which face or faces are being updated. Must
   * be <code>GL_FRONT_AND_BACK</code>.
   * @param pname Specifies the material parameter of the face or
   * faces that is being updated. Must be one of
   * <code>GL_AMBIENT</code>, <code>GL_DIFFUSE</code>,
   * <code>GL_SPECULAR</code>, <code>GL_EMISSION</code>,
   * <code>GL_SHININESS</code>, or
   * <code>GL_AMBIENT_AND_DIFFUSE</code>.
   * @param params Specifies a pointer to the value or values that
   * <code>pname</code> will be set to.
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
  void glMaterialfv(int face, int pname, float[] params, int offset);

  /**
   * Floating-point <code>Buffer</code> version of <code>glMaterial</code>.
   *
   * @see #glMaterialfv(int face, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glMaterialfv(int face, int pname, FloatBuffer params);

  /**
   * Fixed-point version of <code>glMaterial</code>.
   *
   * @see #glMaterialf(int face, int pname, float param)
   */
  void glMaterialx(int face, int pname, int param);

  /**
   * Fixed-point array version of <code>glMaterial</code>.
   *
   * @see #glMaterialfv(int face, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>params.length -
   * offset</code> is smaller than the number of values required by
   * the parameter.
   */
  void glMaterialxv(int face, int pname, int[] params, int offset);

  /**
   * Fixed-point <code>Buffer</code> version of <code>glMaterial</code>.
   *
   * @see #glMaterialfv(int face, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glMaterialxv(int face, int pname, IntBuffer params);

  /**
   * Specify which matrix is the current matrix.
   *
   * <p><code>glMatrixMode</code> sets the current matrix mode. mode
   * can assume one of three values:
   *
   * <ul>
   *
   * <li><code>GL_MODELVIEW</code></li>
   *
   * <p>Applies subsequent matrix operations to the modelview matrix
   * stack.
   *
   * <li><code>GL_PROJECTION</code></li>
   *
   * <p>Applies subsequent matrix operations to the projection matrix
   * stack.
   *
   * <li><code>GL_TEXTURE</code></li>
   *
   * <p>Applies subsequent matrix operations to the texture matrix stack.
   *
   * <li><code>GL_MATRIX_PALETTE_OES</code>
   * (<code>OES_matrix_palette</code> extension)</li>
   *
   * <p>Enables the matrix palette stack extension, and applies
   * subsequent matrix operations to the matrix palette stack.
   *
   * </ul>
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if mode is not an
   * accepted value.
   *
   * @param mode Specifies which matrix stack is the target for
   * subsequent matrix operations. These values are accepted:
   * <code>GL_MODELVIEW</code>, <code>GL_PROJECTION</code>, and
   * <code>GL_TEXTURE</code>. In the <code>OES_matrix_palette</code>
   * extension is available, <code>GL_MATRIX_PALETTE_OES</code> is
   * additionally accepted. The initial value is
   * <code>GL_MODELVIEW</code>.
   */
  void glMatrixMode(int mode);

  /**
   * Multiply the current matrix with the specified matrix.
   *
   * <p><code>glMultMatrix</code> multiplies the current matrix with
   * the one specified using m, and replaces the current matrix with
   * the product.
   *
   * <p>The current matrix is determined by the current matrix mode (see
   * glMatrixMode). It is either the projection matrix, modelview
   * matrix, or the texture matrix.
   *
   * <h4>Examples</h4>
   *
   * <p>If the current matrix is <code>C</code>, and the coordinates
   * to be transformed are, <code>v</code> = (<code>v[0]</code>,
   * <code>v[1]</code>, <code>v[2]</code>, <code>v[3]</code>), then
   * the current transformation is <code>C x v</code>, or
   *
   * <pre>
   * ( c[0] c[4] c[8]  c[12] )   ( v[0] )
   * ( c[1] c[5] c[9]  c[13] ) x ( v[1] )
   * ( c[2] c[6] c[10] c[14] )   ( v[2] )
   * ( c[3] c[7] c[11] c[15] )   ( v[3] )
   * </pre>
   *
   * <p>Calling <code>glMultMatrix</code> with an argument of
   * <code>m</code> = <code>m[0]</code>, <code>m[1]</code>, ...,
   * <code>m[15]</code> replaces the current transformation with
   * <code>(C x M) x v</code>, or
   *
   * <pre>
   * ( c[0] c[4] c[8]  c[12] )   ( m[0] m[4] m[8]  m[12] )   ( v[0] )
   * ( c[1] c[5] c[9]  c[13] ) x ( m[1] m[5] m[9]  m[13] ) x ( v[1] )
   * ( c[2] c[6] c[10] c[14] )   ( m[2] m[6] m[10] m[14] )   ( v[2] )
   * ( c[3] c[7] c[11] c[15] )   ( m[3] m[7] m[11] m[15] )   ( v[3] )
   * </pre>
   *
   * <p>where "x" denotes matrix multiplication, and <code>v</code> is
   * represented as a 4 × 1 matrix.
   *
   * <h4>Notes</h4>
   *
   * <p>While the elements of the matrix may be specified with single or
   * double precision, the GL may store or operate on these values in
   * less than single precision.
   *
   * <p>The array elements are passed in as a one-dimensional
   * array in column-major order. The order of the multiplication
   * is important. For example, if the current transformation is a
   * rotation, and <code>glMultMatrix</code> is called with a
   * translation matrix, the translation is done directly on the
   * coordinates to be transformed, while the rotation is done on the
   * results of that translation.
   *
   * @param m Specifies an array of at least 16 consecutive values,
   * the first 16 of which are used as the elements of a 4 × 4
   * column-major matrix.
   * @param offset the starting offset within the
   * <code>m</code> array.
   *
   * @exception IllegalArgumentException if <code>m</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>m.length -
   * offset</code> is less than 16.
   */
  void glMultMatrixf(float[] m, int offset);

  /**
   * Floating-point <code>Buffer</code> version of <code>glMultMatrix</code>.
   *
   * @see #glMultMatrixf(float[] m, int offset)
   *
   * @exception IllegalArgumentException if <code>m</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>m.remaining()</code>
   * is less than 16.
   */
  void glMultMatrixf(FloatBuffer m);

  /**
   * Fixed-point array version of <code>glMultMatrix</code>.
   *
   * @see #glMultMatrixf(float[] m, int offset)
   *
   * @exception IllegalArgumentException if <code>m</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>offset</code> is
   * less than 0.
   * @exception IllegalArgumentException if <code>m.length -
   * offset</code> is less than 16.
   */
  void glMultMatrixx(int[] m, int offset);

  /** Fixed-point <code>Buffer</code> version of <code>glMultMatrix</code>.
   *
   * @see #glMultMatrixf(float[] m, int offset)
   *
   * @exception IllegalArgumentException if <code>m</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>m.remaining()</code>
   * is less than 16.
   */
  void glMultMatrixx(IntBuffer m);

  /**
   * Set the current texture coordinates.
   *
   * <p><code>glMultiTexCoord</code> specifies the four texture
   * coordinates as <code>(s, t, r, q)</code>.
   *
   * <p>The current texture coordinates are part of the data that is
   * associated with each vertex.
   *
   * <h4>Notes</h4>
   *
   * <p>It is always the case that <code>GL_TEXTURE</code><i>i</i> =
   * <code>GL_TEXTURE0</code> + <i>i</i>.
   *
   * <h4>Associated Gets</h4>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_UNITS</code>.
   *
   * @param target Specifies texture unit whose coordinates should be
   * modified. The number of texture units is implementation
   * dependent, but must be at least one. Must be one of
   * <code>GL_TEXTURE</code><i>i</i>, where <code>0 <= <i>i</i> <
   * GL_MAX_TEXTURE_UNITS</code>, which is an implementation-dependent
   * value.
   * @param s Specifies an s texture coordinate for target texture
   * unit. The initial value is 0.
   * @param t Specifies a t texture coordinate for
   * target texture unit. The initial value is 0.
   * @param r Specifies an r texture coordinate for
   * target texture unit. The initial value is 0.
   * @param q Specifies a q texture coordinate for
   * target texture unit. The initial value is 1.
   */
  void glMultiTexCoord4f(int target, float s, float t, float r, float q);

  /**
   * Fixed-point version of <code>glMultiTexCoord</code>.
   *
   * @see #glMultiTexCoord4f
   */
  void glMultiTexCoord4x(int target, int s, int t, int r, int q);

  /**
   * Set the current normal vector.
   *
   * <p>The current normal is set to the given coordinates whenever
   * <code>glNormal</code> is issued. Byte, short, or integer arguments are
   * converted to floating-point with a linear mapping that maps the
   * most positive representable integer value to 1.0, and the most
   * negative representable integer value to -1.0.
   *
   * <p>Normals specified with <code>glNormal</code> need not have
   * unit length. If <code>GL_NORMALIZE</code> is enabled, then
   * normals of any length specified with <code>glNormal</code> are
   * normalized after transformation. If
   * <code>GL_RESCALE_NORMAL</code> is enabled, normals are scaled by
   * a scaling factor derived from the modelview
   * matrix. <code>GL_RESCALE_NORMAL</code> requires that the
   * originally specified normals were of unit length, and that the
   * modelview matrix contain only uniform scales for proper
   * results. To enable and disable normalization, call
   * <code>glEnable</code> and <code>glDisable</code> with either
   * <code>GL_NORMALIZE</code> or
   * <code>GL_RESCALE_NORMAL</code>. Normalization is initially
   * disabled.
   *
   * @param nx Specifies the x coordinate of the new
   * current normal. The initial value is 0.
   * @param ny Specifies the y coordinate of the new
   * current normal. The initial value is 0.
   * @param nz Specifies the z coordinate of the new
   * current normal. The initial value is 1.
   */
  void glNormal3f(float nx, float ny, float nz);

  /**
   * Fixed-point version of <code>glNormal</code>.
   *
   * @see #glNormal3f
   */
  void glNormal3x(int nx, int ny, int nz);

  /**
   * Define an array of normals.
   *
   * <p><code>glNormalPointer</code> specifies the location and data
   * of an array of normals to use when rendering. type specifies the
   * data type of the normal coordinates and stride gives the byte
   * stride from one normal to the next, allowing vertices and
   * attributes to be packed into a single array or stored in separate
   * arrays. (Single-array storage may be more efficient on some
   * implementations.) When a normal array is specified, type , stride
   * , and pointer are saved as client-side state.
   *
   * <p>If the normal array is enabled, it is used when
   * <code>glDrawArrays</code> or <code>glDrawElements</code> is
   * called. To enable and disable the normal array, call
   * <code>glEnableClientState</code> and
   * <code>glDisableClientState</code> with the argument
   * <code>GL_NORMAL_ARRAY</code>. The normal array is initially
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
   * <p><code>glNormalPointer</code> is typically implemented on the
   * client side.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if type is not an
   * accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if stride is
   * negative.
   *
   * <p> The <code>pointer</code> argument must be a direct buffer
   * with a type matching that specified by the <code>type</code>
   * argument.
   *
   * @param type Specifies the data type of each coordinate in the
   * array. Symbolic constants <code>GL_BYTE</code>,
   * <code>GL_SHORT</code>, <code>GL_FIXED</code>, and
   * <code>GL_FLOAT</code> are accepted. The initial value is
   * <code>GL_FLOAT</code>.
   * @param stride Specifies the byte offset between consecutive
   * normals. If stride is 0, the normals are understood to be tightly
   * packed in the array. The initial value is 0.
   * @param pointer Specifies a pointer to the first coordinate of the
   * first normal in the array. The initial value is 0.
   *
   * @exception IllegalStateException if OpenGL ES 1.1 is being used and
   * VBOs are enabled.
   * @exception IllegalArgumentException if <code>pointer</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>pointer</code> is not direct.
   */
  void glNormalPointer(int type, int stride, Buffer pointer);

  /**
   * Multiply the current matrix with an orthographic matrix.
   *
   * <p><code>glOrtho</code> describes a transformation that produces
   * a parallel projection. The current matrix (see glMatrixMode) is
   * multiplied by this matrix and the result replaces the current
   * matrix, as if <code>glMultMatrix</code> were called with the
   * following matrix as its argument:
   *
   * <pre>
   * ( 2/(right - left)           0                    0            tx )
   * (          0          2/(top - bottom)            0            ty )
   * (          0                 0          (-2)/(far - near)      tz )
   * (          0                 0                    0             1 )
   * </pre>
   *
   * <p>where
   *
   * <pre>
   * tx = - (right + left)/(right - left)
   * ty = - (top + bottom)/(top - bottom)
   * tz = - (far + near)  /(far - near)
   * </pre>
   *
   * <p>Typically, the matrix mode is <code>GL_PROJECTION</code>, and
   * (left, bottom, -near) and (right, top, -near) specify the points
   * on the near clipping plane that are mapped to the lower left and
   * upper right corners of the window, respectively, assuming that
   * the eye is located at (0, 0, 0). -far specifies the location of
   * the far clipping plane. Both near and far can be either positive
   * or negative.
   *
   * <p>Use <code>glPushMatrix</code> and <code>glPopMatrix</code> to
   * save and restore the current matrix stack.
   *
   * @param left Specifies the coordinate for the left
   * vertical clipping plane.
   * @param right Specifies the coordinate for the right
   * vertical clipping plane.
   * @param bottom Specifies the coordinate for the bottom
   * horizontal clipping plane.
   * @param top Specifies the coordinate for the top
   * horizontal clipping plane.
   * @param near Specifies the distance to the nearer
   * depth clipping plane. This value is negative if the plane is
   * to be behind the viewer.
   * @param far Specifies the distance to the farther
   * depth clipping plane. This value is negative if the plane is
   * to be behind the viewer.
   */
  void glOrthof(float left, float right,
                float bottom, float top,
                float near, float far);

  /**
   * Fixed-point version of <code>glOrtho</code>.
   *
   * @see #glOrthof
   */
  void glOrthox(int left, int right,
                int bottom, int top,
                int near, int far);

  /**
   * Set pixel storage modes.
   *
   * <p><code>glPixelStore</code> sets pixel storage modes that affect
   * the operation of subsequent <code>glReadPixels</code> as well as
   * the unpacking of <code>glTexImage2D</code>, and
   * <code>glTexSubImage2D</code>.
   *
   * <p><code>pname</code> is a symbolic constant indicating the
   * parameter to be set, and <code>param</code> is the new value. The
   * following storage parameter affects how pixel data is returned to
   * client memory. This value is significant for
   * <code>glReadPixels</code>:
   *
   * <ul>
   *
   * <li><code>GL_PACK_ALIGNMENT</code></li>
   *
   * <p>Specifies the alignment requirements for the start of each pixel
   * row in memory. The allowable values are 1 (byte-alignment), 2
   * (rows aligned to even-numbered bytes), 4 (word-alignment), and 8
   * (rows start on double-word boundaries). The initial value is 4.
   *
   * <p>The following storage parameter affects how pixel data is read
   * from client memory. This value is significant for
   * <code>glTexImage2D</code> and <code>glTexSubImage2D</code>:
   *
   * <li><code>GL_UNPACK_ALIGNMENT</code></li>
   *
   * <p>Specifies the alignment requirements for the start of each pixel
   * row in memory. The allowable values are 1 (byte-alignment), 2
   * (rows aligned to even-numbered bytes), 4 (word-alignment), and 8
   * (rows start on double-word boundaries). The initial value is 4.
   *
   * </ul>
   *
   * <h4>Notes</h4>
   *
   * <p>Pixel storage modes are client states.
   *
   * <p><code>glCompressedTexImage2D</code> and
   * <code>glCompressedTexSubImage2D</code> are not affected by
   * <code>glPixelStore</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if
   * <code>pname</code> is not an accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if alignment is
   * specified as other than 1, 2, 4, or 8.
   *
   * @param pname Specifies the symbolic name of the parameter to be
   * set. <code>GL_PACK_ALIGNMENT</code> affects the packing of pixel
   * data into memory. <code>GL_UNPACK_ALIGNMENT</code> affects the
   * unpacking of pixel data from memory.
   * @param param Specifies the value that <code>pname</code> is set
   * to.
   */
  void glPixelStorei(int pname, int param);

  /**
   * Specify the diameter of rasterized points.
   *
   * <p><code>glPointSize</code> specifies the rasterized diameter of
   * both aliased and antialiased points. Using a point size other
   * than 1 has different effects, depending on whether point
   * antialiasing is enabled. To enable and disable point
   * antialiasing, call <code>glEnable</code> and
   * <code>glDisable</code> with argument
   * <code>GL_POINT_SMOOTH</code>. Point antialiasing is initially
   * disabled.
   *
   * <p>If point antialiasing is disabled, the actual size is determined
   * by rounding the supplied size to the nearest integer. (If the
   * rounding results in the value 0, it is as if the point size were
   * 1.) If the rounded size is odd, then the center point (x, y) of
   * the pixel fragment that represents the point is computed as
   *
   * <pre>
   * (floor(xw) + 1/2, floor(yw) + 1/2)
   * </pre>
   *
   * <p>where w subscripts indicate window coordinates. All pixels that
   * lie within the square grid of the rounded size centered at (x, y)
   * make up the fragment. If the size is even, the center point is
   *
   * <pre>
   * (floor(xw + 1/2), floor(yw + 1/2))
   * </pre>
   *
   * <p>and the rasterized fragment's centers are the half-integer window
   * coordinates within the square of the rounded size centered at (x,
   * y). All pixel fragments produced in rasterizing a nonantialiased
   * point are assigned the same associated data, that of the vertex
   * corresponding to the point.
   *
   * <p>If antialiasing is enabled, then point rasterization produces a
   * fragment for each pixel square that intersects the region lying
   * within the circle having diameter equal to the current point size
   * and centered at the point's (xw, yw) . The coverage value for
   * each fragment is the window coordinate area of the intersection
   * of the circular region with the corresponding pixel square. This
   * value is saved and used in the final rasterization step. The data
   * associated with each fragment is the data associated with the
   * point being rasterized.
   *
   * <p>Not all sizes are supported when point antialiasing is
   * enabled. If an unsupported size is requested, the nearest
   * supported size is used. Only size 1 is guaranteed to be
   * supported; others depend on the implementation. To query the
   * range of supported sizes, call <code>glGetIntegerv</code> with the argument
   * <code>GL_SMOOTH_POINT_SIZE_RANGE</code>. For aliased points, query the
   * supported ranges <code>glGetIntegerv</code> with the argument
   * <code>GL_ALIASED_POINT_SIZE_RANGE</code>.
   *
   * <h4>Notes</h4>
   *
   * <p>A non-antialiased point size may be clamped to an
   * implementation-dependent maximum. Although this maximum cannot be
   * queried, it must be no less than the maximum value for
   * antialiased points, rounded to the nearest integer value.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if size is less
   * than or equal to 0.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_ALIASED_POINT_SIZE_RANGE</code>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_SMOOTH_POINT_SIZE_RANGE</code>
   *
   * @param size Specifies the diameter of rasterized points. The
   * initial value is 1.
   */
  void glPointSize(float size);

  /**
   * Fixed-point version of <code>glPointSize</code>.
   *
   * @see #glPointSize
   */
  void glPointSizex(int size);

  /**
   * Set the scale and units used to calculate depth values.
   *
   * <p>When <code>GL_POLYGON_OFFSET_FILL</code> is enabled, each
   * fragment's depth value will be offset after it is interpolated
   * from the depth values of the appropriate vertices. The value of
   * the offset is m * factor + r * units, where m is a measurement of
   * the change in depth relative to the screen area of the polygon,
   * and r is the smallest value that is guaranteed to produce a
   * resolvable offset for a given implementation. The offset is added
   * before the depth test is performed and before the value is
   * written into the depth buffer.
   *
   * <p><code>glPolygonOffset</code> is useful for for applying decals
   * to surfaces.
   *
   * @param factor Specifies a scale factor that is used to create a
   * variable depth offset for each polygon. The initial value is 0.
   * @param units Is multiplied by an implementation-specific value to
   * create a constant depth offset. The initial value is 0.
   */
  void glPolygonOffset(float factor, float units);

  /**
   * Fixed-point version of <code>glPolygonOffset</code>.
   *
   * @see #glPolygonOffset
   */
  void glPolygonOffsetx(int factor, int units);

  /**
   * Pop the current matrix stack.
   *
   * @see #glPushMatrix
   */
  void glPopMatrix();

  /**
   * Push the current matrix stack.
   *
   * <p>There is a stack of matrices for each of the matrix modes. In
   * <code>GL_MODELVIEW</code> mode, the stack depth is at least
   * 16. In the other modes, <code>GL_PROJECTION</code>, and
   * <code>GL_TEXTURE</code>, the depth is at least 2. The current
   * matrix in any mode is the matrix on the top of the stack for that
   * mode.
   *
   * <p><code>glPushMatrix</code> pushes the current matrix stack down
   * by one, duplicating the current matrix. That is, after a
   * glPushMatrix call, the matrix on top of the stack is identical to
   * the one below it.
   *
   * <p><code>glPopMatrix</code> pops the current matrix stack,
   * replacing the current matrix with the one below it on the stack.
   *
   * <p>Initially, each of the stacks contains one matrix, an identity matrix.
   *
   * <p>It is an error to push a full matrix stack, or to pop a matrix
   * stack that contains only a single matrix. In either case, the
   * error flag is set and no other change is made to GL state.
   *
   * <h4>Notes</h4>
   *
   * <p>Each texture unit has its own texture matrix stack. Use
   * <code>glActiveTexture</code> to select the desired texture matrix stack.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_STACK_OVERFLOW</code> is generated if
   * <code>glPushMatrix</code> is called while the current matrix
   * stack is full.
   *
   * <p><code>GL_STACK_UNDERFLOW</code> is generated if
   * <code>glPopMatrix</code> is called while the current matrix stack
   * contains only a single matrix.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_MODELVIEW_STACK_DEPTH</code>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_PROJECTION_STACK_DEPTH</code>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_STACK_DEPTH</code>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_UNITS</code>
   */
  void glPushMatrix();

  /**
   * Read a block of pixels from the color buffer.
   *
   * <p><code>glReadPixels</code> returns pixel data from the color
   * buffer, starting with the pixel whose lower left corner is at
   * location (x, y), into client memory starting at location
   * pixels. The processing of the pixel data before it is placed into
   * client memory can be controlled with <code>glPixelStore</code>.
   *
   * <p><code>glReadPixels</code> returns values from each pixel with
   * lower left corner at <code>(x + i, y + j)</code> for <code>0 <=
   * <i>i</i> < width</code> and <code>0 <= j < height</code>. This
   * pixel is said to be the ith pixel in the jth row. Pixels are
   * returned in row order from the lowest to the highest row, left to
   * right in each row.
   *
   * <p><code>format</code> specifies the format of the returned pixel
   * values. <code>GL_RGBA</code> is always accepted, the value of
   * <code>GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES</code> may allow
   * another format:
   *
   * <ul>
   *
   * <li><code>GL_RGBA</code></li>
   *
   * <p>Each color component is converted to floating point such that
   * zero intensity maps to 0 and full intensity maps to 1.
   *
   * <li><code>GL_RGB</code></li>
   *
   * <p>Each element is an RGB triple. The GL converts it to floating
   * point and assembles it into an RGBA element by attaching 1 for
   * alpha.
   *
   * <li><code>GL_LUMINANCE</code></li>
   *
   * <p>Each element is a single luminance value. The GL converts it to
   * floating point and assembles it into an RGBA element by
   * replicating the luminance value three times for red, green and
   * blue and attaching 1 for alpha.
   *
   * <li><code>GL_LUMINANCE_ALPHA</code></li>
   *
   * <p>Each element is a luminance/alpha pair. The GL converts it to
   * floating point and assembles it into an RGBA element by
   * replicating the luminance value three times for red, green and
   * blue.
   *
   * <li><code>GL_ALPHA</code></li>
   *
   * <p>Each element is a single alpha component. The GL converts it to
   * floating point and assembles it into an RGBA element by attaching
   * 0 for red, green and blue.
   *
   * </ul>
   *
   * <p>Unneeded data is then discarded. For example,
   * <code>GL_ALPHA</code> discards the red, green, and blue
   * components, while <code>GL_RGB</code> discards only the alpha
   * component. <code>GL_LUMINANCE</code> computes a single-component
   * value as the sum of the red, green, and blue components, and
   * <code>GL_LUMINANCE_ALPHA</code> does the same, while keeping
   * alpha as a second value. The final values are clamped to the
   * range <code>[0, 1]</code>.
   *
   * <p>Finally, the components are converted to the proper, as specified
   * by type where each component is multiplied by 2^n - 1, where n is
   * the number of bits per component.
   *
   * <p>Return values are placed in memory as follows. If format is
   * <code>GL_ALPHA</code>, or <code>GL_LUMINANCE</code>, a single
   * value is returned and the data for the ith pixel in the jth row
   * is placed in location j * width + i. <code>GL_RGB</code> returns
   * three values, <code>GL_RGBA</code> returns four values, and
   * <code>GL_LUMINANCE_ALPHA</code> returns two values for each
   * pixel, with all values corresponding to a single pixel occupying
   * contiguous space in pixels. Storage parameter
   * <code>GL_PACK_ALIGNMENT</code> set by <code>glPixelStore</code>,
   * affects the way that data is written into memory. See
   * <code>glPixelStore</code> for a description.
   *
   * <h4>Notes</h4>
   *
   * <p>Values for pixels that lie outside the window connected to the
   * current GL context are undefined.
   *
   * <p>If an error is generated, no change is made to the contents of pixels.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if format is not
   * <code>GL_RGBA</code> or the value of
   * <code>GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES</code>.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if type is not
   * <code>GL_UNSIGNED_BYTE</code> or the value of
   * <code>GL_IMPLEMENTATION_COLOR_READ_TYPE_OES</code>.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if either width or
   * height is negative.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if format and
   * type are neither (<code>GL_RGBA</code>,
   * <code>GL_UNSIGNED_BYTE)</code> nor the values of
   * (<code>GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES</code>,
   * <code>GL_IMPLEMENTATION_COLOR_READ_TYPE_OES)</code>.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES</code>
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_IMPLEMENTATION_COLOR_READ_TYPE_OES</code>
   * 
   * @param x Specifies the window x coordinate of the first pixel
   * that is read from the color buffer. This location is the lower
   * left corner of a rectangular block of pixels.
   * @param y Specifies the window y coordinate of the first pixel
   * that is read from the color buffer. This location is the lower
   * left corner of a rectangular block of pixels.
   * @param width Specifies the width of the pixel
   * rectangle. width and height of one correspond to a single pixel.
   * @param height Specifies the height of the pixel
   * rectangle. width and height of one correspond to a single pixel.
   * @param format Specifies the format of the pixel data. Must be
   * either <code>GL_RGBA</code> or the value of
   * <code>GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES</code>.
   * @param type Specifies the data type of the pixel data. Must be
   * either <code>GL_UNSIGNED_BYTE</code> or the value of
   * <code>GL_IMPLEMENTATION_COLOR_READ_TYPE_OES</code>.
   * @param pixels Returns the pixel data.
   *
   * @exception IllegalArgumentException if <code>pixels</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>pixels</code> does
   * not contain enough room for the pixel data.
   */
  void glReadPixels(int x, int y,
                    int width, int height,
                    int format, int type,
                    Buffer pixels);

  /**
   * Multiply the current matrix by a rotation matrix.
   *
   * <p><code>glRotate</code> produces a rotation of angle degrees
   * around the vector (x, y, z) . The current matrix (see
   * glMatrixMode) is multiplied by a rotation matrix with the product
   * replacing the current matrix, as if <code>glMultMatrix</code>
   * were called with the following matrix as its argument:
   *
   * <pre>
   * ( x^2(1 - c) + c        xy (1 - c) - zs       xz (1 - c) + ys       0 )
   * ( xy (1 - c) + zs       y^2(1 - c) + c        yz (1 - c) - xs       0 )
   * ( xz (1 - c) - ys       yz (1 - c) + xs       z^2(1 - c) + c        0 )
   * (         0                    0                     0              1 )
   * </pre>
   *
   * <p>Where c = cos (angle), s = sin (angle), and ||(x, y, z)|| = 1,
   * (if not, the GL will normalize this vector).
   *
   * <p>If the matrix mode is either <code>GL_MODELVIEW</code> or
   * <code>GL_PROJECTION</code>, all objects drawn after
   * <code>glRotate</code> is called are rotated. Use
   * <code>glPushMatrix</code> and <code>glPopMatrix</code> to save
   * and restore the unrotated coordinate system.
   *
   * <h4>Notes</h4>
   *
   * <p>This rotation follows the right-hand rule, so if the vector (x,
   * y, z) points toward the user, the rotation will be
   * counterclockwise.
   *
   * @param angle Specifies the angle of rotation, in degrees.
   * @param x Specifies the x coordinate of a vector.
   * @param y Specifies the y coordinate of a vector.
   * @param z Specifies the z coordinate of a vector.
   */
  void glRotatef(float angle, float x, float y, float z);

  /**
   * Fixed-point version of <code>glRotate</code>.
   *
   * @see #glRotatef
   */
  void glRotatex(int angle, int x, int y, int z);

  /**
   * Specify mask to modify multisampled pixel fragments.
   *
   * <p><code>glSampleCoverage</code> defines a mask to modify the coverage of
   * multisampled pixel fragments. This capability is used for
   * antialiased screen-door transparency and smooth transitions
   * between two renderings of an object (often for level-of-detail
   * management in simulation systems).
   *
   * <p>When multisampling is enabled (see <code>glEnable</code> with
   * argument <code>GL_MULTISAMPLE)</code> a ``fragment mask'' is
   * computed for each fragment generated by a primitive. This mask
   * reflects the amount of the pixel covered by the fragment, and
   * determines the frame buffer samples that may be affected by the
   * fragment.
   *
   * <p>If conversion of alpha values to masks is enabled
   * (<code>glEnable</code> with argument
   * <code>GL_SAMPLE_ALPHA_TO_MASK)</code>, the fragment alpha value
   * is used to generate a temporary modification mask which is then
   * ANDed with the fragment mask. One way to interpret this is as a
   * form of dithering: a multivalued alpha (coverage or opacity) for
   * the whole fragment is converted to simple binary values of
   * coverage at many locations (the samples).
   *
   * <p>After conversion of alpha values to masks, if replacement of
   * alpha values is enabled (<code>glEnable</code> with argument
   * <code>GL_SAMPLE_ALPHA_TO_ONE)</code>, the fragment's alpha is set
   * to the maximum allowable value.
   *
   * <p>Finally, if fragment mask modification is enabled
   * (<code>glEnable</code> with argument
   * <code>GL_SAMPLE_MASK)</code>, <code>glSampleCoverage</code>
   * defines an additional modification mask. value is used to
   * generate a modification mask in much the same way alpha was used
   * above. If invert is <code>GL_TRUE</code>, then the modification
   * mask specified by value will be inverted. The final modification
   * mask will then be ANDed with the fragment mask resulting from the
   * previous steps. This can be viewed as an ``override'' control
   * that selectively fades the effects of multisampled fragments.
   *
   * <p>Note that <code>glSampleCoverage(value, GL_TRUE)</code> is not
   * necessarily equivalent to <code>glSampleCoverage(1.0 - value,
   * GL_FALSE)</code>; due to round-off and other issues, complementing
   * the coverage will not necessarily yield an inverted modification
   * mask.
   *
   * @param value Specifies the coverage of the modification mask. The
   * value is clamped to the range <code>[0, 1]</code>, where 0
   * represents no coverage and 1 full coverage. The initial value is
   * 1.
   * @param invert Specifies whether the modification mask implied by
   * value is inverted or not. The initial value is <code>GL_FALSE</code>.
   */
  void glSampleCoverage(float value, boolean invert);

  /**
   * Fixed-point version of <code>glSampleCoverage</code>.
   *
   * @see #glSampleCoverage
   */
  void glSampleCoveragex(int value, boolean invert);

  /**
   * Multiply the current matrix by a general scaling matrix.
   *
   * <p><code>glScale</code> produces a nonuniform scaling along the
   * x, y, and z axes. The three parameters indicate the desired scale
   * factor along each of the three axes.
   *
   * <p>The current matrix (see glMatrixMode) is multiplied by this scale
   * matrix, and the product replaces the current matrix as if glScale
   * were called with the following matrix as its argument:
   *
   * <pre> 
   * ( x       0       0       0 )
   * ( 0       y       0       0 )
   * ( 0       0       z       0 )
   * ( 0       0       0       1 )
   * </pre> 
   *
   * <p>If the matrix mode is either <code>GL_MODELVIEW</code> or
   * <code>GL_PROJECTION</code>, all objects drawn after
   * <code>glScale</code> is called are scaled.
   *
   * <p>Use <code>glPushMatrix</code> and <code>glPopMatrix</code> to
   * save and restore the unscaled coordinate system.
   *
   * <h4>Notes</h4>
   *
   * <p>If scale factors other than 1 are applied to the modelview matrix
   * and lighting is enabled, lighting often appears wrong. In that
   * case, enable automatic normalization of normals by calling
   * <code>glEnable</code> with the argument <code>GL_NORMALIZE</code>.
   *
   * @param x Specifies the scale factor along the x axis.
   * @param y Specifies the scale factor along the y axis.
   * @param z Specifies the scale factor along the z axis.
   */
  void glScalef(float x, float y, float z);

  /**
   * Fixed-point version of <code>glScale</code>.
   *
   * @see #glScalef
   */
  void glScalex(int x, int y, int z);

  /**
   * Define the scissor box.
   *
   * <p><code>glScissor</code> defines a rectangle, called the scissor
   * box, in window coordinates. The first two arguments, x and y,
   * specify the lower left corner of the box. width and height
   * specify the width and height of the box.
   *
   * <p>To enable and disable the scissor test, call
   * <code>glEnable</code> and <code>glDisable</code> with argument
   * <code>GL_SCISSOR_TEST</code>. The scissor test is initially
   * disabled. While scissor test is enabled, only pixels that lie
   * within the scissor box can be modified by drawing
   * commands. Window coordinates have integer values at the shared
   * corners of frame buffer pixels. glScissor(0, 0, 1, 1) allows
   * modification of only the lower left pixel in the window, and
   * glScissor(0, 0, 0, 0) doesn't allow modification of any pixels in
   * the window.
   *
   * <p>When the scissor test is disabled, it is as though the scissor
   * box includes the entire window.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if either width or
   * height is negative.
   *
   * @param x Specifies the x coordinate of the lower left corner
   * of the scissor box, in pixels. The initial value is 0.
   * @param y Specifies the y coordinate of the lower left corner
   * of the scissor box, in pixels. The initial value is 0.
   * @param width Specifies the width of the scissor box. When a GL
   * context is first attached to a surface (e.g. window), width and
   * height are set to the dimensions of that surface.
   * @param height Specifies the height of the scissor box. When a GL
   * context is first attached to a surface (e.g. window), width and
   * height are set to the dimensions of that surface.
   */
  void glScissor(int x, int y, int width, int height);

  /**
   * Select flat or smooth shading.
   *
   * <p>GL primitives can have either flat or smooth shading. Smooth
   * shading, the default, causes the computed colors of vertices to
   * be interpolated as the primitive is rasterized, typically
   * assigning different colors to each resulting pixel fragment. Flat
   * shading selects the computed color of just one vertex and assigns
   * it to all the pixel fragments generated by rasterizing a single
   * primitive. In either case, the computed color of a vertex is the
   * result of lighting if lighting is enabled, or it is the current
   * color at the time the vertex was specified if lighting is
   * disabled.
   *
   * <p>Flat and smooth shading are indistinguishable for
   * points. Starting at the beginning of the vertex array and
   * counting vertices and primitives from 1, the GL gives each
   * flat-shaded line segment <i>i</i> the computed color of
   * vertex <i>i</i> + 1, its second vertex. Counting similarly from 1,
   * the GL gives each flat-shaded polygon the computed color of
   * vertex <i>i</i> + 2, which is the last vertex to specify the
   * polygon.
   *
   * <p>Flat and smooth shading are specified by
   * <code>glShadeModel</code> with mode set to <code>GL_FLAT</code>
   * and <code>GL_SMOOTH</code>, respectively.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if mode is any value
   * other than <code>GL_FLAT</code> or <code>GL_SMOOTH</code>.
   *
   * @param mode Specifies a symbolic value representing a shading
   * technique. Accepted values are <code>GL_FLAT</code> and
   * <code>GL_SMOOTH</code>. The initial value is
   * <code>GL_SMOOTH</code>.
   */
  void glShadeModel(int mode);

  /**
   * Set function and reference value for stencil testing.
   *
   * <p>Stenciling, like depth-buffering, enables and disables drawing on
   * a per-pixel basis. You draw into the stencil planes using GL
   * drawing primitives, then render geometry and images, using the
   * stencil planes to mask out portions of the screen. Stenciling is
   * typically used in multipass rendering algorithms to achieve
   * special effects, such as decals, outlining, and constructive
   * solid geometry rendering.
   *
   * <p>The stencil test conditionally eliminates a pixel based on the
   * outcome of a comparison between the reference value and the value
   * in the stencil buffer. To enable and disable stencil test, call
   * <code>glEnable</code> and <code>glDisable</code> with argument
   * <code>GL_STENCIL_TEST</code>. Stencil test is initially
   * disabled. To specify actions based on the outcome of the stencil
   * test, call <code>glStencilOp</code>.
   *
   * <p><code>func</code> is a symbolic constant that determines the
   * stencil comparison function. It accepts one of eight values,
   * shown in the following list. <code>ref</code> is an integer
   * reference value that is used in the stencil comparison. It is
   * clamped to the range <code>[0, 2^n - 1]</code>, where <i>n</i> is
   * the number of bitplanes in the stencil buffer. <code>mask</code>
   * is bitwise ANDed with both the reference value and the stored
   * stencil value, with the ANDed values participating in the
   * comparison.
   *
   * <p>If <code>stencil</code> represents the value stored in the
   * corresponding stencil buffer location, the following list shows
   * the effect of each comparison function that can be specified by
   * func. Only if the comparison succeeds is the pixel passed through
   * to the next stage in the rasterization process (see
   * glStencilOp). All tests treat stencil values as unsigned integers
   * in the range <code>[0, 2^n - 1]</code>, where <i>n</i> is the
   * number of bitplanes in the stencil buffer.
   *
   * <p>The following values are accepted by <code>func</code>:
   *
   * <ul>
   *
   * <li><code>GL_NEVER</code></li>
   *
   * <p>Always fails.
   *
   * <li><code>GL_LESS</code></li>
   *
   * <p>Passes if (ref & mask) < (stencil & mask) .
   *
   * <li><code>GL_LEQUAL</code></li>
   *
   * <p>Passes if (ref & mask) <= (stencil & mask) .
   *
   * <li><code>GL_GREATER</code></li>
   *
   * <p>Passes if (ref & mask) > (stencil & mask) .
   *
   * <li><code>GL_GEQUAL</code></li>
   *
   * <p>Passes if (ref & mask) >= (stencil & mask) .
   *
   * <li><code>GL_EQUAL</code></li>
   *
   * <p>Passes if (ref & mask) == (stencil & mask) .
   *
   * <li><code>GL_NOTEQUAL</code></li>
   *
   * <p>Passes if (ref & mask) != (stencil & mask) .
   *
   * <li><code>GL_ALWAYS</code></li>
   *
   * <p>Always passes.
   *
   * </ul>
   * 
   * <h4>Notes</h4>
   *
   * <p>Initially, the stencil test is disabled. If there is no stencil
   * buffer, no stencil modification can occur and it is as if the
   * stencil test always passes.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if func is not one
   * of the eight accepted values.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument <code>GL_STENCIL_BITS</code>
   *
   * @param func Specifies the test function. Eight tokens are valid:
   * <code>GL_NEVER</code>, <code>GL_LESS</code>,
   * <code>GL_LEQUAL</code>, <code>GL_GREATER</code>,
   * <code>GL_GEQUAL</code>, <code>GL_EQUAL</code>,
   * <code>GL_NOTEQUAL</code>, and <code>GL_ALWAYS</code>. The initial
   * value is <code>GL_ALWAYS</code>.
   * @param ref Specifies the reference value for the stencil
   * test. <code>ref</code> is clamped to the range <code>[0, 2^n -
   * 1]</code>, where <i>n</i> is the number of bitplanes in the
   * stencil buffer. The initial value is 0.
   * @param mask Specifies a mask that is ANDed with both the
   * reference value and the stored stencil value when the test is
   * done. The initial value is all 1's.
   */
  void glStencilFunc(int func, int ref, int mask);

  /**
   * Control the writing of individual bits in the stencil planes.
   *
   * <p><code>glStencilMask</code> controls the writing of individual
   * bits in the stencil planes. The least significant n bits of mask,
   * where n is the number of bits in the stencil buffer, specify a
   * mask. Where a 1 appears in the mask, it's possible to write to
   * the corresponding bit in the stencil buffer. Where a 0 appears,
   * the corresponding bit is write-protected. Initially, all bits are
   * enabled for writing.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument <code>GL_STENCIL_BITS</code>
   *
   * @param mask Specifies a bit mask to enable and disable writing of
   * individual bits in the stencil planes. The initial value is all
   * 1's.
   */
  void glStencilMask(int mask);

  /**
   * Set stencil test actions.
   *
   * <p>Stenciling, like depth-buffering, enables and disables drawing on
   * a per-pixel basis. You draw into the stencil planes using GL
   * drawing primitives, then render geometry and images, using the
   * stencil planes to mask out portions of the screen. Stenciling is
   * typically used in multipass rendering algorithms to achieve
   * special effects, such as decals, outlining, and constructive
   * solid geometry rendering.
   *
   * <p>The stencil test conditionally eliminates a pixel based on the
   * outcome of a comparison between the value in the stencil buffer
   * and a reference value. To enable and disable stencil test, call
   * <code>glEnable</code> and <code>glDisable</code> with argument
   * <code>GL_STENCIL_TEST</code>. To control it, call
   * <code>glStencilFunc</code>. Stenciling is initially disabled.
   *
   * <p><code>glStencilOp</code> takes three arguments that indicate
   * what happens to the stored stencil value while stenciling is
   * enabled. If the stencil test fails, no change is made to the
   * pixel's color or depth buffers, and fail specifies what happens
   * to the stencil buffer contents. The following six actions are
   * possible.
   *
   * <ul>
   *
   * <li><code>GL_KEEP</code></li>
   *
   * <p>Keeps the current value.
   *
   * <li><code>GL_ZERO</code></li>
   *
   * <p>Sets the stencil buffer value to 0.
   *
   * <li><code>GL_REPLACE</code></li>
   *
   * <p>Sets the stencil buffer value to ref, as specified by
   * <code>glStencilFunc</code>.
   *
   * <li><code>GL_INCR</code></li>
   *
   * <p>Increments the current stencil buffer value. Clamps to the
   * maximum representable unsigned value.
   *
   * <li><code>GL_DECR</code></li>
   *
   * <p>Decrements the current stencil buffer value. Clamps to 0.
   *
   * <li><code>GL_INVERT</code></li>
   *
   * <p>Bitwise inverts the current stencil buffer value.
   *
   * <--
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
   * -->
   *
   * </ul>
   *
   * <p>Stencil buffer values are treated as unsigned integers. When
   * incremented and decremented, values are clamped to 0 and 2^n - 1,
   * where n is the value returned by querying <code>GL_STENCIL_BITS</code>.
   *
   * <p>The other two arguments to <code>glStencilOp</code> specify
   * stencil buffer actions that depend on whether subsequent depth
   * buffer tests succeed (<code>zpass</code>) or fail
   * (<code>zfail</code>) (see <code>glDepthFunc</code>). The actions
   * are specified using the same six symbolic constants as
   * <code>fail</code>. Note that <code>zfail</code> is ignored when
   * there is no depth buffer, or when the depth buffer is not
   * enabled. In these cases, <code>fail</code> and <code>zpass</code>
   * specify stencil action when the stencil test fails and passes,
   * respectively.
   *
   * <h4>Notes</h4>
   *
   * <p>If there is no stencil buffer, no stencil modification can occur
   * and it is as if the stencil tests always pass, regardless of any
   * call to <code>glStencilOp</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if fail, zfail, or
   * zpass is any value other than the six defined constant values.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument <code>GL_STENCIL_BITS</code>
   *
   * @param fail Specifies the action to take when the stencil test
   * fails. Six symbolic constants are accepted: <code>GL_KEEP</code>,
   * <code>GL_ZERO</code>, <code>GL_REPLACE</code>,
   * <code>GL_INCR</code>, <code>GL_DECR</code>, and
   * <code>GL_INVERT</code> <!-- <code>GL_INCR_WRAP</code>
   * (<code>OES_stencil_wrap</code> extension), and
   * <code>GL_DECR_WRAP</code> (<code>OES_stencil_wrap</code>
   * extension) -->. The initial value is <code>GL_KEEP</code>.
   * @param zfail Specifies the stencil action when the stencil test
   * passes, but the depth test fails. <code>zfail</code> accepts the
   * same symbolic constants as <code>fail</code>. The initial value
   * is <code>GL_KEEP</code>.
   * @param zpass Specifies the stencil action when both the stencil
   * test and the depth test pass, or when the stencil test passes and
   * either there is no depth buffer or depth testing is not
   * enabled. <code>zpass</code> accepts the same symbolic constants
   * as <code>fail</code>. The initial value is <code>GL_KEEP</code>.
   */
  void glStencilOp(int fail, int zfail, int zpass);

  /**
   * Define an array of texture coordinates.
   *
   * <p><code>glTexCoordPointer</code> specifies the location and data
   * of an array of texture coordinates to use when rendering. size
   * specifies the number of coordinates per element, and must be 2,
   * 3, or 4. type specifies the data type of each texture coordinate
   * and stride specifies the byte stride from one array element to
   * the next allowing vertices and attributes to be packed into a
   * single array or stored in separate arrays. (Single-array storage
   * may be more efficient on some implementations.)
   *
   * <p>When a texture coordinate array is specified, size, type, stride,
   * and pointer are saved as client-side state.
   *
   * <p>If the texture coordinate array is enabled, it is used when
   * <code>glDrawArrays</code>, or <code>glDrawElements</code> is
   * called. To enable and disable the texture coordinate array for
   * the client-side active texture unit, call
   * <code>glEnableClientState</code> and
   * <code>glDisableClientState</code> with the argument
   * <code>GL_TEXTURE_COORD_ARRAY</code>. The texture coordinate array
   * is initially disabled for all client-side active texture units
   * and isn't accessed when <code>glDrawArrays</code> or
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
   * <p><code>glTexCoordPointer</code> is typically implemented on the
   * client side.
   *
   * <p><code>glTexCoordPointer</code> updates the texture coordinate
   * array state of the client-side active texture unit, specified
   * with <code>glClientActiveTexture</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if size is not 2, 3, or 4.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if type is not an
   * accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if stride is negative.
   *
   * <p> The <code>pointer</code> argument must be a direct buffer
   * with a type matching that specified by the <code>type</code>
   * argument.
   *
   * @param size Specifies the number of coordinates per array
   * element. Must be 2, 3 or 4. The initial value is 4.
   * @param type Specifies the data type of each texture
   * coordinate. Symbolic constants <code>GL_BYTE</code>,
   * <code>GL_SHORT</code>, <code>GL_FIXED</code>,and
   * <code>GL_FLOAT</code> are accepted. The initial value is
   * <code>GL_FLOAT</code>.
   * @param stride Specifies the byte offset between consecutive array
   * elements. If stride is 0, the array elements are understood to be
   * tightly packed. The initial value is 0.
   * @param pointer Specifies a pointer to the first coordinate of the
   * first element in the array. The initial value is 0.
   *
   * @exception IllegalStateException if OpenGL ES 1.1 is being used and
   * VBOs are enabled.
   * @exception IllegalArgumentException if <code>pointer</code> is
   * <code>null</code> or is not direct.
   */
  void glTexCoordPointer(int size, int type, int stride, Buffer pointer);

  /**
   * Set texture environment parameters.
   *
   * <p>If <code>target</code> is <code>GL_TEXTURE_ENV</code>, then
   * the following applies:
   *
   * <p>A texture environment specifies how texture values are
   * interpreted when a fragment is textured. <code>target</code> must
   * be <code>GL_TEXTURE_ENV</code>. <code>pname</code> can be either
   * <code>GL_TEXTURE_ENV_MODE</code> or
   * <code>GL_TEXTURE_ENV_COLOR</code>.
   *
   * <p>If <code>pname</code> is <code>GL_TEXTURE_ENV_MODE</code>,
   * then <code>params</code> contains the symbolic name of a texture
   * function. Four texture functions may be specified:
   * <code>GL_MODULATE</code>, <code>GL_DECAL</code>,
   * <code>GL_BLEND</code>, and <code>GL_REPLACE</code>.
   *
   * <p>A texture function acts on the fragment to be textured using
   * the texture image value that applies to the fragment (see
   * <code>glTexParameter</code>) and produces an RGBA color for that
   * fragment. The following table shows how the RGBA color is
   * produced for each of the three texture functions that can be
   * chosen. <i>C</i> is a triple of color values (RGB) and <i>A</i>
   * is the associated alpha value. RGBA values extracted from a
   * texture image are in the range <code>[0, 1]</code>. The subscript
   * <i>f</i> refers to the incoming fragment, the subscript <i>t</i>
   * to the texture image, the subscript <i>c</i> to the texture
   * environment color, and subscript <i>v</i> indicates a value
   * produced by the texture function.
   *
   * <p>A texture image can have up to four components per texture
   * element (see <code>glTexImage2D</code>, and
   * <code>glCopyTexImage2D</code>). In a one-component image,
   * <i>Lt</i> indicates that single component. A two-component image
   * uses <i>Lt</i> and <i>At</i>. A three-component image has only a
   * color value, <i>Ct</i>. A four-component image has both a color
   * value <i>Ct</i> and an alpha value <i>At</i>.
   *
   * <p>For texture functions: <code>GL_REPLACE</code>,
   * <code>GL_MODULATE</code>, <code>GL_DECAL</code>,
   * <code>GL_BLEND</code>, or <code>GL_ADD</code>:
   *
   * <pre>
   * Base internal          Texture functions
   * format                 GL_MODULATE                GL_DECAL
   *
   * <p>GL_ALPHA               Cv = Cf                    undefined
   *                        Av = Af*At      
   *
   * <p>GL_LUMINANCE           Cv = Cf*Lt                 undefined
   *                        Av = Af         
   *
   * <p>GL_LUMINANCE_ALPHA     Cv = Cf*Lt                 undefined
   *                        Av = Af*At      
   *
   * <p>GL_RGB                 Cv = Cf*Ct                 Cv = Ct
   *                        Av = Af                    Av = Af
   *
   * <p>GL_RGBA                Cv = Cf*Ct                 Cv = Cf*(1 - At) + Ct*At
   *                        Av = Af*At                 Av = Af
   *
   * <p>Base internal          Texture functions
   * format                 GL_BLEND                   GL_REPLACE
   *
   * <p>GL_ALPHA               Cv = Cf                    Cv = Cf
   *                        Av = Af*At                 Av = At
   *
   * <p>GL_LUMINANCE           Cv = Cf*(1 - Lt) + Cc*Lt   Cv = Lt
   *                        Av = Af                    Av = Af
   *
   * <p>GL_LUMINANCE_ALPHA     Cv = Cf*(1 - Lt) + Cc*Lt   Cv = Lt
   *                        Av = Af*At                 Av = At
   *
   * <p>GL_RGB                 Cv = Cf*(1 - Ct) + Cc*Ct   Cv = Ct
   *                        Av = Af                    Av = Af
   *
   * <p>GL_RGBA                Cv = Cf*(1 - Ct) + Cc*Ct   Cv = Ct
   *                        Av = Af*At                 Av = At
   *
   * <p>Base internal          Texture functions
   * format                 GL_ADD
   *
   * <p>GL_ALPHA               Cv = Cf
   *                        Av = Af*At
   *
   * <p>GL_LUMINANCE           Cv = Cf + Lt
   *                        Av = Af
   *
   * <p>GL_LUMINANCE_ALPHA     Cv = Cf + Lt
   *                        Av = Af*At
   *
   * <p>GL_RGB                 Cv = Cf + Ct
   *                        Av = Af
   *
   * <p>GL_RGBA                Cv = Cf + Ct
   *                        Av = Af*At
   * </pre>
   *
   * <p>If <code>pname</code> is <code>GL_TEXTURE_ENV_COLOR</code>,
   * <code>params</code> is a pointer to an array that holds an RGBA
   * color consisting of four values. The values are clamped to the
   * range <code>[0, 1]</code> when they are specified. <i>Cc</i>
   * takes these four values.
   *
   * <p>The initial value of <code>GL_TEXTURE_ENV_MODE</code> is
   * <code>GL_MODULATE</code>. The initial value of
   * <code>GL_TEXTURE_ENV_COLOR</code> is (0, 0, 0, 0).
   *
   * <h4>1.1 Notes</h4>
   *
   * <p> If the value of <code>GL_TEXTURE_ENV_MODE</code> is
   * <code>GL_COMBINE</code>, then the form of the texture function
   * depends on the values of <code>GL_COMBINE_RGB</code> and
   * <code>GL_COMBINE_ALPHA</code>.
   *
   * <p>The RGB and ALPHA results of the texture function are then
   * multiplied by the values of <code>GL_RGB_SCALE</code> and
   * <code>GL_ALPHA_SCALE</code>, respectively.
   *
   * <p>The results are clamped to <code>[0, 1]</code>.
   *
   * <p>The arguments <i>Arg0</i>, <i>Arg1</i>, <i>Arg2</i> are
   * determined by the values of
   * <code>GL_SRC</code><i>n</i><code>_RGB</code>,
   * <code>GL_SRC</code><i>n</i><code>_ALPHA</code>,
   * <code>GL_OPERAND</code><i>n</i><code>_RGB</code>,
   * <code>GL_OPERAND</code><i>n</i><code>_ALPHA</code>, where
   * <i>n</i> = 0, 1, or 2, <i>Cs</i> and <i>As</i> denote the texture
   * source color and alpha from the texture image bound to texture
   * unit <i>n</i>.
   *
   * <p>The state required for the current texture environment, for
   * each texture unit, consists of a six-valued integer indicating
   * the texture function, an eight-valued integer indicating the RGB
   * combiner function and a six-valued integer indicating the ALPHA
   * combiner function, six four-valued integers indicating the
   * combiner RGB and ALPHA source arguments, three four-valued
   * integers indicating the combiner RGB operands, three two-valued
   * integers indicating the combiner ALPHA operands, and four
   * floating-point environment color values. In the initial state,
   * the texture and combiner functions are each
   * <code>GL_MODULATE</code>, the combiner RGB and ALPHA sources are
   * each <code>GL_TEXTURE</code>, <code>GL_PREVIOUS</code>, and
   * <code>GL_CONSTANT</code> for sources 0, 1, and 2 respectively,
   * the combiner RGB operands for sources 0 and 1 are each
   * <code>SRC_COLOR</code>, the combiner RGB operand for source 2, as
   * well as for the combiner ALPHA operands, are each
   * <code>GL_SRC_ALPHA</code>, and the environment color is (0, 0, 0,
   * 0).
   *
   * <p>The state required for the texture filtering parameters, for
   * each texture unit, consists of a single floating-point level of
   * detail bias. The initial value of the bias is 0.0.
   *
   * <p>If <code>pname</code> is <code>GL_TEXTURE_ENV_COLOR</code>,
   * then <code>params</code> is an array that holds an RGBA color
   * consisting of four values. The values are clamped to the range
   * <code>[0, 1]</code> when they are specified. <i>Cc</i> takes
   * these four values.
   *
   * <p>The initial value of <code>GL_TEXTURE_ENV_MODE</code> is
   * <code>GL_MODULATE</code>. The initial value of
   * <code>GL_TEXTURE_ENV_COLOR</code> is (0, 0, 0, 0).
   *
   * <p>If <code>target</code> is <code>GL_POINT_SPRITE_OES</code>
   * then the following applies:
   *
   * <p>If <code>pname</code> is <code>GL_COORD_REPLACE_OES</code>,
   * then the point sprite texture coordinate replacement mode is set
   * from the value given by param, which may either be
   * <code>GL_FALSE</code> or <code>GL_TRUE</code>. The default value
   * for each texture unit is for point sprite texture coordinate
   * replacement to be disabled.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated when target or pname
   * is not one of the accepted values, or when <code>params</code>
   * should have a defined constant value (based on the value of
   * <code>pname</code>) and does not.
   *
   * <h4>Associated Gets (1.1 only)</h4>
   *
   * <p><code>glGetTexEnv</code>
   *
   * @param target Specifies a texture environment. Can be either
   * <code>GL_TEXTURE_ENV</code> or <code>GL_POINT_SPRITE_OES</code>
   * (<code>OES_point_sprite</code> extension).
   * @param pname Specifies the symbolic name of a single-valued
   * texture environment parameter. Must be one of
   * <code>GL_TEXTURE_ENV_MODE</code>,
   * <code>GL_TEXTURE_ENV_COLOR</code> (1.1 only),
   * <code>GL_COMBINE_RGB</code> (1.1 only),
   * <code>GL_COMBINE_ALPHA</code> (1.1 only), or
   * <code>GL_COORD_REPLACE_OES</code> (<code>OES_point_sprite</code>
   * extension).
   * @param param Specifies a single symbolic constant, one of
   * <code>GL_REPLACE</code>, <code>GL_MODULATE</code>,
   * <code>GL_DECAL</code>, <code>GL_BLEND</code>, or
   * <code>GL_ADD</code> (1.1 only).
   */
  void glTexEnvf(int target, int pname, float param);

  /**
   * Set texture environment parameters (array version).
   *
   * <p>A texture environment specifies how texture values are
   * interpreted when a fragment is textured. <code>target</code> must
   * be <code>GL_TEXTURE_ENV</code>. <code>pname</code> can be either
   * <code>GL_TEXTURE_ENV_MODE</code> or
   * <code>GL_TEXTURE_ENV_COLOR</code>.
   *
   * <p>If <code>pname</code> is <code>GL_TEXTURE_ENV_MODE</code>,
   * then <code>params</code> contains the symbolic name of a texture
   * function. Four texture functions may be specified:
   * <code>GL_MODULATE</code>, <code>GL_DECAL</code>,
   * <code>GL_BLEND</code>, and <code>GL_REPLACE</code>.
   *
   * <p>A texture function acts on the fragment to be textured using
   * the texture image value that applies to the fragment (see
   * <code>glTexParameter</code>) and produces an RGBA color for that
   * fragment. The following table shows how the RGBA color is
   * produced for each of the three texture functions that can be
   * chosen. C is a triple of color values (RGB) and A is the
   * associated alpha value. RGBA values extracted from a texture
   * image are in the range <code>[0, 1]</code>. The subscript
   * <i>f</i> refers to the incoming fragment, the subscript <i>t</i>
   * to the texture image, the subscript <i>c</i> to the texture
   * environment color, and subscript <i>v</i> indicates a value
   * produced by the texture function.
   *
   * <p>A texture image can have up to four components per texture
   * element (see <code>glTexImage2D</code>, and
   * <code>glCopyTexImage2D</code>). In a one-component image,
   * <i>Lt</i> indicates that single component. A two-component image
   * uses <i>Lt</i> and <i>At</i>. A three-component image has only a
   * color value, <i>Ct</i>. A four-component image has both a color
   * value <i>Ct</i> and an alpha value <i>At</i>.
   *
   * <pre>
   * Base internal          Texture functions
   * format                 GL_MODULATE                GL_DECAL
   *
   * <p>GL_ALPHA               Cv = Cf                    undefined
   *                        Av = At Af      
   *
   * <p>GL_LUMINANCE           Cv = Lt Cf                 undefined
   *                        Av = Af         
   *
   * <p>GL_LUMINANCE_ALPHA     Cv = Lt Cf                 undefined
   *                        Av = At Af      
   *
   * <p>GL_RGB                 Cv = Ct Cf                 Cv = Ct
   *                        Av = Af                    Av = Af
   *
   * <p>GL_RGBA                Cv = Ct Cf                 Cv = (1 - At) Cf + At Ct
   *                        Av = At Af                 Av = Af
   *
   * <p>Base internal          Texture functions
   * format                 GL_BLEND                   GL_REPLACE
   *
   * <p>GL_ALPHA               Cv = Cf                    Cv = Cf
   *                        Av = At Af                 Av = At
   *
   * <p>GL_LUMINANCE           Cv = (1 - Lt) Cf + Lt Cc   Cv = Lt
   *                        Av = Af                    Av = Af
   *
   * <p>GL_LUMINANCE_ALPHA     Cv = (1 - Lt) Cf + Lt Cc   Cv = Lt
   *                        Av = At Af                 Av = At
   *
   * <p>GL_RGB                 Cv = (1 - Ct) Cf + Ct Cc   Cv = Ct
   *                        Av = Af                    Av = Af
   *
   * <p>GL_RGBA                Cv = (1 - Ct) Cf + Ct Cc   Cv = Ct
   *                        Av = At Af                 Av = At
   * </pre>
   *
   * <p>If <code>pname</code> is <code>GL_TEXTURE_ENV_COLOR</code>,
   * <code>params</code> holds an RGBA color consisting of four
   * values. The values are clamped to the range <code>[0, 1]</code>
   * when they are specified. <i>Cc</i> takes these four values.
   *
   * <p>The initial value of <code>GL_TEXTURE_ENV_MODE</code> is
   * <code>GL_MODULATE</code>. The initial value of
   * <code>GL_TEXTURE_ENV_COLOR</code> is (0, 0, 0, 0).
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated when target or pname
   * is not one of the accepted values, or when <code>params</code>
   * should have a defined constant value (based on the value of
   * <code>pname</code>) and does not.
   *
   * @param target Specifies a texture environment. Must be
   * <code>GL_TEXTURE_ENV</code>.
   * @param pname Specifies the symbolic name of a texture environment
   * parameter. Accepted values are <code>GL_TEXTURE_ENV_MODE</code> and
   * <code>GL_TEXTURE_ENV_COLOR</code>.
   * @param params Specifies a parameter array that contains either a
   * single symbolic constant or an RGBA color.
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
  void glTexEnvfv(int target, int pname, float[] params, int offset);

  /**
   * Floating-point <code>Buffer</code> version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvfv(int target, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexEnvfv(int target, int pname, FloatBuffer params);

  /**
   * Fixed-point version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvf(int target, int pname, float param)
   */
  void glTexEnvx(int target, int pname, int param);

  /**
   * Fixed-point array version of <code>glTexEnv</code>.
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
  void glTexEnvxv(int target, int pname, int[] params, int offset);

  /**
   * Fixed-point <code>Buffer</code> version of <code>glTexEnv</code>.
   *
   * @see #glTexEnvfv(int target, int pname, float[] params, int offset)
   *
   * @exception IllegalArgumentException if <code>params</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if
   * <code>params.remaining()</code> is smaller than the number of
   * values required by the parameter.
   */
  void glTexEnvxv(int target, int pname, IntBuffer params);

  /**
   * Specify a two-dimensional texture image.
   *
   * <p>Texturing maps a portion of a specified texture image onto
   * each graphical primitive for which texturing is enabled. To
   * enable and disable two-dimensional texturing, call
   * <code>glEnable</code> and glDisable with argument
   * <code>GL_TEXTURE_2D</code>. Two-dimensional texturing is
   * initially disabled.
   *
   * <p>To define texture images, call <code>glTexImage2D</code>. The arguments
   * describe the parameters of the texture image, such as height,
   * width, width of the border, level-of-detail number (see
   * glTexParameter), and number of color components provided. The
   * last three arguments describe how the image is represented in
   * memory.
   *
   * <p>Data is read from pixels as a sequence of unsigned bytes or
   * shorts, depending on type. These values are grouped into sets of
   * one, two, three, or four values, depending on format, to form
   * elements.
   *
   * <p>When type is <code>GL_UNSIGNED_BYTE</code>, each of these
   * bytes is interpreted as one color component, depending on
   * format. When type is one of <code>GL_UNSIGNED_SHORT_5_6_5</code>,
   * <code>GL_UNSIGNED_SHORT_4_4_4_4</code>,
   * <code>GL_UNSIGNED_SHORT_5_5_5_1</code>, each unsigned value is
   * interpreted as containing all the components for a single pixel,
   * with the color components arranged according to format.
   *
   * <p>The first element corresponds to the lower left corner of the
   * texture image. Subsequent elements progress left-to-right through
   * the remaining texels in the lowest row of the texture image, and
   * then in successively higher rows of the texture image. The final
   * element corresponds to the upper right corner of the texture
   * image.
   *
   * <p>By default, adjacent pixels are taken from adjacent memory
   * locations, except that after all width pixels are read, the read
   * pointer is advanced to the next four-byte boundary. The four-byte
   * row alignment is specified by <code>glPixelStore</code> with argument
   * <code>GL_UNPACK_ALIGNMENT</code>, and it can be set to one, two, four, or
   * eight bytes.
   *
   * <p>format determines the composition of each element in pixels. It
   * can assume one of the following symbolic values:
   *
   * <ul>
   *
   * <li><code>GL_ALPHA</code></li>
   *
   * <p>Each element is a single alpha component. The GL converts it to
   * floating point and assembles it into an RGBA element by attaching
   * 0 for red, green, and blue.
   *
   * <li><code>GL_RGB</code></li>
   *
   * <p>Each element is an RGB triple. The GL converts it to fixed-point
   * or floating-point and assembles it into an RGBA element by
   * attaching 1 for alpha.
   *
   * <li><code>GL_RGBA</code></li>
   *
   * <p>Each element contains all four components. The GL converts it to
   * fixed-point or floating-point.
   *
   * <li><code>GL_LUMINANCE</code></li>
   *
   * <p>Each element is a single luminance value. The GL converts it to
   * fixed-point or floating-point, then assembles it into an RGBA
   * element by replicating the luminance value three times for red,
   * green, and blue and attaching 1 for alpha.
   *
   * <li><code>GL_LUMINANCE_ALPHA</code></li>
   *
   * <p>Each element is a luminance/alpha pair. The GL converts it to
   * fixed-point or floating point, then assembles it into an RGBA
   * element by replicating the luminance value three times for red,
   * green, and blue.
   * 
   * </ul>
   *
   * <h4>Notes</h4>
   *
   * <p>pixels may be NULL. In this case texture memory is allocated to
   * accommodate a texture of width width and height height. You can
   * then download subtextures to initialize this texture memory. The
   * image is undefined if the user tries to apply an uninitialized
   * portion of the texture image to a primitive.
   *
   * <p><code>glTexImage2D</code> specifies the two-dimensional texture for the
   * currently bound texture specified with <code>glBindTexture</code>, and the
   * current texture unit, specified with <code>glActiveTexture</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if target is not
   * <code>GL_TEXTURE_2D</code>.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if format is not an
   * accepted constant.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if type is not a
   * type constant.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if level is less
   * than 0.
   *
   * <p><code>GL_INVALID_VALUE</code> may be generated if level is
   * greater than log2max, where max is the returned value of
   * <code>GL_MAX_TEXTURE_SIZE</code>.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if internalformat
   * is not an accepted constant.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if width or height
   * is less than 0 or greater than <code>GL_MAX_TEXTURE_SIZE</code>,
   * or if either cannot be represented as 2^k + 2*border for some
   * integer k.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if border is not 0.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if
   * internalformat and format are not the same.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if type is
   * <code>GL_UNSIGNED_SHORT_5_6_5</code> and format is not
   * <code>GL_RGB</code>.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if typeis one
   * of <code>GL_UNSIGNED_SHORT_4_4_4_4</code>, or
   * <code>GL_UNSIGNED_SHORT_5_5_5_1</code> and formatis not
   * <code>GL_RGBA</code>.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_SIZE</code>
   *
   * @param target Specifies the target texture. Must be
   * <code>GL_TEXTURE_2D</code>.
   * @param level Specifies the level-of-detail number. Level 0 is the
   * base image level. Level n is the nth mipmap reduction image. Must
   * be greater or equal 0.
   * @param internalformat Specifies the color components in the
   * texture. Must be same as format. The following symbolic values
   * are accepted: <code>GL_ALPHA</code>, <code>GL_RGB</code>,
   * <code>GL_RGBA</code>, <code>GL_LUMINANCE</code>, or
   * <code>GL_LUMINANCE_ALPHA</code>.
   * @param width Specifies the width of the texture image. Must be
   * 2^n + 2*border for some integer n. All implementations support
   * texture images that are at least 64 texels wide.
   * @param height Specifies the height of the texture image. Must be
   * 2^m + 2*border for some integer m. All implementations support
   * texture images that are at least 64 texels high.
   * @param border Specifies the width of the border. Must be 0.
   * @param format Specifies the format of the pixel data. Must be
   * same as internalformat. The following symbolic values are
   * accepted: <code>GL_ALPHA</code>, <code>GL_RGB</code>,
   * <code>GL_RGBA</code>, <code>GL_LUMINANCE</code>, and
   * <code>GL_LUMINANCE_ALPHA</code>.
   * @param type Specifies the data type of the pixel data. The
   * following symbolic values are accepted:
   * <code>GL_UNSIGNED_BYTE</code>,
   * <code>GL_UNSIGNED_SHORT_5_6_5</code>,
   * <code>GL_UNSIGNED_SHORT_4_4_4_4</code>, and
   * <code>GL_UNSIGNED_SHORT_5_5_5_1</code>.
   * @param pixels Specifies a pointer to the image data in memory.
   *
   * @exception IllegalArgumentException if <code>pixels</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>pixels</code> does
   * not contain the desired number of pixels.
   */
  void glTexImage2D(int target, int level,
                    int internalformat,
                    int width, int height,
                    int border, int format, int type,
                    Buffer pixels);

  /**
   * Set texture parameters.
   *
   * <p>Texture mapping is a technique that applies an image onto an
   * object's surface as if the image were a decal or cellophane
   * shrink-wrap. The image is created in texture space, with an (s,
   * t) coordinate system. A texture is a one- or two-dimensional
   * image and a set of parameters that determine how samples are
   * derived from the image.
   *
   * <p><code>glTexParameter</code> assigns the value or values in
   * <code>param</code> to the texture parameter specified as
   * <code>pname</code>. <code>target</code> defines the target
   * texture, which must be <code>GL_TEXTURE_2D</code>. The following
   * symbols are accepted in <code>pname</code>:
   *
   * <ul>
   *
   * <li><code>GL_TEXTURE_MIN_FILTER</code></li>
   *
   * <p>The texture minifying function is used whenever the pixel being
   * textured maps to an area greater than one texture element. There
   * are six defined minifying functions. Two of them use the nearest
   * one or nearest four texture elements to compute the texture
   * value. The other four use mipmaps.
   *
   * <p>A mipmap is an ordered set of arrays representing the same image
   * at progressively lower resolutions. If the texture has dimensions
   * 2^n × 2^m, there are max (n, m) + 1 mipmaps. The first mipmap is
   * the original texture, with dimensions 2^n × 2^m. Each subsequent
   * mipmap has dimensions 2^(k - 1) × 2^(l - 1), where 2^k × 2^l are
   * the dimensions of the previous mipmap, until either k = 0 or l =
   * 0. At that point, subsequent mipmaps have dimension 1 × 2^(l - 1)
   * or 2^(k - 1) × 1 until the final mipmap, which has dimension 1 ×
   * 1. To define the mipmaps, call <code>glTexImage2D</code> or
   * glCopyTexImage2D with the level argument indicating the order of
   * the mipmaps. Level 0 is the original texture. Level max (n, m) is
   * the final 1 × 1 mipmap.
   *
   * <p><code>param</code> supplies a function for minifying the
   * texture as one of the following:
   * 
   * <ul>
   * <li><code>GL_NEAREST</code></li>
   *
   * <p>Returns the value of the texture element that is nearest (in
   * Manhattan distance) to the center of the pixel being textured.
   *
   * <li><code>GL_LINEAR</code></li>
   *
   * <p>Returns the weighted average of the four texture elements that
   * are closest to the center of the pixel being textured. These can
   * include border texture elements, depending on the values of
   * <code>GL_TEXTURE_WRAP_S</code> and
   * <code>GL_TEXTURE_WRAP_T</code>, and on the exact mapping.
   *
   * <li><code>GL_NEAREST_MIPMAP_NEAREST</code></li>
   *
   * <p>Chooses the mipmap that most closely matches the size of the
   * pixel being textured and uses the <code>GL_NEAREST</code>
   * criterion (the texture element nearest to the center of the
   * pixel) to produce a texture value.
   *
   * <li><code>GL_LINEAR_MIPMAP_NEAREST</code></li>
   *
   * <p>Chooses the mipmap that most closely matches the size of the
   * pixel being textured and uses the <code>GL_LINEAR</code>
   * criterion (a weighted average of the four texture elements that
   * are closest to the center of the pixel) to produce a texture
   * value.
   *
   * <li><code>GL_NEAREST_MIPMAP_LINEAR</code></li>
   *
   * <p>Chooses the two mipmaps that most closely match the size of the
   * pixel being textured and uses the <code>GL_NEAREST</code> criterion (the
   * texture element nearest to the center of the pixel) to produce a
   * texture value from each mipmap. The final texture value is a
   * weighted average of those two values.
   *
   * <li><code>GL_LINEAR_MIPMAP_LINEAR</code></li>
   *
   * <p>Chooses the two mipmaps that most closely match the size of the
   * pixel being textured and uses the <code>GL_LINEAR</code>
   * criterion (a weighted average of the four texture elements that
   * are closest to the center of the pixel) to produce a texture
   * value from each mipmap. The final texture value is a weighted
   * average of those two values.
   *
   * </ul>
   *
   * <p>As more texture elements are sampled in the minification process,
   * fewer aliasing artifacts will be apparent. While the
   * <code>GL_NEAREST</code> and <code>GL_LINEAR</code> minification
   * functions can be faster than the other four, they sample only one
   * or four texture elements to determine the texture value of the
   * pixel being rendered and can produce moire patterns or ragged
   * transitions.
   *
   * <p>The initial value of <code>GL_TEXTURE_MIN_FILTER</code> is
   * <code>GL_NEAREST_MIPMAP_LINEAR</code>.
   *
   * <li><code>GL_TEXTURE_MAG_FILTER</code></li>
   *
   * <p>The texture magnification function is used when the pixel being
   * textured maps to an area less than or equal to one texture
   * element. It sets the texture magnification function to either
   * <code>GL_NEAREST</code> or <code>GL_LINEAR</code> (see
   * below). <code>GL_NEAREST</code> is generally faster than
   * <code>GL_LINEAR</code>, but it can produce textured images with
   * sharper edges because the transition between texture elements is
   * not as smooth.
   *
   * <p>The initial value of <code>GL_TEXTURE_MAG_FILTER</code> is
   * <code>GL_LINEAR</code>.
   *
   * <li><code>GL_NEAREST</code></li>
   *
   * <p>Returns the value of the texture element that is nearest (in
   * Manhattan distance) to the center of the pixel being textured.
   *
   * <li><code>GL_LINEAR</code></li>
   *
   * <p>Returns the weighted average of the four texture elements that
   * are closest to the center of the pixel being textured. These can
   * include border texture elements, depending on the values of
   * <code>GL_TEXTURE_WRAP_S</code> and
   * <code>GL_TEXTURE_WRAP_T</code>, and on the exact mapping.
   *
   * <li><code>GL_TEXTURE_WRAP_S</code></li>
   *
   * <p>Sets the wrap parameter for texture coordinate <i>s</i> to
   * either <code>GL_CLAMP</code>, <code>GL_CLAMP_TO_EDGE</code>, or
   * <code>GL_REPEAT</code> <!-- , or <code>GL_MIRRORED_REPEAT</code>
   * (<code>OES_texture_mirrored_repeat</code>
   * extension) -->. <code>GL_CLAMP</code> causes <i>s</i> coordinates to
   * be clamped to the range <code>[0, 1]</code> and is useful for
   * preventing wrapping artifacts when mapping a single image onto an
   * object. <code>GL_CLAMP_TO_EDGE</code> causes <i>s</i> coordinates
   * to be clamped to the range <code>[1/(2<i>N</i>), 1 -
   * 1/(2<i>N</i>)]</code>, where <i>N</i> is the size of the texture
   * in the direction of clamping. <code>GL_REPEAT</code> causes the
   * integer part of the <i>s</i> coordinate to be ignored; the GL
   * uses only the fractional part, thereby creating a repeating
   * pattern. Border texture elements are accessed only if wrapping is
   * set to <code>GL_CLAMP</code>.
   *
   * <p>Initially, <code>GL_TEXTURE_WRAP_S</code> is set to
   * <code>GL_REPEAT</code>.
   *
   * <li><code>GL_TEXTURE_WRAP_T</code></li>
   *
   * <p>Sets the wrap parameter for texture coordinate <i>t</i> to
   * either <code>GL_CLAMP</code>, <code>GL_CLAMP_TO_EDGE</code>, or
   * <code>GL_REPEAT</code> <!-- , or <code>GL_MIRRORED_REPEAT</code>
   * (<code>OES_texture_mirrored_repeat</code> extension) -->. See the
   * discussion under <code>GL_TEXTURE_WRAP_S</code>.
   *
   * <p>Initially, <code>GL_TEXTURE_WRAP_T</code> is set to
   * <code>GL_REPEAT</code>.
   *
   * <li><code>GL_GENERATE_MIPMAP</code> (1.1 only)</li>
   *
   * <p>Sets the automatic mipmap generation parameter. If set to
   * <code>GL_TRUE</code>, making any change to the interior or border
   * texels of the levelbase array of a mipmap will also compute a
   * complete set of mipmap arrays derived from the modified levelbase
   * array. Array levels <i>levelbase + 1</i> through <i>p</i> are
   * replaced with the derived arrays, regardless of their previous
   * contents. All other mipmap arrays, including the levelbase array,
   * are left unchanged by this computation.
   *
   * <p>The initial value of <code>GL_GENERATE_MIPMAP</code> is
   * <code>GL_FALSE</code>.
   *
   * </ul>
   *
   * <h4>Notes</h4>
   *
   * <p>Suppose that a program has enabled texturing (by calling
   * glEnable with argument <code>GL_TEXTURE_2D</code> and has set
   * <code>GL_TEXTURE_MIN_FILTER</code> to one of the functions that
   * requires a mipmap. If either the dimensions of the texture images
   * currently defined (with previous calls to
   * <code>glTexImage2D</code>, or glCopyTexImage2D) do not follow the
   * proper sequence for mipmaps (described above), or there are fewer
   * texture images defined than are needed, or the set of texture
   * images have differing numbers of texture components, then it is
   * as if texture mapping were disabled.
   *
   * <p>Linear filtering accesses the four nearest texture elements.
   *
   * <p><code>glTexParameter</code> specifies the texture parameters
   * for the active texture unit, specified by calling
   * <code>glActiveTexture</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if target or pname
   * is not one of the accepted defined values.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if param should have
   * a defined constant value (based on the value of pname) and does
   * not.
   *
   * @param target Specifies the target texture, which must be 
   * <code>GL_TEXTURE_2D</code>.
   * @param pname Specifies the symbolic name of a single-valued
   * texture parameter. <code>pname</code> can be one of the
   * following: <code>GL_TEXTURE_MIN_FILTER</code>,
   * <code>GL_TEXTURE_MAG_FILTER</code>,
   * <code>GL_TEXTURE_WRAP_S</code>, <code>GL_TEXTURE_WRAP_T</code>, or
   * <code>GL_GENERATE_MIPMAP</code> (1.1 only) <!--, or
   * <code>GL_TEXTURE_CROP_RECT_OES</code>
   * (<code>OES_draw_texture</code> extension) -->.
   * @param param Specifies the value of <code>pname</code>.
   */
  void glTexParameterf(int target, int pname, float param);

  /**
   * Fixed-point version of <code>glTexParameter</code>.
   *
   * @see #glTexParameterf(int target, int pname, float param)
   */
  void glTexParameterx(int target, int pname, int param);

  /**
   * Specify a two-dimensional texture subimage.
   *
   * <p>Texturing maps a portion of a specified texture image onto
   * each graphical primitive for which texturing is enabled. To
   * enable and disable two-dimensional texturing, call
   * <code>glEnable</code> and glDisable with argument
   * <code>GL_TEXTURE_2D</code>. Two-dimensional texturing is
   * initially disabled.
   *
   * <p><code>glTexSubImage2D</code> redefines a contiguous subregion
   * of an existing two-dimensional texture image. The texels
   * referenced by pixels replace the portion of the existing texture
   * array with x indices xoffset and xoffset + width - 1, inclusive,
   * and y indices yoffset and yoffset + height - 1, inclusive. This
   * region may not include any texels outside the range of the
   * texture array as it was originally specified. It is not an error
   * to specify a subtexture with zero width or height, but such a
   * specification has no effect.
   *
   * <h4>Notes</h4>
   *
   * <p><code>glPixelStore</code> affects texture images in exactly
   * the way it affects <code>glTexImage2D</code>.
   *
   * <p><code>glTexSubImage2D</code> specifies a two-dimensional sub
   * texture for the currently bound texture, specified with
   * <code>glBindTexture</code> and current texture unit, specified
   * with <code>glActiveTexture</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if target is not
   * <code>GL_TEXTURE_2D</code>.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if the texture
   * array has not been defined by a previous
   * <code>glTexImage2D</code> or glCopyTexImage2D operation.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if level is less
   * than 0.
   *
   * <p><code>GL_INVALID_VALUE</code> may be generated if level is
   * greater than log2max, where max is the returned value of
   * <code>GL_MAX_TEXTURE_SIZE</code>.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if xoffset < - b,
   * xoffset + width > (w - b) , yoffset < - b, or yoffset + height >
   * (h - b) , where w is the texture width, h is the texture height,
   * and b is the border of the texture image being modified. Note
   * that w and h include twice the border width.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if width or height
   * is less than 0.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if format is not an
   * accepted constant.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if type is not a
   * type constant.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if type is
   * <code>GL_UNSIGNED_SHORT_5_6_5</code> and format is not
   * <code>GL_RGB</code>.
   *
   * <p><code>GL_INVALID_OPERATION</code> is generated if type is one
   * of <code>GL_UNSIGNED_SHORT_4_4_4_4</code>, or
   * <code>GL_UNSIGNED_SHORT_5_5_5_1</code> and format is not
   * <code>GL_RGBA</code>.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_TEXTURE_SIZE</code>
   *
   * @param target Specifies the target texture. Must be
   * <code>GL_TEXTURE_2D</code>.
   * @param level Specifies the level-of-detail number. Level 0 is the
   * base image level. Level n is the nth mipmap reduction image.
   * @param xoffset Specifies a texel offset in the x direction within
   * the texture array.
   * @param yoffset Specifies a texel offset in the y direction within
   * the texture array.
   * @param width Specifies the width of the texture subimage.
   * @param height Specifies the height of the texture subimage.
   * @param format Specifies the of the pixel data. The following
   * symbolic values are accepted: <code>GL_ALPHA</code>,
   * <code>GL_RGB</code>, <code>GL_RGBA</code>,
   * <code>GL_LUMINANCE</code>, and <code>GL_LUMINANCE_ALPHA</code>.
   * @param type Specifies the data type of the pixel data. The
   * following symbolic values are accepted:
   * <code>GL_UNSIGNED_BYTE</code>,
   * <code>GL_UNSIGNED_SHORT_5_6_5</code>,
   * <code>GL_UNSIGNED_SHORT_4_4_4_4</code>, and
   * <code>GL_UNSIGNED_SHORT_5_5_5_1</code>.
   * @param pixels Specifies the image data.
   *
   * @exception IllegalArgumentException if <code>pixels</code> is
   * <code>null</code>.
   * @exception IllegalArgumentException if <code>pixels</code> does
   * not contain the desired number of pixels.
   */
  void glTexSubImage2D(int target, int level,
		       int xoffset, int yoffset,
		       int width, int height,
		       int format, int type,
		       Buffer pixels);

  /**
   * Multiply the current matrix by a translation matrix.
   *
   * <p><code>glTranslate</code> produces a translation by (x, y, z).
   * The current matrix (see glMatrixMode) is multiplied by this
   * translation matrix, with the product replacing the current
   * matrix, as if <code>glMultMatrix</code> were called with the
   * following matrix for its argument:
   *
   * <pre>
   * ( 1       0       0       x )
   * ( 0       1       0       y )
   * ( 0       0       1       z )
   * ( 0       0       0       1 )
   * </pre>
   *
   * <p>If the matrix mode is either <code>GL_MODELVIEW</code> or
   * <code>GL_PROJECTION</code>, all objects drawn after a call to
   * <code>glTranslate</code> are translated.
   *
   * <p>Use <code>glPushMatrix</code> and <code>glPopMatrix</code> to
   * save and restore the untranslated coordinate system.
   *
   * @param x Specifies the x coordinate of a translation vector.
   * @param y Specifies the y coordinate of a translation vector.
   * @param z Specifies the z coordinate of a translation vector.
   */
  void glTranslatef(float x, float y, float z);

  /**
   * Fixed-point version of <code>glTranslate</code>.
   *
   * @see #glTranslatef
   */
  void glTranslatex(int x, int y, int z);

  // Need revisit - pointer == null
  /**
   * Define an array of vertex coordinates.
   *
   * <p><code>glVertexPointer</code> specifies the location and data
   * of an array of vertex coordinates to use when
   * rendering. <code>size</code> specifies the number of coordinates
   * per vertex and type the data type of the
   * coordinates. <code>stride</code> specifies the byte stride from
   * one vertex to the next allowing vertices and attributes to be
   * packed into a single array or stored in separate
   * arrays. (Single-array storage may be more efficient on some
   * implementations.)
   *
   * <p>When a vertex array is specified, <code>size</code>,
   * <code>type</code>, <code>stride</code>, and <code>pointer</code>
   * are saved as client-side state.
   *
   * <p>If the vertex array is enabled, it is used when
   * <code>glDrawArrays</code>, or <code>glDrawElements</code> is
   * called. To enable and disable the vertex array, call
   * <code>glEnableClientState</code> and
   * <code>glDisableClientState</code> with the argument
   * <code>GL_VERTEX_ARRAY</code>. The vertex array is initially
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
   * <p><code>glVertexPointer</code> is typically implemented on the
   * client side.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if size is not 2,
   * 3, or 4.
   *
   * <p><code>GL_INVALID_ENUM</code> is generated if type is is not an
   * accepted value.
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if stride is
   * negative.
   *
   * <p> The <code>pointer</code> argument must be a direct buffer
   * with a type matching that specified by the <code>type</code>
   * argument.
   *
   * @param size Specifies the number of coordinates per vertex. Must
   * be 2, 3, or 4. The initial value is 4.
   * @param type Specifies the data type of each vertex coordinate in
   * the array. Symbolic constants <code>GL_BYTE</code>,
   * <code>GL_SHORT</code>, <code>GL_FIXED</code>, and
   * <code>GL_FLOAT</code> are accepted. The initial value is
   * <code>GL_FLOAT</code>.
   * @param stride Specifies the byte offset between consecutive
   * vertices. If stride is 0, the vertices are understood to be
   * tightly packed in the array. The initial value is 0.
   * @param pointer Specifies a Buffer containing the coordinates of
   * the vertices.
   *
   * @exception IllegalStateException if OpenGL ES 1.1 is being used and
   * VBOs are enabled.
   * @exception IllegalArgumentException if <code>pointer</code>
   * is not direct.
   */
  void glVertexPointer(int size, int type, int stride, Buffer pointer);

  /**
   * Set the viewport.
   *
   * <p><code>glViewport</code> specifies the affine transformation of
   * x and y from normalized device coordinates to window
   * coordinates. Let (xnd, ynd) be normalized device
   * coordinates. Then the window coordinates (xw, yw) are computed as
   * follows:
   *
   * <pre>
   * xw = ( xnd + 1 ) width/2 + x
   * yw = ( ynd + 1 ) height/2 + y
   * </pre>
   *
   * <p>Viewport width and height are silently clamped to a range that
   * depends on the implementation. To query this range, call
   * <code>glGetIntegerv</code> with argument
   * <code>GL_MAX_VIEWPORT_DIMS</code>.
   *
   * <h4>Errors</h4>
   *
   * <p><code>GL_INVALID_VALUE</code> is generated if either width or
   * height is negative.
   *
   * <h4>Associated Gets</h4> 
   *
   * <p><code>glGetIntegerv</code> with argument
   * <code>GL_MAX_VIEWPORT_DIMS</code>
   *
   * @param x Specifies the x coordinate of the lower left corner of
   * the viewport rectangle, in pixels. The initial value is 0.
   * @param y Specifies the y coordinate of the lower left corner of
   * the viewport rectangle, in pixels. The initial value is 0.
   * @param width Specifies the width of the viewport. When a GL
   * context is first attached to a surface (e.g. window), width and
   * height are set to the dimensions of that surface.
   * @param height Specifies the height of the viewport. When a GL
   * context is first attached to a surface (e.g. window), width and
   * height are set to the dimensions of that surface.
   */
  void glViewport(int x, int y, int width, int height);
}

