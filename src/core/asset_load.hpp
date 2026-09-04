#pragma once

// ============================================================================
// asset_load - Loud failures for missing assets
// ============================================================================
// Every asset is loaded by a path relative to the working directory
// ("assets/..."), so running the executable from anywhere but build/ leaves
// raylib returning an id-0 texture. Nothing crashes: the affected sprites
// simply draw nothing, which reads on screen as a rendering bug rather than a
// missing file.
//
// These wrappers name the SYSTEM that asked for the file, which raylib's own
// log cannot know, and give one place to change the policy later (fall back to
// a magenta placeholder, or abort in a release build).
//
// Header-only on purpose: a .cpp would have to be added to BOTH executable
// source lists in CMakeLists.txt. See debug_log.hpp for the same reasoning.
// ============================================================================

#include "core/debug_log.hpp"
#include <raylib.h>

namespace assets {

// Failure is detected by id == 0 rather than IsTextureReady/IsTextureValid:
// the build renames one to the other for rlImGui's benefit, and the raw id
// check is immune to which name is in scope.
inline Texture2D loadTexture(const char *path, const char *owner) {
  Texture2D texture = LoadTexture(path);
  if (texture.id == 0) {
    debuglog::log("ASSET", "%s could not load '%s' - it will draw nothing",
                  owner, path);
  }
  return texture;
}

inline Shader loadShader(const char *vsPath, const char *fsPath,
                         const char *owner) {
  Shader shader = LoadShader(vsPath, fsPath);
  if (shader.id == 0) {
    debuglog::log("ASSET", "%s could not load shader '%s' - effect disabled",
                  owner, fsPath ? fsPath : "(default)");
  }
  return shader;
}

} // namespace assets
