// Этот файл служит для подключения реализации библиотеки Boost.Json
#include "boost_json.h"

#include <boost/json.hpp>

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


} // namespace boost_json 

