#include "../include/projectile.h"
#include "../include/player.h"
#include <cmath>

Projectile::Projectile(const SDL_FRect &b, ProjectileType type,
                       std::shared_ptr<Texture> tex)
    : bounds(b), projectileType(type), texture(std::move(tex)) {
  // Create sprite if texture is provided
  if (texture) {
    sprite = std::make_unique<Sprite>(texture.get());
  }

  // Initialize base position for coin bobbing
  if (projectileType == ProjectileType::COIN) {
    baseY = bounds.y;
  }
}

SDL_FRect Projectile::getCollisionBounds() const { return bounds; }

std::pair<float, float> Projectile::getPos() const {
  return {bounds.x, bounds.y};
}

void Projectile::setPos(float x, float y) {
  bounds.x = x;
  bounds.y = y;
}

void Projectile::coinBounce(float dt) {
  // No longer used - coins use visual bobbing instead
}

void Projectile::update(float dt, SDL_Rect &worldBounds, bool bounceCoins) {
  // Handle coin bobbing animation
  if (projectileType == ProjectileType::COIN) {
    bobTimer += dt;
    // Calculate bobbing offset using sine wave
    float bobOffset = sin(bobTimer * bobFrequency * 2.0f * M_PI) * bobAmplitude;
    // Update visual position (don't change actual bounds for collision)
    if (sprite) {
      SDL_FRect renderBounds = bounds;
      renderBounds.y = baseY + bobOffset;
      sprite->setDestRect(renderBounds);
    }
  }

  // Update position (only for non-coins, since coins use visual bobbing)
  if (projectileType != ProjectileType::COIN) {
    bounds.x += vel_x * dt;
    bounds.y += vel_y * dt;
  }

  // Update sprite animation if it exists
  if (sprite && projectileType != ProjectileType::COIN) {
    sprite->update(dt);
  }

  // Remove if out of world bounds (except for arrows which respawn)
  if (bounds.x + bounds.w < worldBounds.x ||
      bounds.x > worldBounds.x + worldBounds.w ||
      bounds.y + bounds.h < worldBounds.y ||
      bounds.y > worldBounds.y + worldBounds.h) {

    if (projectileType == ProjectileType::ARROW) {
      // Respawn arrows at original position
      resetToOriginalPosition();
    } else {
      // Remove other projectiles
      markForRemoval();
    }
  }
}

void Projectile::render(SDL_Renderer *renderer) const {
  if (sprite) {
    // Convert float rect to SDL_Rect for rendering
    SDL_Rect renderRect = {
        static_cast<int>(bounds.x), static_cast<int>(bounds.y),
        static_cast<int>(bounds.w), static_cast<int>(bounds.h)};
    sprite->setDestRect(bounds);
    sprite->render(renderer, SDL_FLIP_NONE);
  }
}

void Projectile::setSpriteSrcRect(const SDL_Rect &srcRect) {
  if (sprite) {
    sprite->setSrcRect(srcRect);
  }
}

void Projectile::onCollision(Collideable *other, float normalX, float normalY,
                             float penetration) {
  ObjectType otherType = other->getType();

  if (otherType == ObjectType::PLAYER) {
    if (projectileType == ProjectileType::COIN) {
      // Coin collected - mark for removal
      markForRemoval();
      // put sound here
      // Add score
    } else if (projectileType == ProjectileType::ARROW) {
      // Arrow hits player - kill player and respawn arrow
      RectPlayer *player = static_cast<RectPlayer *>(other);
      player->setDead(true);
      // put sound here
    }
  } else if (otherType == ObjectType::STATIC_OBJECT) {
    // Projectile hits wall/platform
    if (projectileType == ProjectileType::ARROW) {
      // Arrows respawn when hitting walls
      resetToOriginalPosition();
    } else {
      // Other projectiles disappear on impact
      markForRemoval();
    }
  }
}
