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

/* Native functions for
 * ..\src\com\sun\jsr239\GL10Impl.java
 */

#undef DEBUG

#ifdef DEBUG
#include <stdio.h>
#include <assert.h>
#endif

/* #define DEBUG_FLUSH_OR_FINISH do { glFlush(); } while(0) */
/* #define DEBUG_FLUSH_OR_FINISH do { glFinish(); } while(0) */
#define DEBUG_FLUSH_OR_FINISH do { /* no-op */ } while(0)

#include <kni.h>
#include <gl.h>
#include "JSR239-GLLimits.h"
#include "JSR239-GLCommands.h"
#include "JSR239-KNIInterface.h"

/*
 * The maximum number of entries in the GL command queue.
 */
#define GL_COMMAND_QUEUE_SIZE 4096L

static void
runQueue(int *q, int count)

{
    int i = 0;
    GLfloat *fq = (GLfloat *)q;

#ifdef DEBUG
        printf("Executing queue containing %d entries:\n", count);
        fflush(stdout);
#endif

    while (i < count) {
#ifdef DEBUG
        printf("-> Executing command %s (%d entries left)\n",
	       commandStrings[q[i]], count - i);
        fflush(stdout);
#endif

        switch(q[i++]) {
        case CMD_ACTIVE_TEXTURE:
            {
                GLint texture = (GLint)q[i++];
                glActiveTexture(texture);
#ifdef DEBUG
                printf("glActiveTexture(0x%x)\n", texture);
                fflush(stdout);
#endif
            }
            break;

        case CMD_ALPHA_FUNC:
            {
                GLenum func = (GLenum)q[i++];
                GLclampf ref = (GLclampf)fq[i++];
                glAlphaFunc(func, ref);
            }
            break;

        case CMD_ALPHA_FUNCX:
            {
                GLenum func = (GLenum)q[i++];
                GLclampx ref = (GLfixed)q[i++];
                glAlphaFuncx(func, ref);
            }
            break;
      
        case CMD_BIND_BUFFER:
            {
                GLenum _e = (GLenum)q[i++];
                GLuint _u = (GLuint)q[i++];
#ifdef DEBUG
                printf("glBindBuffer(0x%x, 0x%x)\n", _e, _u);
                fflush(stdout);
#endif
                glBindBuffer(_e, _u);
            }
            break;

        case CMD_BIND_TEXTURE:
            {
                GLenum target = (GLenum)q[i++];
                GLuint texture = (GLuint)q[i++];
                glBindTexture(target, texture);
#ifdef DEBUG
                printf("glBindTexture(0x%x, 0x%x)\n", target, texture);
                fflush(stdout);
#endif
            }
            break;

        case CMD_BLEND_FUNC:
            {
                GLenum sfactor = (GLenum)q[i++];
                GLenum dfactor = (GLenum)q[i++];
                glBlendFunc(sfactor, dfactor);
            }
            break;

        case CMD_BUFFER_DATA:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLsizeiptr _siptr = (GLsizeiptr)q[i++];
                const GLvoid *_buf = (const GLvoid *)q[i++];
                GLenum _e2 = (GLenum)q[i++];
#ifdef DEBUG
                printf("glBufferData(0x%x, 0x%x, 0x%p, 0x%x)\n", _e1, _siptr, _buf, _e2);
                fflush(stdout);
#endif
                glBufferData(_e1, _siptr, _buf, _e2);
            }
            break;

        case CMD_BUFFER_SUB_DATA:
            {
                GLenum _e = (GLenum)q[i++];
                GLintptr _iptr = (GLintptr)q[i++];
                GLsizeiptr _siptr = (GLsizeiptr)q[i++];
                const GLvoid *_buf = (const GLvoid *)q[i++];

                glBufferSubData(_e, _iptr, _siptr, _buf);
            }
            break;

        case CMD_CLEAR:
            {
                GLbitfield mask = (GLbitfield)q[i++];
#ifdef DEBUG
                printf("glClear(0x%x)\n", mask);
                fflush(stdout);
#endif
                glClear(mask);
            }
            break;

        case CMD_CLEAR_COLOR:
            {
                GLclampf red = (GLclampf)fq[i++];
                GLclampf green = (GLclampf)fq[i++];
                GLclampf blue = (GLclampf)fq[i++];
                GLclampf alpha = (GLclampf)fq[i++];
#ifdef DEBUG
                printf("glClearColor(%f, %f, %f, %f)\n", red, green, blue, alpha);
                fflush(stdout);
#endif
                glClearColor(red, green, blue, alpha);
            }
            break;

        case CMD_CLEAR_COLORX:
            {
                GLfixed red = (GLfixed)q[i++];
                GLfixed green = (GLfixed)q[i++];
                GLfixed blue = (GLfixed)q[i++];
                GLfixed alpha = (GLfixed)q[i++];
#ifdef DEBUG
                printf("glClearColorx(%d, %d, %d, %d)\n",
                       red, ///65536.0f,
                       green, ///65536.0f,
                       blue, ///65536.0f,
                       alpha); ///65536.0f);
                fflush(stdout);
#endif
                glClearColorx(red, green, blue, alpha);
            }
            break;

        case CMD_CLEAR_DEPTHF:
            {
                GLclampf depth = (GLclampf)fq[i++];
                glClearDepthf(depth);
            }
            break;

        case CMD_CLEAR_DEPTHX:
            {
                GLclampx depth = (GLclampx)q[i++];
                glClearDepthx(depth);
            }
            break;

        case CMD_CLEAR_STENCIL:
            {
                GLint s = (GLint)q[i++];
                glClearStencil(s);
            }
            break;

        case CMD_CLIENT_ACTIVE_TEXTURE:
            {
                GLenum texture = (GLenum)q[i++];
                glClientActiveTexture(texture);
            }
            break;

        case CMD_CLIP_PLANEF:
            {
                GLenum plane = (GLenum)q[i++];
                const GLfloat *equation = (const GLfloat *)&fq[i];
                glClipPlanef(plane, equation);
                i += 4;
            }
            break;

        case CMD_CLIP_PLANEFB:
            {
                GLenum plane = (GLenum)q[i++];
                const GLfloat *equation = (const GLfloat *)q[i++];
                glClipPlanef(plane, equation);
            }
            break;

        case CMD_CLIP_PLANEX:
            {
                GLenum plane = (GLenum)q[i++];
                const GLfixed *equation = (const GLfixed *)&q[i];
                glClipPlanex(plane, equation);
                i += 4;
            }
            break;

        case CMD_CLIP_PLANEXB:
            {
                GLenum plane = (GLenum)q[i++];
                const GLfixed *equation = (const GLfixed *)q[i++];
                glClipPlanex(plane, equation);
            }
            break;

        case CMD_COLOR4F:
            {
                GLfloat red = (GLfloat)fq[i++];
                GLfloat green = (GLfloat)fq[i++];
                GLfloat blue = (GLfloat)fq[i++];
                GLfloat alpha = (GLfloat)fq[i++];
#ifdef DEBUG
                printf("glColor4f(%f, %f, %f, %f)\n", red, green, blue, alpha);
                fflush(stdout);
#endif
                glColor4f(red, green, blue, alpha);
            }
            break;

        case CMD_COLOR4X:
            {
                GLfixed red = (GLfixed)q[i++];
                GLfixed green = (GLfixed)q[i++];
                GLfixed blue = (GLfixed)q[i++];
                GLfixed alpha = (GLfixed)q[i++];
#ifdef DEBUG
                printf("glColor4x(%f, %f, %f, %f)\n",
                       red, ///65536.0f,
                       green, ///65536.0f,
                       blue, ///65536.0f,
                       alpha); ///65536.0f);
                fflush(stdout);
#endif
                glColor4x(red, green, blue, alpha);
            }
            break;

        case CMD_COLOR4UB:
            {
                GLubyte red = (GLubyte)(GLint)q[i++];
                GLubyte green = (GLubyte)(GLint)q[i++];
                GLubyte blue = (GLubyte)(GLint)q[i++];
                GLubyte alpha = (GLubyte)(GLint)q[i++];
                glColor4ub(red, green, blue, alpha);
            }
            break;

        case CMD_COLOR_MASK:
            {
                GLboolean red = (GLboolean)q[i++];
                GLboolean green = (GLboolean)q[i++];
                GLboolean blue = (GLboolean)q[i++];
                GLboolean alpha = (GLboolean)q[i++];
                glColorMask(red, green, blue, alpha);
            }
            break;

        case CMD_COLOR_POINTER:
        case CMD_COLOR_POINTER_VBO:
            {
                GLint _i = (GLint)q[i++];
                GLenum _e = (GLenum)q[i++];
                GLsizei _s = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
#ifdef DEBUG
                printf("glColorPointer(0x%x, 0x%x, 0x%x, 0x%p)\n", _i, _e, _s, _ptr);
                fflush(stdout);
#endif
                glColorPointer(_i, _e, _s, _ptr);
            }
            break;

        case CMD_COMPRESSED_TEX_IMAGE_2D:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLint _i1 = (GLint)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLsizei _s1 = (GLsizei)q[i++];
                GLsizei _s2 = (GLsizei)q[i++];
                GLint _i2 = (GLint)q[i++];
                GLsizei _s3 = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
                glCompressedTexImage2D(_e1, _i1, _e2, _s1, _s2, _i2, _s3, _ptr);
            }
            break;

        case CMD_COMPRESSED_TEX_SUB_IMAGE_2D:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLint _i1 = (GLint)q[i++];
                GLenum _i2 = (GLenum)q[i++];
                GLenum _i3 = (GLenum)q[i++];
                GLsizei _s1 = (GLsizei)q[i++];
                GLsizei _s2 = (GLsizei)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLsizei _s3 = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
                glCompressedTexSubImage2D(_e1, _i1, _i2, _i3, _s1, _s2, _e2, _s3, _ptr);
            }
            break;

        case CMD_COPY_TEX_IMAGE_2D:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLint _i1 = (GLint)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLint _i2 = (GLint)q[i++];
                GLint _i3 = (GLint)q[i++];
                GLsizei _s1 = (GLsizei)q[i++];
                GLsizei _s2 = (GLsizei)q[i++];
                GLint _i4 = (GLint)q[i++];
                glCopyTexImage2D(_e1, _i1, _e2, _i2, _i3, _s1, _s2, _i4);
#ifdef DEBUG
                printf("glCopyTexImage2D(0x%x, 0x%x, 0x%x,\n", _e1, _i1, _e2);
                printf("                 0x%x, 0x%x, 0x%x,\n", _i2, _i3, _s1);
                printf("                 0x%x, 0x%x)\n", _s2, _i4);
                fflush(stdout);
#endif
            }
            break;

        case CMD_COPY_TEX_SUB_IMAGE_2D:
            {
                GLenum _e = (GLenum)q[i++];
                GLint _i1 = (GLint)q[i++];
                GLint _i2 = (GLint)q[i++];
                GLint _i3 = (GLint)q[i++];
                GLint _i4 = (GLint)q[i++];
                GLint _i5 = (GLint)q[i++];
                GLsizei _s1 = (GLsizei)q[i++];
                GLsizei _s2 = (GLsizei)q[i++];
                glCopyTexSubImage2D(_e, _i1, _i2, _i3, _i4, _i5, _s1, _s2);
#ifdef DEBUG
                printf("glCopyTexSubImage2D(0x%x, 0x%x, 0x%x,\n", _e, _i1, _i2);
                printf("                    0x%x, 0x%x, 0x%x,\n", _i3, _i4, _i5);
                printf("                    0x%x, 0x%x)\n", _s1, _s2);
                fflush(stdout);
#endif
            }
            break;

        case CMD_CULL_FACE:
            {
                GLint _i = (GLint)q[i++];
                glCullFace(_i);
            }
            break;

        case CMD_CURRENT_PALETTE_MATRIX:
            {
                GLuint _u = (GLuint)q[i++];
                glCurrentPaletteMatrixOES(_u);
            }
            break;

        case CMD_DELETE_BUFFERS:
            {
                GLsizei n = (GLsizei)q[i++];
                const GLuint *_uptr1 = (const GLuint *)&q[i];
                glDeleteBuffers(n, _uptr1);
                i += n;
            }
            break;

        case CMD_DELETE_BUFFERSB:
            {
                GLsizei n = (GLsizei)q[i++];
                const GLuint *_uptr2 = (const GLuint *)q[i++];
                glDeleteBuffers(n, _uptr2);
            }
            break;

        case CMD_DELETE_TEXTURES:
            {
                GLsizei n = (GLsizei)q[i++];
                const GLint *_textures = (const GLint *)&q[i];
                glDeleteTextures(n, _textures);
                i += n;
            }
            break;

        case CMD_DELETE_TEXTURESB:
            {
                GLsizei n = (GLsizei)q[i++];
                const GLint *_textures = (const GLint *)q[i++];
                glDeleteTextures(n, _textures);
            }
            break;

        case CMD_DEPTH_FUNC:
            {
                GLenum _e = (GLenum)q[i++];
                glDepthFunc(_e);
            }
            break;

        case CMD_DEPTH_MASK:
            {
                GLboolean _b = (GLboolean)q[i++];
                glDepthMask(_b);
            }
            break;

        case CMD_DEPTH_RANGEF:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                glDepthRangef(_f1, _f2);
            }
            break;

        case CMD_DEPTH_RANGEX:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                glDepthRangex(_x1, _x2);
            }
            break;

        case CMD_DISABLE:
            {
                GLenum _e = (GLenum)q[i++];
#ifdef DEBUG
                printf("glDisable(0x%x)\n", _e);
                fflush(stdout);
#endif
                glDisable(_e);
            }
            break;

        case CMD_DISABLE_CLIENT_STATE:
            {
                GLenum _e = (GLenum)q[i++];
                glDisableClientState(_e);
#ifdef DEBUG
                printf("glDisableClientState(0x%x)\n", _e);
                fflush(stdout);
#endif
            }
            break;

        case CMD_DRAW_ARRAYS:
            {
                GLenum _e = (GLenum)q[i++];
                GLint _i = (GLint)q[i++];
                GLsizei _s = (GLsizei)q[i++];
#ifdef DEBUG
                printf("glDrawArrays(0x%x, 0x%x, 0x%x)\n", _e, _i, _s);
                fflush(stdout);
#endif
                glDrawArrays(_e, _i, _s);
            }
            break;

        case CMD_DRAW_ELEMENTSB:
        case CMD_DRAW_ELEMENTS_VBO:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLsizei _s = (GLsizei)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
#ifdef DEBUG
                printf("glDrawElements(0x%x, 0x%x, 0x%x, 0x%p)\n", _e1, _s, _e2, _ptr);
                fflush(stdout);
#endif
                glDrawElements(_e1, _s, _e2, _ptr);
            }
            break;

        case CMD_DRAW_TEXF:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                GLfloat _f3 = (GLfloat)fq[i++];
                GLfloat _f4 = (GLfloat)fq[i++];
                GLfloat _f5 = (GLfloat)fq[i++];
                glDrawTexfOES(_f1, _f2, _f3, _f4, _f5);
            }
            break;
      
        case CMD_DRAW_TEXFB:
            {
                GLfloat *_fptr = (GLfloat *)q[i++];
                glDrawTexfvOES(_fptr);
            }
            break;
      
        case CMD_DRAW_TEXI:
            {
                GLint _i1 = (GLint)q[i++];
                GLint _i2 = (GLint)q[i++];
                GLint _i3 = (GLint)q[i++];
                GLint _i4 = (GLint)q[i++];
                GLint _i5 = (GLint)q[i++];
                glDrawTexiOES(_i1, _i2, _i3, _i4, _i5);
            }
            break;
      
        case CMD_DRAW_TEXIB:
            {
                GLint *_iptr = (GLint *)q[i++];
                glDrawTexivOES(_iptr);
            }
            break;

        case CMD_DRAW_TEXS:
            {
                GLshort _s1 = (GLshort)(GLint)q[i++];
                GLshort _s2 = (GLshort)(GLint)q[i++];
                GLshort _s3 = (GLshort)(GLint)q[i++];
                GLshort _s4 = (GLshort)(GLint)q[i++];
                GLshort _s5 = (GLshort)(GLint)q[i++];
                glDrawTexsOES(_s1, _s2, _s3, _s4, _s5);
            }
            break;

        case CMD_DRAW_TEXSB:
            {
                GLshort *_sptr1 = (GLshort *)q[i++];
                glDrawTexsvOES(_sptr1);
            }
            break;

        case CMD_DRAW_TEXX:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                GLfixed _x3 = (GLfixed)q[i++];
                GLfixed _x4 = (GLfixed)q[i++];
                GLfixed _x5 = (GLfixed)q[i++];
                glDrawTexxOES(_x1, _x2, _x3, _x4, _x5);
            }
            break;

        case CMD_DRAW_TEXXB:
            {
                GLfixed *_xptr = (GLfixed *)q[i++];
                glDrawTexxvOES(_xptr);
            }
            break;

        case CMD_ENABLE:
            {
                GLint _i = (GLint)q[i++];
#ifdef DEBUG
                printf("glEnable(0x%x)\n", _i);
                fflush(stdout);
#endif
                glEnable(_i);
            }
            break;

        case CMD_ENABLE_CLIENT_STATE:
            {
                GLenum _e = (GLenum)q[i++];
#ifdef DEBUG
                printf("glEnableClientState(0x%x)\n", _e);
                fflush(stdout);
#endif
                glEnableClientState(_e);
            }
            break;

        case CMD_FINISH:
            {
                glFinish();
            }
            break;

        case CMD_FLUSH:
            {
                glFlush();
            }
            break;

        case CMD_FOGF:
            {
                GLenum _e = (GLenum)q[i++];
                GLfloat _f = (GLfloat)fq[i++];
                glFogf(_e, _f);
            }
            break;

        case CMD_FOGFB:
            {
                GLenum _e = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)q[i++];
                glFogfv(_e, _fptr);
            }
            break;

        case CMD_FOGFV:
            {
                int n = q[i++]; /* 1 or 4 */
                GLenum _e = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
                glFogfv(_e, _fptr);
                i += n;
            }
            break;

        case CMD_FOGX:
            {
                GLenum _e = (GLenum)q[i++];
                GLfixed _x = (GLfixed)q[i++];
                glFogx(_e, _x);
            }
            break;

        case CMD_FOGXB:
            {
                GLenum _e = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)q[i++];
                glFogxv(_e, _xptr);
            }
            break;

        case CMD_FOGXV:
            {
                int n = q[i++]; /* 1 or 4 */
                GLenum _e = (GLenum)q[i++];
                const GLfixed *_xptr  = (const GLfixed *)&q[i];
                glFogxv(_e, _xptr);
                i += n;
            }
            break;

        case CMD_FRONT_FACE:
            {
                GLint _i = (GLint)q[i++];
                glFrontFace(_i);
            }
            break;

        case CMD_FRUSTUMF:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                GLfloat _f3 = (GLfloat)fq[i++];
                GLfloat _f4 = (GLfloat)fq[i++];
                GLfloat _f5 = (GLfloat)fq[i++];
                GLfloat _f6 = (GLfloat)fq[i++];
#ifdef DEBUG
                printf("glFrustumf(%f, %f, %f, %f, %f, %f)\n",
                       _f1, _f2, _f3, _f4, _f5, _f6);
                fflush(stdout);
#endif
                glFrustumf(_f1, _f2, _f3, _f4, _f5, _f6);
            }
            break;

        case CMD_FRUSTUMX:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                GLfixed _x3 = (GLfixed)q[i++];
                GLfixed _x4 = (GLfixed)q[i++];
                GLfixed _x5 = (GLfixed)q[i++];
                GLfixed _x6 = (GLfixed)q[i++];
                glFrustumx(_x1, _x2, _x3, _x4, _x5, _x6);
            }
            break;

	case CMD_GEN_BUFFERSB:
	    {
		GLsizei _s = (GLsizei)q[i++];
		GLuint *_uptr9 = (GLuint *)q[i++];
		glGenBuffers(_s, _uptr9);
	    }
	    break;

	case CMD_GEN_TEXTURESB:
	    {
		GLsizei _s = (GLsizei)q[i++];
		GLuint *_uptr10 = (GLuint *)q[i++];
		glGenBuffers(_s, _uptr10);
	    }
	    break;

        case CMD_HINT:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
#ifdef DEBUG
                printf("glHint(0x%x, 0x%x)\n", _e1, _e2);
                fflush(stdout);
#endif
                glHint(_e1, _e2);
            }
            break;

        case CMD_LIGHTF:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLfloat _f = (GLfloat)fq[i++];
                glLightf(_e1, _e2, _f);
            }
            break;

        case CMD_LIGHTFB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)q[i++];
#ifdef DEBUG
                printf("glLightfv(0x%x, 0x%x, 0x%p)\n", _e1, _e2, _fptr);
                fflush(stdout);
#endif
                glLightfv(_e1, _e2, _fptr);
            }
            break;

        case CMD_LIGHTFV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
#ifdef DEBUG
                printf("glLightfv(0x%x, 0x%x, 0x%p (stack, n=0x%x))\n", _e1, _e2, _fptr, n);
                fflush(stdout);
#endif
                glLightfv(_e1, _e2, _fptr);
                i += n;
            }
            break;

        case CMD_LIGHTX:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLfixed _x = (GLfixed)q[i++];
                glLightx(_e1, _e2, _x);
            }
            break;

        case CMD_LIGHTXB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)q[i++];
                glLightxv(_e1, _e2, _xptr);
            }
            break;

        case CMD_LIGHTXV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)&q[i];
                glLightxv(_e1, _e2, _xptr);
                i += n;
#ifdef DEBUG
                if (n == 1) {
                    printf("glLightxv(0x%x, 0x%x, 0x%x)\n",
                           _e1, _e2, _xptr[0]);
                } else if (n == 3) {
                    printf("glLightxv(0x%x, 0x%x, [0x%x, 0x%x, 0x%x])\n",
                           _e1, _e2, _xptr[0], _xptr[1], _xptr[2]);
                } else if (n == 4) {
                    printf("glLightxv(0x%x, 0x%x, [0x%x, 0x%x, 0x%x, 0x%x])\n",
                           _e1, _e2,
                           _xptr[0], _xptr[1], _xptr[2], _xptr[3]);
                }
                fflush(stdout);
#endif 
           }
            break;

        case CMD_LIGHT_MODELF:
            {
                GLenum _e = (GLenum)q[i++];
                GLfloat _f = (GLfloat)fq[i++];
                glLightModelf(_e, _f);
            }
            break;

        case CMD_LIGHT_MODELFB:
            {
                GLenum _e = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)q[i++];
                glLightModelfv(_e, _fptr);
            }
            break;

        case CMD_LIGHT_MODELFV:
            {
                int n = q[i++];
                GLenum _e = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
                glLightModelfv(_e, _fptr);
                i += n;
            }
            break;

        case CMD_LIGHT_MODELX:
            {
                GLenum _e = (GLenum)q[i++];
                GLfixed _x = (GLfixed)q[i++];
                glLightModelx(_e, _x);
            }
            break;

        case CMD_LIGHT_MODELXB:
            {
                GLenum _e = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)q[i++];
                glLightModelxv(_e, _xptr);
            }
            break;

        case CMD_LIGHT_MODELXV:
            {
                int n = q[i++];
                GLenum _e = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)&q[i];
                glLightModelxv(_e, _xptr);
                i += n;
            }
            break;

        case CMD_LINE_WIDTH:
            {
                GLfloat _f = (GLfloat)fq[i++];
                glLineWidth(_f);
            }
            break;

        case CMD_LINE_WIDTHX:
            {
                GLfixed _x = (GLfixed)q[i++];
                glLineWidthx(_x);
            }
            break;

        case CMD_LOAD_IDENTITY:
            {
#ifdef DEBUG
                printf("glLoadIdentity()\n");
                fflush(stdout);
#endif
                glLoadIdentity();
            }
            break;

        case CMD_LOAD_MATRIXF:
            {
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
                glLoadMatrixf(_fptr);
                i += 16;
            }
            break;

        case CMD_LOAD_MATRIXFB:
            {
                const GLfloat *_fptr = (const GLfloat *)q[i++];
                glLoadMatrixf(_fptr);
            }
            break;

        case CMD_LOAD_MATRIXX:
            {
                const GLfixed *_xptr = (const GLfixed *)&q[i];
                glLoadMatrixx(_xptr);
                i += 16;
            }
            break;

        case CMD_LOAD_MATRIXXB:
            {
                const GLfixed *_xptr = (const GLfixed *)q[i++];
                glLoadMatrixx(_xptr);
            }
            break;

        case CMD_LOAD_PALETTE_FROM_MODEL_VIEW_MATRIX:
            {
                glLoadPaletteFromModelViewMatrixOES();
            }
            break;

        case CMD_LOGIC_OP:
            {
                GLint _i = (GLint)q[i++];
                glLogicOp(_i);
            }
            break;

        case CMD_MATERIALF:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLfloat _f = (GLfloat)fq[i++];
                glMaterialf(_e1, _e2, _f);
            }
            break;

        case CMD_MATERIALFB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)q[i++];
#ifdef DEBUG
                printf("glMaterialfv(0x%x, 0x%x, 0x%p)\n", _e1, _e2, _fptr);
                fflush(stdout);
#endif
                glMaterialfv(_e1, _e2, _fptr);
            }
            break;

        case CMD_MATERIALFV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
#ifdef DEBUG
                printf("glMaterialfv(0x%x, 0x%x, 0x%p (stack, n=0x%x))\n", _e1, _e2, _fptr, n);
                fflush(stdout);
#endif
                glMaterialfv(_e1, _e2, _fptr);
                i += n;
            }
            break;

        case CMD_MATERIALX:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLfixed _x = (GLfixed)q[i++];
                printf("\n_e1 %X _e2 %X x %X",_e1, _e2, _x);
                glMaterialx(_e1, _e2, _x);
            }
            break;

        case CMD_MATERIALXB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_fptr = (const GLfixed *)q[i++];
                glMaterialxv(_e1, _e2, _fptr);
            }
            break;

        case CMD_MATERIALXV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_fptr = (const GLfixed *)&q[i];
                glMaterialxv(_e1, _e2, _fptr);
                i += n;
            }
            break;

        case CMD_MATRIX_INDEX_POINTER:
        case CMD_MATRIX_INDEX_POINTER_VBO:
            {
                GLint _i = (GLint)q[i++];
                GLenum _e = (GLenum)q[i++];
                GLsizei _s = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
#ifdef GL_OES_matrix_palette
#ifdef DEBUG
                printf("glMatrixIndexPointerOES(%d, %d, %d, 0x%x)\n",
                       _i, _e, _s, _ptr);
                fflush(stdout);
#endif
                glMatrixIndexPointerOES(_i, _e, _s, _ptr);
#endif
            }
            break;

        case CMD_MATRIX_MODE:
            {
                GLenum _e = (GLenum)q[i++];
#ifdef DEBUG
                printf("glMatrixMode(0x%x)\n", _e);
                fflush(stdout);
#endif
                glMatrixMode(_e);
            }
            break;

        case CMD_MULTI_TEXT_COORD4F:
            {
                GLenum _e = (GLenum)q[i++];
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                GLfloat _f3 = (GLfloat)fq[i++];
                GLfloat _f4 = (GLfloat)fq[i++];
                glMultiTexCoord4f(_e, _f1, _f2, _f3, _f4);
            }
            break;

        case CMD_MULTI_TEXT_COORD4X:
            {
                GLenum _e = (GLenum)q[i++];
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                GLfixed _x3 = (GLfixed)q[i++];
                GLfixed _x4 = (GLfixed)q[i++];
                glMultiTexCoord4x(_e, _x1, _x2, _x3, _x4);
            }
            break;

        case CMD_MULT_MATRIXF:
            {
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
                glMultMatrixf(_fptr);
                i += 16;
            }
            break;

        case CMD_MULT_MATRIXFB:
            {
                const GLfloat *_fptr = (const GLfloat *)q[i++];
                glMultMatrixf(_fptr);
            }
            break;

        case CMD_MULT_MATRIXX:
            {
                const GLfixed *_xptr = (const GLfixed *)&q[i];
                glMultMatrixx(_xptr);
                i += 16;
            }
            break;
      
        case CMD_MULT_MATRIXXB:
            {
                const GLfixed *_xptr = (const GLfixed *)q[i++];
                glMultMatrixx(_xptr);
            }
            break;

        case CMD_NORMAL3F:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                GLfloat _f3 = (GLfloat)fq[i++];
                glNormal3f(_f1, _f2, _f3);
            }
            break;

        case CMD_NORMAL3X:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                GLfixed _x3 = (GLfixed)q[i++];
                glNormal3x(_x1, _x2, _x3);
            }
            break;

        case CMD_NORMAL_POINTER:
        case CMD_NORMAL_POINTER_VBO:
            {
                GLenum _e = (GLenum)q[i++];
                GLsizei _s = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
#ifdef DEBUG
                printf("glNormalPointer(0x%x, 0x%x, 0x%p)\n", _e, _s, _ptr);
                fflush(stdout);
#endif
                glNormalPointer(_e, _s, _ptr);
            }
            break;

        case CMD_ORTHOF:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                GLfloat _f3 = (GLfloat)fq[i++];
                GLfloat _f4 = (GLfloat)fq[i++];
                GLfloat _f5 = (GLfloat)fq[i++];
                GLfloat _f6 = (GLfloat)fq[i++];
#ifdef DEBUG
                printf("glOrthof(%f, %f, %f, %f, %f, %f)\n",
                       _f1, _f2, _f3, _f4, _f5, _f6);
                fflush(stdout);
#endif
                glOrthof(_f1, _f2, _f3, _f4, _f5, _f6);
            }
            break;

        case CMD_ORTHOX:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                GLfixed _x3 = (GLfixed)q[i++];
                GLfixed _x4 = (GLfixed)q[i++];
                GLfixed _x5 = (GLfixed)q[i++];
                GLfixed _x6 = (GLfixed)q[i++];
#ifdef DEBUG
                printf("glOrthox(%d, %d, %d, %d, %d, %d)\n",
                       _x1, ///65536.0f,
                       _x2, ///65536.0f,
                       _x3, ///65536.0f,
                       _x4, ///65536.0f,
                       _x5, ///65536.0f,
                       _x6); ///65536.0f);
                fflush(stdout);
#endif
                glOrthox(_x1, _x2, _x3, _x4, _x5, _x6);
            }
            break;

        case CMD_PIXEL_STOREI:
            {
                GLenum _e = (GLenum)q[i++];
                GLint _i = (GLint)q[i++];
                glPixelStorei(_e, _i);
            }
            break;

        case CMD_POINT_PARAMETERF:
            {
                GLenum _e = (GLenum)q[i++];
                GLfloat _f = (GLfloat)fq[i++];
                glPointParameterf(_e, _f);
            }
            break;

        case CMD_POINT_PARAMETERFB:
            {
                GLenum _e = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)q[i++];
                glPointParameterfv(_e, _fptr);
            }
            break;

        case CMD_POINT_PARAMETERFV:
            {
                int n = q[i++];
                GLenum _e = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
                glPointParameterfv(_e, _fptr);
                i += n;
            }
            break;

        case CMD_POINT_PARAMETERX:
            {
                GLenum _e = (GLenum)q[i++];
                GLfixed _x = (GLfixed)q[i++];
                glPointParameterx(_e, _x);
            }
            break;

        case CMD_POINT_PARAMETERXB:
            {
                GLenum _e = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)q[i++];
                glPointParameterxv(_e, _xptr);
            }
            break;

        case CMD_POINT_PARAMETERXV:
            {
                int n = q[i++];
                GLenum _e = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)&q[i];
                glPointParameterxv(_e, _xptr);
                i += n;
            }
            break;

        case CMD_POINT_SIZE:
            {
                GLfloat _f = (GLfloat)fq[i++];
                glPointSize(_f);
            }
            break;

        case CMD_POINT_SIZEX:
            {
                GLfixed _x = (GLfixed)q[i++];
                glPointSizex(_x);
            }
            break;


        case CMD_POINT_SIZE_POINTER:
        case CMD_POINT_SIZE_POINTER_VBO:
            {
                GLenum _e = (GLenum)q[i++];
                GLsizei _s = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
#ifdef GL_OES_point_size_array
                glPointSizePointerOES(_e, _s, _ptr);
#endif
            }
            break;

        case CMD_POLYGON_OFFSET:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                glPolygonOffset(_f1, _f2);
            }
            break;

        case CMD_POLYGON_OFFSETX:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                glPolygonOffsetx(_x1, _x2);
            }
            break;

        case CMD_POP_MATRIX:
            {
                glPopMatrix();
            }
            break;

        case CMD_PUSH_MATRIX:
            {
                glPushMatrix();
            }
            break;

        case CMD_ROTATEF:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                GLfloat _f3 = (GLfloat)fq[i++];
                GLfloat _f4 = (GLfloat)fq[i++];
#ifdef DEBUG
                printf("glRotatef(%f, %f, %f, %f)\n", _f1, _f2, _f3, _f4);
                fflush(stdout);
#endif
                glRotatef(_f1, _f2, _f3, _f4);
            }
            break;

        case CMD_ROTATEX:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                GLfixed _x3 = (GLfixed)q[i++];
                GLfixed _x4 = (GLfixed)q[i++];
                glRotatex(_x1, _x2, _x3, _x4);
            }
            break;

        case CMD_SAMPLE_COVERAGE:
            {
                GLfloat _f = (GLfloat)fq[i++];
                GLboolean _b = (GLboolean)q[i++];
                glSampleCoverage(_f, _b);
            }
            break;

        case CMD_SAMPLE_COVERAGEX:
            {
                GLfixed _x = (GLfixed)q[i++];
                GLboolean _b = (GLboolean)q[i++];
                glSampleCoveragex(_x, _b);
            }
            break;

        case CMD_SCALEF:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                GLfloat _f3 = (GLfloat)fq[i++];
                glScalef(_f1, _f2, _f3);
            }
            break;

        case CMD_SCALEX:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                GLfixed _x3 = (GLfixed)q[i++];
                glScalex(_x1, _x2, _x3);
            }
            break;

        case CMD_SCISSOR:
            {
                GLint _i1 = (GLint)q[i++];
                GLint _i2 = (GLint)q[i++];
                GLint _i3 = (GLint)q[i++];
                GLint _i4 = (GLint)q[i++];
#ifdef DEBUG
                printf("glScissor(%d, %d, %d, %d)\n", _i1, _i2, _i3, _i4);
                fflush(stdout);
#endif
                glScissor(_i1, _i2, _i3, _i4);
            }
            break;

        case CMD_SHADE_MODEL:
            {
                GLenum _e = (GLenum)q[i++];
#ifdef DEBUG
                printf("glShadeModel(0x%x)\n", _e);
                fflush(stdout);
#endif
                glShadeModel(_e);
            }
            break;

        case CMD_STENCIL_FUNC:
            {
                GLenum _e = (GLenum)q[i++];
                GLint _i = (GLint)q[i++];
                GLuint _u = (GLuint)q[i++];
                glStencilFunc(_e, _i, _u);
            }
            break;

        case CMD_STENCIL_MASK:
            {
                GLuint _u = (GLuint)q[i++];
                glStencilMask(_u);
            }
            break;

        case CMD_STENCIL_OP:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLenum _e3 = (GLenum)q[i++];
                glStencilOp(_e1, _e2, _e3);
            }
            break;

        case CMD_TEX_COORD_POINTER:
        case CMD_TEX_COORD_POINTER_VBO:
            {
                GLint _i = (GLint)q[i++];
                GLenum _e = (GLenum)q[i++];
                GLsizei _s = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
                glTexCoordPointer(_i, _e, _s, _ptr);
#ifdef DEBUG
                printf("glTexCoordPointer(0x%x, 0x%x, 0x%x, 0x%x)\n",
                       _i, _e, _s, _ptr);
#endif
            }
            break;

        case CMD_TEX_ENVF:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLfloat _f = (GLfloat)fq[i++];
                glTexEnvf(_e1, _e2, _f);
            }
            break;

        case CMD_TEX_ENVFB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)q[i++];
                glTexEnvfv(_e1, _e2, _fptr);
            }
            break;

        case CMD_TEX_ENVFV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
                glTexEnvfv(_e1, _e2, _fptr);
                i += n;
            }
            break;

        case CMD_TEX_ENVI:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLint _i = (GLint)q[i++];
                glTexEnvi(_e1, _e2, _i);
            }
            break;

        case CMD_TEX_ENVIB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLint *_iptr = (const GLint *)q[i++];
                glTexEnviv(_e1, _e2, _iptr);
            }
            break;

        case CMD_TEX_ENVIV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLint *_iptr = (const GLint *)&q[i];
                glTexEnviv(_e1, _e2, _iptr);
                i += n;
            }
            break;

        case CMD_TEX_ENVX:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLfixed _x = (GLfixed)q[i++];
                glTexEnvx(_e1, _e2, _x);
            }
            break;

        case CMD_TEX_ENVXB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)q[i++];
                glTexEnvxv(_e1, _e2, _xptr);
            }
            break;

        case CMD_TEX_ENVXV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)&q[i];
                glTexEnvxv(_e1, _e2, _xptr);
                i += n;
            }
            break;

        case CMD_TEX_IMAGE_2D:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLint _i1 = (GLint)q[i++];
                GLint _i2 = (GLint)q[i++];
                GLsizei _s1 = (GLsizei)q[i++];
                GLsizei _s2 = (GLsizei)q[i++];
                GLint _i3 = (GLint)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLenum _e3 = (GLenum)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
                glTexImage2D(_e1, _i1, _i2, _s1, _s2, _i3, _e2, _e3, _ptr);
#ifdef DEBUG
                printf("glTexImage2D(0x%x, 0x%x, 0x%x,\n", _e1, _i1, _i2);
                printf("             0x%x, 0x%x, 0x%x,\n", _s1, _s2, _i3);
                printf("             0x%x, 0x%x, 0x%x)\n", _e2, _e3, _ptr);
                fflush(stdout);
#endif
            }
            break;

        case CMD_TEX_PARAMETERF:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLfloat _f = (GLfloat)fq[i++];
                glTexParameterf(_e1, _e2, _f);
#ifdef DEBUG
                printf("glTexParameterf(0x%x, 0x%x, %f)\n", _e1, _e2, _f);
                fflush(stdout);
#endif
            }
            break;

        case CMD_TEX_PARAMETERFB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)q[i++];
                glTexParameterfv(_e1, _e2, _fptr);
            }
            break;

        case CMD_TEX_PARAMETERFV:
            {
                GLint n = (GLint)q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
                glTexParameterfv(_e1, _e2, _fptr);
                i += n;
            }
            break;

        case CMD_TEX_PARAMETERI:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLint _i = (GLint)q[i++];
                glTexParameterf(_e1, _e2, _i);
            }
            break;

        case CMD_TEX_PARAMETERIB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLint *_iptr = (const GLint *)q[i++];
                glTexParameteriv(_e1, _e2, _iptr);
            }
            break;

        case CMD_TEX_PARAMETERIV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLint *_iptr = (const GLint *)&q[i];
                glTexParameteriv(_e1, _e2, _iptr);
                i += n;
            }
            break;

        case CMD_TEX_PARAMETERX:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLfixed _x = (GLfixed)q[i++];
                glTexParameterx(_e1, _e2, _x);
            }
            break;

        case CMD_TEX_PARAMETERXB:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)q[i++];
                glTexParameterxv(_e1, _e2, _xptr);
            }
            break;

        case CMD_TEX_PARAMETERXV:
            {
                int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)&q[i];
                glTexParameterxv(_e1, _e2, _xptr);
                i += n;
            }
            break;

        case CMD_TEX_SUB_IMAGE_2D:
            {
                GLenum _e1 = (GLenum)q[i++];
                GLint _i1 = (GLint)q[i++];
                GLint _i2 = (GLint)q[i++];
                GLint _i3 = (GLint)q[i++];
                GLsizei _s1 = (GLsizei)q[i++];
                GLsizei _s2 = (GLsizei)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                GLenum _e3 = (GLenum)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
                glTexSubImage2D(_e1, _i1, _i2, _i3, _s1, _s2, _e2, _e3, _ptr);
            }
            break;

        case CMD_TRANSLATEF:
            {
                GLfloat _f1 = (GLfloat)fq[i++];
                GLfloat _f2 = (GLfloat)fq[i++];
                GLfloat _f3 = (GLfloat)fq[i++];
#ifdef DEBUG
                printf("glTranslatef(%f, %f, %f)\n", _f1, _f2, _f3);
                fflush(stdout);
#endif
                glTranslatef(_f1, _f2, _f3);
            }
            break;

        case CMD_TRANSLATEX:
            {
                GLfixed _x1 = (GLfixed)q[i++];
                GLfixed _x2 = (GLfixed)q[i++];
                GLfixed _x3 = (GLfixed)q[i++];
                glTranslatex(_x1, _x2, _x3);
            }
            break;

        case CMD_VERTEX_POINTER:
        case CMD_VERTEX_POINTER_VBO:
            {
                GLint _i = (GLint)q[i++];
                GLenum _e = (GLenum)q[i++];
                GLsizei _s = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
#ifdef DEBUG
                printf("glVertexPointer(0x%x, 0x%x, 0x%x, 0x%p)\n", _i, _e, _s, _ptr);
                fflush(stdout);
#endif
                glVertexPointer(_i, _e, _s, _ptr);
            }
            break;

        case CMD_VIEWPORT:
            {
                GLint _i1 = (GLint)q[i++];
                GLint _i2 = (GLint)q[i++];
                GLsizei _s1 = (GLsizei)q[i++];
                GLsizei _s2 = (GLsizei)q[i++];
#ifdef DEBUG
                printf("glViewport(%d, %d, %d, %d)\n", _i1, _i2, _s1, _s2);
                fflush(stdout);
#endif
                glViewport(_i1, _i2, _s1, _s2);
            }
            break;

        case CMD_WEIGHT_POINTER:
        case CMD_WEIGHT_POINTER_VBO:
            {
                GLint _i = (GLint)q[i++];
                GLenum _e = (GLenum)q[i++];
                GLsizei _s = (GLsizei)q[i++];
                const GLvoid *_ptr = (const GLvoid *)q[i++];
#ifdef GL_OES_matrix_palette
#ifdef DEBUG
                printf("glWeightPointerOES(%d, %d, %d, 0x%x)\n",
                       _i, _e, _s, _ptr);
                fflush(stdout);
#endif
                glWeightPointerOES(_i, _e, _s, _ptr);
#endif
            }
            break;

	case CMD_TEX_GENF:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
	      GLfloat _f = (GLfloat)fq[i++];
#ifdef GL_OES_texture_cube_map
	      glTexGenf(_e1, _pname, _param);
#endif
	  }
	  break;

	case CMD_TEX_GENI:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
	      GLint _i = (GLint)q[i++];
#ifdef GL_OES_texture_cube_map
	      glTexGeni(_e1, _e2, _i);
#endif
	  }
	  break;

	case CMD_TEX_GENX:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
	      GLfixed _x = (GLfixed)q[i++];
#ifdef GL_OES_texture_cube_map
	      glTexGenx(_e1, _e2, _x);
#endif
	  }
	  break;

	case CMD_TEX_GENFB:
	    {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)q[i++];
#ifdef GL_OES_texture_cube_map
                glTexGenfv(_e1, _e2, _fptr);
#endif
	    }
	    break;

	case CMD_TEX_GENFV:
	    {
		int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfloat *_fptr = (const GLfloat *)&fq[i];
#ifdef GL_OES_texture_cube_map
                glTexGenfv(_e1, _e2, _fptr);
#endif
		i += n;
	    }
	    break;

	case CMD_TEX_GENXB:
	    {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)q[i++];
#ifdef GL_OES_texture_cube_map
                glTexGenxv(_e1, _e2, _xptr);
#endif
	    }
	    break;

	case CMD_TEX_GENXV:
	    {
		int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLfixed *_xptr = (const GLfixed *)&q[i];
#ifdef GL_OES_texture_cube_map
                glTexGenxv(_e1, _e2, _xptr);
#endif
		i += n;
	    }
	    break;

	case CMD_TEX_GENIB:
	    {
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLint *_iptr = (const GLint *)q[i++];
#ifdef GL_OES_texture_cube_map
                glTexGeniv(_e1, _e2, _iptr);
#endif
	    }
	    break;
	  
	case CMD_TEX_GENIV:
	    {
		int n = q[i++];
                GLenum _e1 = (GLenum)q[i++];
                GLenum _e2 = (GLenum)q[i++];
                const GLint *_iptr = (const GLint *)&q[i];
#ifdef GL_OES_texture_cube_map
                glTexGeniv(_e1, _e2, _iptr);
#endif
		i += n;
	    }
	    break;

	case CMD_BLEND_EQUATION:
	  {
	      GLenum _e = (GLenum)q[i++];
#ifdef GL_OES_blend_subtract
	      glBlendEquation(_e);
#endif
	  }
	  break;
	  
	case CMD_BLEND_FUNC_SEPARATE:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
	      GLenum _e3 = (GLenum)q[i++];
	      GLenum _e4 = (GLenum)q[i++];
#ifdef GL_OES_blend_func_separate
	      glBlendFuncSeparate(_e1, _e2, _e3, _e4);
#endif
	  }
	  break;
	  
	case CMD_BLEND_EQUATION_SEPARATE:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
#ifdef GL_OES_blend_equation_separate
	      glBlendEquationSeparate(_e1, _e2);
#endif
	  }
	  break;
	  
	case CMD_BIND_RENDERBUFFER:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
#ifdef GL_OES_framebuffer_object
	      glBindRenderbufferOES(_e1, _e2);
#endif
	  }
	  break;

	case CMD_DELETE_RENDERBUFFERS:
	  {
	      GLsizei n = (GLsizei)q[i++];
	      const GLuint *_uptr3 = (const GLuint *)&q[i];
#ifdef GL_OES_framebuffer_object
	      glDeleteRenderbuffersOES(_n, _uptr3);
#endif
	      i += n;
	  }
	  break;
	  
	case CMD_DELETE_RENDERBUFFERSB:
	  {
	      GLsizei _s = (GLsizei)q[i++];
	      const GLuint *_uptr4 = (const GLuint *)q[i++];
#ifdef GL_OES_framebuffer_object
	      glDeleteRenderbuffersOES(_s, _uptr4);
#endif
	  }
	  break;
	  
	case CMD_GEN_RENDERBUFFERSB:
	    {
		GLsizei _s = (GLsizei)q[i++];
		GLuint *_uptr8 = (GLuint *)q[i++];
#ifdef GL_OES_framebuffer_object
		glGenRenderbuffersOES(_s, _uptr8);
#endif
	    }
	    break;

	case CMD_RENDERBUFFER_STORAGE:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
	      GLsizei _s1 = (GLsizei)q[i++];
	      GLsizei _s2 = (GLsizei)q[i++];
#ifdef GL_OES_framebuffer_object
	      glRenderbufferStorageOES(_e1, _e2, _s1, _s2);
#endif
	  }
	  break;
	  
	case CMD_BIND_FRAMEBUFFER:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
#ifdef GL_OES_framebuffer_object
	      glBindFramebufferOES(_e1, _e2);
#endif
	  }
	  break;
	  
	case CMD_DELETE_FRAMEBUFFERS:
	  {
	      GLsizei n = (GLsizei)q[i++];
	      const GLuint *_uptr5 = (const GLuint *)&q[i];
#ifdef GL_OES_framebuffer_object
	      glDeleteFramebuffersOES(_n, _uptr5);
#endif
	      i += n;
	  }
	  break;
	  
	case CMD_DELETE_FRAMEBUFFERSB:
	  {
	      GLsizei _s = (GLsizei)q[i++];
	      const GLuint *_uptr6 = (const GLuint *)q[i++];
#ifdef GL_OES_framebuffer_object
	      glDeleteFramebuffersOES(_s,_uptr6);
#endif
	  }
	  break;
	  
	case CMD_GEN_FRAMEBUFFERSB:
	    {
		GLsizei _s = (GLsizei)q[i++];
		GLuint *_uptr7 = (GLuint *)q[i++];
#ifdef GL_OES_framebuffer_object
		glGenFramebuffersOES(_s, _uptr7);
#endif
	    }
	    break;

	case CMD_FRAMEBUFFER_TEXTURE2D:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
	      GLenum _e3 = (GLenum)q[i++];
	      GLuint _u = (GLenum)q[i++];
	      GLuint _i = (GLenum)q[i++];
#ifdef GL_OES_framebuffer_object
	      glFramebufferTexture2DOES(_e1, _e2, _e3, _u, _i);
#endif
	  }
	  break;
	  
	case CMD_FRAMEBUFFER_RENDERBUFFER:
	  {
	      GLenum _e1 = (GLenum)q[i++];
	      GLenum _e2 = (GLenum)q[i++];
	      GLenum _e3 = (GLenum)q[i++];
	      GLuint _u = (GLenum)q[i++];
#ifdef GL_OES_framebuffer_object
	      glFramebufferRenderbufferOES(_e1, _e2, _e3, _u);
#endif
	  }
	  break;
	  
	case CMD_GENERATE_MIPMAP:
	  {
	      GLenum _e = (GLenum)q[i++];
#ifdef GL_OES_framebuffer_object
	      glGenerateMipmapOES(_e);
#endif
	  }
	  break;
	}


	DEBUG_FLUSH_OR_FINISH;
      
#ifdef DEBUG
	{
	  GLint error = glGetError();
	  if (error != 0) {
	    printf("*** glGetError() = %d\n", error);
	    fflush(stdout);
	  }
	}
#endif
    }

#ifdef DEBUG
    printf("\nFinished execting queue.\n");
    fflush(stdout);
#endif
}

#define MAX(a,b) ((a)>(b)?(a):(b))


/*  private native int _getNativeAddress ( Buffer buffer ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1getNativeAddress() {

    jint offset = KNI_GetParameterAsInt(2);
    jint nativeAddress;

    static jfieldID byteBufferField = 0;
    static jfieldID shortBufferField;
    static jfieldID intBufferField;
    static jfieldID floatBufferField;

    KNI_StartHandles(5);
    KNI_DeclareHandle(bufferHandle);
    KNI_DeclareHandle(byteBufferImplClass);
    KNI_DeclareHandle(shortBufferImplClass);
    KNI_DeclareHandle(intBufferImplClass);
    KNI_DeclareHandle(floatBufferImplClass);

    KNI_GetParameterAsObject(1, bufferHandle);
    KNI_FindClass("java/nio/ByteBufferImpl", byteBufferImplClass);
    KNI_FindClass("java/nio/ShortBufferImpl", shortBufferImplClass);
    KNI_FindClass("java/nio/IntBufferImpl", intBufferImplClass);
    KNI_FindClass("java/nio/FloatBufferImpl", floatBufferImplClass);
        
    if (!byteBufferField) {
        byteBufferField =
          KNI_GetFieldID(byteBufferImplClass, "arrayOffset", "I");
        shortBufferField =
            KNI_GetFieldID(shortBufferImplClass, "arrayOffset", "I");
        intBufferField =
            KNI_GetFieldID(intBufferImplClass, "arrayOffset", "I");
        floatBufferField =
          KNI_GetFieldID(floatBufferImplClass, "arrayOffset", "I");
    }

    if (KNI_IsInstanceOf(bufferHandle, byteBufferImplClass)) {
      nativeAddress = KNI_GetIntField(bufferHandle, byteBufferField);
      nativeAddress += offset;
    } else if (KNI_IsInstanceOf(bufferHandle, shortBufferImplClass)) {
      nativeAddress = KNI_GetIntField(bufferHandle, shortBufferField);
      nativeAddress += offset << 1;
    } else if (KNI_IsInstanceOf(bufferHandle, intBufferImplClass)) {
      nativeAddress = KNI_GetIntField(bufferHandle, intBufferField);
      nativeAddress += offset << 2;
    } else if (KNI_IsInstanceOf(bufferHandle, floatBufferImplClass)) {
      nativeAddress = KNI_GetIntField(bufferHandle, floatBufferField);
      nativeAddress += offset << 2;
    } else {
      // can't be here
    }

    KNI_EndHandles();
    KNI_ReturnInt(nativeAddress);
}

/*  private native void _glGenBuffers ( int n , int [ ] buffers , int offset ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGenBuffers() {

    jint n = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    GLuint *buffers;
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(buffersHandle);

    KNI_GetParameterAsObject(2, buffersHandle);

    buffers = (GLuint *)JSR239_malloc(MAX(n, 1)*sizeof(GLuint));
    if (!buffers) {
      KNI_ThrowNew("java.lang.OutOfMemoryException", "glGenBuffers");	
    }
    glGenBuffers((GLsizei)n, buffers);
    DEBUG_FLUSH_OR_FINISH;
    for (i = 0; i < n; i++) {
	KNI_SetIntArrayElement(buffersHandle, offset + i, buffers[i]);
    }
    JSR239_free(buffers);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGenTextures ( int n , int [ ] textures , int offset ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGenTextures() {

    jint n = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    GLuint *textures;
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(texturesHandle);

    KNI_GetParameterAsObject(2, texturesHandle);

    textures = (GLuint *)JSR239_malloc(MAX(n, 1)*sizeof(GLuint));
    if (!textures) {
        KNI_ThrowNew("java.lang.OutOfMemoryException", "glGenTextures");
    }
    glGenTextures((GLsizei)n, textures);
    DEBUG_FLUSH_OR_FINISH;
    for (i = 0; i < n; i++) {
#ifdef DEBUG
        printf("glGenTextures: element %d = %d\n", i, textures[i]);
#endif
        KNI_SetIntArrayElement(texturesHandle, offset + i, textures[i]);
    }
    JSR239_free(textures);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native int _glGetError ( ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glGetError() {

    jint returnValue = (jint)glGetError();
    DEBUG_FLUSH_OR_FINISH;

    KNI_ReturnInt(returnValue);
}

/* private native void _glGenerateError ( int error ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGenerateError() {

    jint error = KNI_GetParameterAsInt(1);

    switch (error) {
    case GL_INVALID_ENUM:
      {
        glDepthFunc(-1);
      }
      break;

    default:
      printf("Don't know how to generate error 0x%x!\n", error);
      break;
    }

    KNI_ReturnVoid();
}

/*  private native void _glGetIntegerv ( int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetIntegerv() {

    jint pname = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);
    GLint params[MAX_GET_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(2, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_PARAMS);
#endif
    
    if (KNI_IsNullHandle(paramsHandle)) {
      glGetIntegerv((GLenum)pname, (GLint *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetIntegerv((GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native String _glGetString ( int name ) ; */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_jsr239_GL10Impl__1glGetString() {

    jint name = KNI_GetParameterAsInt(1);
    const GLubyte *string;

    KNI_StartHandles(1);
    KNI_DeclareHandle(stringHandle);

    string = glGetString(name);
#ifdef DEBUG
    printf("glGetString(%d) returns \"%s\"\n", name, string);
#endif

    if (string) {
        KNI_NewStringUTF(string, stringHandle);
    } else {
        /* Set stringHandle to null. */
        KNI_ReleaseHandle(stringHandle);
    }
    KNI_EndHandlesAndReturnObject(stringHandle);
}

/*  private native void _glGetBooleanv ( int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetBooleanv() {

    jint pname = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);
    GLboolean params[MAX_GET_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(2, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_PARAMS);
#endif

    glGetBooleanv((GLenum)pname, &params[0]);

    if (KNI_IsNullHandle(paramsHandle)) {
        DEBUG_FLUSH_OR_FINISH;
        for (i = 0; i < length; i++) {
            ((jint *) offset)[i] = (params[i] == GL_TRUE) ? 1 : 0;
        }
    } else {
        DEBUG_FLUSH_OR_FINISH;
        for (i = 0; i < length; i++) {
            KNI_SetIntArrayElement(paramsHandle, offset + i,
                                   (params[i] == GL_TRUE) ? 1 : 0);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetFixedv ( int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetFixedv() {

    jint pname = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);
    GLfixed params[MAX_GET_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(2, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetFixedv((GLenum)pname, (GLfixed *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetFixedv((GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetFloatv ( int pname , float [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetFloatv() {

    jint pname = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);
    GLfloat params[MAX_GET_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(2, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetFloatv((GLenum)pname, (GLfloat *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetFloatv((GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetFloatArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetLightfv ( int light , int pname , float [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetLightfv() {

    jint light = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfloat params[MAX_GET_LIGHT_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_LIGHT_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetLightfv((GLenum)light, (GLenum)pname, (GLfloat *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetLightfv((GLenum)light, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetFloatArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetLightxv ( int light , int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetLightxv() {

    jint light = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfixed params[MAX_GET_LIGHT_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_LIGHT_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetLightxv((GLenum)light, (GLenum)pname, (GLfixed *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetLightxv((GLenum)light, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetMaterialfv ( int face , int pname , float [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetMaterialfv() {

    jint face = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfloat params[MAX_GET_MATERIAL_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_MATERIAL_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetMaterialfv((GLenum)face, (GLenum)pname, (GLfloat *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetMaterialfv((GLenum)face, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetFloatArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetMaterialxv ( int face , int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetMaterialxv() {

    jint face = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfixed params[MAX_GET_MATERIAL_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_MATERIAL_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetMaterialxv((GLenum)face, (GLenum)pname, (GLfixed *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetMaterialxv((GLenum)face, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetTexEnvfv ( int env , int pname , float [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetTexEnvfv() {

    jint env = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfloat params[MAX_GET_TEX_ENV_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_TEX_ENV_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexEnvfv((GLenum)env, (GLenum)pname, (GLfloat *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexEnvfv((GLenum)env, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetFloatArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetTexEnviv ( int env , int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetTexEnviv() {

    jint env = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLint params[MAX_GET_TEX_ENV_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_TEX_ENV_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexEnviv((GLenum)env, (GLenum)pname, (GLint *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexEnviv((GLenum)env, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetTexEnvxv ( int env , int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetTexEnvxv() {

    jint env = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfixed params[MAX_GET_TEX_ENV_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_TEX_ENV_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexEnvxv((GLenum)env, (GLenum)pname, (GLfixed *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexEnvxv((GLenum)env, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetTexParameterfv ( int target , int pname , float [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetTexParameterfv() {

    jint target = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfloat params[MAX_GET_TEX_PARAMETER_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_TEX_PARAMETER_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexParameterfv((GLenum)target, (GLenum)pname, (GLfloat *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexParameterfv((GLenum)target, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetFloatArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetTexParameteriv ( int target , int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetTexParameteriv() {

    jint target = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLint params[MAX_GET_TEX_PARAMETER_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_TEX_PARAMETER_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexParameteriv((GLenum)target, (GLenum)pname, (GLint *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexParameteriv((GLenum)target, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetTexParameterxv ( int target , int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetTexParameterxv() {

    jint target = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfixed params[MAX_GET_TEX_PARAMETER_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_TEX_PARAMETER_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexParameterxv((GLenum)target, (GLenum)pname, (GLfixed *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexParameterxv((GLenum)target, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetBufferParameteriv ( int target , int pname , int [ ] params , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetBufferParameteriv() {

    jint target = KNI_GetParameterAsInt(1);
    jint pname = KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLint params[MAX_GET_BUFFER_PARAMETER_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_BUFFER_PARAMETER_PARAMS);
#endif

    if (KNI_IsNullHandle(paramsHandle)) {
      glGetBufferParameteriv((GLenum)target, (GLenum)pname, (GLint *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetBufferParameteriv((GLenum)target, (GLenum)pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetClipPlanef ( int pname , float [ ] eqn , int offset ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetClipPlanef() {

    jint pname = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    GLfloat eqn[4];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(eqnHandle);

    KNI_GetParameterAsObject(2, eqnHandle);

    if (KNI_IsNullHandle(eqnHandle)) {
      glGetClipPlanef((GLenum)pname, (GLfloat *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetClipPlanef((GLenum)pname, &eqn[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < 4; i++) {
	KNI_SetFloatArrayElement(eqnHandle, offset + i, eqn[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetClipPlanex ( int pname , int [ ] eqn , int offset ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetClipPlanex() {

    jint pname = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    GLfixed eqn[4];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(eqnHandle);

    KNI_GetParameterAsObject(2, eqnHandle);

    if (KNI_IsNullHandle(eqnHandle)) {
      glGetClipPlanex((GLenum)pname, (GLfixed *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetClipPlanex((GLenum)pname, &eqn[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < 4; i++) {
	KNI_SetIntArrayElement(eqnHandle, offset + i, eqn[i]);
      }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native int _glIsBuffer ( int buffer ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glIsBuffer() {

    jint buffer = KNI_GetParameterAsInt(1);

    jint returnValue = (jint)glIsBuffer((GLuint)buffer);

    KNI_ReturnInt(returnValue);
}

/*  private native int _glIsEnabled ( int cap ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glIsEnabled() {

    jint cap = KNI_GetParameterAsInt(1);

    jint returnValue = glIsEnabled((GLenum)cap);

    KNI_ReturnInt(returnValue);
}

/*  private native int _glIsTexture ( int texture ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_jsr239_GL10Impl__1glIsTexture() {

    jint texture = KNI_GetParameterAsInt(1);

    jint returnValue = (jint)glIsTexture((GLuint)texture);

    KNI_ReturnInt(returnValue);
}

/*  private native void _glReadPixelsPtr ( int x , int y , int width , int height , int format , int type , int pointer ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glReadPixelsPtr() {

    GLint x = (GLint)KNI_GetParameterAsInt(1);
    GLint y = (GLint)KNI_GetParameterAsInt(2);
    GLsizei width = (GLsizei)KNI_GetParameterAsInt(3);
    GLsizei height = (GLsizei)KNI_GetParameterAsInt(4);
    GLenum format = (GLenum)KNI_GetParameterAsInt(5);
    GLenum type = (GLenum)KNI_GetParameterAsInt(6);
    jint pointer = KNI_GetParameterAsInt(7);

    glReadPixels(x, y, width, height, format, type, (GLvoid *)pointer);

    DEBUG_FLUSH_OR_FINISH;

    KNI_ReturnVoid();
}

/*  private native void _glReadPixelsByte ( int x , int y , int width , int height , int format , int type , byte [ ] array , int offset ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glReadPixelsByte() {

    GLint x = (GLint)KNI_GetParameterAsInt(1);
    GLint y = (GLint)KNI_GetParameterAsInt(2);
    GLsizei width = (GLsizei)KNI_GetParameterAsInt(3);
    GLsizei height = (GLsizei)KNI_GetParameterAsInt(4);
    GLenum format = (GLenum)KNI_GetParameterAsInt(5);
    GLenum type = (GLenum)KNI_GetParameterAsInt(6);
    jint offset = KNI_GetParameterAsInt(8);
    GLbyte *data;
    jint length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(arrayHandle);

    KNI_GetParameterAsObject(7, arrayHandle);
    
    length = KNI_GetArrayLength(arrayHandle);
    data = (GLbyte *)JSR239_malloc(length);
    if (!data) {
      KNI_ThrowNew("java.lang.OutOfMemoryException", "glReadPixelsByte");
    }

    /* 
     * For simplicity, we copy the incoming buffer to local storage,
     * perform the read pixels call, and copy the data back.
     *
     * Need revisit - optimize this by only writing bytes that are affected by
     * the glReadPixels call.  This requires keeping track of
     * the glPixelStore settings for stride, padding, etc.
     */

    KNI_GetRawArrayRegion(arrayHandle, 0, length, (jbyte *) data);
    glReadPixels(x, y, width, height, format, type, (GLvoid *)data);
    KNI_SetRawArrayRegion(arrayHandle, 0, length, (jbyte *) data);

    DEBUG_FLUSH_OR_FINISH;

    JSR239_free(data);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glReadPixelsInt ( int x , int y , int width , int height , int format , int type , int [ ] array , int offset ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glReadPixelsInt() {
    GLint x = (GLint)KNI_GetParameterAsInt(1);
    GLint y = (GLint)KNI_GetParameterAsInt(2);
    GLsizei width = (GLsizei)KNI_GetParameterAsInt(3);
    GLsizei height = (GLsizei)KNI_GetParameterAsInt(4);
    GLenum format = (GLenum)KNI_GetParameterAsInt(5);
    GLenum type = (GLenum)KNI_GetParameterAsInt(6);
    jint offset = KNI_GetParameterAsInt(8);
    GLbyte *data;
    jint length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(arrayHandle);

    KNI_GetParameterAsObject(7, arrayHandle);
    length = KNI_GetArrayLength(arrayHandle);
    
    data = (GLbyte *)JSR239_malloc(length);
    if (!data) {
      KNI_ThrowNew("java.lang.OutOfMemoryException", "glReadPixelsInt");
    }

    KNI_GetRawArrayRegion(arrayHandle, 0, length, (jbyte *) data);
    glReadPixels(x, y, width, height, format, type, (GLvoid *)data);
    KNI_SetRawArrayRegion(arrayHandle, 0, length, (jbyte *) data);

    DEBUG_FLUSH_OR_FINISH;

    JSR239_free(data);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native int     _glQueryMatrixxOES(int[] mantissa, int mantissaOffset, int[] exponent, int exponentOffset); */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glQueryMatrixxOES() {
    GLint mantissa[16], exponent[16];

    GLint mantissaOffset = (GLint)KNI_GetParameterAsInt(2);
    GLint exponentOffset = (GLint)KNI_GetParameterAsInt(4);

    KNI_StartHandles(2);
    KNI_DeclareHandle(mantissaHandle);
    KNI_DeclareHandle(exponentHandle);

    KNI_GetParameterAsObject(1, mantissaHandle);
    KNI_GetParameterAsObject(3, exponentHandle);

#ifdef GL_OES_query_matrix
    glQueryMatrixxOES(&mantissa[0], &exponent[0]);
    DEBUG_FLUSH_OR_FINISH;

    KNI_SetRawArrayRegion(mantissaHandle, mantissaOffset*sizeof(jint), 
                          16*sizeof(jint), (const char *)&mantissa[0]);
    KNI_SetRawArrayRegion(exponentHandle, exponentOffset*sizeof(jint), 
                          16*sizeof(jint), (const char *)&exponent[0]);
#endif

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/*  private native int _glGetTexGenfv(int coord, int pname, float[] params, int offset, int length); */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glGetTexGenfv() {

    GLenum coord = (GLenum)KNI_GetParameterAsInt(1);
    GLenum pname = (GLenum)KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfloat params[MAX_GET_TEX_GEN_PARAMS];
    jint i;
    jint returnValue = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef GL_OES_texture_cube_map
    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexGenfv(coord, pname, (GLfloat *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexGenfv(coord, pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetFloatArrayElement(paramsHandle, offset + i, params[i]);
      }
    }
#endif

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}


/* private native int _glGetTexGeniv(int coord, int pname, int[] params, int offset, int length); */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glGetTexGeniv() {

    GLenum coord = (GLenum)KNI_GetParameterAsInt(1);
    GLenum pname = (GLenum)KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLint params[MAX_GET_TEX_GEN_PARAMS];
    jint i;
    jint returnValue = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef GL_OES_texture_cube_map
    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexGeniv(coord, pname, (GLint *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexGeniv(coord, pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }
#endif

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/* private native int _glGetTexGenxv(int coord, int pname, int[] params, int offset, int length); */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glGetTexGenxv() {

    GLenum coord = (GLenum)KNI_GetParameterAsInt(1);
    GLenum pname = (GLenum)KNI_GetParameterAsInt(2);
    jint offset = KNI_GetParameterAsInt(4);
    jint length = KNI_GetParameterAsInt(5);
    GLfixed params[MAX_GET_TEX_GEN_PARAMS];
    jint i;
    jint returnValue = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(3, paramsHandle);

#ifdef GL_OES_texture_cube_map
    if (KNI_IsNullHandle(paramsHandle)) {
      glGetTexGenxv(coord, pname, (GLfixed *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetTexGenxv(coord, pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH; 
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }
#endif

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _glIsRenderbufferOES(int renderbuffer); */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glIsRenderbufferOES() {
    GLuint renderbuffer = KNI_GetParameterAsInt(1);
    jint returnValue = 0;

#ifdef GL_OES_framebuffer_object
    returnValue = glIsRenderbufferOES(renderbuffer);
    DEBUG_FLUSH_OR_FINISH;
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native void _glGenRenderbuffersOES(int n, int[] renderbuffers, int offset) */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGenRenderbuffersOES() {
    jint n = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    GLuint *renderbuffers;
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(renderbuffersHandle);

    KNI_GetParameterAsObject(1, renderbuffersHandle);

    renderbuffers = (GLuint *)JSR239_malloc(MAX(n, 1)*sizeof(GLuint));
    if (!renderbuffers) {
      KNI_ThrowNew("java.lang.OutOfMemoryException", "glGenRenderbuffers");
    }

#ifdef GL_OES_framebuffer_object
    glGenRenderbuffersOES(n, renderbuffers);
    DEBUG_FLUSH_OR_FINISH;
    for (i = 0; i < n; i++) {
	KNI_SetIntArrayElement(renderbuffersHandle, offset + i,
			       renderbuffers[i]);
    }
#endif

    JSR239_free(renderbuffers);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _glGetRenderbufferParameterivOES(int target, int pname, int[] params, int offset, int length) */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetRenderbufferParameterivOES() {
    GLenum target = (GLenum)KNI_GetParameterAsInt(1);
    GLenum pname = (GLenum)KNI_GetParameterAsInt(2);
    GLenum offset = (GLenum)KNI_GetParameterAsInt(4);
    GLenum length = (GLenum)KNI_GetParameterAsInt(5);
    GLint params[MAX_GET_RENDERBUFFER_PARAMETER_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_RENDERBUFFER_PARAMETER_PARAMS);
#endif

#ifdef GL_OES_framebuffer_object
    if (KNI_IsNullHandle(paramsHandle)) {
      glGetRenderbufferParameterivOES(target, pname, (GLint *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetRenderbufferParameterivOES(target, pname, &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }
#endif

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native int _glIsFramebufferOES(int framebuffer); */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glIsFramebufferOES() {
    GLuint framebuffer = (GLuint)KNI_GetParameterAsInt(1);
    jint returnValue = 0;

#ifdef GL_OES_framebuffer_object
    returnValue = glIsFramebufferOES(framebuffer);
    DEBUG_FLUSH_OR_FINISH;
#endif

    KNI_ReturnInt(returnValue);
}

/* private native void _glGenFramebuffersOES(int n, int[] framebuffers, int offset); */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGenFramebuffersOES() {
    jint n = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    GLuint *framebuffers;
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(framebuffersHandle);

    KNI_GetParameterAsObject(1, framebuffersHandle);

    framebuffers = (GLuint *)JSR239_malloc(MAX(n, 1)*sizeof(GLuint));
    if (!framebuffers) {
      KNI_ThrowNew("java.lang.OutOfMemoryException", "glGenFramebuffers");
    }

#ifdef GL_OES_framebuffer_object
    glGenFramebuffersOES(n, framebuffers);
    DEBUG_FLUSH_OR_FINISH;
    for (i = 0; i < n; i++) {
	KNI_SetIntArrayElement(framebuffersHandle, offset + i,
			       framebuffers[i]);
    }
#endif

    JSR239_free(framebuffers);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/* private native int _glCheckFramebufferStatusOES(int target); */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr239_GL10Impl__1glCheckFramebufferStatusOES() {
    GLenum target = KNI_GetParameterAsInt(1);

    jint returnValue = 0;

#ifdef GL_OES_framebuffer_object
    returnValue = glCheckFramebufferStatusOES(target);
    DEBUG_FLUSH_OR_FINISH;
#endif
    
    KNI_ReturnInt(returnValue);
}

/* private native void _glGetFramebufferAttachmentParameterivOES(int target, int attachment, int pname, int[] params, int offset, int length); */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1glGetFramebufferAttachmentParameterivOES() {
    GLenum target = (GLenum)KNI_GetParameterAsInt(1);
    GLenum attachment = (GLenum)KNI_GetParameterAsInt(2);
    GLenum pname = (GLenum)KNI_GetParameterAsInt(3);
    jint offset = KNI_GetParameterAsInt(5);
    jint length = KNI_GetParameterAsInt(6);
    GLint params[MAX_GET_FRAMEBUFFER_ATTACHMENT_PARAMETER_PARAMS];
    jint i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(paramsHandle);

    KNI_GetParameterAsObject(4, paramsHandle);

#ifdef DEBUG
    assert(length <= MAX_GET_FRAMEBUFFER_ATTACHMENT_PARAMETER_PARAMS);
#endif

#ifdef GL_OES_framebuffer_object
    if (KNI_IsNullHandle(paramsHandle)) {
      glGetFramebufferAttachmentParameterivOES(target, attachment,
                                               pname, (GLint *)offset);
      DEBUG_FLUSH_OR_FINISH;
    } else {
      glGetFramebufferAttachmentParameterivOES(target, attachment, pname,
                                               &params[0]);
      DEBUG_FLUSH_OR_FINISH;
      for (i = 0; i < length; i++) {
	KNI_SetIntArrayElement(paramsHandle, offset + i, params[i]);
      }
    }
#endif

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native void _execute ( int [ ] queue , int count ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_GL10Impl__1execute() {

    jint theQueue[GL_COMMAND_QUEUE_SIZE];
    jint size;

    jint count = KNI_GetParameterAsInt(2);

    KNI_StartHandles(1);
    KNI_DeclareHandle(queueHandle);

    KNI_GetParameterAsObject(1, queueHandle);

    size = KNI_GetArrayLength(queueHandle);
    KNI_GetRawArrayRegion(queueHandle, 0, count*sizeof(jint),
			  (jbyte *) &theQueue[0]);

    runQueue(theQueue, count);

    KNI_EndHandles();
    KNI_ReturnVoid();
}
