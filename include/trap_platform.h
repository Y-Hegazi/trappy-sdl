#ifndef TRAP_PLATFORM_H
#define TRAP_PLATFORM_H

#include "config.h"
#include "platform.h"

/**
 * Platform that kills the player on contact and has a reduced collision box
 */
class TrapPlatform : public Platform {
public:
  TrapPlatform(const SDL_FRect &bounds, std::shared_ptr<Texture> tex);
  ~TrapPlatform() override = default;

  // Override collision bounds to be x% smaller
  SDL_FRect getCollisionBounds() const override;

  SDL_FRect getOriginalBounds() const { return originalBounds; }

  // Override collision to kill player
  void onCollision(Collideable *other, float normalX, float normalY,
                   float penetration) override;

  PlatformType getPlatformType() const override { return type; }

private:
  PlatformType type = PlatformType::TRAP;
  SDL_FRect originalBounds;
  SDL_FRect reducedBounds;
};

#endif // TRAP_PLATFORM_H
