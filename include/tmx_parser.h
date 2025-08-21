#pragma once

#include <memory>
#include <string>
#include <vector>

namespace tinyxml2 {
class XMLDocument;
class XMLElement;
} // namespace tinyxml2

class TMXParser {
public:
  struct MapInfo {
    int mapWidth;
    int mapHeight;
    int tileWidth;
    int tileHeight;
    std::string orientation;
    std::string renderOrder;
  };

  struct TilesetInfo {
    int firstGid;
    int tilesWidth;
    int tilesHeight;
    int tileCount;
    int columns;
    int rows;
    int imageWidth;
    int imageHeight;
    std::string imagePath;
  };

  struct Layer {
    int id;
    std::string name;
    int width;
    int height;
    bool visible;
    float opacity;
    std::vector<int> data;
  };

  // Constructor
  explicit TMXParser(const std::string &tmxFilePath);

  // Destructor
  ~TMXParser();

  // Load and parse the TMX file
  void loadFile();

  // Get parsed information
  MapInfo getMapInfo() const;
  std::vector<TilesetInfo> getTilesetInfo() const;
  std::vector<Layer> getLayersInfo() const;

private:
  std::string tmxFilePath;
  std::unique_ptr<tinyxml2::XMLDocument> doc;
  tinyxml2::XMLElement *mapElement;

  // Helper to trim whitespace
  std::string trim(const std::string &str) const;
};
