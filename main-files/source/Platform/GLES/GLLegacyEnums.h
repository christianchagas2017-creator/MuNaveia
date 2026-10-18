#pragma once

// Constantes do OpenGL classico que nao existem no OpenGL ES 3 (valores identicos aos do OpenGL).

#ifndef GL_QUADS
#define GL_QUADS                   0x0007
#define GL_QUAD_STRIP              0x0008
#define GL_POLYGON                 0x0009
#define GL_MODELVIEW               0x1700
#define GL_PROJECTION              0x1701
#define GL_VERTEX_ARRAY            0x8074
#define GL_NORMAL_ARRAY            0x8075
#define GL_COLOR_ARRAY             0x8076
#define GL_TEXTURE_COORD_ARRAY     0x8078
#define GL_ALPHA_TEST              0x0BC0
#define GL_FOG                     0x0B60
#define GL_FOG_DENSITY             0x0B62
#define GL_FOG_START               0x0B63
#define GL_FOG_END                 0x0B64
#define GL_FOG_MODE                0x0B65
#define GL_FOG_COLOR               0x0B66
#define GL_EXP                     0x0800
#define GL_EXP2                    0x0801
#define GL_TEXTURE_ENV             0x2300
#define GL_TEXTURE_ENV_MODE        0x2200
#define GL_MODULATE                0x2100
#define GL_DECAL                   0x2101
#define GL_ADD                     0x0104
#define GL_CLAMP                   0x2900
#define GL_CURRENT_COLOR           0x0B00
#define GL_MODELVIEW_MATRIX        0x0BA6
#define GL_PROJECTION_MATRIX       0x0BA7
#define GL_POLYGON_MODE            0x0B40
#define GL_FILL                    0x1B02
#define GL_LINE                    0x1B01
#define GL_POINT                   0x1B00
#define GL_ALL_ATTRIB_BITS         0x000FFFFF
#define GL_MULTISAMPLE_ARB         0x809D
#define GL_VERTEX_PROGRAM_ARB      0x8620
#define GL_FRAGMENT_PROGRAM_ARB    0x8804
#endif
