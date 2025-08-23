# RageBait Game

A modern C++17 2D platformer demonstrating SDL2 game development with cross-platform build automation.

## About This Project

This project showcases **modern C++ game development** using SDL2, featuring:

- **Modern C++17**: RAII patterns, smart pointers, and type-safe design
- **Cross-platform architecture**: Builds natively on Linux, Windows, and macOS
- **Automated CI/CD**: GitHub Actions for multi-platform executable distribution
- **Modular design**: Clean separation of concerns with dedicated systems for audio, collision, rendering, and map management
- **Resource management**: Automatic SDL subsystem initialization/cleanup and texture management
- **Real-time audio**: Background music, sound effects, and dynamic audio state management

### Technical Stack
- **Language**: C++17 with modern STL features
- **Graphics**: SDL2 + SDL2_image for hardware-accelerated 2D rendering
- **Audio**: SDL2_mixer for multi-channel audio playback
- **Text**: SDL2_ttf for TrueType font rendering
- **Data**: TinyXML2 for TMX map parsing
- **Build**: CMake with pkg-config integration and CPack packaging

## Game Features

- **Dynamic platformer mechanics**: Jump, run, and navigate through challenging levels
- **Coin collection system**: Gather all coins to win the level
- **Smart respawn system**: Player death resets coins to original positions while preserving level state
- **Pause system**: Non-intrusive pause menu with game instructions (press ESC)
- **Audio feedback**: Contextual sound effects for actions (coin collection, player death, victory)
- **Win condition**: Celebratory screen with audio and restart functionality
- **Real-time collision detection**: Efficient AABB collision system for platforms, traps, and collectibles

## Architecture Highlights

### Modern C++ Patterns
- **RAII Resource Management**: `SubSystemWrapper` automatically handles SDL initialization/cleanup
- **Smart Pointers**: `std::unique_ptr` and `std::shared_ptr` for automatic memory management  
- **Type Safety**: Strong typing with custom enums (`ProjectileType`, `PlayerSounds`)
- **Move Semantics**: Efficient resource transfers and container operations
- **Range-based Loops**: Modern iteration patterns throughout the codebase

### System Design
- **`Game`**: Core game loop with state management (running, paused, won)
- **`Map`**: Level loading from TMX files with layer-based rendering and coin tracking
- **`Player`**: Physics-based character controller with collision response
- **`AudioManager`**: Centralized audio system with resource caching
- **`CollisionSystem`**: Optimized spatial collision detection and response
- **`Projectile`**: Polymorphic system for coins, arrows, and interactive objects

### Cross-Platform Build System
- **CMake**: Modern CMake (3.16+) with `find_package` and pkg-config integration
- **GitHub Actions**: Automated builds for Linux (x64), Windows (x64), and macOS (Universal)
- **Package Management**: CPack integration for distributable archives
- **Dependency Handling**: Automatic SDL2 library detection and linking

## Building

### Prerequisites

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y \
  libsdl2-dev \
  libsdl2-image-dev \
  libsdl2-mixer-dev \
  libsdl2-ttf-dev \
  libtinyxml2-dev \
  cmake \
  build-essential
```

#### macOS
```bash
brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf tinyxml2 cmake
```

#### Windows
Install [vcpkg](https://github.com/Microsoft/vcpkg) and run:
```cmd
vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf tinyxml2 --triplet x64-windows
```

### Build Instructions

#### Using CMake (Recommended)
```bash
cd trappy-sdl
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

On Windows with vcpkg:
```cmd
cd trappy-sdl
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build . --config Release
```

#### Legacy build (Linux only)
```bash
cd trappy-sdl/src
g++ -std=c++17 -I../include *.cpp -o game -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -ltinyxml2
```

### Running

The executable will be in the `build/` directory:
```bash
cd build
./RageBaitGame    # Linux/macOS
RageBaitGame.exe  # Windows
```

Make sure the `resources/` folder is in the same directory as the executable.

## Controls

- **Arrow Keys / WASD**: Move
- **Space**: Jump
- **ESC**: Pause/Unpause
- **Space (on win screen)**: Restart game

## Cross-Platform Builds

This project includes GitHub Actions CI that automatically builds for:
- Linux (x64)
- Windows (x64) 
- macOS (x64/ARM64)

Download the latest builds from the [Releases](../../releases) page.

## Development Philosophy

This project demonstrates **production-quality C++ game development** practices:

### Code Quality
- **Const-correctness**: Immutable data where possible
- **Exception safety**: RAII ensures resources are properly cleaned up
- **Header/implementation separation**: Clean module boundaries
- **Minimal dependencies**: Only essential libraries (SDL2 ecosystem + TinyXML2)

### Performance Considerations
- **Efficient rendering**: Sprite batching and texture reuse
- **Optimized collision detection**: Spatial partitioning for large numbers of entities
- **Memory management**: Stack allocation preferred, smart pointers for dynamic allocation
- **Audio streaming**: Efficient loading and playback of compressed audio formats

### Maintainability
- **Modular architecture**: Each system has a single responsibility
- **Clear interfaces**: Public APIs are simple and well-documented
- **Cross-platform compatibility**: No platform-specific code in game logic
- **Build automation**: One-command builds and packaging for all platforms

### Learning Outcomes
- **SDL2 mastery**: Complete coverage of graphics, audio, input, and windowing
- **Modern C++**: Practical application of C++17 features in a real project
- **Game architecture**: Scalable patterns for larger game projects
- **DevOps integration**: CI/CD for game distribution and deployment

## Development

The project structure follows modern C++ conventions:

```
sdl/
├── include/          # Header files (.h)
│   ├── game.h        # Main game controller
│   ├── audio_manager.h  # Audio subsystem
│   ├── collision_system.h  # Physics and collision
│   ├── map.h         # Level loading and management
│   └── ...
├── src/              # Implementation files (.cpp)
│   ├── main.cpp      # Entry point
│   ├── game.cpp      # Game loop and state management
│   └── ...
├── resources/        # Game assets
│   ├── *.png         # Textures and sprites
│   ├── *.wav/*.mp3   # Audio files
│   ├── *.ttf         # Fonts
│   └── map.tmx       # Level data
├── CMakeLists.txt    # Modern CMake build configuration
└── build/            # Generated build artifacts (gitignored)
```

### Key Classes and Their Responsibilities

- **`Game`**: Main game loop, event handling, state transitions, rendering coordination
- **`Map`**: TMX file parsing, layer management, coin tracking, level reset functionality  
- **`Player`**: Character physics, input processing, collision response, animation state
- **`AudioManager`**: Sound loading, playback, volume control, audio state management
- **`Projectile`**: Base class for interactive objects (coins, arrows, bullets) with polymorphic behavior
- **`CollisionSystem`**: AABB collision detection, spatial optimization, collision event dispatch
- **`Texture` / `Sprite`**: Hardware-accelerated rendering with SDL2, texture management and sprite animation
