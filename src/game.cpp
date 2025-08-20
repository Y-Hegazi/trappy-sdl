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
      playerTexturePath(playerTexture), targetWidth(windowWidth),
      targetHeight(windowHeight) {

  // Create main game window (windowWidth x windowHeight, centered on screen)
  window.reset(SDL_CreateWindow(
      name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth,
      windowHeight, windowFlags | SDL_WINDOW_RESIZABLE));
  if (nullptr == window) {
    throw std::runtime_error("Window could not be created! Error: " +
                             std::string(SDL_GetError()));
  }

  // Prefer nearest-neighbor scaling for crisp pixels (important for pixel art)
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // "0" = nearest

  // Create hardware-accelerated renderer with VSync enabled
  renderer.reset(SDL_CreateRenderer(window.get(), -1, rendererFlags));
  if (nullptr == renderer) {
    throw std::runtime_error("Renderer could not be created! Error: " +
                             std::string(SDL_GetError()));
  }

  // Enable integer scaling (if supported) to avoid subpixel scaling artifacts
  // and keep pixel-art crisp when the window is resized.
  SDL_RenderSetIntegerScale(renderer.get(), SDL_TRUE);

  // Set logical size for scaling (this defines the virtual resolution)
  SDL_RenderSetLogicalSize(renderer.get(), targetWidth, targetHeight);
}

void Game::init() {
  // Initialize high-resolution timer for precise delta time calculation
  perfFreq = SDL_GetPerformanceFrequency();

  // Create player at starting position with texture
  auto playerTexture =
      std::make_shared<Texture>(renderer.get(), playerTexturePath);

  player =
      std::make_unique<RectPlayer>(SDL_FRect{80, 100, 48, 72}, playerTexture);

  player->init();
  /*Sudo
   *
   *
   *
   */
  map = std::make_unique<Map>(90, 15, 32);

  // Create standard platforms and load textures
  auto tex = std::make_shared<Texture>(renderer.get(), "wooden.png");
  auto tex2 = std::make_shared<Texture>(renderer.get(), "Grass_01.png");
  for (int x = 0; x < 90; x++) {
    SDL_FRect platformRect = {
        static_cast<float>(x * 32),  // x position (32 pixels per tile)
        static_cast<float>(14 * 32), // y position
        32.0f,                       // width
        32.0f                        // height
    };
    map->setTile(x, 14, std::make_shared<Platform>(platformRect, tex));
  }
  for (int x = 5; x < 90; x++) {
    SDL_FRect platformRect = {
        static_cast<float>(x * 32),  // x position (32 pixels per tile)
        static_cast<float>(10 * 32), // y position
        32.0f,                       // width
        32.0f                        // height
    };
    map->setTile(x, 10, std::make_shared<Platform>(platformRect, tex2));
  }
  /*
  *
  *
  *
  Sudo
  */
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

  // Ground check - check if player is still touching ground by looking slightly
  // below
  if (player->grounded()) {
    SDL_FRect playerBounds = player->getCollisionBounds();
    SDL_FRect groundCheckBounds = {
        playerBounds.x,
        playerBounds.y + playerBounds.h, // Start just below player
        playerBounds.w,
        2.0f // Small height to check for ground
    };

    bool stillOnGround = false;

    // Check against map tiles instead of hardcoded platforms
    auto nearbyTiles = map->getTilesInRect(groundCheckBounds);
    for (auto &tile : nearbyTiles) {
      if (CollisionSystem::checkAABB(groundCheckBounds,
                                     tile->getCollisionBounds())) {
        stillOnGround = true;
        break;
      }
    }

    if (!stillOnGround) {
      player->setGrounded(false);
    }
  }

  // Collision detection
  std::vector<Collideable *> colliders;

  // Get tiles near the player for efficient collision detection
  SDL_FRect playerBounds = player->getCollisionBounds();
  auto nearbyTiles = map->getTilesInRect(playerBounds);
  for (auto &tile : nearbyTiles) {
    colliders.push_back(tile.get());
  }

  CollisionSystem::resolveCollisions(player.get(), colliders);

  player->setLastDirection(
      vel_x > 0 ? 1 : (vel_x < 0 ? -1 : player->getLastDirection()));
  // Apply movement
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

  // Initialize game objects (platforms, etc.)
  init();

  // Set initial animation frame for IDLE state
  player->animationHandle();

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

    // Draw map tiles (this replaces individual platform rendering)
    map->render(renderer.get());

    // Draw player sprite
    if (player) {
      player->renderAnimation(renderer.get(), dt, true);
    }
    // Present completed frame
    SDL_RenderPresent(renderer.get());
  }
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
