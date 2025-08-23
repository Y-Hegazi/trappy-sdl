#pragma once

#include "audio_manager.h"
#include "collideable.h"
#include "config.h"
#include "disappearing_platform.h"
#include "layer.h"
#include "platform.h"
#include "projectile.h"
#include "sprite.h"
#include "texture.h"
#include "tmx_parser.h"
#include "trap_platform.h"
#include <SDL2/SDL.h>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>
class Map {
public:
  Map(int width, int height, int tileSizeW = 32, int tileSizeH = 32,
      const std::string &tmxFilePath = "");
  ~Map();

  void init(SDL_Renderer *renderer);
  void init(const char *txmFilePath);

  // basic info
  int getWidth() const;
  int getHeight() const;
  // Back-compat single-size getter (returns width size)
  int getTileSize() const;
  // Preferred explicit getters
  int getTileWidth() const { return tileSizeW; }
  int getTileHeight() const { return tileSizeH; }

  // Layer management
  int getLayerCount() const;
  Layer *getLayer(int index) const;
  Layer *getLayer(const std::string &name) const;
  int addLayer(const std::string &name);
  void removeLayer(int index);
  void setLayerVisible(int index, bool visible);
  void setLayerCollidable(int index, bool collidable);

  // Back-compat tile access (operates on layer 0 if it exists)
  void setTile(int x, int y, std::shared_ptr<Platform> tile);
  std::shared_ptr<Platform> getTile(int x, int y) const;
  void removeTile(int x, int y);
  void clearTiles();

  // helpers
  bool inBounds(int x, int y) const;
  void worldToTile(int wx, int wy, int &tx, int &ty) const;
  SDL_FRect tileToWorldRect(int tx, int ty) const;

  // queries (searches all collidable layers)
  std::vector<std::shared_ptr<Platform>>
  getTilesInRect(const SDL_FRect &rect) const;
  std::vector<std::shared_ptr<Platform>> getAllTiles() const;

  // rendering
  void render(SDL_Renderer *renderer, float dt) const;
  void renderLayer(SDL_Renderer *renderer, int index) const;

  // projectile management
  void updateProjectiles(float dt);
  void removeDeadProjectiles();
  std::vector<std::shared_ptr<Projectile>> &getProjectiles() {
    return projectiles;
  }

  // special platform management
  void updateDisappearingPlatforms(float dt);
  void removeDisappearedPlatforms();

  // layer-based status effects
  bool isPlayerOnSlowLayer(const SDL_FRect &playerBounds) const;
  bool isPlayerOnTrapLayer(const SDL_FRect &playerBounds) const;

  void setAudioManager(std::shared_ptr<AudioManager> audioManager) {
    this->audioManager = audioManager;
  }

private:
  TMXParser tmxParser;
  int width;
  int height;
  int tileSizeW;
  int tileSizeH;

  // Layer system, includes traps
  std::vector<std::unique_ptr<Layer>> layers;

  // Tileset textures (shared across layers)
  std::vector<std::shared_ptr<Texture>> tilesetTextures;

  std::vector<std::shared_ptr<Platform>>
      tiles; // Deprecated, use layers instead but it causes errors when removed
             // so...

  std::vector<std::shared_ptr<Projectile>> projectiles;
  std::vector<std::shared_ptr<DisappearingPlatform>> disappearingPlatforms;

  std::shared_ptr<AudioManager>
      audioManager;                // Optional audio manager for sound effects
  std::shared_ptr<Texture> assets; // Optional texture for rendering
};
