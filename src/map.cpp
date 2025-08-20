#include "../include/map.h"
#include <algorithm>

Map::Map(int width, int height, int tileSize)
    : width(width), height(height), tileSize(tileSize) {
  tiles.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
}

Map::~Map() = default;

// void Map::init(){
// for (int x = 0; x < 90; x++) {
//     SDL_FRect platformRect = {
//         static_cast<float>(x * 32),  // x position (32 pixels per tile)
//         static_cast<float>(14 * 32), // y position
//         32.0f,                       // width
//         32.0f                        // height
//     };
//     std::make_shared<Texture>
//     setTile(x, 14, std::make_shared<Platform>(platformRect, tex2));
//   }
// }

int Map::getWidth() const { return width; }
int Map::getHeight() const { return height; }
int Map::getTileSize() const { return tileSize; }

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
  tx = wx / tileSize;
  ty = wy / tileSize;
}

SDL_FRect Map::tileToWorldRect(int tx, int ty) const {
  return SDL_FRect{static_cast<float>(tx * tileSize),
                   static_cast<float>(ty * tileSize),
                   static_cast<float>(tileSize), static_cast<float>(tileSize)};
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
      tile->getSprite()->setSrcRect({0, 0, 500, 500});
      tile->getSprite()->setDestRect(tile->getCollisionBounds());
      tile->getSprite()->render(renderer);
    }
  }
}