#include "json_loader.h"
#include "tagged.h"
// #include "boost_json.cpp"
// #include "boost_json.h"


#include <boost/json/src.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

namespace json_loader {

int GetValue(const boost::json::value& value, const string& key ){
    return static_cast<int>(value.as_object().at(key).as_int64());
}

void AddRoads(model::Map& map, boost::json::array& jroads){
    for(auto& jroad : jroads){
        bool horizontal_road = jroad.as_object().contains("x1");
        if(horizontal_road){
            model::Road road(model::Road::HORIZONTAL
            , {GetValue(jroad, "x0"), GetValue(jroad, "y0")}
            , GetValue(jroad, "x1"));
            map.AddRoad(move(road));
        } else {
            model::Road road(model::Road::VERTICAL
            , {GetValue(jroad, "x0"), GetValue(jroad, "y0")}
            , GetValue(jroad, "y1"));
            map.AddRoad(move(road));
        };
    };
}


void AddBuildings(model::Map& map, boost::json::array& jbuildings){
    for(auto& jbuilding : jbuildings){
        model::Building building(
            { {GetValue(jbuilding, "x"), GetValue(jbuilding, "y")}
            , {GetValue(jbuilding, "w"), GetValue(jbuilding, "h")}}
        );
        map.AddBuilding(move(building));
    };
        
}

void AddOffices(model::Map& map, boost::json::array& joffices){
    using IdOffice = util::Tagged<std::string, model::Office>;
    for(auto& joffice : joffices){
        std::string id =  joffice.at("id").get_string().c_str();
        model::Office office(IdOffice(id)
            , {GetValue(joffice, "x"), GetValue(joffice, "y")}
            , {GetValue(joffice, "offsetX"), GetValue(joffice, "offsetY")}
        );
        map.AddOffice(move(office));
    };
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    using IdMap = util::Tagged<std::string, model::Map>;

    model::Game game;

    std::ifstream in_file(json_path);
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
    };  


    return game;
}

std::string GetMapsJson(const std::vector<model::Map>& maps){
    boost::json::array arr;
    for(const auto&map : maps){
        boost::json::object obj;
        obj["id"] = *map.GetId();
        obj["name"] = map.GetName();
        arr.emplace_back(std::move(obj));
    };
    return serialize(arr);
}

boost::json::array GetRoadsArr(const model::Map::Roads & roads){
    boost::json::array res; 
    for(const auto& road : roads){
        boost::json::object obj;
        auto start = road.GetStart();
        auto end = road.GetEnd();
        obj["x0"] = start.x;
        obj["y0"] = start.y;
        if(road.IsHorizontal()){
            obj["x1"] = end.x;
        } else {
            obj["y1"] = end.y;
        };
        res.emplace_back(std::move(obj));
    };
    return res;
}

boost::json::array GetBuildingsArr(const model::Map::Buildings & buildings){ 
    boost::json::array res; 
    for(const auto& building : buildings){
        boost::json::object obj;
        auto bounds = building.GetBounds();
        obj["x"] = bounds.position.x;
        obj["y"] = bounds.position.y;
        obj["w"] = bounds.size.width;
        obj["h"] = bounds.size.height;
        res.emplace_back(std::move(obj));
    };
    return res;
}

boost::json::array GetOfficesArr(const model::Map::Offices & offices){ 
    boost::json::array res; 
    for(const auto& office : offices){
        boost::json::object obj;
        auto position = office.GetPosition();
        auto offset = office.GetOffset();
        obj["id"] = *office.GetId();
        obj["x"] = position.x;
        obj["y"] = position.y;
        obj["offsetX"] = offset.dx;
        obj["offsetY"] = offset.dy;
        res.emplace_back(std::move(obj));
    };
    return res;
}

std::string GetMapJson(const model::Map *map){
    boost::json::object obj;
    obj["id"] = *map->GetId();
    obj["name"] = map->GetName();
    obj["roads"] = GetRoadsArr(map->GetRoads());
    obj["buildings"] = GetBuildingsArr(map->GetBuildings());
    obj["offices"] = GetOfficesArr(map->GetOffices());

    return serialize(obj);
}

std::string GetErrorMes(const std::string& code, const std::string& message){
    boost::json::object obj;
    obj["code"] = code;
    obj["message"] = message;
    return serialize(obj);   
}

}  // namespace json_loader
