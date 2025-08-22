#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "collideable.h"
#include "config.h"
#include "platform.h"
#include "sprite.h"
#include "texture.h"
#include <memory>
/**
 * Base class for projectiles (arrows, bullets, coins, etc.)
 * Can be collected by player or bounce off walls
 */
class Projectile : public Collideable {
public:
  enum class ProjectileType {
    COIN,  // Collectible - gives points/score
    ARROW, // Harmful - damages player
    BULLET // Harmful - damages player
  };

  Projectile(const SDL_FRect &bounds, ProjectileType type,
             std::shared_ptr<Texture> tex = nullptr);
  ~Projectile() override = default;

  // Collideable interface
  SDL_FRect getCollisionBounds() const override;
  std::pair<float, float> getPos() const override;
  void setPos(float x, float y) override;
  void onCollision(Collideable *other, float normalX, float normalY,
                   float penetration) override;
  bool isStatic() const override { return false; }
  ObjectType getType() const override { return ObjectType::PROJECTILE; }

  // Projectile-specific methods
  ProjectileType getProjectileType() const { return projectileType; }
  void setVelocity(float vx, float vy) {
    vel_x = vx;
    vel_y = vy;
  }
  std::pair<float, float> getVelocity() const { return {vel_x, vel_y}; }

  // Update projectile physics
  void coinBounce(float dt);
  void update(float dt, SDL_Rect &worldBounds,
              bool bounceCoins = COINS_BOUNCING_DEFAULT);

  // Mark for removal (after collision)
  void markForRemoval() { shouldRemove = true; }
  bool shouldBeRemoved() const { return shouldRemove; }

  // Arrow respawn functionality
  void setOriginalPosition(float x, float y) { 
    originalX = x; 
    originalY = y; 
  }
  void resetToOriginalPosition() {
    bounds.x = originalX;
    bounds.y = originalY;
    shouldRemove = false;
  }
  std::pair<float, float> getOriginalPosition() const { return {originalX, originalY}; }

  // Rendering
  std::shared_ptr<Texture> getTexture() const { return texture; }
  Sprite *getSprite() const { return sprite.get(); }
  void transferSpriteOwnership(std::unique_ptr<Sprite> newSprite) {
    sprite = std::move(newSprite);
  }

  void setSprite(std::shared_ptr<Sprite> sprite_) { sprite = (sprite_); }

  // Render the projectile
  void setSpriteSrcRect(const SDL_Rect &srcRect);
  void render(SDL_Renderer *renderer) const;

private:
  SDL_FRect bounds;
  ProjectileType projectileType;
  std::shared_ptr<Texture> texture;
  std::shared_ptr<Sprite> sprite;

  // Physics
  float vel_x = 0.0f;
  float vel_y = 0.0f;
  float gravity = PROJECTILE_GRAVITY; // For arrows/bullets that fall
  int dampingDir = 1; // for coins, 1 means bounce up, -1 means bounce down

  float bounceDurationTimer = 0;

  // Coin bobbing animation (visual only, no physics)
  float baseY = 0.0f;         // Original Y position
  float bobTimer = 0.0f;      // Animation timer
  float bobAmplitude = 16.0f; // Bounce height in pixels
  float bobFrequency = 2.0f;  // Bounces per second

  // State
  bool shouldRemove = false;
  
  // Arrow respawn system
  float originalX = 0.0f;
  float originalY = 0.0f;
};

#endif // PROJECTILE_H
