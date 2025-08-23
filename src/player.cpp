#include "../include/player.h"
#include <iostream>

RectPlayer::RectPlayer(SDL_FRect rect_in, std::shared_ptr<Texture> texture)
    : texture(texture), sprite(nullptr), rect(rect_in),
      pos_x(static_cast<float>(rect_in.x)),
      pos_y(static_cast<float>(rect_in.y)), state(MovementState::IDLE),
      previousState(MovementState::IDLE), vel_x(0.0f), vel_y(0.0f),
      gravity(PLAYER_GRAVITY), onGround(false), isJumping(false),
      jumpDuration(PLAYER_JUMP_DURATION), jumpTimer(0.f), lastDirection(1),
      crouching(false), dashing(false), dashSpeed(PLAYER_DASH_SPEED),
      dashDuration(PLAYER_DASH_DURATION), dashTimer(0.f),
      dashCooldown(PLAYER_DASH_COOLDOWN), dashCooldownTimer(0.f),
      dashDirection(Direction::RIGHT), audioManager(nullptr) {

  if (!texture) {
    throw std::invalid_argument("Texture shared_ptr must not be null");
  }
  this->texture = std::move(texture);
}
void RectPlayer::init() {
  // Create player at starting position with texture
  sprite = std::make_unique<Sprite>(texture.get());
  sprite->setDestRect(
      {pos_x, pos_y, static_cast<float>(rect.w), static_cast<float>(rect.h)});

  // Initialize animation frames using config constants
  animations = {{MovementState::IDLE,
                 {{SPRITE_SHEET_TILE_WIDTH * 0, SPRITE_SHEET_TILE_HEIGHT * 1,
                   SPRITE_SHEET_TILE_WIDTH, SPRITE_SHEET_TILE_HEIGHT}}},
                {MovementState::MOVING,
                 {{SPRITE_SHEET_TILE_WIDTH * 1, SPRITE_SHEET_TILE_HEIGHT * 1,
                   SPRITE_SHEET_TILE_WIDTH, SPRITE_SHEET_TILE_HEIGHT},
                  {SPRITE_SHEET_TILE_WIDTH * 2, SPRITE_SHEET_TILE_HEIGHT * 1,
                   SPRITE_SHEET_TILE_WIDTH, SPRITE_SHEET_TILE_HEIGHT},
                  {SPRITE_SHEET_TILE_WIDTH * 3, SPRITE_SHEET_TILE_HEIGHT * 1,
                   SPRITE_SHEET_TILE_WIDTH, SPRITE_SHEET_TILE_HEIGHT},
                  {SPRITE_SHEET_TILE_WIDTH * 4, SPRITE_SHEET_TILE_HEIGHT * 1,
                   SPRITE_SHEET_TILE_WIDTH, SPRITE_SHEET_TILE_HEIGHT}}},
                {MovementState::JUMPING,
                 {{SPRITE_SHEET_TILE_WIDTH * 9, SPRITE_SHEET_TILE_HEIGHT * 1,
                   SPRITE_SHEET_TILE_WIDTH, SPRITE_SHEET_TILE_HEIGHT}}},
                {MovementState::CROUCHING,
                 {{SPRITE_SHEET_TILE_WIDTH * 7, SPRITE_SHEET_TILE_HEIGHT * 1,
                   SPRITE_SHEET_TILE_WIDTH, SPRITE_SHEET_TILE_HEIGHT}}}};

  initializeDashParams();
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
    sprite->setFrames(animations[state], ANIMATION_FRAME_TIME / 1000.0f, true);
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

void RectPlayer::handleMovement(float dt, bool moveLeft, bool moveRight,
                                bool jump, bool fastFall, bool dash,
                                bool crouch) {
  // Handle crouch
  if (crouch && onGround) {
    setCrouch(true);
    return; // Early return if crouching
  } else {
    setCrouch(false);
  }

  // Handle dash input - improved to work while button is held
  if (dash) {
    if (canDash()) {
      Direction dashDir =
          (lastDirection > 0) ? Direction::RIGHT : Direction::LEFT;
      startDash(dashDir);
    }
    // Continue dash as long as button is held and duration not exceeded
    if (dashing && dashTimer < dashDuration) {
      // Dash continues - no additional action needed, updateDash handles timing
    }
  } else {
    // Stop dash when button is released
    if (dashing) {
      stopDash();
    }
  }

  // Calculate movement velocities
  vel_x = 0.0f;

  // Horizontal movement (disabled during dash)
  if (!dashing) {
    if (moveLeft) {
      vel_x -= getEffectiveSpeed();
      lastDirection = -1;
    }
    if (moveRight) {
      vel_x += getEffectiveSpeed();
      lastDirection = 1;
    }
  }

  // Vertical movement
  vel_y = gravity * dt; // Apply gravity

  // Jump handling
  if (jump) {
    if (onGround) {
      // Play sound effect if u want, Too Annoying for me
      /*
      if (audioManager) {
        audioManager->playSound(PlayerSounds::JUMP);
      }
      */
      // Initial jump
      isJumping = true;
      setGrounded(false);
      setCrouch(false);
      vel_y = -getEffectiveJumpForce() * dt;
      jumpTimer = 0.0f;
    } else if (isJumping && jumpTimer < jumpDuration / 1000.0f) {
      // Variable height jump
      float jumpPower = (jumpTimer < (jumpDuration / 2000.0f))
                            ? getEffectiveJumpForce()
                            : PLAYER_JUMP_REDUCED_FORCE;

      if (getSlowed()) {
        jumpPower *= SLOW_JUMP_MULTIPLIER;
      }

      vel_y = -jumpPower * dt;
      jumpTimer += dt;
    }
  } else {
    resetJump(); // Stop jump when button released
  }

  // Fast fall
  if (fastFall && !onGround) {
    vel_y += PLAYER_FAST_FALL_SPEED * dt;
  }
}

void RectPlayer::update(float dt) {
  // Update dash system
  updateDash(dt);

  // Handle crouching (early return if crouching)
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

  // Apply movement
  if (dashing) {
    // Dash movement
    float dashVelX =
        (dashDirection == Direction::RIGHT) ? dashSpeed : -dashSpeed;
    pos_x += dashVelX * dt;
    if (!onGround) {
      pos_y += gravity * dt; // Still apply gravity during dash if airborne
    }
  } else {
    // Normal movement
    pos_x += vel_x * dt;
    if (onGround) {
      vel_y = (vel_y > 0) ? 0 : vel_y; // Don't fall through ground
    }
    pos_y += vel_y;
  }

  // Update rect position
  rect.x = pos_x;
  rect.y = pos_y;

  // Update sprite position
  if (sprite)
    sprite->setPosition(pos_x, pos_y);

  // Update state and animation
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

void RectPlayer::initializeDashParams() {
  dashSpeed = PLAYER_DASH_SPEED;
  dashDuration = PLAYER_DASH_DURATION / 1000.0f; // Convert to seconds
  dashCooldown = PLAYER_DASH_COOLDOWN / 1000.0f; // Convert to seconds
}

bool RectPlayer::canDash() const {
  return dashCooldownTimer <= 0.0f && !dashing;
}

bool RectPlayer::isDashing() const { return dashing; }

void RectPlayer::startDash(Direction direction) {
  if (!canDash())
    return;

  dashing = true;
  dashDirection = direction;
  dashTimer = 0.0f;
  isJumping = false; // Cancel jump when dashing

  // Play sound effect if u want, Too Annoying for me
  /*
  if (audioManager) {
    audioManager->playSound(PlayerSounds::DASH);
  }
  */
}

void RectPlayer::updateDash(float dt) {
  // Update dash cooldown
  if (dashCooldownTimer > 0.0f) {
    dashCooldownTimer -= dt;
  }

  // Update active dash
  if (dashing) {
    dashTimer += dt;
    // Automatically stop dash when duration is exceeded
    if (dashTimer >= dashDuration) {
      stopDash();
    }
  }
}

void RectPlayer::stopDash() {
  dashing = false;
  dashTimer = 0.0f;
  dashCooldownTimer = dashCooldown; // Start cooldown
}

void RectPlayer::resetDashCooldown() { dashCooldownTimer = 0.0f; }

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
    yOffsetPercent = CROUCH_Y_OFFSET_PERCENT;
    heightReductionPercent = CROUCH_HEIGHT_REDUCTION_PERCENT;
    xOffsetRightPercent = CROUCH_X_OFFSET_RIGHT_PERCENT;
    xOffsetLeftPercent = CROUCH_X_OFFSET_LEFT_PERCENT;
    widthReductionPercent = CROUCH_WIDTH_REDUCTION_PERCENT;
    break;
  case MovementState::MOVING:
    yOffsetPercent = MOVING_Y_OFFSET_PERCENT;
    heightReductionPercent = MOVING_HEIGHT_REDUCTION_PERCENT;
    xOffsetRightPercent = MOVING_X_OFFSET_RIGHT_PERCENT;
    xOffsetLeftPercent = MOVING_X_OFFSET_LEFT_PERCENT;
    widthReductionPercent = MOVING_WIDTH_REDUCTION_PERCENT;
    break;
  case MovementState::IDLE:
    yOffsetPercent = IDLE_Y_OFFSET_PERCENT;
    heightReductionPercent = IDLE_HEIGHT_REDUCTION_PERCENT;
    xOffsetRightPercent = IDLE_X_OFFSET_RIGHT_PERCENT;
    xOffsetLeftPercent = IDLE_X_OFFSET_LEFT_PERCENT;
    widthReductionPercent = IDLE_WIDTH_REDUCTION_PERCENT;
    break;
  case MovementState::JUMPING:
    yOffsetPercent = JUMPING_Y_OFFSET_PERCENT;
    heightReductionPercent = JUMPING_HEIGHT_REDUCTION_PERCENT;
    xOffsetRightPercent = JUMPING_X_OFFSET_RIGHT_PERCENT;
    xOffsetLeftPercent = JUMPING_X_OFFSET_LEFT_PERCENT;
    widthReductionPercent = JUMPING_WIDTH_REDUCTION_PERCENT;
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

float RectPlayer::getEffectiveSpeed() const {
  float baseSpeed = PLAYER_SPEED;
  if (isSlowed) {
    baseSpeed *= SLOW_SPEED_MULTIPLIER;
  }
  return baseSpeed;
}

float RectPlayer::getEffectiveJumpForce() const {
  float baseJump = PLAYER_JUMP_FORCE;
  if (isSlowed) {
    baseJump *= SLOW_JUMP_MULTIPLIER;
  }
  return baseJump;
}

void RectPlayer::setAudioManager(std::shared_ptr<AudioManager> audioMgr) {
  audioManager = audioMgr;
}
