#include "../include/map.h"
#include "../include/config.h"
#include "collision_system.h"
#include <algorithm>
#include <cmath>
#include <iostream>

Map::Map(int width, int height, int tileSizeW, int tileSizeH,
         const std::string &tmxFilePath)
    : width(width), height(height), tileSizeW(tileSizeW), tileSizeH(tileSizeH),
      tmxParser(tmxFilePath), audioManager(nullptr) {
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
        auto texture =
            std::make_shared<Texture>(renderer, tileset.imagePath.c_str());
        tilesetTextures.push_back(texture);
        std::cout << "Loaded tileset texture: " << tileset.imagePath
                  << std::endl;
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
    auto layer = std::make_unique<Layer>(layerInfo.name, width, height,
                                         tileSizeW, tileSizeH);
    layer->loadFromTMXLayer(layerInfo, tilesetInfo, tilesetTextures);
    // Make background layer non-collidable
    // Set background name from the config file

    if (layer->getName() == BACK_GROUND) {
      layer->setCollidable(false);
    }

    // handle trophies layer
    if (layer->getName() == COINS_LAYER_NAME) {
      auto preCoins = layer->getAllTiles();
      for (auto pc : preCoins) {
        // Handle each coin tile
        SDL_FRect bounds = pc->getCollisionBounds();
        auto coin = std::make_shared<Projectile>(
            bounds, Projectile::ProjectileType::COIN, pc->getTexture());
        // Don't take ownership of the platform's sprite (it is owned by the
        // platform via a unique_ptr). Instead create a new Sprite that uses
        // the same Texture and copy the source rect. This avoids double-free
        // and keeps platform rendering intact.
        if (pc->getTexture()) {
          auto spriteShared = std::make_shared<Sprite>(pc->getTexture().get());
          if (pc->getSprite()) {
            spriteShared->setSrcRect(pc->getSprite()->getSrcRect());
          }
          spriteShared->setDestRect(bounds);
          coin->setSprite(spriteShared);
        }
        // Set audio manager for coin sound
        coin->setAudioManager(audioManager);

        // Add coin to map's projectile list so it will be updated and rendered
        // with the rest of projectiles.
        projectiles.push_back(coin);
      }

      std::cout << "Created layer: " << layerInfo.name
                << " (visible: " << layerInfo.visible << ")" << std::endl;
      continue;
    }

    // handle disappearing platforms layer
    if (layer->getName() == DISAPPEAR_LAYER_NAME) {
      auto disappearTiles = layer->getAllTiles();
      for (auto tile : disappearTiles) {
        SDL_FRect bounds = tile->getCollisionBounds();
        auto disappearPlatform =
            std::make_shared<DisappearingPlatform>(bounds, tile->getTexture());

        // Copy sprite properties
        if (tile->getSprite()) {
          disappearPlatform->getSprite()->setSrcRect(
              tile->getSprite()->getSrcRect());
          disappearPlatform->getSprite()->setDestRect(bounds);
        }

        disappearingPlatforms.push_back(disappearPlatform);

        // Replace the regular platform with the disappearing one in the layer
        layer->clearTiles();
        // Note: We'll manage disappearing platforms separately
      }

      std::cout << "Created disappearing layer: " << layerInfo.name << " ("
                << disappearTiles.size() << " platforms)" << std::endl;
    }

    // handle trap platforms layer
    if (layer->getName() == TRAPS_LAYER_NAME) {
      auto trapTiles = layer->getAllTiles();
      for (auto tile : trapTiles) {
        SDL_FRect bounds = tile->getCollisionBounds();
        auto trapPlatform =
            std::make_shared<TrapPlatform>(bounds, tile->getTexture());

        // Copy sprite properties
        if (tile->getSprite()) {
          trapPlatform->getSprite()->setSrcRect(
              tile->getSprite()->getSrcRect());
          trapPlatform->getSprite()->setDestRect(bounds);
        }

        // Replace the regular platform with the trap platform in the layer
        // We need to find the tile position and replace it
        for (int y = 0; y < height; ++y) {
          for (int x = 0; x < width; ++x) {
            auto layerTile = layer->getTile(x, y);
            if (layerTile == tile) {
              layer->setTile(x, y,
                             std::static_pointer_cast<Platform>(trapPlatform));
              break;
            }
          }
        }
      }

      std::cout << "Created trap layer: " << layerInfo.name << " ("
                << trapTiles.size() << " traps)" << std::endl;
    }

    // handle arrow projectiles layer
    if (layer->getName() == ARROW_LAYER_NAME) {
      auto arrowTiles = layer->getAllTiles();
      for (auto tile : arrowTiles) {
        SDL_FRect bounds = tile->getCollisionBounds();

        // Create smaller arrow bounds
        SDL_FRect arrowBounds = {bounds.x + (bounds.w - ARROW_WIDTH) / 2,
                                 bounds.y + (bounds.h - ARROW_HEIGHT) / 2,
                                 ARROW_WIDTH, ARROW_HEIGHT};

        auto arrow = std::make_shared<Projectile>(
            arrowBounds, Projectile::ProjectileType::ARROW, tile->getTexture());

        // Store original position for respawning
        arrow->setOriginalPosition(arrowBounds.x, arrowBounds.y);

        // Copy sprite properties
        if (tile->getTexture()) {
          auto spriteShared =
              std::make_shared<Sprite>(tile->getTexture().get());
          if (tile->getSprite()) {
            spriteShared->setSrcRect(tile->getSprite()->getSrcRect());
          }
          spriteShared->setDestRect(arrowBounds);
          arrow->setSprite(spriteShared);
        }

        // Determine arrow direction based on tile position
        // Arrows on the left side of map move right, right side move left
        // Arrows on top move down, bottom move up
        float velocityX = 0.0f;
        float velocityY = 0.0f;

        // Get tile position in grid
        int tileX = static_cast<int>(bounds.x / DEFAULT_TILE_WIDTH);
        int tileY = static_cast<int>(bounds.y / DEFAULT_TILE_HEIGHT);

        // Determine direction based on position
        if (tileX < width / 2) {
          // Left side of map - arrow moves right
          velocityX = ARROW_SPEED;
        } else {
          // Right side of map - arrow moves left
          velocityX = -ARROW_SPEED;
        }

        // Optional: Add vertical movement for top/bottom tiles
        if (tileY < height / 4) {
          // Top area - also move down
          velocityY = ARROW_SPEED * 0.5f;
        } else if (tileY > height * 3 / 4) {
          // Bottom area - also move up
          velocityY = -ARROW_SPEED * 0.5f;
        }

        arrow->setVelocity(velocityX, velocityY);
        // Add the sound effect
        if (audioManager) {
          arrow->setAudioManager(audioManager);
        }
        projectiles.push_back(arrow);
      }

      std::cout << "Created arrow layer: " << layerInfo.name << " ("
                << arrowTiles.size() << " arrows)" << std::endl;

      // Make layer non-collidable since arrows are handled as projectiles
      layer->setCollidable(false);
    }

    std::cout << "Created layer: " << layerInfo.name
              << " (visible: " << layerInfo.visible << ")" << std::endl;
    layers.push_back(std::move(layer));
  }

  // Legacy support: copy first layer's tiles to the legacy tiles vector
  tiles.assign(static_cast<size_t>(width) * static_cast<size_t>(height),
               nullptr);
  if (!layers.empty()) {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        auto tile = layers[0]->getTile(x, y);
        if (tile) {
          size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) +
                         static_cast<size_t>(x);
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
int Map::getLayerCount() const { return static_cast<int>(layers.size()); }

Layer *Map::getLayer(int index) const {
  if (index < 0 || index >= static_cast<int>(layers.size()))
    return nullptr;
  return layers[index].get();
}

Layer *Map::getLayer(const std::string &name) const {
  for (const auto &layer : layers) {
    if (layer->getName() == name)
      return layer.get();
  }
  return nullptr;
}

int Map::addLayer(const std::string &name) {
  auto layer =
      std::make_unique<Layer>(name, width, height, tileSizeW, tileSizeH);
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
    if (!layer->isCollidable())
      continue;

    auto layerTiles = layer->getTilesInRect(rect);
    result.insert(result.end(), layerTiles.begin(), layerTiles.end());
  }

  // Add disappearing platforms that can collide and intersect with rect
  for (const auto &platform : disappearingPlatforms) {
    if (!platform->canCollide()) {
      continue; // Skip platforms that are disappeared
    }

    SDL_FRect platformBounds = platform->getCollisionBounds();
    if (platformBounds.x < rect.x + rect.w &&
        platformBounds.x + platformBounds.w > rect.x &&
        platformBounds.y < rect.y + rect.h &&
        platformBounds.y + platformBounds.h > rect.y) {
      result.push_back(std::static_pointer_cast<Platform>(platform));
    }
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

void Map::render(SDL_Renderer *renderer, float dt) const {
  if (!renderer)
    return;

  // Render all visible layers in order
  for (const auto &layer : layers) {
    layer->render(renderer);
  }

  // render disappearing platforms (only if visible)
  for (const auto &platform : disappearingPlatforms) {
    if (platform->isVisible() && platform->getSprite()) {
      platform->getSprite()->render(renderer);
    }
  }

  // render projectiles
  for (const auto &project : projectiles) {
    project->render(renderer);
  }
}

void Map::updateProjectiles(float dt) {
  SDL_Rect worldBounds = {0, 0, width * tileSizeW, height * tileSizeH};
  for (auto &project : projectiles) {
    project->update(dt, worldBounds);
  }
}

void Map::removeDeadProjectiles() {
  auto it = std::remove_if(projectiles.begin(), projectiles.end(),
                           [](const std::shared_ptr<Projectile> &proj) {
                             return proj->shouldBeRemoved();
                           });
  projectiles.erase(it, projectiles.end());
}

void Map::renderLayer(SDL_Renderer *renderer, int index) const {
  if (auto layer = getLayer(index)) {
    layer->render(renderer);
  }
}

void Map::updateDisappearingPlatforms(float dt) {
  for (auto &platform : disappearingPlatforms) {
    platform->update(dt);
  }
}

void Map::removeDisappearedPlatforms() {
  // No longer remove platforms permanently - they reappear after a delay
  // This method is kept for API compatibility but does nothing
}

bool Map::isPlayerOnSlowLayer(const SDL_FRect &playerBounds) const {
  Layer *slowLayer = getLayer(SLOW_LAYER_NAME);
  if (!slowLayer || !slowLayer->isCollidable()) {
    return false;
  }

  auto tilesInRect = slowLayer->getTilesInRect(playerBounds);
  return !tilesInRect.empty();
}

bool Map::isPlayerOnTrapLayer(const SDL_FRect &playerBounds) const {
  Layer *trapLayer = getLayer(TRAPS_LAYER_NAME);
  if (!trapLayer) {
    return false;
  }

  // Fetch potentially intersecting tiles based on grid
  auto tilesInRect = trapLayer->getTilesInRect(playerBounds);
  if (tilesInRect.empty())
    return false;

  // Only count as on a trap if the player's collision bounds
  // truly intersect the (possibly reduced) trap collision bounds.
  for (const auto &tile : tilesInRect) {
    if (!tile)
      continue;
    if (tile->getPlatformType() != PlatformType::TRAP)
      continue;
    const SDL_FRect trapBounds =
        tile->getCollisionBounds(); // reduced bounds for TrapPlatform
    if (CollisionSystem::checkAABB(playerBounds, trapBounds)) {
      return true;
    }
  }
  return false;
}
