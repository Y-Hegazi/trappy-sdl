#include "../include/disappearing_platform.h"
#include "../include/collideable.h"
#include <iostream>
DisappearingPlatform::DisappearingPlatform(const SDL_FRect &bounds,
                                           std::shared_ptr<Texture> tex)
    : Platform(bounds, tex) {}

void DisappearingPlatform::onCollision(Collideable *other, float normalX,
                                       float normalY, float penetration) {
  // Only handle collision when platform is visible and can collide
  if (state != State::VISIBLE) {
    return;
  }

  // Call parent collision handling
  Platform::onCollision(other, normalX, normalY, penetration);

  // Check if collision is from above (player landing on platform)
  // normalY > 0 means player center is above platform center (landing on top)
  if (other->getType() == ObjectType::PLAYER && normalY > 0 && !triggered) {
    std::cout << "DisappearingPlatform: Player landed on platform! normalY="
              << normalY << std::endl;
    triggered = true;
    state = State::DISAPPEARING;
    timer = 0.0f;
  } else if (other->getType() == ObjectType::PLAYER) {
    std::cout << "DisappearingPlatform: Player collision but not triggering - "
                 "normalY="
              << normalY << ", triggered=" << triggered
              << ", state=" << static_cast<int>(state) << std::endl;
  }
}

void DisappearingPlatform::update(float dt) {
  if (state != State::VISIBLE) {
    timer += dt;
  }

  switch (state) {
  case State::VISIBLE:
    // Nothing to do, waiting for trigger
    break;

  case State::DISAPPEARING:
    if (timer >= disappearDelay) {
      std::cout << "DisappearingPlatform: Platform disappeared! Timer=" << timer
                << std::endl;
      state = State::DISAPPEARED;
      timer = 0.0f;
    }
    break;

  case State::DISAPPEARED:
    if (timer >= reappearDelay) {
      state = State::REAPPEARING;
      timer = 0.0f;
    }
    break;

  case State::REAPPEARING:
    // Instant reappear for now, could add fade-in animation here
    state = State::VISIBLE;
    triggered = false; // Reset trigger so it can be activated again
    timer = 0.0f;
    break;
  }
}

SDL_FRect DisappearingPlatform::getCollisionBounds() const {
  // Return empty bounds when platform can't collide
  if (!canCollide()) {
    return {0, 0, 0, 0};
  }
  return Platform::getCollisionBounds();
}
