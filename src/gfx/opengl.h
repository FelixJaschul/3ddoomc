#pragma once

#ifdef EMSCRIPTEN
    #include <GLES3/gl3.h>
    #include "emscripten.h"
#else
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/gl3.h>
#endif // ifdef EMSCRIPTEN
