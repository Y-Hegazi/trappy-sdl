#include "../include/tmx_parser.h"
#include "tinyxml2.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

using namespace tinyxml2;

TMXParser::TMXParser(const std::string &tmxFilePath)
    : tmxFilePath(tmxFilePath), doc(std::make_unique<XMLDocument>()),
      mapElement(nullptr) {}

TMXParser::~TMXParser() = default;

void TMXParser::loadFile() {
  XMLError error = doc->LoadFile(tmxFilePath.c_str());
  if (error != XML_SUCCESS) {
    throw std::runtime_error(
        "Failed to load TMX file: " + tmxFilePath +
        " (Error: " + std::to_string(static_cast<int>(error)) + ")");
  }

  mapElement = doc->RootElement();
  if (!mapElement || std::string(mapElement->Name()) != "map") {
    throw std::runtime_error(
        "Invalid TMX file: missing or invalid map element");
  }
}

TMXParser::MapInfo TMXParser::getMapInfo() const {
  if (!mapElement) {
    throw std::runtime_error("TMX file not loaded. Call loadFile() first.");
  }

  MapInfo info;
  info.mapWidth = mapElement->IntAttribute("width");
  info.mapHeight = mapElement->IntAttribute("height");
  info.tileWidth = mapElement->IntAttribute("tilewidth");
  info.tileHeight = mapElement->IntAttribute("tileheight");

  const char *orientation = mapElement->Attribute("orientation");
  info.orientation = orientation ? orientation : "orthogonal";

  const char *renderOrder = mapElement->Attribute("renderorder");
  info.renderOrder = renderOrder ? renderOrder : "right-down";

  return info;
}

std::vector<TMXParser::TilesetInfo> TMXParser::getTilesetInfo() const {
  if (!mapElement) {
    throw std::runtime_error("TMX file not loaded. Call loadFile() first.");
  }

  std::vector<TilesetInfo> infos;
  XMLElement *tilesetElem = mapElement->FirstChildElement("tileset");

  while (tilesetElem) {
    TilesetInfo info{};
    info.firstGid = tilesetElem->IntAttribute("firstgid");
    info.tilesWidth = tilesetElem->IntAttribute("tilewidth");
    info.tilesHeight = tilesetElem->IntAttribute("tileheight");
    info.tileCount = tilesetElem->IntAttribute("tilecount");
    info.columns = tilesetElem->IntAttribute("columns");

    // Calculate rows from tilecount and columns
    if (info.columns > 0) {
      info.rows = (info.tileCount + info.columns - 1) /
                  info.columns; // ceiling division
    } else {
      info.rows = 0;
    }

    // Get image information from the image child element
    XMLElement *imageElem = tilesetElem->FirstChildElement("image");
    if (imageElem) {
      info.imageWidth = imageElem->IntAttribute("width");
      info.imageHeight = imageElem->IntAttribute("height");

      const char *source = imageElem->Attribute("source");
      info.imagePath = source ? source : "";
    } else {
      info.imageWidth = 0;
      info.imageHeight = 0;
      info.imagePath = "";
    }

    infos.push_back(info);
    tilesetElem = tilesetElem->NextSiblingElement("tileset");
  }

  return infos;
}

std::vector<TMXParser::Layer> TMXParser::getLayersInfo() const {
  if (!mapElement) {
    throw std::runtime_error("TMX file not loaded. Call loadFile() first.");
  }

  std::vector<Layer> layers;
  XMLElement *layerElem = mapElement->FirstChildElement("layer");

  while (layerElem) {
    Layer layer{};
    layer.id = layerElem->IntAttribute("id");

    const char *name = layerElem->Attribute("name");
    layer.name = name ? name : "";

    layer.width = layerElem->IntAttribute("width");
    layer.height = layerElem->IntAttribute("height");
    layer.visible = layerElem->BoolAttribute("visible", true);
    layer.opacity = layerElem->FloatAttribute("opacity", 1.0f);

    // Parse layer data
    XMLElement *dataElem = layerElem->FirstChildElement("data");
    if (dataElem) {
      const char *encoding = dataElem->Attribute("encoding");

      if (encoding && std::string(encoding) == "csv") {
        // Parse CSV data
        const char *csvText = dataElem->GetText();
        if (csvText) {
          std::string csvData = csvText;
          std::istringstream ss(csvData);
          std::string token;

          while (std::getline(ss, token, ',')) {
            std::string trimmedToken = trim(token);
            if (!trimmedToken.empty()) {
              try {
                int gid = std::stoi(trimmedToken);
                layer.data.push_back(gid);
              } catch (const std::exception &) {
                // Skip invalid tokens
                layer.data.push_back(0);
              }
            }
          }
        }
      } else {
        // Other encodings (base64, etc.) not implemented
        throw std::runtime_error("Unsupported data encoding: " +
                                 std::string(encoding ? encoding : "unknown"));
      }
    }

    layers.push_back(layer);
    layerElem = layerElem->NextSiblingElement("layer");
  }

  return layers;
}

std::string TMXParser::trim(const std::string &str) const {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }

  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}
