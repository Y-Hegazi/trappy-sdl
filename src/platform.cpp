#include "../include/platform.h"

Platform::Platform(const SDL_FRect &b, std::shared_ptr<Texture> tex)
    : bounds(b), texture(std::move(tex)) {
  if (texture) {
    sprite = std::make_unique<Sprite>(texture.get());
  }
}

SDL_FRect Platform::getCollisionBounds() const { return bounds; }

std::pair<float, float> Platform::getPos() const {
  return {bounds.x, bounds.y};
}

void Platform::setPos(float x, float y) {
  bounds.x = x;
  bounds.y = y;
}

void Platform::onCollision(Collideable * /*other*/, float /*normalX*/,
                           float /*normalY*/, float /*penetration*/) {
  // Static platform: no response required. Game-level collision resolver should
  // push dynamic objects out of penetration.
}
