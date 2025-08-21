#pragma once

#include "collideable.h"
#include "layer.h"
#include "platform.h"
#include "sprite.h"
#include "texture.h"
#include "tmx_parser.h"
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
  Layer* getLayer(int index) const;
  Layer* getLayer(const std::string &name) const;
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
  void render(SDL_Renderer *renderer) const;
  void renderLayer(SDL_Renderer *renderer, int index) const;

private:
  TMXParser tmxParser;
  int width;
  int height;
  int tileSizeW;
  int tileSizeH;
  
  // Layer system
  std::vector<std::unique_ptr<Layer>> layers;
  
  // Tileset textures (shared across layers)
  std::vector<std::shared_ptr<Texture>> tilesetTextures;
  
  // Legacy support
  std::vector<std::shared_ptr<Platform>> tiles; // Deprecated, use layers instead
  std::shared_ptr<Texture> assets; // Optional texture for rendering
};
