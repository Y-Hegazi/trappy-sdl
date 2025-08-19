#ifndef PLAYER_H
#define PLAYER_H

#include "collideable.h"
#include "sprite.h"
#include "texture.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <memory>
#include <tuple>
#include <vector>

// Direction enum for movement - used for directional movement logic
enum class Direction { LEFT, UP, RIGHT, DOWN };
enum class MovementState { IDLE, MOVING, JUMPING, CROUCHING };

/**
 * RectPlayer - A rectangular player entity with physics-based movement
 *
 * Design philosophy:
 * - Uses float precision for smooth movement calculations (pos_x, pos_y)
 * - Syncs to integer SDL_Rect for rendering (avoids rounding bias)
 * - Separates velocity from position for physics-based movement
 * - Supports running multiplier and jump mechanics
 * - Implements Collideable interface for collision system
 */
class RectPlayer : public Collideable {
private:
  MovementState state = MovementState::IDLE;
  std::shared_ptr<Texture> texture;
  std::unique_ptr<Sprite> sprite;

  // Rendering representation - integer coordinates for SDL (single source)
  SDL_FRect rect;

  // Physics state - precise floating-point position
  float pos_x, pos_y;

  // Movement parameters
  float vel_x = 0.0f;
  float vel_y = 0.0f;
  // Tuned gravity: treated as a baseline downward speed (px/s).
  // Use a value that feels responsive but not too fast. Adjust later if
  // you change how gravity is applied (acceleration vs fixed velocity).
  float gravity = 450.f;

  // Jump system state
  bool onGround = false;
  bool isJumping = false;

  // Jump timing: configuration vs runtime timer
  // Total maximum time (ms) that a held jump can extend ascent
  float jumpDuration = 250.f; // configuration (ms)
  float jumpTimer = 0.f;      // runtime countdown (ms)

  // (jump timers above are in milliseconds)

  int lastDirection = 1;

  // crouch/dash state
  bool crouching = false;
  bool dashing = false;
  // Tuned dash: faster and snappier
  float dashSpeed = 1000.f;      // px/s dash velocity
  float dashDuration = 0.2f;     // seconds
  float dashTimer = 0.f;         // remaining dash time (seconds)
  float dashCooldown = 0.6f;     // seconds
  float dashCooldownTimer = 0.f; // seconds
  int dashDir = 1;

public:
  /**
   * Constructor - Initialize player with SDL_Rect position and size
   * @param rect Initial position and dimensions for the player
   * Note: Copies rect data to internal members for independent state
   * management
   */
  // Construct a player at `rect` using a shared texture. The sprite is
  // created to reference the provided texture (non-owning inside Sprite).
  RectPlayer(SDL_FRect rect, std::shared_ptr<Texture> texture);

  // Legacy movement function (currently commented out)
  // void move(float dt, Direction dir, bool run = false);

  // === Getters - Read-only access to internal state ===

  /**
   * Get player dimensions
   * @return pair<width, height> in pixels
   */
  std::pair<float, float> getSize() const;

  // === Setters - Modify player state ===

  /**
   * Set player dimensions and update both internal cache and SDL_Rect
   */
  void setSize(float width, float height);

  /**
   * Get read-only pointer to SDL_Rect for rendering
   * @return const pointer to internal rect (safe for SDL rendering functions)
   */
  const SDL_FRect &getRect() const;

  /**
   * Set velocity components directly
   * @param vel_x Horizontal velocity in pixels/second (+ = right, - = left)
   * @param vel_y Vertical velocity in pixels/second (+ = down, - = up)
   */
  void setVel(float vel_x, float vel_y) {
    this->vel_x = vel_x;
    this->vel_y = vel_y;
  }

  /**
   * Get current velocity
   * @return pair<vel_x, vel_y> in pixels/second
   */
  std::pair<float, float> getVel() const { return {vel_x, vel_y}; }

  /**
   * Set running speed multiplier
   * @param runMult Multiplier applied to movement when running (e.g., 1.5 = 50%
   * faster)
   */

  /**
   * Set animation frames and play
   * @param frames Vector of SDL_Rect defining the animation frames
   * @param frameTime Time per frame in seconds
   *
   * This function sets the specified frames for the sprite's animation and
   * starts playback.
   */
  void setAnimation(const std::vector<SDL_Rect> &frames, float frameTime);

  // === Core update functions ===

  /**
   * Update player position based on current velocity
   * @param dt Delta time in seconds since last update
   * @param run Whether player is running (applies runMult to horizontal
   * movement)
   *
   * This function:
   * 1. Updates precise float position using velocity * deltaTime
   * 2. Applies running multiplier to horizontal movement if enabled
   * 3. Syncs integer SDL_Rect coordinates using proper rounding
   * 4. Updates onGround state based on vertical velocity
   */
  void update(float dt, float vel_x, float vel_y, bool dash);
  void setDashing(bool dashing);

  /**
   * Render the player entity using the associated sprite
   * @param renderer SDL_Renderer instance for rendering the sprite
   *
   * This function calls the sprite's render method to draw the player on the
   * screen.
   */
  void render(SDL_Renderer *renderer) const;

  // === Physics and state management ===

  /**
   * Get gravity constant
   * @return Current gravity value (interpretation depends on physics
   * implementation)
   */
  float getGravity() const;

  /**
   * Stop vertical movement (typically called when landing on ground)
   * Sets vel_y to 0, effectively stopping falling/jumping motion
   */
  void stopFalling();

  /**
   * Set gravity constant
   * @param g New gravity value (can be acceleration or fixed fall speed)
   */
  void setGravity(float g);

  /**
   * Reset jump system state - typically called when landing or respawning
   * Restores jump and double-jump abilities, resets cooldown timer
   */
  void resetJump();
  void setGrounded(bool grounded);
  const bool grounded() const;
  void setJumping(bool jumping);
  const bool getJumping() const;

  // --- Jump configuration and runtime timers ---
  void setJumpDuration(float ms);
  float getJumpDuration() const;
  void setJumpDurationTimer(float ms);
  float getJumpDurationTimer() const;

  // --- Dash configuration and runtime timers ---
  void setDashCooldown(float ms);
  float getDashCooldown() const;
  void setDashCooldownTimer(float ms);
  float getDashCooldownTimer() const;
  float getDashDuration() const;
  void setDashDuration(float duration);
  void setDashDurationTimer(float ms);
  float getDashDurationTimer() const;

  // Dash speed access
  float getDashSpeed() const;
  void setDashSpeed(float speed);
  // crouch methods

  void setDashParams(float speed, float duration, float cooldown);
  bool isCrouching() const;
  void setCrouch(bool enable);

  bool canDash() const;
  bool isDashing() const;
  void startDash(int direction);
  void setLastDirection(int dir);
  int getLastDirection() const;
  void resetDash();
  void setState(MovementState state);
  Sprite *getSprite() const { return sprite.get(); };

  // === Collideable interface implementation ===

  /**
   * Get collision bounds for this player
   * @return SDL_FRect representing player's collision box
   */
  SDL_FRect getCollisionBounds() const override;

  /**
   * Get current position
   * @return pair<x, y> as floats
   */
  std::pair<float, float> getPos() const override;

  /**
   * Set position
   * @param x New X coordinate
   * @param y New Y coordinate
   */
  void setPos(float x, float y) override;

  /**
   * Handle collision with another object
   * @param other The other collideable object
   * @param normalX X component of collision normal
   * @param normalY Y component of collision normal
   * @param penetration Overlap amount
   */
  void onCollision(Collideable *other, float normalX, float normalY,
                   float penetration) override;

  /**
   * Check if player is static (always false for player)
   * @return false - player is always dynamic
   */
  bool isStatic() const override { return false; }

  /**
   * Get object type for collision handling
   * @return ObjectType::PLAYER
   */
  ObjectType getType() const override { return ObjectType::PLAYER; }
};

#endif
