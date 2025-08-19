#include "../include/sprite.h"
#include <cassert>

Sprite::Sprite(Texture *tex) : texture(tex) {
  // Validate texture pointer at construction
  // Note: Using assert for debug builds; consider throwing exception for
  // production
  assert(tex && "Sprite requires valid Texture pointer");
}

int Sprite::render(SDL_Renderer *renderer,
                   SDL_RendererFlip flipOverride) const {
  // Early exit if invisible or invalid state
  if (!visible || !renderer || !texture || !texture->get()) {
    return -1;
  }

  // Use parameter flip if provided, otherwise use member flip
  SDL_RendererFlip activeFlip =
      (flipOverride != SDL_FLIP_NONE) ? flipOverride : flip;

  // Render using extended copy function with float destination rect
  // SDL_RenderCopyExF allows for sub-pixel positioning and smooth movement
  return SDL_RenderCopyExF(renderer, texture->get(), &src, &dest, 0.0, nullptr,
                           activeFlip);
}

void Sprite::setSrcRect(const SDL_Rect &rect) {
  src = rect;
  // Clear animation frames when manually setting source rect
  // This prevents conflicts between manual rect setting and animation
  frames.clear();
  playing = false;
}

SDL_Rect Sprite::getSrcRect() const { return src; }

void Sprite::setDestRect(const SDL_FRect &rect) { dest = rect; }

SDL_FRect Sprite::getDestRect() const { return dest; }

Texture *Sprite::getTexture() const { return texture; }

void Sprite::changeTexture(Texture *tex) {
  texture = tex;
  // Reset animation state when changing texture
  // New texture may have different dimensions or frame layout
  frames.clear();
  playing = false;
  currentFrame = 0;
  frameTimer = 0.0f;
}

void Sprite::setPosition(float x, float y) {
  dest.x = x;
  dest.y = y;
}

SDL_Point Sprite::position() const {
  // Convert float position to integer point
  // Note: This truncates fractional parts
  return {static_cast<int>(dest.x), static_cast<int>(dest.y)};
}

void Sprite::setSize(float w, float h) {
  dest.w = w;
  dest.h = h;
}

SDL_Point Sprite::size() const {
  // Convert float size to integer point
  // Note: This truncates fractional parts
  return {static_cast<int>(dest.w), static_cast<int>(dest.h)};
}

void Sprite::scale(float factor_w, float factor_h) {
  // Multiply current size by scaling factors
  dest.w *= factor_w;
  dest.h *= factor_h;
}

void Sprite::setFrames(const std::vector<SDL_Rect> &f, float secondsPerFrame,
                       bool loop) {
  frames = f;
  frameDuration = secondsPerFrame;
  looping = loop;
  currentFrame = 0;
  frameTimer = 0.0f;

  // Start playing if frames are provided
  playing = !frames.empty();

  // Set initial frame as source rect
  if (!frames.empty()) {
    src = frames[0];
  }
}

void Sprite::play() {
  // Only play if we have frames to animate
  if (!frames.empty()) {
    playing = true;
  }
}

void Sprite::stop() {
  playing = false;
  currentFrame = 0;
  frameTimer = 0.0f;

  // Reset to first frame
  if (!frames.empty()) {
    src = frames[0];
  }
}

void Sprite::update(float dt) {
  // Skip update if not playing, no frames, or invalid frame duration
  if (!playing || frames.empty() || frameDuration <= 0.0f) {
    return;
  }

  // Accumulate delta time
  frameTimer += dt;

  // Advance frames while we have enough accumulated time
  // Using while loop handles cases where dt > frameDuration
  while (frameTimer >= frameDuration) {
    frameTimer -= frameDuration;
    currentFrame++;

    // Handle end of animation
    if (currentFrame >= frames.size()) {
      if (looping) {
        // Loop back to start
        currentFrame = 0;
      } else {
        // Stop on last frame
        currentFrame = frames.size() - 1;
        playing = false;
        break;
      }
    }

    // Update source rect to current frame
    src = frames[currentFrame];
  }
}

SDL_Rect Sprite::boundingBox() const {
  // Convert float destination rect to integer bounding box
  // Uses static_cast which truncates towards zero
  return SDL_Rect{static_cast<int>(dest.x), static_cast<int>(dest.y),
                  static_cast<int>(dest.w), static_cast<int>(dest.h)};
}

bool Sprite::containsPoint(int x, int y) const {
  SDL_Rect bbox = boundingBox();

  // Point-in-rectangle test using half-open intervals
  // Inclusive on left/top edges, exclusive on right/bottom edges
  return x >= bbox.x && y >= bbox.y && x < bbox.x + bbox.w &&
         y < bbox.y + bbox.h;
}

bool Sprite::intersects(const Sprite &a, const Sprite &b) {
  // Convert both sprites to integer bounding boxes
  SDL_Rect rectA = a.boundingBox();
  SDL_Rect rectB = b.boundingBox();

  // Use SDL's built-in intersection test
  return SDL_HasIntersection(&rectA, &rectB) == SDL_TRUE;
}

bool Sprite::intersectsF(const Sprite &a, const Sprite &b) {
  // Float-precision intersection test for sub-pixel accuracy
  const SDL_FRect &rectA = a.dest;
  const SDL_FRect &rectB = b.dest;

  // Separating axis test - if any axis shows separation, no intersection
  return !(rectA.x + rectA.w <= rectB.x || // A is completely left of B
           rectB.x + rectB.w <= rectA.x || // B is completely left of A
           rectA.y + rectA.h <= rectB.y || // A is completely above B
           rectB.y + rectB.h <= rectA.y);  // B is completely above A
}

bool Sprite::intersectsF(const Sprite &a, const SDL_Rect &b) {
  // Convert SDL_Rect to SDL_FRect for float-precision test
  SDL_FRect floatRect = {static_cast<float>(b.x), static_cast<float>(b.y),
                         static_cast<float>(b.w), static_cast<float>(b.h)};
  return intersectsF(a, floatRect);
}

bool Sprite::intersectsF(const Sprite &a, const SDL_FRect &b) {
  // Float-precision intersection test for sub-pixel accuracy
  const SDL_FRect &rectA = a.dest;
  const SDL_FRect &rectB = b;

  // Separating axis test - if any axis shows separation, no intersection
  return !(rectA.x + rectA.w <= rectB.x || // A is completely left of B
           rectB.x + rectB.w <= rectA.x || // B is completely left of A
           rectA.y + rectA.h <= rectB.y || // A is completely above B
           rectB.y + rectB.h <= rectA.y);  // B is completely above A
}
