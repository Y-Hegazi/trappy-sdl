#pragma once

#include "collideable.h"
#include "platform.h"
#include "sprite.h"
#include "texture.h"
#include <SDL2/SDL.h>
#include <memory>
#include <unordered_map>
#include <vector>

class Map {
public:
  Map(int width, int height, int tileSize = 32);
  ~Map();

  void init();

  // basic info
  int getWidth() const;
  int getHeight() const;
  int getTileSize() const;

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
  int width;
  int height;
  int tileSize;
  std::vector<std::shared_ptr<Platform>> tiles;
  std::shared_ptr<Texture> assets; // Optional texture for rendering
};