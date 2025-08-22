#ifndef COLLIDEABLE_H
#define COLLIDEABLE_H

#include "config.h"
#include <SDL2/SDL.h>
#include <utility>

/**
 * Object types for collision handling
 */
enum class ObjectType {
  PLAYER,        // Dynamic player character
  STATIC_OBJECT, // Platforms, spikes, lava, walls
  PROJECTILE     // Arrows, bullets, coins, collectibles
};

/**
 * Abstract base class for any object that can participate in collisions
 *
 * Provides a common interface for:
 * - Getting collision bounds (bounding box)
 * - Position access and modification
 * - Collision response callbacks
 *
 * Classes that inherit from Collideable must implement all pure virtual
 * methods.
 */
class Collideable {
public:
  virtual ~Collideable() = default;

  // === Pure virtual methods that must be implemented ===

  /**
   * Get the collision bounding box for this object
   * @return SDL_FRect representing the object's collision bounds
   */
  virtual SDL_FRect getCollisionBounds() const = 0;

  /**
   * Get the object type for collision handling
   * @return ObjectType enum value
   */
  virtual ObjectType getType() const = 0;

  /**
   * Get the current position of the object
   * @return pair<x, y> as floats
   */
  virtual std::pair<float, float> getPos() const = 0;

  /**
   * Set the position of the object
   * @param x New X coordinate
   * @param y New Y coordinate
   */
  virtual void setPos(float x, float y) = 0;

  /**
   * Called when this object collides with another
   * @param other The other collideable object
   * @param normal Collision normal vector (pointing from other to this)
   * @param penetration How much the objects are overlapping
   */
  virtual void onCollision(Collideable *other, float normalX, float normalY,
                           float penetration) = 0;

  /**
   * Check if this object is static (doesn't move during physics updates)
   * @return true if static, false if dynamic
   */
  virtual bool isStatic() const = 0;
};

#endif // COLLIDEABLE_H
