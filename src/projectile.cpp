#include "../include/projectile.h"

Projectile::Projectile(const SDL_FRect &b, ProjectileType type,
                       std::shared_ptr<Texture> tex)
    : bounds(b), projectileType(type), texture(std::move(tex)) {
  // Create sprite if texture is provided
  if (texture) {
    sprite = std::make_unique<Sprite>(texture.get());
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

void Projectile::update(float dt) {
  // Apply gravity to arrows/bullets (not coins)
  if (projectileType != ProjectileType::COIN) {
    vel_y += gravity * dt;
  }

  // Update position
  bounds.x += vel_x * dt;
  bounds.y += vel_y * dt;

  // Update sprite animation if it exists
  if (sprite) {
    sprite->update(dt);
  }
}

void Projectile::render(SDL_Renderer *renderer) const {
  if (sprite) {
    // Convert float rect to SDL_Rect for rendering
    SDL_Rect renderRect = {
        static_cast<int>(bounds.x), static_cast<int>(bounds.y),
        static_cast<int>(bounds.w), static_cast<int>(bounds.h)};
    sprite->render(renderer, SDL_FLIP_NONE);
  }
}

void Projectile::onCollision(Collideable *other, float normalX, float normalY,
                             float penetration) {
  ObjectType otherType = other->getType();

  if (otherType == ObjectType::PLAYER) {
    if (projectileType == ProjectileType::COIN) {
      // Coin collected - mark for removal
      markForRemoval();
      // Add score
    } else {
      // Arrow/bullet hits player - damage and remove
      markForRemoval();
      // 1- Player damage 2- make a player respawn
    }
  } else if (otherType == ObjectType::STATIC_OBJECT) {
    // Projectile hits wall/platform
    if (projectileType == ProjectileType::COIN) {
      // Coins bounce
      if (normalX != 0)
        vel_x = -vel_x * 0.7f; // Bounce horizontally with damping
      if (normalY != 0)
        vel_y = -vel_y * 0.7f; // Bounce vertically with damping
    } else {
      // Arrows/bullets stick to walls
      vel_x = 0;
      vel_y = 0;
      // Could mark for removal after some time
    }
  }
}
