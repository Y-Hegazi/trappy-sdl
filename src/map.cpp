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
    return;
  }
  
  TMXParser::MapInfo mapInfo = tmxParser.getMapInfo();
  auto tilesetInfo = tmxParser.getTilesetInfo();
  auto layersInfo = tmxParser.getLayersInfo();

  width = mapInfo.mapWidth;
  height = mapInfo.mapHeight;
  tileSizeW = mapInfo.tileWidth;
  tileSizeH = mapInfo.tileHeight;
  
  // Clear existing layers
  layers.clear();
  tilesetTextures.clear();

  // Load textures for all tilesets
  for (const auto &tileset : tilesetInfo) {
    if (!tileset.imagePath.empty()) {
      try {
        auto texture = std::make_shared<Texture>(renderer, tileset.imagePath.c_str());
        tilesetTextures.push_back(texture);
        std::cout << "Loaded tileset texture: " << tileset.imagePath << std::endl;
      } catch (const std::exception &e) {
        std::cerr << "Failed to load tileset texture: " << tileset.imagePath 
                  << " - " << e.what() << std::endl;
        tilesetTextures.push_back(nullptr);
      }
    } else {
      tilesetTextures.push_back(nullptr);
    }
  }

  // Store first texture as default for backward compatibility
  if (!tilesetTextures.empty() && tilesetTextures[0]) {
    assets = tilesetTextures[0];
  }

  // Create layers from TMX data
  for (const auto &layerInfo : layersInfo) {
    auto layer = std::make_unique<Layer>(layerInfo.name, width, height, tileSizeW, tileSizeH);
    layer->loadFromTMXLayer(layerInfo, tilesetInfo, tilesetTextures);
    layers.push_back(std::move(layer));
    std::cout << "Created layer: " << layerInfo.name << " (visible: " << layerInfo.visible << ")" << std::endl;
  }

  // Legacy support: copy first layer's tiles to the legacy tiles vector
  tiles.assign(static_cast<size_t>(width) * static_cast<size_t>(height), nullptr);
  if (!layers.empty()) {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        auto tile = layers[0]->getTile(x, y);
        if (tile) {
          size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
          tiles[index] = tile;
        }
      }
    }
  }
}

int Map::getWidth() const { return width; }
int Map::getHeight() const { return height; }
int Map::getTileSize() const { return tileSizeW; }

// Layer management
int Map::getLayerCount() const {
  return static_cast<int>(layers.size());
}

Layer* Map::getLayer(int index) const {
  if (index < 0 || index >= static_cast<int>(layers.size()))
    return nullptr;
  return layers[index].get();
}

Layer* Map::getLayer(const std::string &name) const {
  for (const auto &layer : layers) {
    if (layer->getName() == name)
      return layer.get();
  }
  return nullptr;
}

int Map::addLayer(const std::string &name) {
  auto layer = std::make_unique<Layer>(name, width, height, tileSizeW, tileSizeH);
  layers.push_back(std::move(layer));
  return static_cast<int>(layers.size() - 1);
}

void Map::removeLayer(int index) {
  if (index >= 0 && index < static_cast<int>(layers.size())) {
    layers.erase(layers.begin() + index);
  }
}

void Map::setLayerVisible(int index, bool visible) {
  if (auto layer = getLayer(index)) {
    layer->setVisible(visible);
  }
}

void Map::setLayerCollidable(int index, bool collidable) {
  if (auto layer = getLayer(index)) {
    layer->setCollidable(collidable);
  }
}

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
  std::vector<std::shared_ptr<Platform>> result;
  
  // Search all collidable layers
  for (const auto &layer : layers) {
    if (!layer->isCollidable()) continue;
    
    auto layerTiles = layer->getTilesInRect(rect);
    result.insert(result.end(), layerTiles.begin(), layerTiles.end());
  }
  
  return result;
}

std::vector<std::shared_ptr<Platform>> Map::getAllTiles() const {
  std::vector<std::shared_ptr<Platform>> result;
  
  // Collect from all layers
  for (const auto &layer : layers) {
    auto layerTiles = layer->getAllTiles();
    result.insert(result.end(), layerTiles.begin(), layerTiles.end());
  }
  
  return result;
}

void Map::render(SDL_Renderer *renderer) const {
  if (!renderer)
    return;
    
  // Render all visible layers in order
  for (const auto &layer : layers) {
    layer->render(renderer);
  }
}

void Map::renderLayer(SDL_Renderer *renderer, int index) const {
  if (auto layer = getLayer(index)) {
    layer->render(renderer);
  }
}
