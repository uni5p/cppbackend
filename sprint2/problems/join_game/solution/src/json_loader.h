#pragma once

#include <filesystem>

#include "model.h"

namespace json_loader {

// model::Game LoadGame(const std::filesystem::path& json_path);
void LoadGame(model::Game& game,const std::filesystem::path& json_path);

std::string GetMapsJson(const std::vector<model::Map>& maps);

std::string GetMapJson(const model::Map* map);

// std::string GetErrorMes(std::string_view code, std::string_view message);




}  // namespace json_loader
