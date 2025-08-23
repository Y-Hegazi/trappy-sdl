#include "../include/trap_platform.h"
#include "../include/collideable.h"
#include "../include/player.h"

TrapPlatform::TrapPlatform(const SDL_FRect &bounds,
                           std::shared_ptr<Texture> tex)
    : Platform(bounds, tex), originalBounds(bounds) {

  // Calculate reduced bounds for collision
  float reduction = TRAP_HITBOX_REDUCTION;
  float widthReduction = bounds.w * reduction;
  float heightReduction = bounds.h * reduction;

  reducedBounds = {bounds.x + widthReduction / 2.0f,
                   bounds.y + heightReduction / 2.0f, bounds.w - widthReduction,
                   bounds.h - heightReduction};
}

SDL_FRect TrapPlatform::getCollisionBounds() const { return reducedBounds; }

void TrapPlatform::onCollision(Collideable *other, float normalX, float normalY,
                               float penetration) {
  // Call parent collision handling first
  Platform::onCollision(other, normalX, normalY, penetration);
  // killing player is handled in game.cpp
}
