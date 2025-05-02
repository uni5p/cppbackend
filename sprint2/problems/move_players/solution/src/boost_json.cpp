// Этот файл служит для подключения реализации библиотеки Boost.Json
#include "boost_json.h"

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std::literals;

namespace boost_json {

std::string GetErrorMes(std::string_view code, std::string_view message){
    boost::json::object obj{
          {"code", code}
        , {"message", message}
    };
    return serialize(obj);   
}

JoinRequest ParseJoinRequest(const std::string& object){
    boost::json::value j = boost::json::parse(object);
    JoinRequest res;
    res.user_name = j.as_object().at("userName").get_string().c_str();
    res.map_id = j.as_object().at("mapId").get_string().c_str();
    return res;
}    

// JoinRequest ParsePlaerActionRequest(const std::string& object){
//     boost::json::value j = boost::json::parse(object);
//     JoinRequest res;
//     res.user_name = j.as_object().at("userName").get_string().c_str();
//     res.map_id = j.as_object().at("mapId").get_string().c_str();
//     return res;
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

std::string GetPlayerJsonBody(const app::join_game::Result& player_data){
    boost::json::object obj;
    obj["authToken"] = *player_data.token;
    obj["playerId"] = *player_data.player_id;
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

std::string GetPlayersJsonBody(const app::players_list::Result& dogs){
    boost::json::object obj;
    for(const auto& dog : dogs){
        boost::json::object player_obj;
        player_obj["name"] = dog.name;
        obj[dog.id] = player_obj;
    }
    return serialize(obj);
}



JsonValue::JsonValue(const boost::json::value& value)
: value_(value){
}

const std::vector<JsonValue> JsonValue::GetParamAsArray(const std::string& name_array) const {
    auto& json_array = value_.as_object().at(name_array).as_array();
    return {json_array.begin(), json_array.end()};
}

std::string JsonValue::GetParamAsString(const std::string& name_param) const {
    return value_.at(name_param).get_string().c_str();
}

int JsonValue::GetParamAsInt(const std::string& name_param) const {
    return static_cast<int>(value_.as_object().at(name_param).as_int64());
}

double JsonValue::GetParamAsDouble(const std::string& name_param) const {
    return static_cast<double>(value_.as_object().at(name_param).as_double());
}

bool JsonValue::ContainsParam(const std::string& name_param) const{
    return value_.as_object().contains(name_param);
}

void JsonValue::SetParentPtr(std::shared_ptr<boost::json::value> ptr){
    parent_ptr_ = ptr;
}



JsonValue ParseFile(const std::filesystem::path& json_path){
    std::ifstream in_file(json_path);
    if (!in_file){
        throw std::runtime_error("Can't open config file: "s + json_path.c_str());
    };
    std::ostringstream sstr;
    sstr << in_file.rdbuf();
    return ParseStr(sstr.str());
}

JsonValue ParseStr(const std::string& str_json){
    //Создаем в куче дерево Json, чтобы потом не него могли ссылаться объекты JsonValue
    auto val_shptr = std::make_shared<boost::json::value>(boost::json::parse(str_json)); 
    JsonValue res{*val_shptr};
    // Сохраняем shared ссылку на дерево Json в первом объекте JsonValue, чтобы продлить 
    // срок жизни дерева до завершения обработки информации
    res.SetParentPtr(val_shptr);
    return res;
}



} // namespace boost_json 


/*

    // auto j = std::shared_ptr<boost::json::value>(new boost::json::value(boost::json::parse(sstr.str())));


JsonValue::JsonValue(const boost::json::value& value, JsonValue parent)
: value_(value)
, parent_ptr_(parent.GetParetnPtr()){
}

JsonValue::JsonValue(const std::shared_ptr<boost::json::value> parent_ptr)
: value_(*parent_ptr)
, parent_ptr_(parent_ptr){
}


std::shared_ptr<boost::json::value> JsonValue::GetParetnPtr() const {
    return parent_ptr_;
}

*/

