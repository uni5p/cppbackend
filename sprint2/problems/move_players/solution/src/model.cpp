#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
: start_{start}
, end_{end_x, start.y} {
}

Road::Road(VerticalTag, Point start, Coord end_y) noexcept
: start_{start}
, end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

Building::Building(Rectangle bounds) noexcept
: bounds_{bounds} {
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

Office::Office(Id id, Point position, Offset offset) noexcept
: id_{std::move(id)}
, position_{position}
, offset_{offset} {
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

Point Office::GetPosition() const noexcept {
    return position_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}


Map::Map(Id id, std::string name) noexcept
: id_(std::move(id))
, name_(std::move(name)) {
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}


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

Dog::Pos Map::GetRandomPos() const {
    auto roads = GetRoads();
    if(roads.empty()){
        return {0.0, 0.0};
    }
    std::random_device random_device_;
    std::uniform_int_distribution<int> dist_roads(0, roads.size()-1);
    int random_road = dist_roads(random_device_);
    auto road = roads[random_road];
    auto start = road.GetStart();
    auto end = road.GetEnd();
    if(road.IsHorizontal()){
        std::uniform_real_distribution<Dog::Coord> dist_x(start.x, end.x);
        return {dist_x(random_device_), static_cast<Dog::Coord>(start.y)};
    } else {
        std::uniform_real_distribution<Dog::Coord> dist_y(start.y, end.y);
        return {static_cast<Dog::Coord>(start.x), dist_y(random_device_)};
    };
}

void Map::SetDogSpeed(Dog::Dimension dog_speed){
    dog_speed_ = dog_speed;
}

Dog::Dimension Map::GetDogSpeed() const {
    return dog_speed_;
}

Dog::Dog(std::string user_name, Id id) noexcept
: name_{std::move(user_name)} 
    , id_{std::move(id)} {
}

const Dog::Id& Dog::GetId() const noexcept {
    return id_;
}

const std::string& Dog::GetName() const noexcept {
    return name_;
}

void Dog::SetPos(Pos pos){
    pos_ = pos;
}  

Dog::Pos Dog::GetPos() const {
    return pos_;
}  

Dog::Speed Dog::GetSpeed() const {
    return speed_;
}  

void Dog::SetSpeed(Speed speed){
    speed_ = speed;
}

void Dog::SetSpeed(Dir dir, Dimension dog_speed){
    dog_speed *= dir==Dir::Left || dir==Dir::Down ? -1.0 : 1.0;
    speed_ = dir==Dir::Left || dir==Dir::Right ? Speed{dog_speed, 0.0} : Speed{0.0, dog_speed};
}


char Dog::GetDirSymbol() const {
    switch (dir_)
    {
    case Dir::Left:
                    return 'L';
    case Dir::Right:
                    return 'R';
    case Dir::Up:
                    return 'U';
    case Dir::Down:
                    return 'D';
    default:
        throw std::out_of_range("Unknown direction of movement"); 
    } 
}  

void Dog::SetDir(Dir dir, Dimension dog_speed){
    if(dir == dir_){
        return;
    }

    if(dir==Dir::None){
        SetSpeed(Speed{0.0f, 0.0f});
    } else {
        dir_ = dir;
        SetSpeed(dir, dog_speed);
    }
}



GameSession::GameSession(const Map* map) noexcept
: map_(map) {
}

void GameSession::AddDog(Dog* dog){
    dogs_.push_back(dog);
    dog->SetPos(map_->GetRandomPos());
    dog->SetSpeed({0.0, 0.0});
    dog->SetDir(Dog::Dir::Up, 0.0);

}

const GameSession::Dogs& GameSession::GetDogs() const {
    return dogs_;
}

const Map& GameSession::GetMap() const{
    return *map_;
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

GameSession* Game::GetSession(const Map* map){
    auto sessions_map = &sessions_[map];
    if(sessions_map->empty()){ // если пока нет сессий для этой карты
        sessions_map->emplace_back(map);
    }
    return &(*sessions_map)[0]; // возвращаем "нулевую" сессию для карты, пока она одна 
}

void Game::SetDefaultDogSpeed(Dog::Dimension dog_speed) {
    default_dog_speed_ = dog_speed;
}

Dog::Dimension Game::GetDefaultDogSpeed() const {
    return default_dog_speed_;
}


const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

Dog* Game::AddDog(std::string name){
    Dog::Id id(dogs_.size());
    dogs_.push_back(std::make_unique<Dog>(name, id));
    return dogs_.back().get();
}


const Dog& Game::GetDog(Dog::Id id) const {
    if(*id < dogs_.size()){
        return *dogs_[*id];
    }
    throw;
}

}  // namespace model
