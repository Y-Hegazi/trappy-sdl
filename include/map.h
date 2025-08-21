#pragma once

#include "collideable.h"
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

  // tile access / mutation
  void setTile(int x, int y, std::shared_ptr<Platform> tile);
  std::shared_ptr<Platform> getTile(int x, int y) const;
  void removeTile(int x, int y);
  void clearTiles();

  // helpers
  bool inBounds(int x, int y) const;
  void worldToTile(int wx, int wy, int &tx, int &ty) const;
  SDL_FRect tileToWorldRect(int tx, int ty) const;

  // queries
  std::vector<std::shared_ptr<Platform>>
  getTilesInRect(const SDL_FRect &rect) const;
  std::vector<std::shared_ptr<Platform>> getAllTiles() const;
  void render(SDL_Renderer *renderer) const;

private:
  TMXParser tmxParser;
  int width;
  int height;
  int tileSizeW;
  int tileSizeH;
  std::vector<std::shared_ptr<Platform>> tiles;
  std::shared_ptr<Texture> assets; // Optional texture for rendering
};
