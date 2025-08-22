#ifndef DISAPPEARING_PLATFORM_H
#define DISAPPEARING_PLATFORM_H

#include "config.h"
#include "platform.h"
#include <chrono>

/**
 * Platform that disappears after being stepped on from above, then reappears
 * after a delay
 */
class DisappearingPlatform : public Platform {
public:
  enum class State { VISIBLE, DISAPPEARING, DISAPPEARED, REAPPEARING };

  DisappearingPlatform(const SDL_FRect &bounds, std::shared_ptr<Texture> tex);
  ~DisappearingPlatform() override = default;

  // Override collision to detect top collision
  void onCollision(Collideable *other, float normalX, float normalY,
                   float penetration) override;

  // Update timer and state transitions
  void update(float dt);

  // Override collision bounds - no collision when disappeared
  SDL_FRect getCollisionBounds() const override;

  // State queries
  bool isVisible() const {
    return state == State::VISIBLE || state == State::DISAPPEARING;
  }
  bool canCollide() const { return state == State::VISIBLE; }
  State getState() const { return state; }

private:
  State state = State::VISIBLE;
  bool triggered = false;
  float timer = 0.0f;
  float disappearDelay = DISAPPEAR_DELAY_MS / 1000.0f; // Convert ms to seconds
  float reappearDelay = REAPPEAR_DELAY_MS / 1000.0f;   // Convert ms to seconds
};

#endif // DISAPPEARING_PLATFORM_H
