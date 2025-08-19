#include "../include/collision_system.h"
#include <algorithm>
#include <cmath>

bool CollisionSystem::checkAABB(const SDL_FRect &a, const SDL_FRect &b) {
  return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h &&
          a.y + a.h > b.y);
}

void CollisionSystem::resolveCollisions(Collideable *player,
                                        std::vector<Collideable *> &objects) {
  // Check player against all other objects
  for (auto *obj : objects) {
    if (checkAABB(player->getCollisionBounds(), obj->getCollisionBounds())) {
      handleCollision(player, obj);
    }
  }

  // Check projectiles against static objects
  /*for (size_t i = 0; i < objects.size(); ++i) {
    if (objects[i]->getType() != ObjectType::PROJECTILE)
      continue;

    for (size_t j = 0; j < objects.size(); ++j) {
      if (i == j || objects[j]->getType() != ObjectType::STATIC_OBJECT)
        continue;

      if (checkAABB(objects[i]->getCollisionBounds(),
                    objects[j]->getCollisionBounds())) {
        handleCollision(objects[i], objects[j]);
      }
    }
  }*/
}

void CollisionSystem::handleCollision(Collideable *a, Collideable *b) {
  ObjectType typeA = a->getType();
  ObjectType typeB = b->getType();

  // Handle collision based on object types
  if (typeA == ObjectType::PLAYER && typeB == ObjectType::STATIC_OBJECT) {
    handlePlayerVsStatic(a, b);
  } else if (typeA == ObjectType::STATIC_OBJECT &&
             typeB == ObjectType::PLAYER) {
    handlePlayerVsStatic(b, a);
  } else if (typeA == ObjectType::PLAYER && typeB == ObjectType::PROJECTILE) {
    handlePlayerVsProjectile(a, b);
  } else if (typeA == ObjectType::PROJECTILE && typeB == ObjectType::PLAYER) {
    handlePlayerVsProjectile(b, a);
  } else if ((typeA == ObjectType::PROJECTILE &&
              typeB == ObjectType::STATIC_OBJECT) ||
             (typeA == ObjectType::STATIC_OBJECT &&
              typeB == ObjectType::PROJECTILE)) {
    Collideable *projectile = (typeA == ObjectType::PROJECTILE) ? a : b;
    Collideable *staticObj = (typeA == ObjectType::STATIC_OBJECT) ? a : b;
    handleProjectileVsStatic(projectile, staticObj);
  }
}

void CollisionSystem::handlePlayerVsStatic(Collideable *player,
                                           Collideable *staticObj) {
  float normalX, normalY, penetration;
  computeCollisionInfo(player->getCollisionBounds(),
                       staticObj->getCollisionBounds(), normalX, normalY,
                       penetration);

  // Call collision callbacks
  player->onCollision(staticObj, normalX, normalY, penetration);
  staticObj->onCollision(player, -normalX, -normalY, penetration);

  // Move player out of static object
  auto pos = player->getPos();
  player->setPos(pos.first + normalX * penetration,
                 pos.second + normalY * penetration);
}

void CollisionSystem::handlePlayerVsProjectile(Collideable *player,
                                               Collideable *projectile) {
  float normalX, normalY, penetration;
  computeCollisionInfo(player->getCollisionBounds(),
                       projectile->getCollisionBounds(), normalX, normalY,
                       penetration);

  // Call collision callbacks (projectile handles collection/damage logic)
  player->onCollision(projectile, normalX, normalY, penetration);
  projectile->onCollision(player, -normalX, -normalY, penetration);
}

void CollisionSystem::handleProjectileVsStatic(Collideable *projectile,
                                               Collideable *staticObj) {
  float normalX, normalY, penetration;
  computeCollisionInfo(projectile->getCollisionBounds(),
                       staticObj->getCollisionBounds(), normalX, normalY,
                       penetration);

  // Call collision callbacks
  projectile->onCollision(staticObj, normalX, normalY, penetration);
  staticObj->onCollision(projectile, -normalX, -normalY, penetration);

  // Move projectile out of static object
  auto pos = projectile->getPos();
  projectile->setPos(pos.first + normalX * penetration,
                     pos.second + normalY * penetration);
}

void CollisionSystem::computeCollisionInfo(const SDL_FRect &a,
                                           const SDL_FRect &b, float &normalX,
                                           float &normalY, float &penetration) {
  // Compute overlap on both axes
  float overlapX = std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x);
  float overlapY = std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y);

  // Use the smaller overlap as the separation axis
  if (overlapX < overlapY) {
    // Horizontal separation
    penetration = overlapX;
    normalX = (a.x + a.w / 2 < b.x + b.w / 2) ? -1.0f : 1.0f;
    normalY = 0.0f;
  } else {
    // Vertical separation
    penetration = overlapY;
    normalX = 0.0f;
    normalY = (a.y + a.h / 2 < b.y + b.h / 2) ? -1.0f : 1.0f;
  }
}
