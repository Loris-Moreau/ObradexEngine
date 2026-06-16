// miniaudio_impl.cpp - Single compilation unit for the miniaudio library.
// Must be compiled exactly once. All other files include AudioSystem.h
// which includes miniaudio.h without the IMPLEMENTATION define.
#define MINIAUDIO_IMPLEMENTATION
#include "../../third_party/miniaudio/miniaudio.h"
