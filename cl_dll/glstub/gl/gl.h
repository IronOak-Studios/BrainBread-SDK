/* Minimal OpenGL header for Linux builds.
 *
 * This stub supplies the base GL types, constants, and function prototypes
 * that the bundled glext.h expects, removing the need to install
 * libgl-dev:i386 or mesa-common-dev:i386 on the build machine.
 *
 * Only referenced by the Linux Makefile (-I glstub).  The Windows build
 * uses the real gl.h from the Windows SDK instead.
 */
#ifndef __gl_h_
#define __gl_h_

/* Calling-convention macros expected by glext.h */
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

/* Base GL types */
typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef void            GLvoid;
typedef signed char     GLbyte;
typedef short           GLshort;
typedef int             GLint;
typedef unsigned char   GLubyte;
typedef unsigned short  GLushort;
typedef unsigned int    GLuint;
typedef int             GLsizei;
typedef float           GLfloat;
typedef float           GLclampf;
typedef double          GLdouble;
typedef double          GLclampd;
typedef char            GLchar;
typedef ptrdiff_t       GLsizeiptr;
typedef ptrdiff_t       GLintptr;

/* Constants used by the BrainBread client DLL */
#define GL_FALSE                0
#define GL_TRUE                 1
#define GL_ZERO                 0
#define GL_ONE                  1

#define GL_DEPTH_BUFFER_BIT     0x00000100
#define GL_COLOR_BUFFER_BIT     0x00004000

#define GL_POINTS               0x0000
#define GL_LINES                0x0001
#define GL_TRIANGLES            0x0004
#define GL_TRIANGLE_STRIP       0x0005
#define GL_TRIANGLE_FAN         0x0006
#define GL_QUADS                0x0007

#define GL_SRC_ALPHA            0x0302
#define GL_ONE_MINUS_SRC_ALPHA  0x0303
#define GL_DST_ALPHA            0x0304
#define GL_DST_COLOR            0x0306

#define GL_BYTE                 0x1400
#define GL_UNSIGNED_BYTE        0x1401
#define GL_SHORT                0x1402
#define GL_UNSIGNED_SHORT       0x1403
#define GL_INT                  0x1404
#define GL_UNSIGNED_INT         0x1405
#define GL_FLOAT                0x1406

#define GL_MODELVIEW            0x1700
#define GL_PROJECTION           0x1701
#define GL_TEXTURE              0x1702

#define GL_ALPHA                0x1906
#define GL_RGB                  0x1907
#define GL_RGBA                 0x1908
#define GL_LUMINANCE            0x1909
#define GL_LUMINANCE_ALPHA      0x190A

#define GL_NEAREST              0x2600
#define GL_LINEAR               0x2601
#define GL_TEXTURE_MAG_FILTER   0x2800
#define GL_TEXTURE_MIN_FILTER   0x2801
#define GL_TEXTURE_WRAP_S       0x2802
#define GL_TEXTURE_WRAP_T       0x2803
#define GL_CLAMP                0x2900
#define GL_REPEAT               0x2901

#define GL_VENDOR               0x1F00
#define GL_RENDERER             0x1F01
#define GL_VERSION              0x1F02
#define GL_EXTENSIONS           0x1F03

#define GL_TEXTURE_2D           0x0DE1
#define GL_BLEND                0x0BE2
#define GL_DEPTH_TEST           0x0B71
#define GL_CULL_FACE            0x0B44
#define GL_ALPHA_TEST           0x0BC0

#define GL_FOG                  0x0B60
#define GL_FOG_DENSITY          0x0B62
#define GL_FOG_START            0x0B63
#define GL_FOG_END              0x0B64
#define GL_FOG_MODE             0x0B65
#define GL_FOG_COLOR            0x0B66
#define GL_FOG_HINT             0x0C54
#define GL_NICEST               0x1102

#define GL_VIEWPORT             0x0BA2
#define GL_MAX_TEXTURE_SIZE     0x0D33
#define GL_PACK_ALIGNMENT       0x0D05
#define GL_UNPACK_ALIGNMENT     0x0CF5

/* Commonly used GL entry points */
GLAPI void APIENTRY glEnable(GLenum cap);
GLAPI void APIENTRY glDisable(GLenum cap);
GLAPI void APIENTRY glBegin(GLenum mode);
GLAPI void APIENTRY glEnd(void);
GLAPI void APIENTRY glVertex2f(GLfloat x, GLfloat y);
GLAPI void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glVertex3fv(const GLfloat *v);
GLAPI void APIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
GLAPI void APIENTRY glColor3f(GLfloat r, GLfloat g, GLfloat b);
GLAPI void APIENTRY glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
GLAPI void APIENTRY glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
GLAPI void APIENTRY glTexCoord2f(GLfloat s, GLfloat t);
GLAPI void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor);
GLAPI void APIENTRY glDepthMask(GLboolean flag);
GLAPI void APIENTRY glBindTexture(GLenum target, GLuint texture);
GLAPI void APIENTRY glGenTextures(GLsizei n, GLuint *textures);
GLAPI void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures);
GLAPI void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *data);
GLAPI void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data);
GLAPI void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);
GLAPI void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param);
GLAPI void APIENTRY glPixelStorei(GLenum pname, GLint param);
GLAPI void APIENTRY glFogf(GLenum pname, GLfloat param);
GLAPI void APIENTRY glFogfv(GLenum pname, const GLfloat *params);
GLAPI void APIENTRY glFogi(GLenum pname, GLint param);
GLAPI void APIENTRY glHint(GLenum target, GLenum mode);
GLAPI void APIENTRY glMatrixMode(GLenum mode);
GLAPI void APIENTRY glPushMatrix(void);
GLAPI void APIENTRY glPopMatrix(void);
GLAPI void APIENTRY glLoadIdentity(void);
GLAPI void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLAPI void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
GLAPI void APIENTRY glClear(GLbitfield mask);
GLAPI void APIENTRY glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
GLAPI void APIENTRY glGetIntegerv(GLenum pname, GLint *params);
GLAPI void APIENTRY glGetFloatv(GLenum pname, GLfloat *params);
GLAPI const GLubyte * APIENTRY glGetString(GLenum name);
GLAPI void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
GLAPI GLenum APIENTRY glGetError(void);

#endif /* __gl_h_ */
