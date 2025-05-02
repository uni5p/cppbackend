#pragma once

#include <filesystem>
#include <string>
#include <boost/json.hpp>
#include "application.h"


namespace boost_json {
std::string GetErrorMes(std::string_view code, std::string_view message);

struct JoinRequest {
    std::string user_name;
    std::string map_id;   
};

JoinRequest ParseJoinRequest(const std::string& object);

std::string GetMapsJson(const app::list_maps::Result& maps);
std::string GetMapJson(const app::map_info::Result& map);
std::string GetGameSateJsonBody(const app::game_state::Result& dogs);
std::string GetPlayerJsonBody(const app::join_game::Result& player_data);
std::string GetPlayersJsonBody(const app::players_list::Result& dogs);

class JsonValue;
using ArrayJsonValue = std::vector<JsonValue>;

class JsonValue {
public:
JsonValue(const boost::json::value& value);

const ArrayJsonValue GetParamAsArray(const std::string& name_array) const;
std::string GetParamAsString(const std::string& name_param) const;
int GetParamAsInt(const std::string& name_param) const;
double GetParamAsDouble(const std::string& name_param) const; 
bool ContainsParam(const std::string& name_param) const;

void SetParentPtr(std::shared_ptr<boost::json::value> ptr);

private:
const boost::json::value& value_;
std::shared_ptr<boost::json::value> parent_ptr_;
};

JsonValue ParseFile(const std::filesystem::path& json_path);
JsonValue ParseStr(const std::string& str_json);


    
} // namespace boost_json 


/*

class JsonValue {
public:
JsonValue(const boost::json::value& value, JsonValue parent);
JsonValue(const std::shared_ptr<boost::json::value> parent_ptr);

const ArrayJsonValue GetParamAsArray(std::string name_array) const;
std::string GetParamAsString(std::string name_param) const;
int GetParamAsInt(std::string name_param) const;
bool ContainsParam(std::string name_param) const;

private:
std::shared_ptr<boost::json::value> GetParetnPtr() const ;

const boost::json::value& value_;
std::shared_ptr<boost::json::value> parent_ptr_;
};

*/