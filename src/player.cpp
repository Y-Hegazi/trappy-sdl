#include "../include/player.h"
#include <iostream>

RectPlayer::RectPlayer(SDL_FRect rect_in, std::shared_ptr<Texture> texture)
    : texture(texture), sprite(nullptr), rect(rect_in),
      pos_x(static_cast<float>(rect_in.x)),
      pos_y(static_cast<float>(rect_in.y)), state(MovementState::IDLE),
      previousState(MovementState::IDLE), vel_x(0.0f), vel_y(0.0f),
      gravity(450.f), onGround(false), isJumping(false), jumpDuration(250.f),
      jumpTimer(0.f), lastDirection(1), crouching(false), dashing(false),
      dashSpeed(1000.f), dashDuration(0.2f), dashTimer(0.f), dashCooldown(0.6f),
      dashCooldownTimer(0.f), dashDir(1) {

  if (!texture) {
    throw std::invalid_argument("Texture shared_ptr must not be null");
  }

  animations = {{MovementState::IDLE, {{32 * 0, 48 * 1, 32, 48}}},
                {MovementState::MOVING,
                 {{32 * 1, 48 * 1, 32, 48},
                  {32 * 2, 48 * 1, 32, 48},
                  {32 * 3, 48 * 1, 32, 48},
                  {32 * 4, 48 * 1, 32, 48}}},
                {MovementState::JUMPING, {{32 * 9, 48 * 1, 32, 48}}},
                {MovementState::CROUCHING, {{32 * 7, 48 * 1, 32, 48}}}};

  sprite = std::make_unique<Sprite>(texture.get());
  sprite->setDestRect(
      {pos_x, pos_y, static_cast<float>(rect.w), static_cast<float>(rect.h)});
}

std::pair<float, float> RectPlayer::getSize() const { return {rect.w, rect.h}; }

void RectPlayer::setSize(float width, float height) {
  rect.w = width;
  rect.h = height;
}

const SDL_FRect &RectPlayer::getRect() const { return rect; }

void RectPlayer::setVel(float vel_x, float vel_y) {
  this->vel_x = vel_x;
  this->vel_y = vel_y;
}

std::pair<float, float> RectPlayer::getVel() const { return {vel_x, vel_y}; }

Sprite *RectPlayer::getSprite() const { return sprite.get(); }

void RectPlayer::setAnimation(const std::vector<SDL_Rect> &frames,
                              float frameTime) {
  if (sprite) {
    sprite->setFrames(frames, frameTime, true);
    sprite->play();
  }
}

void RectPlayer::animationHandle() {
  if (sprite) {
    sprite->setFrames(animations[state], 0.1f, true);
    sprite->play();
  }
}

void RectPlayer::renderAnimation(SDL_Renderer *renderer, float dt,
                                 bool boundingBox) const {
  if (!sprite) {
    throw std::runtime_error("Sprite is null, cannot render animation");
  }
  sprite->update(dt);

  SDL_RendererFlip flip =
      (lastDirection == -1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
  sprite->render(renderer, flip);

  if (boundingBox) {
    auto bb = getCollisionBounds();
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRectF(renderer, &bb);
  }
}

void RectPlayer::SetAnimationMap(
    std::unordered_map<MovementState, std::vector<SDL_Rect>> anims) {
  animations = std::move(anims);
}

std::unordered_map<MovementState, std::vector<SDL_Rect>>
RectPlayer::getAnimationMap() const {
  return animations;
}

void RectPlayer::update(float dt, float vel_x, float vel_y, bool dash) {
  this->vel_x = vel_x;
  this->vel_y = grounded() ? 0 : vel_y;

  if (crouching) {
    if (sprite)
      sprite->setPosition(pos_x, pos_y);
    stateHandle();
    if (state != previousState) {
      animationHandle();
      previousState = state;
    }
    return;
  }

  if ((dash && dashCooldownTimer <= 0.f) ||
      (dashing && dashTimer < dashDuration)) {
    startDash(lastDirection);
    setJumping(false);
    if (lastDirection != dashDir)
      setDashing(false);

    pos_x += dashSpeed * lastDirection * dt;
    pos_y += grounded() ? 0 : gravity * dt;
    dashTimer += dt;
    dashCooldownTimer = dashCooldown;

    if (dashTimer >= dashDuration) {
      setDashing(false);
    }
    rect.x = pos_x;
    rect.y = pos_y;
  } else {
    if (dashCooldownTimer > 0.f) {
      dashCooldownTimer -= dt;
      if (dashCooldownTimer <= 0.f) {
        resetDash();
      }
    }

    if (onGround)
      vel_y = vel_y > 0 ? 0 : vel_y;
    pos_y += vel_y * dt;
    pos_x += vel_x * dt;
    rect.x = pos_x;
    rect.y = pos_y;
  }

  if (sprite)
    sprite->setPosition(pos_x, pos_y);
  stateHandle();
  if (state != previousState) {
    animationHandle();
    previousState = state;
  }
}

void RectPlayer::render(SDL_Renderer *renderer) const {
  if (sprite && renderer) {
    sprite->render(renderer);
  }
}

float RectPlayer::getGravity() const { return gravity; }

void RectPlayer::setGravity(float g) { gravity = g; }

void RectPlayer::stopFalling() { vel_y = 0.0f; }

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

bool RectPlayer::grounded() const { return onGround; }

void RectPlayer::setJumping(bool jumping) { isJumping = jumping; }

bool RectPlayer::getJumping() const { return isJumping; }

void RectPlayer::setJumpDuration(float ms) { jumpDuration = ms; }

float RectPlayer::getJumpDuration() const { return jumpDuration; }

void RectPlayer::setJumpDurationTimer(float ms) { jumpTimer = ms; }

float RectPlayer::getJumpDurationTimer() const { return jumpTimer; }

void RectPlayer::setDashParams(float speed, float duration, float cooldown) {
  dashSpeed = speed;
  dashDuration = duration;
  dashCooldown = cooldown;
}

bool RectPlayer::canDash() const {
  return dashCooldownTimer <= 0.f && !dashing;
}

bool RectPlayer::isDashing() const { return dashing; }

void RectPlayer::setDashing(bool dashing) { this->dashing = dashing; }

void RectPlayer::startDash(int direction) {
  dashing = true;
  dashDir = direction;
}

void RectPlayer::resetDash() {
  dashing = false;
  dashTimer = 0.f;
  dashCooldownTimer = 0.f;
}

void RectPlayer::setDashCooldown(float ms) { dashCooldown = ms; }

float RectPlayer::getDashCooldown() const { return dashCooldown; }

void RectPlayer::setDashCooldownTimer(float ms) { dashCooldownTimer = ms; }

float RectPlayer::getDashCooldownTimer() const { return dashCooldownTimer; }

float RectPlayer::getDashDuration() const { return dashDuration; }

void RectPlayer::setDashDuration(float duration) { dashDuration = duration; }

void RectPlayer::setDashDurationTimer(float ms) { dashTimer = ms; }

float RectPlayer::getDashDurationTimer() const { return dashTimer; }

float RectPlayer::getDashSpeed() const { return dashSpeed; }

void RectPlayer::setDashSpeed(float speed) { dashSpeed = speed; }

bool RectPlayer::isCrouching() const { return crouching; }

void RectPlayer::setCrouch(bool enable) {
  crouching = enable;
  if (enable) {
    vel_x = 0.f;
    vel_y = 0.f;
  }
}

void RectPlayer::setState(MovementState state) { this->state = state; }

void RectPlayer::setLastDirection(int dir) { lastDirection = dir; }

int RectPlayer::getLastDirection() const { return lastDirection; }

SDL_FRect RectPlayer::getCollisionBounds() const {
  SDL_FRect smallerBounds = rect;
  float yOffsetPercent = 0.0f;
  float heightReductionPercent = 0.0f;
  float xOffsetRightPercent = 0.0f;
  float xOffsetLeftPercent = 0.0f;
  float widthReductionPercent = 0.0f;

  switch (state) {
  case MovementState::CROUCHING:
    yOffsetPercent = 0.42f;
    heightReductionPercent = 0.52f;
    xOffsetRightPercent = 0.25f;
    xOffsetLeftPercent = 0.0f;
    widthReductionPercent = 0.25f;
    break;
  case MovementState::MOVING:
    yOffsetPercent = 0.21f;
    heightReductionPercent = 0.29f;
    xOffsetRightPercent = 0.25f;
    xOffsetLeftPercent = 0.0f;
    widthReductionPercent = 0.34f;
    break;
  case MovementState::IDLE:
    yOffsetPercent = 0.21f;
    heightReductionPercent = 0.29f;
    xOffsetRightPercent = 0.31f;
    xOffsetLeftPercent = 0.16f;
    widthReductionPercent = 0.53f;
    break;
  case MovementState::JUMPING:
    yOffsetPercent = 0.0f;
    heightReductionPercent = 0.0f;
    xOffsetRightPercent = 0.375f;
    xOffsetLeftPercent = 0.375f;
    widthReductionPercent = 0.66f;
    break;
  }

  float yOffset = rect.h * yOffsetPercent;
  float heightReduction = rect.h * heightReductionPercent;
  float widthReduction = rect.w * widthReductionPercent;

  smallerBounds.y += yOffset;
  smallerBounds.h -= heightReduction;
  smallerBounds.w -= widthReduction;

  if (lastDirection == 1) {
    smallerBounds.x += rect.w * xOffsetRightPercent;
  } else {
    smallerBounds.x += rect.w * xOffsetLeftPercent;
  }

  return smallerBounds;
}

std::pair<float, float> RectPlayer::getPos() const { return {pos_x, pos_y}; }

void RectPlayer::setPos(float x, float y) {
  pos_x = x;
  pos_y = y;
  rect.x = x;
  rect.y = y;
  if (sprite)
    sprite->setPosition(x, y);
}

void RectPlayer::onCollision(Collideable *other, float normalX, float normalY,
                             float penetration) {
  if (std::abs(normalY) > 0.5f) {
    if (normalY < 0.f) {
      vel_y = 0.f;
      setGrounded(true);
      resetJump();
    } else {
      if (vel_y < 0.f)
        vel_y = 0.f;
      setGrounded(false);
    }
  } else if (std::abs(normalX) > 0.5f) {
    vel_x = 0.f;
    setGrounded(false);
  }
}

bool RectPlayer::isStatic() const { return false; }

ObjectType RectPlayer::getType() const { return ObjectType::PLAYER; }

void RectPlayer::stateHandle() {
  if (crouching) {
    state = MovementState::CROUCHING;
  } else if (isJumping) {
    state = MovementState::JUMPING;
  } else if (dashing) {
    state = MovementState::MOVING;
  } else if (vel_x != 0) {
    state = MovementState::MOVING;
  } else {
    state = MovementState::IDLE;
  }
}
