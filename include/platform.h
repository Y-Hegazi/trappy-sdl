#ifndef PLATFORM_H
#define PLATFORM_H

#include "collideable.h"
#include "sprite.h"
#include "texture.h"
#include <memory>

/**
 * Simple static platform that participates in collisions.
 * Stores collision bounds in SDL_FRect (float-based) and an optional texture
 * for rendering.
 */
class Platform : public Collideable {
public:
  Platform(const SDL_FRect &bounds, std::shared_ptr<Texture> tex);
  ~Platform() override = default;

  // Collideable interface
  SDL_FRect getCollisionBounds() const override;
  std::pair<float, float> getPos() const override;
  void setPos(float x, float y) override;
  void onCollision(Collideable *other, float normalX, float normalY,
                   float penetration) override;
  bool isStatic() const override { return true; }

  /**
   * Get object type for collision handling
   * @return ObjectType::STATIC_OBJECT
   */
  ObjectType getType() const override { return ObjectType::STATIC_OBJECT; }

  // Rendering helper: returns the texture or nullptr
  std::shared_ptr<Texture> getTexture() const { return texture; }
  Sprite *getSprite() const { return sprite.get(); }

private:
  SDL_FRect bounds;
  std::shared_ptr<Texture> texture;
  std::unique_ptr<Sprite> sprite; // Optional sprite for rendering
};

#endif // PLATFORM_H
