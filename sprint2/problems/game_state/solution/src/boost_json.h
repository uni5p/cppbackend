#pragma once

#include <string>

namespace boost_json {
std::string GetErrorMes(std::string_view code, std::string_view message);

struct JoinRequest {
    std::string user_name;
    std::string map_id;   
};

JoinRequest ParseJoinRequest(const std::string& object);
    
} // namespace boost_json 