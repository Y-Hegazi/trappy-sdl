#pragma once

#include "collideable.h"
#include "config.h"
#include "platform.h"
#include "texture.h"
#include "tmx_parser.h"
#include "trap_platform.h"
#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include <vector>

class Layer {
public:
  Layer(const std::string &name, int width, int height, int tileSizeW,
        int tileSizeH);
  ~Layer() = default;

  // Layer properties
  void setName(const std::string &name) { this->name = name; }
  const std::string &getName() const { return name; }

  void setVisible(bool visible) { this->visible = visible; }
  bool isVisible() const { return visible; }

  void setCollidable(bool collidable) { this->collidable = collidable; }
  bool isCollidable() const { return collidable; }

  void setOpacity(float opacity) { this->opacity = opacity; }
  float getOpacity() const { return opacity; }

  // Dimensions
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  int getTileWidth() const { return tileSizeW; }
  int getTileHeight() const { return tileSizeH; }

  // Tile management
  void setTile(int x, int y, std::shared_ptr<Platform> tile);
  std::shared_ptr<Platform> getTile(int x, int y) const;
  void removeTile(int x, int y);
  void clearTiles();

  // Queries
  bool inBounds(int x, int y) const;
  std::vector<std::shared_ptr<Platform>>
  getTilesInRect(const SDL_FRect &rect) const;
  std::vector<std::shared_ptr<Platform>> getAllTiles() const;

  // Rendering
  void render(SDL_Renderer *renderer) const;

  // Layer data loading from TMX
  void loadFromTMXLayer(
      const TMXParser::Layer &tmxLayer,
      const std::vector<TMXParser::TilesetInfo> &tilesets,
      const std::vector<std::shared_ptr<Texture>> &tilesetTextures);

private:
  std::string name;
  bool visible = true;
  bool collidable = true;
  float opacity = 1.0f;

  int width;
  int height;
  int tileSizeW;
  int tileSizeH;

  std::vector<std::shared_ptr<Platform>> tiles;

  // Helper methods
  size_t getIndex(int x, int y) const;
  void worldToTile(int wx, int wy, int &tx, int &ty) const;
};
