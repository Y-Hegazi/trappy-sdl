#include "../include/layer.h"
#include "../include/config.h"
#include <algorithm>
#include <cmath>
#include <iostream>

Layer::Layer(const std::string &name, int width, int height, int tileSizeW,
             int tileSizeH)
    : name(name), width(width), height(height), tileSizeW(tileSizeW),
      tileSizeH(tileSizeH) {
  tiles.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
}

void Layer::setTile(int x, int y, std::shared_ptr<Platform> tile) {
  if (!inBounds(x, y))
    return;
  tiles[getIndex(x, y)] = std::move(tile);
}

std::shared_ptr<Platform> Layer::getTile(int x, int y) const {
  if (!inBounds(x, y))
    return nullptr;
  return tiles[getIndex(x, y)];
}

void Layer::removeTile(int x, int y) {
  if (!inBounds(x, y))
    return;
  tiles[getIndex(x, y)].reset();
}

void Layer::clearTiles() {
  std::fill(tiles.begin(), tiles.end(), std::shared_ptr<Platform>());
}

bool Layer::inBounds(int x, int y) const {
  return x >= 0 && x < width && y >= 0 && y < height;
}

std::vector<std::shared_ptr<Platform>>
Layer::getTilesInRect(const SDL_FRect &rect) const {
  int x0, y0, x1, y1;
  worldToTile(static_cast<int>(std::floor(rect.x)),
              static_cast<int>(std::floor(rect.y)), x0, y0);
  worldToTile(static_cast<int>(std::floor(rect.x + rect.w)),
              static_cast<int>(std::floor(rect.y + rect.h)), x1, y1);

  x0 = std::max(0, x0);
  y0 = std::max(0, y0);
  x1 = std::min(width - 1, x1);
  y1 = std::min(height - 1, y1);

  std::vector<std::shared_ptr<Platform>> result;
  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      auto tile = getTile(x, y);
      if (tile)
        result.push_back(tile);
    }
  }
  return result;
}

std::vector<std::shared_ptr<Platform>> Layer::getAllTiles() const {
  std::vector<std::shared_ptr<Platform>> result;
  result.reserve(std::count_if(
      tiles.begin(), tiles.end(),
      [](const std::shared_ptr<Platform> &p) { return static_cast<bool>(p); }));
  for (auto &tile : tiles) {
    if (tile)
      result.push_back(tile);
  }
  return result;
}

void Layer::render(SDL_Renderer *renderer) const {
  if (!renderer || !visible)
    return;

  // Set layer opacity if supported (SDL2 doesn't have direct layer opacity,
  // but we can apply it to individual sprites)
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      auto tile = getTile(x, y);
      if (!tile)
        continue;

      auto sprite = tile->getSprite();
      if (!sprite)
        continue;

      // Apply opacity to the sprite's texture
      if (opacity < 1.0f) {
        SDL_SetTextureAlphaMod(sprite->getTexture()->get(),
                               static_cast<Uint8>(ALPHA_OPAQUE * opacity));
      }

      if (tile->getPlatformType() == PlatformType::TRAP) {
        sprite->setDestRect(
            std::static_pointer_cast<TrapPlatform>(tile)->getOriginalBounds());
      } else
        sprite->setDestRect(tile->getCollisionBounds());
      sprite->render(renderer);

      // Reset alpha mod if we changed it
      if (opacity < 1.0f) {
        SDL_SetTextureAlphaMod(sprite->getTexture()->get(), ALPHA_OPAQUE);
      }
    }
  }
}

void Layer::loadFromTMXLayer(
    const TMXParser::Layer &tmxLayer,
    const std::vector<TMXParser::TilesetInfo> &tilesets,
    const std::vector<std::shared_ptr<Texture>> &tilesetTextures) {
  name = tmxLayer.name;
  visible = tmxLayer.visible;
  opacity = tmxLayer.opacity;

  // Clear existing tiles
  clearTiles();

  for (size_t i = 0; i < tmxLayer.data.size(); ++i) {
    int gid = tmxLayer.data[i];
    if (gid == 0)
      continue; // Skip empty tiles

    // Find which tileset this GID belongs to
    int tilesetIndex = -1;
    int localGid = gid;
    for (size_t j = 0; j < tilesets.size(); ++j) {
      if (gid >= tilesets[j].firstGid) {
        if (j == tilesets.size() - 1 || gid < tilesets[j + 1].firstGid) {
          tilesetIndex = static_cast<int>(j);
          localGid = gid - tilesets[j].firstGid;
          break;
        }
      }
    }

    if (tilesetIndex == -1 ||
        tilesetIndex >= static_cast<int>(tilesetTextures.size()) ||
        !tilesetTextures[tilesetIndex]) {
      continue; // Skip if tileset not found or texture failed to load
    }

    int tx = static_cast<int>(i % static_cast<size_t>(width));
    int ty = static_cast<int>(i / static_cast<size_t>(width));

    // Skip if tile position is out of bounds
    if (!inBounds(tx, ty))
      continue;

    float x_ = static_cast<float>(tx * tileSizeW);
    float y_ = static_cast<float>(ty * tileSizeH);
    float w_ = static_cast<float>(tileSizeW);
    float h_ = static_cast<float>(tileSizeH);
    SDL_FRect destRect = {x_, y_, w_, h_};

    const auto &currentTileset = tilesets[tilesetIndex];
    auto tile =
        std::make_shared<Platform>(destRect, tilesetTextures[tilesetIndex]);

    // Calculate source rectangle in the tileset
    const int cols = currentTileset.columns;
    const int srcTilesWidth = currentTileset.tilesWidth;
    const int srcTilesHeight = currentTileset.tilesHeight;
    int _x = (localGid % cols) * srcTilesWidth;
    int _y = (localGid / cols) * srcTilesHeight;
    int _w = srcTilesWidth;
    int _h = srcTilesHeight;
    SDL_Rect srcRect = {_x, _y, _w, _h};

    tile->getSprite()->setSrcRect(srcRect);
    tile->getSprite()->setDestRect(destRect);
    setTile(tx, ty, tile);
  }
}

size_t Layer::getIndex(int x, int y) const {
  return static_cast<size_t>(y) * static_cast<size_t>(width) +
         static_cast<size_t>(x);
}

void Layer::worldToTile(int wx, int wy, int &tx, int &ty) const {
  tx = wx / tileSizeW;
  ty = wy / tileSizeH;
}
