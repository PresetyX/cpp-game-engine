// Single translation unit holding the stb_image implementation (no warnings
// policy). STBI_NO_STDIO is set by CMake: files are read by engine/assets/file_io and decoded
// from memory.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
