#ifndef SPRITE_H
#define SPRITE_H

/**
 * 2D sprite class for rendering textured quads with animation support.
 * Holds a non-owning reference to a Texture and manages rendering state.
 *
 * Features:
 * - Position, size, and flip transformations
 * - Frame-based sprite sheet animation
 * - Collision detection (both integer and float precision)
 * - Sub-pixel positioning with SDL_FRect
 */

#include "config.h"
#include "texture.h"
#include <vector>

class Sprite {
private:
  // Non-owning pointer to a Texture. Sprite does not manage the Texture's
  // lifetime — the caller must ensure the Texture remains valid while this
  // Sprite is used.
  Texture *texture;

  // Source rectangle within texture (pixel coordinates)
  SDL_Rect src = {0, 0, 0, 0};

  // Destination rectangle on screen (float for smooth movement)
  SDL_FRect dest = {0, 0, 0, 0};

  // Rendering flip state
  SDL_RendererFlip flip{SDL_FLIP_NONE};

  // Visibility flag (may be unused in current implementation)
  bool visible{true};

  // Animation system - frame-based sprite sheet animation
  std::vector<SDL_Rect> frames; // Source rects for each frame
  size_t currentFrame{0};       // Current frame index
  float frameDuration{0.0f};    // Seconds per frame
  float frameTimer{0.0f};       // Time accumulator
  bool looping{true};           // Loop animation when it ends
  bool playing{false};          // Animation playing state

public:
  /**
   * Construct a Sprite for an existing Texture.
   * @param tex Non-owning pointer to texture (must outlive sprite)
   * @note Passing nullptr will cause undefined behavior when render() is called
   */
  explicit Sprite(Texture *tex);

  /**
   * Render sprite to screen using current state.
   * @param renderer Pointer to SDL_Renderer (must be non-null)
   * @param flip Optional flip mode override (defaults to SDL_FLIP_NONE)
   * @return 0 on success, negative error code on failure
   * @note This method is logically const: it does not modify sprite's members
   */
  int render(SDL_Renderer *renderer,
             SDL_RendererFlip flip = SDL_FLIP_NONE) const;

  // Source rectangle management
  void setSrcRect(const SDL_Rect &rect);
  SDL_Rect getSrcRect() const;

  // Destination rectangle management
  void setDestRect(const SDL_FRect &rect);
  SDL_FRect getDestRect() const;

  // Texture access
  Texture *getTexture() const;
  void changeTexture(Texture *tex);

  // Position and size utilities
  void setPosition(float x, float y);
  SDL_Point position() const;
  void setSize(float w, float h);
  SDL_Point size() const;
  void scale(float factor_w, float factor_h);

  // Animation control
  void setFrames(const std::vector<SDL_Rect> &f, float secondsPerFrame,
                 bool loop = true);
  void play();
  void stop();

  /**
   * Update animation state.
   * @param dt Delta time in seconds since last update
   * @note Call this from your game loop to advance animation frames
   */
  void update(float dt);

  // Collision detection helpers
  SDL_Rect boundingBox() const;
  bool containsPoint(int x, int y) const;
  static bool intersects(const Sprite &a, const Sprite &b);
  static bool intersectsF(const Sprite &a, const Sprite &b);
  static bool intersectsF(const Sprite &a, const SDL_Rect &b);
  static bool intersectsF(const Sprite &a, const SDL_FRect &b);
};

#endif