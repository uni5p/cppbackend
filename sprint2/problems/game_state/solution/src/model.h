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

    Road(HorizontalTag, Point start, Coord end_x) noexcept;
    Road(VerticalTag, Point start, Coord end_y) noexcept;
    bool IsHorizontal() const noexcept ;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept ;
private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept;
    const Rectangle& GetBounds() const noexcept ;
private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;
    Office(Id id, Point position, Offset offset) noexcept;
    const Id& GetId() const noexcept ;
    Point GetPosition() const noexcept ;
    Offset GetOffset() const noexcept ;
private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Dog {
    public:
        using Id = util::Tagged<int, Dog>;
        Dog(std::string user_name, Id id) noexcept;
        const Id& GetId() const noexcept ;
        const std::string& GetName() const noexcept ;
    
        using Dimension = double;
        using Coord = Dimension;
        struct Pos {
            Coord x{}, y{};
        };         
        struct Speed {
            // Dimension dir_x, dir_y;
            Dimension dir_x{}, dir_y{};
        };    
        enum class Dir {Left, Right, Up, Down}; 
    
        void SetPos(Pos pos);
        Pos GetPos() const;
        Speed GetSpeed() const;
        void SetSpeed(Speed speed);
        char GetDirSymbol() const;
        void SetDir(Dir dir);
    
    private:
        Id id_;
        std::string name_;
        Pos pos_;
        Speed speed_;
        Dir dir_ = Dir::Up;
    };
        
class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept;
    const Id& GetId() const noexcept ;
    const std::string& GetName() const noexcept ;
    const Buildings& GetBuildings() const noexcept ;
    const Roads& GetRoads() const noexcept ;
    const Offices& GetOffices() const noexcept ;
    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);
    Dog::Pos GetRandomPos() const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class GameSession {
public:
    using Dogs = std::vector<const Dog*>;
    GameSession(const Map* map) noexcept;
    void AddDog(Dog* dog);
    const Dogs& GetDogs() const;
private:
    Dogs dogs_;
    const Map* map_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    Game() = default;
    void AddMap(Map map);
    const Maps& GetMaps() const noexcept ;
    const Map* FindMap(const Map::Id& id) const noexcept ;
    // No copy functions.
    Game(const Game&) = delete;
    void operator=(const Game&) = delete;
    Dog* AddDog(std::string name);
    const Dog& GetDog(Dog::Id id) const ;
    GameSession* GetSession(const Map* map);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapToSessions = std::unordered_map<const Map*, std::vector<GameSession>>;
    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    std::vector<std::unique_ptr<Dog>> dogs_;
    MapToSessions sessions_;
};

}  // namespace model
