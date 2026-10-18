#pragma once

// Escolhe o header de OpenGL ES 3 da plataforma.
#if defined(__APPLE__)
#  include <TargetConditionals.h>
#  if TARGET_OS_IPHONE
#    include <OpenGLES/ES3/gl.h>
#    include <OpenGLES/ES3/glext.h>
#  else
#    include <OpenGL/gl3.h>
#  endif
#else
#  include <GLES3/gl3.h>
#endif
