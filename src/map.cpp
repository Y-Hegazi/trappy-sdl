#include "../include/map.h"
#include <algorithm>
#include <cmath>
#include <iostream>

Map::Map(int width, int height, int tileSizeW, int tileSizeH,
         const std::string &tmxFilePath)
    : width(width), height(height), tileSizeW(tileSizeW), tileSizeH(tileSizeH),
      tmxParser(tmxFilePath) {
  tiles.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
}

Map::~Map() = default;

void Map::init(SDL_Renderer *renderer) {
  try {
    tmxParser.loadFile();
  } catch (const std::exception &e) {
    std::cerr << "Error loading TMX file: " << e.what() << std::endl;
  }
  TMXParser::MapInfo mapInfo = tmxParser.getMapInfo();
  auto tilesetInfo = tmxParser.getTilesetInfo();
  auto layersInfo = tmxParser.getLayersInfo();

  width = mapInfo.mapWidth;
  height = mapInfo.mapHeight;
  tileSizeW = mapInfo.tileWidth;
  tileSizeH = mapInfo.tileHeight;
  tiles.assign(static_cast<size_t>(width) * static_cast<size_t>(height),
               nullptr);
  const int cols = tilesetInfo[0].columns;
  const int srcTilesWidth = tilesetInfo[0].tilesWidth;
  const int srcTilesHeight = tilesetInfo[0].tilesHeight;
  assets =
      std::make_shared<Texture>(renderer, tilesetInfo[0].imagePath.c_str());

  for (size_t i = 0; i < layersInfo[0].data.size(); ++i) {
    int gid = layersInfo[0].data[i];
    if (gid) {
      int tx = static_cast<int>(i % static_cast<size_t>(width));
      int ty = static_cast<int>(i / static_cast<size_t>(width));
      float x_ = static_cast<float>(tx * tileSizeW);
      float y_ = static_cast<float>(ty * tileSizeH);
      float w_ = static_cast<float>(tileSizeW);
      float h_ = static_cast<float>(tileSizeH);
      SDL_FRect destRect = {x_, y_, w_, h_};
      auto tile = std::make_shared<Platform>(destRect, assets);
      int local = gid - tilesetInfo[0].firstGid;
      int _x = (local % cols) * srcTilesWidth;
      int _y = (local / cols) * srcTilesHeight;
      int _w = srcTilesWidth;
      int _h = srcTilesHeight;
      SDL_Rect srcRect = {_x, _y, _w, _h};
      tile->getSprite()->setSrcRect(srcRect);
      tile->getSprite()->setDestRect(destRect);
      setTile(tx, ty, tile);
    }
  }
}

int Map::getWidth() const { return width; }
int Map::getHeight() const { return height; }
int Map::getTileSize() const { return tileSizeW; }

void Map::setTile(int x, int y, std::shared_ptr<Platform> tile) {
  if (!inBounds(x, y))
    return;
  tiles[static_cast<size_t>(y) * width + static_cast<size_t>(x)] =
      std::move(tile);
}

std::shared_ptr<Platform> Map::getTile(int x, int y) const {
  if (!inBounds(x, y))
    return nullptr;
  return tiles[static_cast<size_t>(y) * width + static_cast<size_t>(x)];
}

void Map::removeTile(int x, int y) {
  if (!inBounds(x, y))
    return;
  tiles[static_cast<size_t>(y) * width + static_cast<size_t>(x)].reset();
}

void Map::clearTiles() {
  std::fill(tiles.begin(), tiles.end(), std::shared_ptr<Platform>());
}

bool Map::inBounds(int x, int y) const {
  return x >= 0 && x < width && y >= 0 && y < height;
}

void Map::worldToTile(int wx, int wy, int &tx, int &ty) const {
  tx = wx / tileSizeW;
  ty = wy / tileSizeH;
}

SDL_FRect Map::tileToWorldRect(int tx, int ty) const {
  return SDL_FRect{
      static_cast<float>(tx * tileSizeW), static_cast<float>(ty * tileSizeH),
      static_cast<float>(tileSizeW), static_cast<float>(tileSizeH)};
}

std::vector<std::shared_ptr<Platform>>
Map::getTilesInRect(const SDL_FRect &rect) const {
  // Convert world rect to tile indices, clamp to map bounds
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
      auto t = getTile(x, y);
      if (t)
        result.push_back(t);
    }
  }
  return result;
}

std::vector<std::shared_ptr<Platform>> Map::getAllTiles() const {
  std::vector<std::shared_ptr<Platform>> result;
  result.reserve(std::count_if(
      tiles.begin(), tiles.end(),
      [](const std::shared_ptr<Platform> &p) { return static_cast<bool>(p); }));
  for (auto &t : tiles)
    if (t)
      result.push_back(t);
  return result;
}

void Map::render(SDL_Renderer *renderer) const {
  if (!renderer)
    return;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      auto tile = getTile(x, y);
      if (!tile)
        continue;
      // Ensure sprite destination matches the tile's world rect
      // tile->getSprite()->setSrcRect({0, 0, 500, 500});
      tile->getSprite()->setDestRect(tile->getCollisionBounds());
      tile->getSprite()->render(renderer);
    }
  }
}
