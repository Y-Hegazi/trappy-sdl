#include "../include/player.h"

/**
 * Constructor - create player and its owned Sprite.
 * The Sprite stores a non-owning pointer to the Texture managed by the
 * provided shared_ptr. Caller retains ownership of the texture shared_ptr.
 */
RectPlayer::RectPlayer(SDL_FRect rect_in, std::shared_ptr<Texture> texture)
    : texture(texture), sprite(nullptr), rect(rect_in),
      pos_x(static_cast<float>(rect_in.x)),
      pos_y(static_cast<float>(rect_in.y)) {
  if (!texture) {
    throw std::invalid_argument("Texture shared_ptr must not be null");
  }
  // Create owned sprite referencing the texture
  sprite = std::make_unique<Sprite>(texture.get());
  // Initialize sprite destination to match player rect
  sprite->setDestRect(
      {pos_x, pos_y, static_cast<float>(rect.w), static_cast<float>(rect.h)});
}

const SDL_FRect &RectPlayer::getRect() const { return rect; }

void RectPlayer::setAnimation(const std::vector<SDL_Rect> &frames,
                              float frameTime) {
  if (sprite) {
    sprite->setFrames(frames, frameTime, true);
    sprite->play();
  }
}
void RectPlayer::setState(MovementState state) { this->state = state; }
void RectPlayer::setLastDirection(int dir) { lastDirection = dir; }
int RectPlayer::getLastDirection() const { return lastDirection; }

void RectPlayer::render(SDL_Renderer *renderer) const {
  if (sprite && renderer) {
    sprite->render(renderer);
  }
}

float RectPlayer::getGravity() const { return gravity; }

void RectPlayer::stopFalling() { vel_y = 0.0f; }

void RectPlayer::setGravity(float g) { gravity = g; }

void RectPlayer::resetJump() {
  jumpTimer = 0.f;
  isJumping = false;
}

void RectPlayer::setGrounded(bool grounded) {
  onGround = grounded;
  if (grounded) {
    resetJump();
  }
}

const bool RectPlayer::grounded() const { return onGround; }

void RectPlayer::setJumping(bool jumping) {
  isJumping = jumping;
  if (jumping)
    state = MovementState::JUMPING;
}

const bool RectPlayer::getJumping() const { return isJumping; }

// --- Jump getters/setters
void RectPlayer::setJumpDuration(float ms) { jumpDuration = ms; }
float RectPlayer::getJumpDuration() const { return jumpDuration; }
void RectPlayer::setJumpDurationTimer(float ms) { jumpTimer = ms; }
float RectPlayer::getJumpDurationTimer() const { return jumpTimer; }

// --- Dash getters/setters
void RectPlayer::setDashDuration(float ms) { dashDuration = ms; }
float RectPlayer::getDashDuration() const { return dashDuration; }
void RectPlayer::setDashDurationTimer(float ms) { dashTimer = ms; }
float RectPlayer::getDashDurationTimer() const { return dashTimer; }
void RectPlayer::setDashCooldownTimer(float ms) { dashCooldownTimer = ms; }
float RectPlayer::getDashCooldownTimer() const { return dashCooldownTimer; }
void RectPlayer::setDashCooldown(float ms) { dashCooldown = ms; }
float RectPlayer::getDashCooldown() const { return dashCooldown; }

float RectPlayer::getDashSpeed() const { return dashSpeed; }
void RectPlayer::setDashSpeed(float speed) { dashSpeed = speed; }

/**
 * Set player dimensions - updates both internal cache and SDL_Rect
 * This ensures consistency between our cached values and the rendering rect
 */
void RectPlayer::setSize(float width, float height) {

  // Sync the SDL_Rect used for rendering
  rect.w = width;
  rect.h = height;
}
/**
 * Set player position - updates both precise float and integer render
 * coordinates
 *
 * Strategy: Keep float precision for physics, round to nearest integer for
 * rendering This avoids directional bias (where negative movements round
 * differently than positive)
 */
void RectPlayer::setPos(int x, int y) {
  // Update precise float position (used for physics calculations)
  pos_x = (float)x;
  pos_y = (float)y;

  // Sync integer render position using proper rounding (not truncation)
  // std::lround rounds to nearest integer, avoiding negative/positive bias
  rect.x = static_cast<int>(std::lround(pos_x));
  rect.y = static_cast<int>(std::lround(pos_y));
}
// === Getter implementations - Simple accessor functions ===

std::pair<float, float> RectPlayer::getSize() const { return {rect.w, rect.h}; }

std::pair<float, float> RectPlayer::getPos() const { return {pos_x, pos_y}; }

std::pair<int, int> RectPlayer::getRealPos() const { return {rect.x, rect.y}; }

/**
 * Core update function - Apply velocity to position and sync render coordinates
 */
#include <iostream>
void RectPlayer::update(float dt, float vel_x, float vel_y, bool dash) {
  if (crouching) {
    // No change in positions while crouching
    // Set the animation for crouching state
    float dir = (vel_x >= 0) ? 1.0f : -1.0f;
    lastDirection = dir;
    if (sprite)
      sprite->setPosition(pos_x, pos_y);
    return;
  }
  if ((dash && dashCooldownTimer <= 0.f) ||
      (dashing && dashTimer < dashDuration)) {
    // Apply dash movement
    setDashing(true);
    setJumping(false);
    int dir = (vel_x >= 0) ? 1 : -1;
    lastDirection = dir;
    pos_x += dashSpeed * lastDirection * dt; // Dash in the current direction
    pos_y += 0;
    dashTimer += dt;
    dashCooldownTimer = dashCooldown;
    if (dashTimer >= dashDuration) {
      dashing = false;
    }
    rect.x = pos_x;
    rect.y = pos_y;
  } else {
    // Normal movement.
    if (dashCooldownTimer > 0.f) {
      dashCooldownTimer -= dt; // Decrease cooldown timer
      if (dashCooldownTimer <= 0.f) {
        resetDash();
      }
    }
    int dir = (vel_x > 0) ? 1 : lastDirection;
    lastDirection = dir;
    if (onGround)
      vel_y = vel_y > 0 ? 0 : vel_y;
    pos_y += vel_y * dt;
    pos_x += vel_x * dt;
    // Sync rect x,y
    rect.x = pos_x;
    rect.y = pos_y;
  }

  if (sprite)
    sprite->setPosition(pos_x, pos_y);
}

// Configure dash defaults
void RectPlayer::setDashParams(float speed, float duration, float cooldown) {
  dashSpeed = speed;
  dashDuration = duration;
  dashCooldown = cooldown;
}

// Query
bool RectPlayer::isCrouching() const { return crouching; }

// Dash helpers
bool RectPlayer::canDash() const {
  return dashCooldownTimer <= 0.f && !dashing;
}
bool RectPlayer::isDashing() const { return dashing; }
void RectPlayer::setDashing(bool dashing) {
  this->dashing = dashing;
  if (dashing)
    state = MovementState::MOVING;
}

void RectPlayer::setCrouch(bool enable) {
  crouching = enable;

  if (enable)
    state = MovementState::CROUCHING;
}

void RectPlayer::startDash(int direction) {
  dashing = true;
  dashDir = direction;
}
void RectPlayer::resetDash() {
  dashing = false;
  dashTimer = 0.f;
  dashCooldownTimer = 0.f;
}
