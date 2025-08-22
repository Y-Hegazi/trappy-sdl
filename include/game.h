// Include guard to prevent multiple inclusions
#ifndef GAME_H
#define GAME_H

#include "config.h"
#include "map.h"
#include "platform.h"
#include "player.h"
#include <SDL2/SDL_image.h>
#include <memory>
/**
 * SubSystemWrapper - RAII wrapper for SDL subsystem initialization and cleanup
 *
 * Responsibilities:
 * - Initializes SDL subsystems on construction
 * - Cleans up SDL subsystems on destruction
 * - Prevents copy and move semantics to ensure single ownership
 *
 * Usage:
 * Create an instance at the start of your application to guarantee
 * SDL is properly initialized and cleaned up.
 */
class SubSystemWrapper {
public:
  /**
   * Constructor - Initializes SDL subsystems
   * @param flags SDL initialization flags (default: SDL_INIT_VIDEO)
   * @throws std::runtime_error if SDL initialization fails
   */
  SubSystemWrapper(Uint32 SDL_flags = SDL_INIT_VIDEO,
                   Uint32 IMG_flags = IMG_INIT_PNG) {
    if (SDL_Init(SDL_flags) < 0) {
      throw std::runtime_error("Failed to initialize SDL: " +
                               std::string(SDL_GetError()));
    }
    // Initialize PNG support (if needed)
    if (!(IMG_Init(IMG_flags) & IMG_flags)) {
      throw std::runtime_error("Failed to initialize SDL_image: " +
                               std::string(IMG_GetError()));
    }
  }

  /**
   * Destructor - Cleans up SDL subsystems
   * Automatically called when SubSystemWrapper goes out of scope
   */
  ~SubSystemWrapper() {
    IMG_Quit();
    SDL_Quit();
  }

  // Delete copy constructor and copy assignment operator
  SubSystemWrapper(const SubSystemWrapper &) = delete;
  SubSystemWrapper &operator=(const SubSystemWrapper &) = delete;

  // Delete move constructor and move assignment operator
  SubSystemWrapper(SubSystemWrapper &&) = delete;
  SubSystemWrapper &operator=(SubSystemWrapper &&) = delete;
};

/**
 * Game - Main game class that manages the SDL window, renderer, and game loop
 *
 * Responsibilities:
 * - SDL initialization and cleanup
 * - Window and renderer management
 * - Main game loop (input, update, render)
 * - Player management and input handling
 * - Basic collision detection with static floor
 *
 * Design pattern: This follows a simple game object architecture where
 * the Game class acts as the main controller/manager
 */
class Game {
public:
  /**
   * Constructor - Initialize SDL, create window and renderer
   * @param name Window title string
   * @throws std::runtime_error if SDL initialization fails
   */
  Game(const char *name, const char *playerTexture, int windowWidth = 800,
       int windowHeight = 600, int windowFlags = SDL_WINDOW_SHOWN,
       int rendererFlags = SDL_RENDERER_ACCELERATED |
                           SDL_RENDERER_PRESENTVSYNC);

  /**
   * Destructor - Clean up SDL resources and player object
   * Automatically called when Game object goes out of scope
   */
  ~Game();

  /**
   * Main game loop - Run until user quits
   * Contains the core game loop: input -> update -> render -> repeat
   */
  void run();

  /**
   * Handle SDL events (quit, keyboard input, etc.)
   * @param e SDL_Event reference to process
   */
  void handleEvents(SDL_Event &e);

  void playerInit(SDL_Rect rect, std::shared_ptr<Texture> texture);
  void init();

private:
  // === SDL Core Objects ===
  SubSystemWrapper sdlSubsystem;
  std::unique_ptr<SDL_Window, void (*)(SDL_Window *)> window;
  std::unique_ptr<SDL_Renderer, void (*)(SDL_Renderer *)> renderer;
  std::unique_ptr<Map> map;

  // === Game Objects ===

  std::unique_ptr<RectPlayer> player =
      nullptr; // Smart pointer for automatic cleanup
  Platform *platform;
  Platform *platform2;

  // === Timing and Performance ===
  Uint64 perfFreq; // SDL performance counter frequency (for delta time
                   // calculation)
  const char *playerTexturePath; // Path to player texture file

  // === Scaling and Resolution ===
  int targetWidth;  // Target logical resolution width
  int targetHeight; // Target logical resolution height

  // === Game State ===
  bool isRunning = false; // Main game loop control flag

  // === Game World ===
  SDL_Rect floor = {0, 300, 800, 50};   // Static floor collision rectangle
  SDL_Rect floor2 = {200, 260, 50, 50}; // Static floor collision rectangle

  /**
   * Update player position based on keyboard input
   * @param dt Delta time in seconds since last frame
   *
   * Input mapping:
   * - A/Left Arrow: Move left
   * - D/Right Arrow: Move right
   * - W/Up Arrow: Jump (negative vertical velocity)
   * - S/Down Arrow: Fast fall (additional downward velocity)
   * - Left Shift: Dash (applies dash multiplier)
   * - Left Control: Crouch (reduces hitbox size)
   *
   * Also handles collision detection with the floor
   */
  void updatePlayerPos(float dt);
};

#endif // GAME_H