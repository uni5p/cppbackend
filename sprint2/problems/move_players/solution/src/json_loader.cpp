#include "json_loader.h"
#include "tagged.h"
#include "boost_json.h"


#include <filesystem>
#include <fstream>

using namespace std;
using namespace std::literals;

namespace json_loader {

void AddRoads(model::Map& map, const boost_json::ArrayJsonValue& roads){
    for(const auto& road : roads){
        bool horizontal_road = road.ContainsParam("x1");
        if (horizontal_road){
            model::Road road_model(model::Road::HORIZONTAL
            , {road.GetParamAsInt("x0"), road.GetParamAsInt("y0")}
            , road.GetParamAsInt("x1"));
            map.AddRoad(move(road_model));
        } else {
            model::Road road_model(model::Road::VERTICAL
            , {road.GetParamAsInt("x0"), road.GetParamAsInt("y0")}
            , road.GetParamAsInt("y1"));
            map.AddRoad(move(road_model));
        }
    }
}

void AddBuildings(model::Map& map, const boost_json::ArrayJsonValue& buildings){
    for(const auto& building : buildings){
        model::Building building_model(
            { {building.GetParamAsInt("x"), building.GetParamAsInt("y")}
            , {building.GetParamAsInt("w"), building.GetParamAsInt("h")}}
        );
        map.AddBuilding(move(building_model));
    }
}

void AddOffices(model::Map& map, const boost_json::ArrayJsonValue& offices){
    using IdOffice = util::Tagged<std::string, model::Office>;
    for(const auto& office : offices){
        std::string id =  office.GetParamAsString("id");
        model::Office office_model(IdOffice(id)
            , {office.GetParamAsInt("x"), office.GetParamAsInt("y")}
            , {office.GetParamAsInt("offsetX"), office.GetParamAsInt("offsetY")}
        );
        map.AddOffice(move(office_model));
    }
}

void LoadGame(model::Game& game,const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    using IdMap = util::Tagged<std::string, model::Map>;

    const auto json_obj = boost_json::ParseFile(json_path);
    const auto jmaps = json_obj.GetParamAsArray("maps"s);
    // Устанавливаем скорость собак по умолчаниюю для всех карт
    if(json_obj.ContainsParam("dogSpeed"s)){ //если есть
        game.SetDefaultDogSpeed(json_obj.GetParamAsDouble("defaultDogSpeed"s));
    } else { // если нет - записываем скорость по умолчанию 1.0
        game.SetDefaultDogSpeed(1.0);
    };
    
    for(const auto& jmap : jmaps){
        std::string id =  jmap.GetParamAsString("id"s);
        std::string name =  jmap.GetParamAsString("name"s);
        
        model::Map map(IdMap(id), name);

        AddRoads(map, jmap.GetParamAsArray("roads"s));
        AddBuildings(map, jmap.GetParamAsArray("buildings"s));
        AddOffices(map, jmap.GetParamAsArray("offices"s));
        // Если для карты есть скорость записываем ее в модель
        if(jmap.ContainsParam("dogSpeed"s)){
            map.SetDogSpeed(jmap.GetParamAsDouble("dogSpeed"s));
        } else { // если нет - записываем скорость по умолчанию для всех карт
            map.SetDogSpeed(game.GetDefaultDogSpeed());
        };

        game.AddMap(move(map));
    }
}

}  // namespace json_loader


/*
// перед переделкой на boost_json.h 2025-04-29 12:10
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


*/
