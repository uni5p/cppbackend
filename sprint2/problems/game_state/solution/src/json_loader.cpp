#include "json_loader.h"
#include "tagged.h"
// #include "boost_json.cpp"
// #include "boost_json.h"


#include <boost/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

namespace json_loader {

int GetValue(const boost::json::value& value, const string& key ){
    return static_cast<int>(value.as_object().at(key).as_int64());
}

void AddRoads(model::Map& map, boost::json::array& roads){
    for(const auto& road : roads){
        bool horizontal_road = road.as_object().contains("x1");
        if (horizontal_road){
            model::Road road_model(model::Road::HORIZONTAL
            , {GetValue(road, "x0"), GetValue(road, "y0")}
            , GetValue(road, "x1"));
            map.AddRoad(move(road_model));
        } else {
            model::Road road_model(model::Road::VERTICAL
            , {GetValue(road, "x0"), GetValue(road, "y0")}
            , GetValue(road, "y1"));
            map.AddRoad(move(road_model));
        }
    }
}


void AddBuildings(model::Map& map, boost::json::array& buildings){
    for(const auto& building : buildings){
        model::Building building_model(
            { {GetValue(building, "x"), GetValue(building, "y")}
            , {GetValue(building, "w"), GetValue(building, "h")}}
        );
        map.AddBuilding(move(building_model));
    }
}

void AddOffices(model::Map& map, boost::json::array& offices){
    using IdOffice = util::Tagged<std::string, model::Office>;
    for(const auto& office : offices){
        std::string id =  office.at("id").get_string().c_str();
        model::Office office_model(IdOffice(id)
            , {GetValue(office, "x"), GetValue(office, "y")}
            , {GetValue(office, "offsetX"), GetValue(office, "offsetY")}
        );
        map.AddOffice(move(office_model));
    }
}

void LoadGame(model::Game& game,const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    using IdMap = util::Tagged<std::string, model::Map>;

    std::ifstream in_file(json_path);
    if (!in_file){
        throw std::runtime_error("Can't open config file: "s + json_path.c_str());
    };
    std::ostringstream sstr;
    sstr << in_file.rdbuf();

    boost::json::value j = boost::json::parse(sstr.str());
    auto jmaps = j.as_object().at("maps").as_array();
    for(auto& jmap : jmaps){
        std::string id =  jmap.at("id").get_string().c_str();
        std::string name =  jmap.at("name").get_string().c_str();
        
        model::Map map(IdMap(id), name);

        AddRoads(map, jmap.as_object().at("roads").as_array());
        AddBuildings(map, jmap.as_object().at("buildings").as_array());
        AddOffices(map, jmap.as_object().at("offices").as_array());

        game.AddMap(move(map));
    }
}

// std::string custom_serialize(const boost::json::value& jv) {
//     std::ostringstream oss;
//     oss << std::fixed << std::setprecision(2); // Устанавливаем точность для double
//     boost::json::serializer sr(jv);
    
//     char buffer[4096];
//     while (!sr.done()) {
//         auto result = sr.read(buffer);
//         oss.write(buffer, result.size());
//     }

//     return oss.str();
// }

std::string GetMapsJson(const app::list_maps::Result& maps){
    boost::json::array arr;
    for(const auto&map : maps){
        boost::json::object obj;
        obj["id"] = map.id;
        obj["name"] = map.name;
        arr.emplace_back(std::move(obj));
    }
    return serialize(arr);
}

boost::json::array GetRoadsArr(const app::map_info::Roads & roads){
    boost::json::array res; 
    for(const auto& road : roads){
        boost::json::object obj;
        obj["x0"] = road.start.x;
        obj["y0"] = road.start.y;
        if (road.start.y == road.end.y){
            obj["x1"] = road.end.x;
        } else {
            obj["y1"] = road.end.y;
        }
        res.emplace_back(std::move(obj));
    }
    return res;
}

boost::json::array GetBuildingsArr(const app::map_info::Buildings & buildings){ 
    boost::json::array res; 
    for(const auto& building : buildings){
        boost::json::object obj;
        auto bounds = building.bounds;
        obj["x"] = bounds.position.x;
        obj["y"] = bounds.position.y;
        obj["w"] = bounds.size.width;
        obj["h"] = bounds.size.height;
        res.emplace_back(std::move(obj));
    }
    return res;
}

boost::json::array GetOfficesArr(const app::map_info::Offices & offices){ 
    boost::json::array res; 
    for(const auto& office : offices){
        boost::json::object obj;
        auto position = office.position;
        auto offset = office.offset;
        obj["id"] = office.id;
        obj["x"] = position.x;
        obj["y"] = position.y;
        obj["offsetX"] = offset.dx;
        obj["offsetY"] = offset.dy;
        res.emplace_back(std::move(obj));
    }
    return res;
}

std::string GetMapJson(const app::map_info::Result& map){
    boost::json::object obj;
    obj["id"] = map.id_map;
    obj["name"] = map.name_map;
    obj["roads"] = GetRoadsArr(map.roads_);
    obj["buildings"] = GetBuildingsArr(map.buildings_);
    obj["offices"] = GetOfficesArr(map.offices_);

    return serialize(obj);
}

std::string GetGameSateJsonBody(const app::game_state::Result& dogs){
    boost::json::object res; 
    boost::json::object players;

    for(const auto& dog : dogs){
        boost::json::object player;
        player["pos"] = {dog.pos.x, dog.pos.y};
        player["speed"] = {dog.speed.dir_x, dog.speed.dir_y};
        player["dir"] = dog.dir ;
        players[dog.id] = player;
    }
    res["players"] = players;
    return serialize(res);
}


}  // namespace json_loader
