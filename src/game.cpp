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
      targetHeight(windowHeight), audioManager(nullptr),
      font(nullptr, [](TTF_Font *f) {
        if (f)
          TTF_CloseFont(f);
      }) {

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

  // Load default font for text rendering
  font.reset(TTF_OpenFont(FONT_PATH, 16));
  if (!font) {
    // Try alternative font path
    font.reset(TTF_OpenFont("/System/Library/Fonts/Arial.ttf", 16));
    if (!font) {
      // Create a simple fallback - we'll use rectangles if no font available
      std::cout << "Warning: Could not load font: " << TTF_GetError()
                << std::endl;
    }
  }

  // Initialize audio manager
  audioManager = std::make_shared<AudioManager>();
  audioManager->init();

  audioManager->loadMusic(PATH_TO_MUSIC);

  // Preload all player sounds
  audioManager->loadSound(PlayerSounds::DEAD_BY_TRAP,
                          PATH_TO_DEAD_BY_TRAP_SOUND);
  audioManager->loadSound(PlayerSounds::WIN, PATH_TO_WIN_SOUND);
  audioManager->loadSound(PlayerSounds::JUMP, PATH_TO_JUMP_SOUND);
  audioManager->loadSound(PlayerSounds::DASH, PATH_TO_DASH_SOUND);
  audioManager->loadSound(PlayerSounds::COLLECT_COIN,
                          PATH_TO_COLLECT_COIN_SOUND);
  audioManager->loadSound(PlayerSounds::HIT_BY_ARROW,
                          PATH_TO_HIT_BY_ARROW_SOUND);

  PLAY_MUSIC_DEFAULT ? audioManager->playMusic() : []() {};

  // Create player at starting position with texture
  auto playerTexture =
      std::make_shared<Texture>(renderer.get(), PLAYER_TEXTURE_PATH);

  player = std::make_unique<RectPlayer>(
      SDL_FRect{PLAYER_START_X, PLAYER_START_Y, PLAYER_WIDTH, PLAYER_HEIGHT},
      playerTexture);

  player->init();
  player->setAudioManager(audioManager);

  map = std::make_unique<Map>(DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT,
                              DEFAULT_TILE_WIDTH, DEFAULT_TILE_HEIGHT,
                              MAP_FILE_PATH);
  map->setAudioManager(audioManager);

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
    } else if (e.type == SDL_KEYDOWN) {
      if (e.key.keysym.scancode == KEY_PAUSE && !hasWon) {
        isPaused = !isPaused; // Toggle pause state (only if not won)
      } else if ((e.key.keysym.scancode == SDL_SCANCODE_SPACE ||
                  e.key.keysym.scancode == KEY_JUMP_ALT2) &&
                 hasWon) {
        // Reset game when space is pressed after winning
        resetGame();
      }
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
  // Early return if player not initialized or game is paused
  if (!player || isPaused)
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
  bool dash = keyboardState[KEY_DASH] || keyboardState[KEY_DASH_ALT];
  bool crouch = keyboardState[KEY_CROUCH] && player->grounded() ||
                keyboardState[KEY_CROUCH_ALT] && player->grounded();

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

    // Check against map tiles
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
  // Not Owning
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
    handleEvents(e); // Handle quit events and pause

    // Only update game state if not paused and not won
    if (!isPaused && !hasWon) {
      updatePlayerPos(dt); // Handle movement input and physics

      // Update projectiles
      map->updateProjectiles(dt);

      // Update disappearing platforms
      map->updateDisappearingPlatforms(dt);

      // Apply layer-based status effects to player
      if (player) {
        // Check if player is on slow layer
        bool onSlowLayer =
            map->isPlayerOnSlowLayer(player->getCollisionBounds());
        player->setSlowed(onSlowLayer);

        // Check if player died from trap layer
        if (map->isPlayerOnTrapLayer(player->getCollisionBounds())) {
          player->setDead(true);
          audioManager->playSound(PlayerSounds::DEAD_BY_TRAP);
        }

        // Handle player death (simple respawn for now)
        if (player->getDead()) {
          // Reset player position to start
          player->setPos(PLAYER_START_X, PLAYER_START_Y);
          player->setDead(false);
          // Reset all coins when player dies
          map->resetCoins();
        }
      }

      // Check collisions between player and projectiles
      if (player) {
        for (auto &projectile : map->getProjectiles()) {
          if (CollisionSystem::checkAABB(player->getCollisionBounds(),
                                         projectile->getCollisionBounds())) {
            // Check if it's a coin before collision to track collection
            bool wasCoin = (projectile->getProjectileType() ==
                            Projectile::ProjectileType::COIN);

            // Handle collision directly instead of using private method
            float normalX, normalY, penetration;
            CollisionSystem::computeCollisionInfo(
                player->getCollisionBounds(), projectile->getCollisionBounds(),
                normalX, normalY, penetration);
            // Call collision callbacks (projectile handles collection/damage
            // logic)
            player->onCollision(projectile.get(), normalX, normalY,
                                penetration);
            projectile->onCollision(player.get(), -normalX, -normalY,
                                    penetration);

            // If a coin was collected, increment counter
            if (wasCoin && projectile->shouldBeRemoved()) {
              map->collectCoin();
            }
          }
        }
      }

      // Check for win condition
      if (map->areAllCoinsCollected() && !hasWon) {
        hasWon = true;
        audioManager->stopMusic(); // Stop background music
        audioManager->playSound(PlayerSounds::WIN,
                                -1); // Play win sound on loop
        std::cout << "You collected all coins and won!" << std::endl;
      }

      // Remove dead projectiles and disappeared platforms
      map->removeDeadProjectiles();
      map->removeDisappearedPlatforms();
    }

    // === RENDERING: Draw frame to screen ===
    // Clear screen to white background
    SDL_SetRenderDrawColor(renderer.get(), ALPHA_OPAQUE, ALPHA_OPAQUE,
                           ALPHA_OPAQUE, ALPHA_OPAQUE);
    SDL_RenderClear(renderer.get());

    // Draw map tiles (this replaces individual platform rendering)
    map->render(renderer.get(), dt);

    // Draw player sprite
    if (player) {
      player->renderAnimation(renderer.get(), dt);
    }

    // Draw pause menu if paused
    if (isPaused && !hasWon) {
      renderPauseMenu();
    }

    // Draw win screen if won
    if (hasWon) {
      renderWinScreen();
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

/**
 * Render text to a texture
 */
SDL_Texture *Game::renderText(const char *text, SDL_Color color) {
  if (!font) {
    return nullptr; // No font available
  }

  SDL_Surface *surface = TTF_RenderText_Solid(font.get(), text, color);
  if (!surface) {
    return nullptr;
  }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer.get(), surface);
  SDL_FreeSurface(surface);

  return texture;
}

/**
 * Render the pause menu with instructions
 * Uses basic SDL rectangles to create a visual representation of the pause menu
 * Since SDL_ttf is not available, this creates a simple overlay
 */
void Game::renderPauseMenu() {
  // Semi-transparent overlay background
  SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 180); // Dark semi-transparent
  SDL_Rect overlayRect = {0, 0, targetWidth, targetHeight};
  SDL_RenderFillRect(renderer.get(), &overlayRect);

  // Main menu background (light gray)
  SDL_SetRenderDrawColor(renderer.get(), 200, 200, 200, 255);
  SDL_Rect menuRect = {targetWidth / 4, targetHeight / 4, targetWidth / 2,
                       targetHeight / 2};
  SDL_RenderFillRect(renderer.get(), &menuRect);

  // Menu border (dark gray)
  SDL_SetRenderDrawColor(renderer.get(), 100, 100, 100, 255);
  SDL_RenderDrawRect(renderer.get(), &menuRect);

  // If we have a font, render actual text
  if (font) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color black = {0, 0, 0, 255};
    SDL_Color orange = {255, 165, 0, 255};

    int currentY = menuRect.y + 20;
    int centerX = menuRect.x + menuRect.w / 2;

    // Render "GAME PAUSED" title
    SDL_Texture *titleTexture = renderText("GAME PAUSED", black);
    if (titleTexture) {
      int w, h;
      SDL_QueryTexture(titleTexture, NULL, NULL, &w, &h);
      SDL_Rect titleRect = {centerX - w / 2, currentY, w, h};
      SDL_RenderCopy(renderer.get(), titleTexture, NULL, &titleRect);
      SDL_DestroyTexture(titleTexture);
      currentY += h + 20;
    }

    // Render "HOW TO PLAY:" header
    SDL_Texture *howToPlayTexture = renderText("HOW TO PLAY:", black);
    if (howToPlayTexture) {
      int w, h;
      SDL_QueryTexture(howToPlayTexture, NULL, NULL, &w, &h);
      SDL_Rect howToRect = {menuRect.x + 20, currentY, w, h};
      SDL_RenderCopy(renderer.get(), howToPlayTexture, NULL, &howToRect);
      SDL_DestroyTexture(howToPlayTexture);
      currentY += h + 10;
    }

    // Render control instructions
    const char *controls[] = {"A/D or Arrow Keys: Move Left/Right",
                              "W/Space: Jump", "S: Fast Fall", "Shift: Dash",
                              "Ctrl: Crouch"};

    for (int i = 0; i < 5; i++) {
      SDL_Texture *controlTexture = renderText(controls[i], black);
      if (controlTexture) {
        int w, h;
        SDL_QueryTexture(controlTexture, NULL, NULL, &w, &h);
        SDL_Rect controlRect = {menuRect.x + 30, currentY, w, h};
        SDL_RenderCopy(renderer.get(), controlTexture, NULL, &controlRect);
        SDL_DestroyTexture(controlTexture);
        currentY += h + 5;
      }
    }

    currentY += 15;

    // Render objective
    SDL_Texture *objectiveTexture =
        renderText("COLLECT ALL BANANAS TO WIN!", orange);
    if (objectiveTexture) {
      int w, h;
      SDL_QueryTexture(objectiveTexture, NULL, NULL, &w, &h);
      SDL_Rect objectiveRect = {centerX - w / 2, currentY, w, h};
      SDL_RenderCopy(renderer.get(), objectiveTexture, NULL, &objectiveRect);
      SDL_DestroyTexture(objectiveTexture);
      currentY += h + 15;
    }

    // Render resume instruction
    SDL_Texture *resumeTexture = renderText("Press ESC to resume", black);
    if (resumeTexture) {
      int w, h;
      SDL_QueryTexture(resumeTexture, NULL, NULL, &w, &h);
      SDL_Rect resumeRect = {centerX - w / 2, currentY, w, h};
      SDL_RenderCopy(renderer.get(), resumeTexture, NULL, &resumeRect);
      SDL_DestroyTexture(resumeTexture);
    }
  } else {
    // Fallback to rectangles if no font is available
    // Title area (darker gray)
    SDL_SetRenderDrawColor(renderer.get(), 150, 150, 150, 255);
    SDL_Rect titleRect = {menuRect.x + 10, menuRect.y + 10, menuRect.w - 20,
                          40};
    SDL_RenderFillRect(renderer.get(), &titleRect);

    // Create visual representations of text lines using colored rectangles
    // "GAME PAUSED" title representation
    SDL_SetRenderDrawColor(renderer.get(), 80, 80, 80, 255);
    SDL_Rect pausedTextRect = {titleRect.x + 10, titleRect.y + 8,
                               titleRect.w - 20, 24};
    SDL_RenderFillRect(renderer.get(), &pausedTextRect);

    // Instructions section
    int currentY = titleRect.y + titleRect.h + 20;
    int lineHeight = 25;
    int lineWidth = menuRect.w - 40;
    int textX = menuRect.x + 20;

    // "HOW TO PLAY:" header
    SDL_SetRenderDrawColor(renderer.get(), 60, 60, 60, 255);
    SDL_Rect howToPlayRect = {textX, currentY, lineWidth / 2, 20};
    SDL_RenderFillRect(renderer.get(), &howToPlayRect);
    currentY += lineHeight;

    // Movement controls representation
    SDL_SetRenderDrawColor(renderer.get(), 120, 120, 120, 255);

    // Control instruction rectangles
    SDL_Rect moveRect = {textX, currentY, lineWidth - 50, 16};
    SDL_RenderFillRect(renderer.get(), &moveRect);
    currentY += lineHeight;

    SDL_Rect jumpRect = {textX, currentY, lineWidth / 3, 16};
    SDL_RenderFillRect(renderer.get(), &jumpRect);
    currentY += lineHeight;

    SDL_Rect fallRect = {textX, currentY, lineWidth / 3, 16};
    SDL_RenderFillRect(renderer.get(), &fallRect);
    currentY += lineHeight;

    SDL_Rect dashRect = {textX, currentY, lineWidth / 4, 16};
    SDL_RenderFillRect(renderer.get(), &dashRect);
    currentY += lineHeight;

    SDL_Rect crouchRect = {textX, currentY, lineWidth / 3, 16};
    SDL_RenderFillRect(renderer.get(), &crouchRect);
    currentY += lineHeight + 10;

    // Objective section
    SDL_SetRenderDrawColor(renderer.get(), 180, 140, 0,
                           255); // Orange/yellow for bananas
    SDL_Rect bananaObjectiveRect = {textX, currentY, lineWidth - 30, 18};
    SDL_RenderFillRect(renderer.get(), &bananaObjectiveRect);
    currentY += lineHeight + 10;

    // Resume instruction
    SDL_SetRenderDrawColor(renderer.get(), 80, 80, 80, 255);
    SDL_Rect resumeRect = {textX, currentY, lineWidth - 50, 16};
    SDL_RenderFillRect(renderer.get(), &resumeRect);
  }

  // Reset blend mode
  SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_NONE);
}

/**
 * Render the win screen with celebration text
 */
void Game::renderWinScreen() {
  // Semi-transparent overlay background (darker than pause menu)
  SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0,
                         200); // Very dark semi-transparent
  SDL_Rect overlayRect = {0, 0, targetWidth, targetHeight};
  SDL_RenderFillRect(renderer.get(), &overlayRect);

  // Victory background (bright gold/yellow)
  SDL_SetRenderDrawColor(renderer.get(), 255, 215, 0, 255); // Gold color
  SDL_Rect winRect = {targetWidth / 6, targetHeight / 3, targetWidth * 2 / 3,
                      targetHeight / 3};
  SDL_RenderFillRect(renderer.get(), &winRect);

  // Win border (darker gold)
  SDL_SetRenderDrawColor(renderer.get(), 218, 165, 32, 255); // Dark goldenrod
  SDL_RenderDrawRect(renderer.get(), &winRect);

  if (font) {
    SDL_Color black = {0, 0, 0, 255};
    SDL_Color darkBlue = {0, 0, 139, 255};

    int centerX = targetWidth / 2;
    int currentY = winRect.y + 30;

    // Render some message title (large)
    SDL_Texture *messageTexture = renderText("winMessage!", black);
    if (messageTexture) {
      int w, h;
      SDL_QueryTexture(messageTexture, NULL, NULL, &w, &h);
      // Make it bigger by scaling
      w *= 3;
      h *= 3;
      SDL_Rect messageRect = {centerX - w / 2, currentY, w, h};
      SDL_RenderCopy(renderer.get(), messageTexture, NULL, &messageRect);
      SDL_DestroyTexture(messageTexture);
      currentY += h + 20;
    }

    // Render "You collected all bananas!"
    SDL_Texture *victoryTexture =
        renderText("You collected all bananas!", darkBlue);
    if (victoryTexture) {
      int w, h;
      SDL_QueryTexture(victoryTexture, NULL, NULL, &w, &h);
      SDL_Rect victoryRect = {centerX - w / 2, currentY, w, h};
      SDL_RenderCopy(renderer.get(), victoryTexture, NULL, &victoryRect);
      SDL_DestroyTexture(victoryTexture);
      currentY += h + 15;
    }

    // Render "Press SPACE to play again"
    SDL_Texture *playAgainTexture =
        renderText("Press SPACE to play again", black);
    if (playAgainTexture) {
      int w, h;
      SDL_QueryTexture(playAgainTexture, NULL, NULL, &w, &h);
      SDL_Rect playAgainRect = {centerX - w / 2, currentY, w, h};
      SDL_RenderCopy(renderer.get(), playAgainTexture, NULL, &playAgainRect);
      SDL_DestroyTexture(playAgainTexture);
    }
  } else {
    // Fallback to rectangles if no font
    // Large win message from earlier representation (big golden rectangle)
    SDL_SetRenderDrawColor(renderer.get(), 255, 140, 0, 255); // Dark orange
    SDL_Rect messageRect = {winRect.x + 20, winRect.y + 20, winRect.w - 40, 60};
    SDL_RenderFillRect(renderer.get(), &messageRect);

    // Victory message representation
    SDL_SetRenderDrawColor(renderer.get(), 0, 100, 0, 255); // Dark green
    SDL_Rect victoryRect = {winRect.x + 30, winRect.y + 100, winRect.w - 60,
                            30};
    SDL_RenderFillRect(renderer.get(), &victoryRect);

    // Play again message representation
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 100, 255); // Dark blue
    SDL_Rect playAgainRect = {winRect.x + 40, winRect.y + 150, winRect.w - 80,
                              25};
    SDL_RenderFillRect(renderer.get(), &playAgainRect);
  }

  // Reset blend mode
  SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_NONE);
}

/**
 * Reset game to initial state for replay
 */
void Game::resetGame() {
  hasWon = false;
  isPaused = false;

  // Reset player
  if (player) {
    player->setPos(PLAYER_START_X, PLAYER_START_Y);
    player->setDead(false);
  }

  // Reset coins
  if (map) {
    map->resetCoins();
  }

  // Stop win sound and restart background music
  audioManager->stopAll();
  if (PLAY_MUSIC_DEFAULT) {
    audioManager->playMusic();
  }
}
