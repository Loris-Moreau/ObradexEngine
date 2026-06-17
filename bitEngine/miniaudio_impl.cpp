// miniaudio_impl.cpp - Single compilation unit for the miniaudio library.
// Must be compiled exactly once. All other files include AudioSystem.h
// which includes miniaudio.h without the IMPLEMENTATION define.
#define MINIAUDIO_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4244)  // miniaudio: uint64->uint32 conversion in library code
#include "../../third_party/miniaudio/miniaudio.h"
#pragma warning(pop)
