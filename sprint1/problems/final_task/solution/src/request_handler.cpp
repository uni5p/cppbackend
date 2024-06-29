#include "request_handler.h"
#include "json_loader.h"

namespace http_handler {
    

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                bool keep_alive, http::verb method,
                                std::string_view content_type /*= ContentType::TEXT_HTML*/ ) { 
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    if (method != http::verb::head) {
        response.body() = body;
    }
    if (method != http::verb::get && method != http::verb::head) {
        response.set(http::field::allow, "GET, HEAD");
    }
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

std::vector<std::string_view> SplitQueryLine(std::string_view sv, char ch){
    std::vector<std::string_view> res{};
    for(auto pos = sv.find(ch, 0); pos != std::string::npos; pos = sv.find(ch, 0))
    {
        res.push_back(sv.substr(0, pos));
        sv.remove_prefix(pos + 1);
    }
    res.push_back(sv);
    return res;
}

StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
    const auto text_response = [&req](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method());
    };

    std::string body;
    http::verb method = req.method();

    if (method != http::verb::get && method != http::verb::head) {
        body = "Invalid method";
        return text_response(http::status::method_not_allowed, body);
    }
    
    auto target = req.target();
    std::string query(target.substr(1).data(), target.size()-1);
    // Разбираем запрос по словам
    auto query_words = SplitQueryLine(query, '/');  
    query_words.resize(10);

    if (query_words[0] == "api"s){
        if (query_words[1] == "v1"s && query_words[2] == "maps"s){
            if (query_words[3].empty()){ // Если запрос  - список карт
                body = json_loader::GetMapsJson(game_.GetMaps());
            } else { // Если запрос  - описание карты
                std::string map_id = std::string(query_words[3]);
                auto map = game_.FindMap(model::Map::Id{map_id});
                if (map){
                    body = json_loader::GetMapJson(map);
                } else {
                    body = json_loader::GetErrorMes("mapNotFound", "Map not found");
                    return text_response(http::status::not_found, body);
                }
            }
        } else { // если не удается обработать запрос
            body = json_loader::GetErrorMes("badRequest", "Bad request");
            return text_response(http::status::bad_request, body);
        }

    }  
    // body = "<strong>Hello, "+str+"</strong>";
    return text_response(http::status::ok, body);
}    



}  // namespace http_handler
