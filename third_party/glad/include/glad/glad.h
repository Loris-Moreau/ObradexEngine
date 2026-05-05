/*
 * GLAD OpenGL 4.1 Core — stub header.
 *
 * Replace this file with a real GLAD-generated header.
 * Generate one at: https://glad.dav1d.de/
 *   API  : gl
 *   Version: 4.1
 *   Profile: Core
 *   Options: Generate a loader
 *
 * Then copy glad.h → third_party/glad/include/glad/glad.h
 *      and glad.c → third_party/glad/src/glad.c
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef void* (* GLADloadproc)(const char* name);
int gladLoadGLLoader(GLADloadproc);
#ifdef __cplusplus
}
#endif
