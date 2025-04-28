#pragma once

#include <filesystem>

#include "model.h"
#include "application.h"

namespace json_loader {

// model::Game LoadGame(const std::filesystem::path& json_path);
void LoadGame(model::Game& game,const std::filesystem::path& json_path);
std::string GetMapsJson(const app::list_maps::Result& maps);
std::string GetMapJson(const app::map_info::Result& map);
std::string GetGameSateJsonBody(const app::game_state::Result& dogs);

// std::string GetErrorMes(std::string_view code, std::string_view message);




}  // namespace json_loader
