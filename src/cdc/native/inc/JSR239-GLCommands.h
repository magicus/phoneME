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
   
#ifndef _GL_COMMANDS_H
#define _GL_COMMANDS_H

/*
 * This list must be synchronized with the one in 
 * com.sun.jsr239.GL11Impl.java.
 */
#define CMD_ACTIVE_TEXTURE                        1L
#define CMD_ALPHA_FUNC                            2L
#define CMD_ALPHA_FUNCX                           3L
#define CMD_BIND_BUFFER                           4L
#define CMD_BIND_TEXTURE                          5L
#define CMD_BLEND_FUNC                            6L
#define CMD_BUFFER_DATA                           7L
#define CMD_BUFFER_SUB_DATA                       8L
#define CMD_CLEAR                                 9L
#define CMD_CLEAR_COLOR                          10L
#define CMD_CLEAR_COLORX                         11L
#define CMD_CLEAR_DEPTHF                         12L
#define CMD_CLEAR_DEPTHX                         13L
#define CMD_CLEAR_STENCIL                        14L
#define CMD_CLIENT_ACTIVE_TEXTURE                15L
#define CMD_CLIP_PLANEF                          16L
#define CMD_CLIP_PLANEFB                         17L
#define CMD_CLIP_PLANEX                          18L
#define CMD_CLIP_PLANEXB                         19L
#define CMD_COLOR4F                              20L
#define CMD_COLOR4X                              21L
#define CMD_COLOR4UB                             22L
#define CMD_COLOR_MASK                           23L
#define CMD_COLOR_POINTER                        24L
#define CMD_COLOR_POINTER_VBO                    25L
#define CMD_COMPRESSED_TEX_IMAGE_2D              26L
#define CMD_COMPRESSED_TEX_SUB_IMAGE_2D          27L
#define CMD_COPY_TEX_IMAGE_2D                    28L
#define CMD_COPY_TEX_SUB_IMAGE_2D                29L
#define CMD_CULL_FACE                            30L
#define CMD_CURRENT_PALETTE_MATRIX               31L
#define CMD_DELETE_BUFFERS                       32L
#define CMD_DELETE_BUFFERSB                      33L
#define CMD_DELETE_TEXTURES                      34L
#define CMD_DELETE_TEXTURESB                     35L
#define CMD_DEPTH_FUNC                           36L
#define CMD_DEPTH_MASK                           37L
#define CMD_DEPTH_RANGEF                         38L
#define CMD_DEPTH_RANGEX                         39L
#define CMD_DISABLE                              40L
#define CMD_DISABLE_CLIENT_STATE                 41L
#define CMD_DRAW_ARRAYS                          42L
#define CMD_DRAW_ELEMENTSB                       43L
#define CMD_DRAW_ELEMENTS_VBO                    44L
#define CMD_DRAW_TEXF                            45L
#define CMD_DRAW_TEXFB                           46L
#define CMD_DRAW_TEXI                            47L
#define CMD_DRAW_TEXIB                           48L
#define CMD_DRAW_TEXS                            49L
#define CMD_DRAW_TEXSB                           50L
#define CMD_DRAW_TEXX                            51L
#define CMD_DRAW_TEXXB                           52L
#define CMD_ENABLE                               53L
#define CMD_ENABLE_CLIENT_STATE                  54L
#define CMD_FOGF                                 55L
#define CMD_FOGFB                                56L
#define CMD_FOGFV                                57L
#define CMD_FOGX                                 58L
#define CMD_FOGXB                                59L
#define CMD_FOGXV                                60L
#define CMD_FRONT_FACE                           61L
#define CMD_FRUSTUMF                             62L
#define CMD_FRUSTUMX                             63L
#define CMD_HINT                                 64L
#define CMD_LIGHTF                               65L
#define CMD_LIGHTFB                              66L
#define CMD_LIGHTFV                              67L
#define CMD_LIGHTX                               68L
#define CMD_LIGHTXB                              69L
#define CMD_LIGHTXV                              70L
#define CMD_LIGHT_MODELF                         71L
#define CMD_LIGHT_MODELFB                        72L
#define CMD_LIGHT_MODELFV                        73L
#define CMD_LIGHT_MODELX                         74L
#define CMD_LIGHT_MODELXB                        75L
#define CMD_LIGHT_MODELXV                        76L
#define CMD_LINE_WIDTH                           77L
#define CMD_LINE_WIDTHX                          78L
#define CMD_LOAD_IDENTITY                        79L
#define CMD_LOAD_MATRIXF                         80L
#define CMD_LOAD_MATRIXFB                        81L
#define CMD_LOAD_MATRIXX                         82L
#define CMD_LOAD_MATRIXXB                        83L
#define CMD_LOAD_PALETTE_FROM_MODEL_VIEW_MATRIX  84L
#define CMD_LOGIC_OP                             85L
#define CMD_MATERIALF                            86L
#define CMD_MATERIALFB                           87L
#define CMD_MATERIALFV                           88L
#define CMD_MATERIALX                            89L
#define CMD_MATERIALXB                           90L
#define CMD_MATERIALXV                           91L
#define CMD_MATRIX_INDEX_POINTER                 92L
#define CMD_MATRIX_INDEX_POINTER_VBO             93L
#define CMD_MATRIX_MODE                          94L
#define CMD_MULTI_TEXT_COORD4F                   95L
#define CMD_MULTI_TEXT_COORD4X                   96L
#define CMD_MULT_MATRIXF                         97L
#define CMD_MULT_MATRIXFB                        98L
#define CMD_MULT_MATRIXX                         99L
#define CMD_MULT_MATRIXXB                       100L
#define CMD_NORMAL3F                            101L
#define CMD_NORMAL3X                            102L
#define CMD_NORMAL_POINTER                      103L
#define CMD_NORMAL_POINTER_VBO                  104L
#define CMD_ORTHOF                              105L
#define CMD_ORTHOX                              106L
#define CMD_PIXEL_STOREI                        107L
#define CMD_POINT_PARAMETERF                    108L
#define CMD_POINT_PARAMETERFB                   109L
#define CMD_POINT_PARAMETERFV                   110L
#define CMD_POINT_PARAMETERX                    111L
#define CMD_POINT_PARAMETERXB                   112L
#define CMD_POINT_PARAMETERXV                   113L
#define CMD_POINT_SIZE                          114L
#define CMD_POINT_SIZEX                         115L
#define CMD_POINT_SIZE_POINTER                  116L
#define CMD_POINT_SIZE_POINTER_VBO              117L
#define CMD_POLYGON_OFFSET                      118L
#define CMD_POLYGON_OFFSETX                     119L
#define CMD_POP_MATRIX                          120L
#define CMD_PUSH_MATRIX                         121L
#define CMD_ROTATEF                             122L
#define CMD_ROTATEX                             123L
#define CMD_SAMPLE_COVERAGE                     124L
#define CMD_SAMPLE_COVERAGEX                    125L
#define CMD_SCALEF                              126L
#define CMD_SCALEX                              127L
#define CMD_SCISSOR                             128L
#define CMD_SHADE_MODEL                         129L
#define CMD_STENCIL_FUNC                        130L
#define CMD_STENCIL_MASK                        131L
#define CMD_STENCIL_OP                          132L
#define CMD_TEX_COORD_POINTER                   133L
#define CMD_TEX_COORD_POINTER_VBO               134L
#define CMD_TEX_ENVF                            135L
#define CMD_TEX_ENVFB                           136L
#define CMD_TEX_ENVFV                           137L
#define CMD_TEX_ENVI                            138L
#define CMD_TEX_ENVIB                           139L
#define CMD_TEX_ENVIV                           140L
#define CMD_TEX_ENVX                            141L
#define CMD_TEX_ENVXB                           142L
#define CMD_TEX_ENVXV                           143L
#define CMD_TEX_IMAGE_2D                        144L
#define CMD_TEX_PARAMETERF                      145L
#define CMD_TEX_PARAMETERFB                     146L
#define CMD_TEX_PARAMETERFV                     147L
#define CMD_TEX_PARAMETERI                      148L
#define CMD_TEX_PARAMETERIB                     149L
#define CMD_TEX_PARAMETERIV                     150L
#define CMD_TEX_PARAMETERX                      151L
#define CMD_TEX_PARAMETERXB                     152L
#define CMD_TEX_PARAMETERXV                     153L
#define CMD_TEX_SUB_IMAGE_2D                    154L
#define CMD_TRANSLATEF                          155L
#define CMD_TRANSLATEX                          156L
#define CMD_VERTEX_POINTER                      157L
#define CMD_VERTEX_POINTER_VBO                  158L
#define CMD_VIEWPORT                            159L
#define CMD_WEIGHT_POINTER                      160L
#define CMD_WEIGHT_POINTER_VBO                  161L
#define CMD_FINISH                              162L
#define CMD_FLUSH                               163L
#define CMD_TEX_GENF                            164L
#define CMD_TEX_GENI                            165L
#define CMD_TEX_GENX                            166L
#define CMD_TEX_GENFB                           167L
#define CMD_TEX_GENIB                           168L
#define CMD_TEX_GENXB                           169L
#define CMD_TEX_GENFV                           170L
#define CMD_TEX_GENIV                           171L
#define CMD_TEX_GENXV                           172L
#define CMD_BLEND_EQUATION                      173L
#define CMD_BLEND_FUNC_SEPARATE                 174L
#define CMD_BLEND_EQUATION_SEPARATE             175L
#define CMD_BIND_RENDERBUFFER                   176L
#define CMD_DELETE_RENDERBUFFERS                177L
#define CMD_DELETE_RENDERBUFFERSB               178L
#define CMD_GEN_RENDERBUFFERSB                  179L
#define CMD_RENDERBUFFER_STORAGE                180L
#define CMD_BIND_FRAMEBUFFER                    181L
#define CMD_DELETE_FRAMEBUFFERS                 182L
#define CMD_DELETE_FRAMEBUFFERSB                183L
#define CMD_GEN_FRAMEBUFFERSB                   184L
#define CMD_FRAMEBUFFER_TEXTURE2D               185L
#define CMD_FRAMEBUFFER_RENDERBUFFER            186L
#define CMD_GENERATE_MIPMAP                     187L
#define CMD_GEN_BUFFERSB                        188L
#define CMD_GEN_TEXTURESB                       189L

/**
 * This list must be synchronized with the one above. 
 */
#ifdef DEBUG
static const char *commandStrings[] = {
    NULL,
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
    "GEN_TEXTURESB",
};
#endif /* DEBUG */

#endif /* #ifndef _GL_COMMANDS_H */
