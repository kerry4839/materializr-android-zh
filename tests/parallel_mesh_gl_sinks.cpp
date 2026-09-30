// The integration uses the real renderer's CPU tessellation and tag maps.
// Only the GL uploads are sinks: no window or graphics context is created.
#include "gl_common.h"

extern "C" {
void glGenVertexArrays(GLsizei n, GLuint* ids) { while (n--) *ids++ = 0; }
void glGenBuffers(GLsizei n, GLuint* ids) { while (n--) *ids++ = 0; }
void glBindVertexArray(GLuint) {}
void glBindBuffer(GLenum, GLuint) {}
void glBufferData(GLenum, GLsizeiptr, const void*, GLenum) {}
void glEnableVertexAttribArray(GLuint) {}
void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) {}
void glDeleteVertexArrays(GLsizei, const GLuint*) {}
void glDeleteBuffers(GLsizei, const GLuint*) {}
void glGenFramebuffers(GLsizei n, GLuint* ids) { while (n--) *ids++ = 0; }
void glBindFramebuffer(GLenum, GLuint) {}
void glGenTextures(GLsizei n, GLuint* ids) { while (n--) *ids++ = 0; }
void glBindTexture(GLenum, GLuint) {}
void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
void glTexParameteri(GLenum, GLenum, GLint) {}
void glFramebufferTexture2D(GLenum, GLenum, GLenum, GLuint, GLint) {}
void glGenRenderbuffers(GLsizei n, GLuint* ids) { while (n--) *ids++ = 0; }
void glBindRenderbuffer(GLenum, GLuint) {}
void glRenderbufferStorage(GLenum, GLenum, GLsizei, GLsizei) {}
void glRenderbufferStorageMultisample(GLenum, GLsizei, GLenum, GLsizei, GLsizei) {}
void glFramebufferRenderbuffer(GLenum, GLenum, GLenum, GLuint) {}
}
