#ifndef TEXTURE_H
#define TEXTURE_H

#include "config.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * RAII wrapper for SDL_Texture that handles automatic cleanup.
 * Loads textures from image files and manages their lifetime safely.
 *
 * This class provides:
 * - Automatic resource management via RAII
 * - Exception-safe loading with detailed error messages
 * - Move semantics for efficient transfer of ownership
 * - Non-copyable semantics to prevent accidental duplication
 */
class Texture {
private:
  // Temporary surface used during loading (freed immediately after texture
  // creation)
  SDL_Surface *loadedSurface;

  // Smart pointer with custom deleter for automatic SDL_Texture cleanup
  std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)> texture;

public:
  /**
   * Load texture from image file.
   * @param renderer SDL renderer to create texture with (must be non-null)
   * @param filePath Path to image file (PNG, JPG, etc.) (must be non-null)
   * @throws std::runtime_error if loading fails or parameters are invalid
   */
  Texture(SDL_Renderer *renderer, const char *filePath);

  /**
   * Get raw SDL_Texture pointer for rendering operations.
   * @return Non-owning pointer to SDL_Texture, or nullptr if invalid
   */
  SDL_Texture *get() const;

  // Rule of 5: destructor, copy/move constructors and assignments
  ~Texture() = default;
  Texture(const Texture &) =
      delete; // Non-copyable - prevents texture duplication
  Texture &operator=(const Texture &) = delete; // Non-copyable assignment
  Texture(Texture &&) = default; // Movable - allows efficient transfer
  Texture &operator=(Texture &&) = default; // Movable assignment
};

#endif
