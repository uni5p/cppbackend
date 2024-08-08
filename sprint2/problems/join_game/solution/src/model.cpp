#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

// model::Player::Token Game::AddPlayer(const std::string& user_name, const Map* map){
const Player* Game::AddPlayer(const std::string& user_name, const Map* map) {
    const Dog* dog = AddDog(user_name); // Создаем собаку для игрока
    auto session = GetSession(map); // находим сессию для карты, которую запросил игрок
    session->AddDog(dog); // добавляем собаку в сессию игры
    return players_.AddPlayer(session, dog); // создаем игрока
}

GameSession* Game::GetSession(const Map* map){
    auto sessions_map = &sessions_[map];
    if(sessions_map->empty()){ // если пока нет сессий для этой карты
        sessions_map->emplace_back(map);
    }
    return &(*sessions_map)[0]; // возвращаем "нулевую" сессию для карты, пока она одна 
}


}  // namespace model
