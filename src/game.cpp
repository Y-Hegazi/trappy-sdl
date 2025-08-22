#include "../include/game.h"
#include "../include/collision_system.h"
#include "../include/config.h"
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
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
              RENDER_SCALE_QUALITY); // "0" = nearest

  // Create hardware-accelerated renderer with VSync enabled
  renderer.reset(SDL_CreateRenderer(window.get(), -1, rendererFlags));
  if (nullptr == renderer) {
    throw std::runtime_error("Renderer could not be created! Error: " +
                             std::string(SDL_GetError()));
  }

  // Disable integer scaling so content scales continuously with window size
  SDL_RenderSetIntegerScale(renderer.get(), SDL_FALSE);

  // Set logical size for scaling (this defines the virtual resolution)
  SDL_RenderSetLogicalSize(renderer.get(), targetWidth, targetHeight);
}

void Game::init() {
  // Initialize high-resolution timer for precise delta time calculation
  perfFreq = SDL_GetPerformanceFrequency();

  // Create player at starting position with texture
  auto playerTexture =
      std::make_shared<Texture>(renderer.get(), PLAYER_TEXTURE_PATH);

  player = std::make_unique<RectPlayer>(
      SDL_FRect{PLAYER_START_X, PLAYER_START_Y, PLAYER_WIDTH, PLAYER_HEIGHT},
      playerTexture);

  player->init();

  map = std::make_unique<Map>(DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT,
                              DEFAULT_TILE_WIDTH, DEFAULT_TILE_HEIGHT,
                              MAP_FILE_PATH);

  map->init(renderer.get());
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
    if (e.type == SDL_QUIT) {
      isRunning = false;
    } else if (e.type == SDL_WINDOWEVENT &&
               e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
      int w = e.window.data1;
      int h = e.window.data2;
      // Stretch to fill entire window (no letterboxing)
      SDL_RenderSetViewport(renderer.get(), nullptr);
      SDL_RenderSetScale(renderer.get(), static_cast<float>(w) / targetWidth,
                         static_cast<float>(h) / targetHeight);
    }
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

  // Read input states
  bool moveLeft =
      keyboardState[KEY_MOVE_LEFT] || keyboardState[KEY_MOVE_LEFT_ALT];
  bool moveRight =
      keyboardState[KEY_MOVE_RIGHT] || keyboardState[KEY_MOVE_RIGHT_ALT];
  bool jump = keyboardState[KEY_JUMP] || keyboardState[KEY_JUMP_ALT1] ||
              keyboardState[KEY_JUMP_ALT2];
  bool fastFall =
      (keyboardState[KEY_FAST_FALL] || keyboardState[KEY_FAST_FALL_ALT]) &&
      !player->grounded();
  bool dash = keyboardState[KEY_DASH];
  bool crouch = keyboardState[KEY_CROUCH] && player->grounded();

  // Handle movement through the new system
  player->handleMovement(dt, moveLeft, moveRight, jump, fastFall, dash, crouch);

  // Ground check - check if player is still touching ground by looking slightly
  // below
  if (player->grounded()) {
    SDL_FRect playerBounds = player->getCollisionBounds();
    SDL_FRect groundCheckBounds = {
        playerBounds.x,
        playerBounds.y + playerBounds.h, // Start just below player
        playerBounds.w,
        GROUND_CHECK_HEIGHT // Small height to check for ground
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

  // Apply movement update
  player->update(dt);
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

    // Update projectiles
    map->updateProjectiles(dt);

    // Update disappearing platforms
    map->updateDisappearingPlatforms(dt);

    // Apply layer-based status effects to player
    if (player) {
      // Check if player is on slow layer
      bool onSlowLayer = map->isPlayerOnSlowLayer(player->getCollisionBounds());
      player->setSlowed(onSlowLayer);

      // Check if player died from trap layer
      if (map->isPlayerOnTrapLayer(player->getCollisionBounds())) {
        player->setDead(true);
      }

      // Handle player death (simple respawn for now)
      if (player->getDead()) {
        // Reset player position to start
        player->setPos(PLAYER_START_X, PLAYER_START_Y);
        player->setDead(false);
      }
    }

    // Check collisions between player and projectiles
    if (player) {
      for (auto &projectile : map->getProjectiles()) {
        if (CollisionSystem::checkAABB(player->getCollisionBounds(),
                                       projectile->getCollisionBounds())) {
          // Handle collision directly instead of using private method
          float normalX, normalY, penetration;
          CollisionSystem::computeCollisionInfo(
              player->getCollisionBounds(), projectile->getCollisionBounds(),
              normalX, normalY, penetration);
          // Call collision callbacks (projectile handles collection/damage
          // logic)
          player->onCollision(projectile.get(), normalX, normalY, penetration);
          projectile->onCollision(player.get(), -normalX, -normalY,
                                  penetration);
        }
      }
    }

    // Remove dead projectiles and disappeared platforms
    map->removeDeadProjectiles();
    map->removeDisappearedPlatforms();

    // === RENDERING: Draw frame to screen ===
    // Clear screen to white background
    SDL_SetRenderDrawColor(renderer.get(), ALPHA_OPAQUE, ALPHA_OPAQUE,
                           ALPHA_OPAQUE, ALPHA_OPAQUE);
    SDL_RenderClear(renderer.get());

    // Draw map tiles (this replaces individual platform rendering)
    map->render(renderer.get(), dt);

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
