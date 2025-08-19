#include "../include/game.h"
#include "../include/collision_system.h"
#include "../include/platform.h"
#include <SDL2/SDL_image.h>
#include <iostream>
#include <memory>
/**
 * Game Constructor - Initialize SDL and create window/renderer
 *
 * @param name Window title
 * @throws std::runtime_error if any SDL initialization step fails
 *
 * Initialization sequence:
 * 1. Initialize SDL video subsystem
 * 2. Create centered window with specified dimensions
 * 3. Create hardware-accelerated renderer with VSync
 * 4. Initialize performance counter for precise timing
 */
Game::Game(const char *name, const char *playerTexture, int windowWidth,
           int windowHeight, int windowFlags, int rendererFlags)
    : sdlSubsystem(), window(nullptr,
                             [](SDL_Window *window) {
                               if (window)
                                 SDL_DestroyWindow(window);
                             }),
      renderer(nullptr,
               [](SDL_Renderer *renderer) {
                 if (renderer)
                   SDL_DestroyRenderer(renderer);
               }),
      playerTexturePath(playerTexture) {

  // Create main game window (windowWidth x windowHeight, centered on screen)
  window.reset(SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, windowWidth,
                                windowHeight, windowFlags));
  if (nullptr == window) {
    throw std::runtime_error("Window could not be created! Error: " +
                             std::string(SDL_GetError()));
  }

  // Create hardware-accelerated renderer with VSync enabled
  // ACCELERATED: Use GPU acceleration when available
  // PRESENTVSYNC: Sync to monitor refresh rate for smooth animation
  renderer.reset(SDL_CreateRenderer(window.get(), -1, rendererFlags));
  if (nullptr == renderer) {
    throw std::runtime_error("Renderer could not be created! Error: " +
                             std::string(SDL_GetError()));
  }

  // Initialize high-resolution timer for precise delta time calculation
  perfFreq = SDL_GetPerformanceFrequency();
  auto tex2 = std::make_shared<Texture>(renderer.get(), "Grass_01.png");
  platform = new Platform({00, 250, 900, 50}, tex2);
  platform2 = new Platform({150, 150, 150, 200}, tex2);
}
/**
 * Handle SDL events - Process user input and system events
 *
 * @param e SDL_Event reference to process
 *
 * Currently handles:
 * - SDL_QUIT: User closes window (sets isRunning = false)
 *
 * Future expansion could include:
 * - Key press/release events for more responsive input
 * - Window resize events
 * - Mouse input
 */
void Game::handleEvents(SDL_Event &e) {
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT)
      isRunning = false;
  }
}

/**
 * Update player position based on keyboard input and apply physics
 *
 * @param dt Delta time in seconds since last frame
 *
 * Input System:
 * - Uses SDL_GetKeyboardState for smooth, continuous input
 * - Horizontal: A/D keys control left/right movement (180 px/s)
 * - Vertical: W/Space for jumping with variable height control
 * - S key for fast-fall when airborne
 * - Shift for dash, Ctrl for crouch
 *
 * Physics:
 * - Gravity applied as constant downward velocity
 * - Jump impulse with duration-based height control
 * - Side-aware collision detection for proper platformer feel
 *
 * Collision System:
 * - Projects player movement to detect intersection type
 * - Bottom collisions: land and stop falling
 * - Side collisions: stop horizontal movement, preserve vertical
 */
void Game::updatePlayerPos(float dt) {
  // Early return if player not initialized
  if (!player)
    return;

  const Uint8 *keyboardState = SDL_GetKeyboardState(NULL);
  // Crouch handling (left control)
  bool crouch = false;
  if (keyboardState[SDL_SCANCODE_LCTRL] && player->grounded()) {
    crouch = true;
    player->setCrouch(true);
  } else {
    player->setCrouch(false);
  }
  bool dash = false;
  if (keyboardState[SDL_SCANCODE_LSHIFT]) {
    dash = true;
  }

  // Initialize velocities for this frame
  float vel_x = 0;                    // Horizontal velocity (reset each frame)
  float vel_y = player->getGravity(); // Start with gravity as downward velocity

  // Horizontal movement input (A/D or Arrow Keys)
  if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT]) {
    vel_x -= 180.f; // Move left at 180 pixels/second (slightly faster)
  }
  if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT]) {
    vel_x += 180.f; // Move right at 180 pixels/second (slightly faster)
  }

  // Vertical movement: Jump system with variable height
  if (keyboardState[SDL_SCANCODE_UP] || keyboardState[SDL_SCANCODE_W] ||
      keyboardState[SDL_SCANCODE_SPACE]) {
    if (player->grounded()) {
      // Initial jump: strong upward impulse
      player->setJumping(true);
      player->setDashing(false);
      player->setCrouch(false);
      player->setGrounded(false);
      vel_y -= 1200.f; // Strong initial jump velocity (px/s)
    } else if (player->getJumping() &&
               player->getJumpDurationTimer() < player->getJumpDuration()) {
      // Variable height: extend jump while button held
      if (player->getJumpDurationTimer() < player->getJumpDuration() / 2.0) {
        vel_y -= 1200.f; // Full power during first half
      } else
        vel_y -= 800.f; // Reduced power during second half
      player->setJumping(true);
      player->setJumpDurationTimer(player->getJumpDurationTimer() +
                                   dt * 1000.f);
    }
  } else {
    player->resetJump(); // Stop jump when button released
  }
  // Fast-fall: increase downward velocity when falling
  if (keyboardState[SDL_SCANCODE_S] ||
      keyboardState[SDL_SCANCODE_DOWN] && !(player->grounded())) {
    vel_y += 350.f; // Extra downward speed for quick descent (px/s)
  }

  // Set movement state based on input activity
  if (float(vel_x + vel_y) == 0.f)
    player->setState(MovementState::IDLE);

  // Side-aware collision detection system
  // Projects movement to distinguish landing vs side-hits
  SDL_Rect temp;
  temp.x = static_cast<int>(lround(player->getRect().x));
  temp.y = static_cast<int>(lround(player->getRect().y));
  temp.w = static_cast<int>(lround(player->getRect().w));
  temp.h = static_cast<int>(lround(player->getRect().h));

  // // Simple collision check (will be enhanced with side-aware logic)
  // if (SDL_HasIntersection(&temp, &floor) && vel_y > 0) {
  //   player->stopFalling();     // Set vertical velocity to 0
  //   player->setGrounded(true); // Mark player as on ground
  //   player->resetJump();
  // } else
  //   player->setGrounded(false); // Reset ground state if not on floor
  std::vector<Collideable *> colliders = {platform, platform2};
  CollisionSystem::resolveCollisions(player.get(), colliders);

  // Apply movement and handle collisions
  player->update(dt, vel_x, vel_y, dash);
}
/**
 * Main game loop - Core execution and rendering
 *
 * Architecture:
 * 1. High-precision timing using SDL performance counters
 * 2. Input processing and physics updates
 * 3. Render cycle: clear → draw objects → present
 *
 * Performance:
 * - VSync limits framerate to monitor refresh
 * - Delta time ensures frame-rate independent movement
 * - Hardware acceleration for smooth rendering
 */
void Game::run() {

  // Initialize game state
  isRunning = true;
  SDL_Event e;

  // Initialize high-resolution timing
  Uint64 t0 = SDL_GetPerformanceCounter();

  // Create player at starting position with texture
  auto playerTexture =
      std::make_shared<Texture>(renderer.get(), playerTexturePath);
  playerInit({100, 100, 32, 48}, playerTexture);

  // Configure sprite rendering (temporary setup)
  auto sprite = player->getSprite();
  sprite->setDestRect(player->getRect());
  sprite->setSrcRect({32, 48, 32, 48});
  player->render(renderer.get());

  // Main game loop - continues until user quits
  while (isRunning) {
    // === TIMING: Calculate frame delta time ===
    Uint64 t1 = SDL_GetPerformanceCounter();
    double dt = (double)(t1 - t0) / (double)perfFreq; // Convert to seconds
    t0 = t1;                                          // Update for next frame

    // === INPUT & PHYSICS: Process input and update game state ===
    handleEvents(e);     // Handle quit events
    updatePlayerPos(dt); // Handle movement input and physics

    // === RENDERING: Draw frame to screen ===
    // Clear screen to white background
    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    SDL_RenderClear(renderer.get());

    // Draw player sprite
    if (player) {
      player->render(renderer.get());
    }

    // Draw floor platforms
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderFillRect(renderer.get(), &floor);
    player->render(renderer.get()); // Second render for layering
    platform->getSprite()->setDestRect(platform->getCollisionBounds());
    platform->getSprite()->setSrcRect({0, 0, 500, 500});
    platform->getSprite()->render(renderer.get());
    platform->getSprite()->setDestRect(platform->getCollisionBounds());
    platform->getSprite()->setSrcRect({0, 0, 500, 500});
    platform->getSprite()->render(renderer.get());
    platform2->getSprite()->setDestRect(platform2->getCollisionBounds());
    platform2->getSprite()->setSrcRect({0, 0, 500, 500});
    platform2->getSprite()->render(renderer.get());
    // Present completed frame
    SDL_RenderPresent(renderer.get());
  }
}

/**
 * Initialize player object with position and texture
 * @param rect Initial position and size as SDL_Rect
 * @param texture Shared texture resource for rendering
 */
void Game::playerInit(SDL_Rect rect, std::shared_ptr<Texture> texture) {
  if (!texture)
    throw std::invalid_argument("texture must not be null");

  // Convert integer SDL_Rect to floating-point SDL_FRect for player system
  SDL_FRect frect = {(float)rect.x, (float)rect.y, (float)rect.w,
                     (float)rect.h};

  // Create player object (exception-safe construction)
  auto newPlayer = std::make_unique<RectPlayer>(frect, texture);

  // Commit to game state
  player = std::move(newPlayer);
}
/**
 * Game Destructor - Clean up all allocated resources
 *
 * Cleanup order is important:
 * 1. unique_ptr automatically cleans up player
 * 2. Destroy SDL renderer (handled by unique_ptr)
 * 3. Destroy SDL window (handled by unique_ptr)
 * 4. Quit SDL subsystems (handled by SubSystemWrapper)
 *
 * This ensures proper resource deallocation and prevents memory leaks
 */
Game::~Game() {
  // No manual cleanup needed - unique_ptr handles player cleanup automatically
}
