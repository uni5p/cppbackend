#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <sstream>
#include <memory>
#include <iomanip>

#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};


class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Dog {
public:
    using Id = util::Tagged<int, Dog>;
    Dog(std::string user_name, Id id) noexcept
        : user_name_{std::move(user_name)} 
        , id_{std::move(id)} {
    }
    const Id& GetId() const noexcept {
        return id_;
    }
    const std::string* GetUserName() const noexcept {
        return &user_name_;
    }
private:
    Id id_;
    std::string user_name_;
};

class GameSession {
public:
    GameSession(const Map* map) noexcept
        : map_(map) {
    }
    void AddDog(const Dog* dog){
        dogs_.push_back(dog);
    }
private:
    using Dogs = std::vector<const Dog*>;
    Dogs dogs_;
    const Map* map_;
};

class Player {
public:
    using Token = util::Tagged<std::string, Player>;
    Player(Token token, const Dog* dog, GameSession* session) noexcept
        : token_(std::move(token)) 
        , dog_(dog) 
        , session_(session) 
    {}
    const Token& GetToken() const noexcept{
        return token_;
    }
    const Dog& GetDog() const noexcept{
        return *dog_;
    }
private:
    Token token_;
    const Dog* dog_;
    GameSession* session_;
};

class Players {
public:
    using ArrPlayers = std::vector<Player>;
    Players() = default;
    const Player* AddPlayer(GameSession* session, const Dog* dog){
        Player::Token token = GetToken();
        players_.emplace_back(Player(token, dog, session));
        // players_.emplace(token, Player(token, dog, session));
        // players_[token] = Player(token, dog, session);
        auto player_ptr = &players_.back();
        token_to_player_[token] = player_ptr;
        return player_ptr;
    }
    // No copy functions.
    Players(const Players&) = delete;
    void operator=(const Players&) = delete;


    const Player* FindPlayer(const Player::Token& token) const noexcept {
        if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const ArrPlayers& GetPlayers(){
        return players_;
    }
private:
    using TokenToPlayer = std::unordered_map<Player::Token, Player*, util::TaggedHasher<Player::Token>>;
    ArrPlayers players_;
    TokenToPlayer token_to_player_;

    Player::Token GetToken(){
        std::stringstream stream;
        // токен это строка из 32 символов полученная из двух 
        // 64-битных псевдо-случайных чисел в hex-представлении
        // переведенных в строки по 16 символов с ведущими нулями
        stream << std::hex << std::right << std::setfill(ZERO_SYMBOL) 
            << std::setw(LENGHT_HEX_NUMBER) << generator1_() 
            << std::setw(LENGHT_HEX_NUMBER) << generator2_();
        return Player::Token(stream.str());     
    }

    static const char ZERO_SYMBOL = '0';
    static const unsigned LENGHT_HEX_NUMBER = 16;


    std::random_device random_device_;
    // std::random_device random_device2_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

};

class Game {
public:
    using Maps = std::vector<Map>;
    Game(){};

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }
    
    const Player* AddPlayer(const std::string& user_name, const Map* map);
// model::Player::Token Game::AddPlayer(const std::string& user_name, const Map* map){
    const Player* FindPlayer(const std::string& token) const noexcept {
        return players_.FindPlayer(model::Player::Token(std::string(token)));
    }
    

    // No copy functions.
    Game(const Game&) = delete;
    void operator=(const Game&) = delete;

    const Players::ArrPlayers& GetPlayers(){
        return players_.GetPlayers();
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapToSessions = std::unordered_map<const Map*, std::vector<GameSession>>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<std::unique_ptr<Dog>> dogs_;
    // std::vector<GameSession> sessions_;
    MapToSessions sessions_;
    Players players_;
    GameSession* GetSession(const Map* map);



    const Dog* AddDog(std::string user_name){
        Dog::Id id(dogs_.size());
        dogs_.push_back(std::make_unique<Dog>(user_name, id));
        return &GetDog(id);
    }


    const Dog& GetDog(Dog::Id id) const {
        if(*id < dogs_.size()){
            return *dogs_[*id];
        }
        throw;
    }




};

}  // namespace model
