#include <opengl.h>

#ifndef MAC_PLUGIN
struct {
	bool installed;

	PFNGLCREATEPROGRAMPROC glCreateProgram;
	PFNGLCREATESHADERPROC glCreateShader;
	PFNGLATTACHSHADERPROC glAttachShader;
	PFNGLCOMPILESHADERPROC glCompileShader;
	PFNGLLINKPROGRAMPROC glLinkProgram;
	PFNGLSHADERSOURCEPROC glShaderSource;
	PFNGLDELETESHADERPROC glDeleteShader;
	PFNGLDELETEPROGRAMPROC glDeleteProgram;
	PFNGLUSEPROGRAMPROC glUseProgram;
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	PFNGLGETSHADERIVPROC glGetShaderiv;
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	PFNGLGETPROGRAMIVPROC glGetProgramiv;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	PFNGLUNIFORM1FPROC glUniform1f;
	PFNGLUNIFORM2FPROC glUniform2f;
	PFNGLUNIFORM3FPROC glUniform3f;
	PFNGLUNIFORM4FPROC glUniform4f;
	PFNGLUNIFORM1IPROC glUniform1i;

	PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	PFNGLBINDBUFFERPROC glBindBuffer;
} s_gl = { false };

#ifndef MAC_PLUGIN
bool installGLExtensions() {
	if (s_gl.installed)
		return true;

	if ((s_gl.glCreateProgram = (PFNGLCREATEPROGRAMPROC) GETPROCADDRESS("glCreateProgram")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glCreateShader = (PFNGLCREATESHADERPROC) GETPROCADDRESS("glCreateShader")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}
	if ((s_gl.glAttachShader = (PFNGLATTACHSHADERPROC) GETPROCADDRESS("glAttachShader")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glCompileShader = (PFNGLCOMPILESHADERPROC) GETPROCADDRESS("glCompileShader")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glLinkProgram = (PFNGLLINKPROGRAMPROC) GETPROCADDRESS("glLinkProgram")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glShaderSource = (PFNGLSHADERSOURCEPROC) GETPROCADDRESS("glShaderSource")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glDeleteShader = (PFNGLDELETESHADERPROC) GETPROCADDRESS("glDeleteShader")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glDeleteProgram = (PFNGLDELETEPROGRAMPROC) GETPROCADDRESS("glDeleteProgram")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glUseProgram = (PFNGLUSEPROGRAMPROC) GETPROCADDRESS("glUseProgram")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) GETPROCADDRESS("glGetShaderInfoLog")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glGetShaderiv = (PFNGLGETSHADERIVPROC) GETPROCADDRESS("glGetShaderiv")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) GETPROCADDRESS("glGetProgramInfoLog")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glGetProgramiv = (PFNGLGETPROGRAMIVPROC) GETPROCADDRESS("glGetProgramiv")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) GETPROCADDRESS("glGetUniformLocation")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glUniform2f = (PFNGLUNIFORM2FPROC) GETPROCADDRESS("glUniform2f")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	/*
	 * New functions
	 */

	if ((s_gl.glUniform1f = (PFNGLUNIFORM1FPROC) GETPROCADDRESS("glUniform1f")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glUniform3f = (PFNGLUNIFORM3FPROC) GETPROCADDRESS("glUniform3f")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glUniform4f = (PFNGLUNIFORM4FPROC) GETPROCADDRESS("glUniform4f")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glUniform1i = (PFNGLUNIFORM1IPROC) GETPROCADDRESS("glUniform1i")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) GETPROCADDRESS("glGetAttribLocation")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) GETPROCADDRESS("glEnableVertexAttribArray")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) GETPROCADDRESS("glDisableVertexAttribArray")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) GETPROCADDRESS("glVertexAttribPointer")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	if ((s_gl.glBindBuffer = (PFNGLBINDBUFFERPROC) GETPROCADDRESS("glBindBuffer")) == NULL) {
		std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
		return false;
	}

	s_gl.installed = true;

	return true;
}
#endif /* N MAC_PLUGIN */

GLAPI GLuint APIENTRY glCreateProgram (void) {
	return s_gl.glCreateProgram();
}

GLAPI GLuint APIENTRY glCreateShader (GLenum type) {
	return s_gl.glCreateShader(type);
}

GLAPI void APIENTRY glDeleteProgram (GLuint program) {
	s_gl.glDeleteProgram(program);
}

GLAPI void APIENTRY glDeleteShader (GLuint shader) {
	s_gl.glDeleteShader(shader);
}

GLAPI void APIENTRY glAttachShader (GLuint program, GLuint shader) {
	s_gl.glAttachShader(program, shader);
}

GLAPI void APIENTRY glCompileShader (GLuint shader) {
	s_gl.glCompileShader(shader);
}

GLAPI void APIENTRY glLinkProgram (GLuint program) {
	s_gl.glLinkProgram(program);
}

GLAPI void APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar* *string, const GLint *length) {
	s_gl.glShaderSource(shader, count, string, length);
}

GLAPI void APIENTRY glUseProgram (GLuint program) {
	s_gl.glUseProgram(program);
}

GLAPI void APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint *params) {
	s_gl.glGetProgramiv(program, pname, params);
}

GLAPI void APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	s_gl.glGetProgramInfoLog(program, bufSize, length, infoLog);
}

GLAPI void APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint *params) {
	s_gl.glGetShaderiv(shader, pname, params);
}

GLAPI void APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	s_gl.glGetShaderInfoLog(shader, bufSize, length, infoLog);
}

GLAPI GLint APIENTRY glGetUniformLocation (GLuint program, const GLchar *name) {
	return s_gl.glGetUniformLocation(program, name);
}

GLAPI void APIENTRY glUniform1f (GLint location, GLfloat v0) {
	s_gl.glUniform1f(location, v0);
}

GLAPI void APIENTRY glUniform2f (GLint location, GLfloat v0, GLfloat v1) {
	s_gl.glUniform2f(location, v0, v1);
}

GLAPI void APIENTRY glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	s_gl.glUniform4f(location, v0, v1, v2, v3);
}

GLAPI void APIENTRY glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
	s_gl.glUniform3f(location, v0, v1, v2);
}

GLAPI void APIENTRY glUniform1i (GLint location, GLint v0) {
	s_gl.glUniform1i(location, v0);
}

GLAPI void APIENTRY glBindBuffer (GLenum target, GLuint buffer) {
	s_gl.glBindBuffer(target, buffer);
}

GLAPI void APIENTRY glDisableVertexAttribArray (GLuint index) {
	s_gl.glDisableVertexAttribArray(index);
}

GLAPI void APIENTRY glEnableVertexAttribArray (GLuint index) {
	s_gl.glEnableVertexAttribArray(index);
}

GLAPI GLint APIENTRY glGetAttribLocation (GLuint program, const GLchar *name) {
	return s_gl.glGetAttribLocation(program, name);
}

GLAPI void APIENTRY glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) {
	return s_gl.glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

#endif /* N MAC_PLUGIN */