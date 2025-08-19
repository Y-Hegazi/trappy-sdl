#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include "collideable.h"
#include <vector>
/**
 * Simple collision system for a trap-land platformer game.
 *
 * Handles collision detection and response between:
 * - Player vs Static Objects (platforms, traps)
 * - Player vs Projectiles (collectibles, arrows)
 * - Projectiles vs Static Objects (bouncing, sticking)
 */
class CollisionSystem {
public:
  /**
   * Check and resolve collisions for all objects
   * @param player The player object
   * @param objects Vector of all other collideable objects
   */
  static void resolveCollisions(Collideable *player,
                                std::vector<Collideable *> &objects);

  /**
   * Simple AABB collision detection
   * @param a First bounding box
   * @param b Second bounding box
   * @return true if boxes overlap
   */
  static bool checkAABB(const SDL_FRect &a, const SDL_FRect &b);

  /**
   * Handle collision between two specific objects
   * @param a First object
   * @param b Second object
   */
  static void handleCollision(Collideable *a, Collideable *b);

private:
  // Specific collision handlers
  static void handlePlayerVsStatic(Collideable *player, Collideable *staticObj);
  static void handlePlayerVsProjectile(Collideable *player,
                                       Collideable *projectile);
  static void handleProjectileVsStatic(Collideable *projectile,
                                       Collideable *staticObj);

  // Helper to compute collision normal and penetration
  static void computeCollisionInfo(const SDL_FRect &a, const SDL_FRect &b,
                                   float &normalX, float &normalY,
                                   float &penetration);
};

#endif // COLLISION_SYSTEM_H
