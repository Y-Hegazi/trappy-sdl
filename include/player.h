#ifndef PLAYER_H
#define PLAYER_H

#include "audio_manager.h"
#include "collideable.h"
#include "config.h"
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

  // Audio
  void setAudioManager(std::shared_ptr<AudioManager> audioMgr);

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
  void update(float dt);
  void handleMovement(float dt, bool moveLeft, bool moveRight, bool jump,
                      bool fastFall, bool dash, bool crouch);
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
  void initializeDashParams();
  bool canDash() const;
  bool isDashing() const;
  void startDash(Direction direction);
  void updateDash(float dt);
  void stopDash();
  void resetDashCooldown();

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
  void init();

private:
  std::unordered_map<MovementState, std::vector<SDL_Rect>> animations;
  MovementState state;
  MovementState previousState;
  std::shared_ptr<Texture> texture;
  std::unique_ptr<Sprite> sprite;

  std::shared_ptr<AudioManager> audioManager;

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
  Direction dashDirection;

  // Status effects
  bool isSlowed = false;
  bool isDead = false;

  void stateHandle();

public:
  // Status effect methods
  void setSlowed(bool slowed) { isSlowed = slowed; }
  bool getSlowed() const { return isSlowed; }
  void setDead(bool dead) { isDead = dead; }
  bool getDead() const { return isDead; }
  float getEffectiveSpeed() const;
  float getEffectiveJumpForce() const;
};

#endif
