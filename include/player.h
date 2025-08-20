#ifndef PLAYER_H
#define PLAYER_H

#include "collideable.h"
#include "sprite.h"
#include "texture.h"
#include <SDL2/SDL.h>
#include <memory>
#include <unordered_map>
#include <vector>

enum class Direction { LEFT, UP, RIGHT, DOWN };
enum class MovementState { IDLE, MOVING, JUMPING, CROUCHING };

class RectPlayer : public Collideable {
public:
  RectPlayer(SDL_FRect rect, std::shared_ptr<Texture> texture);

  // Basic getters/setters
  std::pair<float, float> getSize() const;
  void setSize(float width, float height);
  const SDL_FRect &getRect() const;
  void setVel(float vel_x, float vel_y);
  std::pair<float, float> getVel() const;
  Sprite *getSprite() const;

  // Animation
  void setAnimation(const std::vector<SDL_Rect> &frames, float frameTime);
  void animationHandle();
  void renderAnimation(SDL_Renderer *renderer, float dt,
                       bool boundingBox = false) const;
  void SetAnimationMap(
      std::unordered_map<MovementState, std::vector<SDL_Rect>> anims);
  std::unordered_map<MovementState, std::vector<SDL_Rect>>
  getAnimationMap() const;

  // Core functionality
  void update(float dt, float vel_x, float vel_y, bool dash);
  void render(SDL_Renderer *renderer) const;

  // Physics
  float getGravity() const;
  void setGravity(float g);
  void stopFalling();

  // Jump system
  void resetJump();
  void setGrounded(bool grounded);
  bool grounded() const;
  void setJumping(bool jumping);
  bool getJumping() const;
  void setJumpDuration(float ms);
  float getJumpDuration() const;
  void setJumpDurationTimer(float ms);
  float getJumpDurationTimer() const;

  // Dash system
  void setDashParams(float speed, float duration, float cooldown);
  bool canDash() const;
  bool isDashing() const;
  void setDashing(bool dashing);
  void startDash(int direction);
  void resetDash();
  void setDashCooldown(float ms);
  float getDashCooldown() const;
  void setDashCooldownTimer(float ms);
  float getDashCooldownTimer() const;
  float getDashDuration() const;
  void setDashDuration(float duration);
  void setDashDurationTimer(float ms);
  float getDashDurationTimer() const;
  float getDashSpeed() const;
  void setDashSpeed(float speed);

  // Crouch system
  bool isCrouching() const;
  void setCrouch(bool enable);

  // State management
  void setState(MovementState state);
  void setLastDirection(int dir);
  int getLastDirection() const;

  // Collideable interface
  SDL_FRect getCollisionBounds() const override;
  std::pair<float, float> getPos() const override;
  void setPos(float x, float y) override;
  void onCollision(Collideable *other, float normalX, float normalY,
                   float penetration) override;
  bool isStatic() const override;
  ObjectType getType() const override;

private:
  std::unordered_map<MovementState, std::vector<SDL_Rect>> animations;
  MovementState state;
  MovementState previousState;
  std::shared_ptr<Texture> texture;
  std::unique_ptr<Sprite> sprite;

  SDL_FRect rect;
  float pos_x, pos_y;
  float vel_x, vel_y;
  float gravity;

  bool onGround;
  bool isJumping;
  float jumpDuration;
  float jumpTimer;

  int lastDirection;

  bool crouching;
  bool dashing;
  float dashSpeed;
  float dashDuration;
  float dashTimer;
  float dashCooldown;
  float dashCooldownTimer;
  int dashDir;

  void stateHandle();
};

#endif
