#include "../include/texture.h"
#include <stdexcept>
#include <string>

Texture::Texture(SDL_Renderer *renderer, const char *filePath)
    : loadedSurface(nullptr), texture(nullptr, [](SDL_Texture *tex) {
        if (tex)
          SDL_DestroyTexture(tex);
      }) {

  // Validate input parameters
  if (!renderer) {
    throw std::runtime_error("Renderer cannot be null");
  }
  if (!filePath) {
    throw std::runtime_error("File path cannot be null");
  }

  // Load surface from image file using SDL_image
  loadedSurface = IMG_Load(filePath);
  if (!loadedSurface) {
    throw std::runtime_error("Failed to load image: " + std::string(filePath) +
                             " - " + IMG_GetError());
  }

  // Create texture from surface
  SDL_Texture *newTexture =
      SDL_CreateTextureFromSurface(renderer, loadedSurface);

  // Free surface immediately - no longer needed after texture creation
  SDL_FreeSurface(loadedSurface);
  loadedSurface = nullptr;

  // Check if texture creation succeeded
  if (!newTexture) {
    throw std::runtime_error("Failed to create texture from: " +
                             std::string(filePath) + " - " + SDL_GetError());
  }

  // Transfer ownership to smart pointer
  texture.reset(newTexture);
}

SDL_Texture *Texture::get() const { return texture.get(); }

SDL_Texture *Texture::release() noexcept { return texture.release(); }
