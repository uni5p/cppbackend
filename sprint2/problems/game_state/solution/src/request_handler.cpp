#include "request_handler.h"
#include "json_loader.h"
#include "boost_json.h"
#include <boost/beast.hpp>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <sstream>
#include <boost/algorithm/string.hpp> 
#include <optional>
#include <boost/json.hpp>

using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace http_handler {


struct ChekParam {
    WaitingMethod wiating_method; 
    CheckToken check_token;
};

const std::unordered_map<TypeApiRequest, ChekParam> CHECK_LIST_REQUEST{
    {TypeApiRequest::ListMaps,           {WaitingMethod::GET_HEAD, CheckToken::No}},
    {TypeApiRequest::GetMapInfo,              {WaitingMethod::GET_HEAD, CheckToken::No}},
    {TypeApiRequest::AddPlayer,               {WaitingMethod::POST,     CheckToken::No}},
    {TypeApiRequest::GetListOfPlayersForUser, {WaitingMethod::GET_HEAD, CheckToken::Yes}},
    {TypeApiRequest::GameState, {WaitingMethod::GET_HEAD, CheckToken::Yes}}
};


const std::unordered_map<std::string, std::string> CONTENT_TYPES_FOR_FILES{
      {".htm"s, "text/html"s}
    , {".html"s, "text/html"s}
    , {".css"s, "text/css"s}
    , {".txt"s, "text/plain"s}
    , {".js"s, "text/javascript"s}
    , {".json"s, "application/json"s}
    , {".xml"s, "application/xml"s}
    , {".png"s, "image/png"s}
    , {".jpg"s, "image/jpeg"s}
    , {".jpe"s, "image/jpeg"s}
    , {".jpeg"s, "image/jpeg"s}
    , {".gif"s, "image/gif"s}
    , {".bmp"s, "image/bmp"s}
    , {".ico"s, "image/vnd.microsoft.icon"s}
    , {".tif"s, "image/tiff"s}
    , {".tiff"s, "image/tiff"s}
    , {".svg"s, "image/svg+xml"s}
    , {".svgz"s, "image/svg+xml"s}
    , {".mp3"s, "audio/mpeg"s}
    , {"."s, "application/octet-stream"s}
    , {"octet-stream"s, "application/octet-stream"s}
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                bool keep_alive, http::verb method,
                                std::string_view content_type,
                                std::string_view allow) { 
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    // auto test_base = response.base().empty();
    // auto test_content_type = response.base().at(http::field::content_type);
    if (method != http::verb::head) {
        response.body() = body;
    }
    if (!allow.empty()) {
        response.set(http::field::allow, allow);
    }
    // if (method != http::verb::get && method != http::verb::head) {
    //     response.set(http::field::allow, "GET, HEAD");
    // }
    response.set(http::field::cache_control, "no-cache"s);
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    // std::string(response.base().at(http::field::content_type));
    return response;
}

StringResponse ErrorResponseJson(http::status status, std::string_view code, std::string_view message
                                 , const StringRequest& req){
    auto body = boost_json::GetErrorMes(code, message);
    return MakeStringResponse(status, body, req.version(), req.keep_alive(), req.method()); 
}

StringResponse ErrorResponseJson(http::status status, const StringRequest& req){
    if(status == http::status::bad_request){
        return ErrorResponseJson(status, "badRequest", "Bad request", req);
    } else {
        throw;
    }
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

// Возвращает true, если каталог path содержится внутри base_path.
bool IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

// Определяет тип контента по расширению передаваемого файла
std::string GetContentType(std::string extension){
    boost::to_lower(extension);
    if(CONTENT_TYPES_FOR_FILES.count(extension.c_str()) == 0){
        return CONTENT_TYPES_FOR_FILES.at("octet-stream");
    }
    return CONTENT_TYPES_FOR_FILES.at(extension.c_str());
}


bool fromHex(std::string_view hexValue, char& result){
    std::stringstream ss;
    ss << std::hex << hexValue;
    ss >> result;
    return !ss.fail();
}

// Возвращает пустую строку если перевод чисел из Hex походит с ошибками
std::string UriPercentDecoding(std::string_view sv){
    std::string res;
    res.reserve(sv.size());
    char prc = '%';
    char ch;
    for (auto pos = sv.find(prc, 0); pos != std::string::npos; pos = sv.find(prc, 0)) {
        if (pos+3>sv.size() || !fromHex(sv.substr(pos+1, 2), ch)) {
            return {}; // если не смогли перевести число из Hex
        }
        res.append(sv.substr(0, pos));
        res.push_back(ch);
        sv.remove_prefix(pos + 3);
    }
    res.append(sv);
    return res;
}

fs::path GetAbsolutePath(const fs::path& static_content_path, const StringRequest& req){
    auto target = req.target();
    std::string query(target.substr(1).data(), target.size()-1);
    fs::path rel_path = {UriPercentDecoding(query)};
    return fs::weakly_canonical(static_content_path / rel_path);
}

ApiHandler::ApiHandler(model::Game& game, app::Application& app)
: game_{game}
, app_{app}{
}

std::string GetPlayerJsonBody(const app::join_game::Result& player_data){
    boost::json::object obj;
    obj["authToken"] = *player_data.token;
    obj["playerId"] = *player_data.player_id;
    return serialize(obj);
}

std::string GetPlayersJsonBody(const model::GameSession::Dogs& dogs){
    boost::json::object obj;
    for(const auto& dog : dogs){
        boost::json::object player_obj;
        player_obj["name"] = (*dog).GetName();
        auto dog_id = *((*dog).GetId());
        std::string dog_id_str = std::to_string(dog_id);
        obj[dog_id_str.c_str()] = player_obj;
    }
    return serialize(obj);
}

std::vector<std::string> GetQueryWords(const StringRequest& req){
    auto target = req.target();
    std::string query(target.substr(1).data(), target.size()-1);
    // Разбираем запрос по словам
    auto query_words = SplitQueryLine(query, '/');  
    return std::vector<std::string>(query_words.begin(), query_words.end());
}

TypeApiRequest ApiHandler::GetTypeApiRequest(std::vector<std::string>& query_words){
    if (query_words.size()>2 && query_words[0] == "api"sv && query_words[1] == "v1"sv) {
        query_words.resize(query_words.size()+1); // чтобы не проверять размер вектора на каждом элементе 
        if (query_words[2] == "maps"s){  //если запрос по картам
            if (query_words[3].empty()) {
                return TypeApiRequest::ListMaps;
            } else {
                return TypeApiRequest::GetMapInfo;
            };
        } else if (query_words[2] == "game"sv) { // если запросы по игре
            if (query_words[3] == "join"sv) {
                return TypeApiRequest::AddPlayer;
            } else if (query_words[3] == "players"sv) {
                return TypeApiRequest::GetListOfPlayersForUser;
            } else if (query_words[3] == "state"sv) {
                return TypeApiRequest::GameState;
            };
        }
    }
    return TypeApiRequest::Unknown;
};

std::optional<StringResponse> ApiHandler::CheckMethodRequest(const StringRequest& req, WaitingMethod waiting_method){
    std::string human_mes;
    std::string allow;
    http::verb method = req.method();
    if (waiting_method == WaitingMethod::GET_HEAD && method != http::verb::get && method != http::verb::head) {
        // return {text_response(http::status::method_not_allowed, "Invalid method")};
        human_mes = "Only GET and HEAD methods are expected";
        allow = "GET, HEAD"s;
    } else if (waiting_method == WaitingMethod::POST && method != http::verb::post ) {
        human_mes = "Only POST method is expected";
        allow = "POST"s;
    } else {
        return {};
    } 
    auto body = boost_json::GetErrorMes("invalidMethod", human_mes);
    return MakeStringResponse(http::status::method_not_allowed, body
        , req.version(), req.keep_alive(), req.method()
        , ContentType::APP_JSON, allow);
}

std::optional<StringResponse> ApiHandler::CheckPlayerToken(const StringRequest& req){
    if(req.count(http::field::authorization)==0 || req.at(http::field::authorization).size() != 39){
        return ErrorResponseJson(http::status::unauthorized, "invalidToken","Authorization header is missing", req);
    }

    std::string token = std::string(req.at(http::field::authorization).substr(7));
    auto player = app_.FindPlayer(token);

    if(!player){
        return ErrorResponseJson(http::status::unauthorized, "unknownToken","Player token has not been found", req);
    }
    return {};                                     
}

std::optional<StringResponse>  ApiHandler::CheckRequest(const StringRequest& req, TypeApiRequest type_rec){
    auto error_message = CheckMethodRequest(req, CHECK_LIST_REQUEST.at(type_rec).wiating_method);      
    if(error_message) {
        return error_message;  
    }           
    if(CHECK_LIST_REQUEST.at(type_rec).check_token == CheckToken::Yes){
        error_message = CheckPlayerToken(req);      
        if(error_message) {
            return error_message;  
        }            
    }                               
    return {};                                     
}

StringResponse ApiHandler::ListMaps(const StringRequest& req) const{
    ResponseParam res;
    res.body = json_loader::GetMapsJson(app_.ListMaps());
    return MakeStringResponse(res.status, res.body
        , req.version(), req.keep_alive(), req.method(), ContentType::APP_JSON);
}

StringResponse ApiHandler::GetMapInfo(std::string_view map_name, const StringRequest& req) const{
    ResponseParam res;
    try {
        res.body = json_loader::GetMapJson(app_.GetMapInfo(map_name));
    } catch (const app::map_info::Error&) {
        res.body = boost_json::GetErrorMes("mapNotFound", "Map not found");
        res.status = http::status::not_found;
    }

    return MakeStringResponse(res.status, res.body
        , req.version(), req.keep_alive(), req.method(), ContentType::APP_JSON);
}

StringResponse ApiHandler::RequestAddPlayer(const StringRequest& req) {
    // CHECK_METHOD_REQUEST(WaitingMethod::POST)
    boost_json::JoinRequest req_param;
    try{
        req_param = boost_json::ParseJoinRequest(req.body());
    } catch (...) {  //Если при парсинге JSON или получении его свойств произошла ошибка:
        return ErrorResponseJson(http::status::bad_request, "invalidArgument","Join game request parse error", req);
    }
    std::string body;
    try{
        body = GetPlayerJsonBody(app_.AddPlayer(req_param.user_name, req_param.map_id));
    } catch (const app::join_game::Error& e) {  //Если при добавлении игрока произошла ошибка:
        switch (e.reason_) {
            case app::join_game::ErrorReason::InvalidMap: 
               return ErrorResponseJson(http::status::not_found, "mapNotFound","Map not found", req); break;
            case app::join_game::ErrorReason::InvalidName:
               return ErrorResponseJson(http::status::bad_request, "invalidArgument","Invalid name", req);break;
        }
    }
    return MakeStringResponse(http::status::ok, body
        , req.version(), req.keep_alive(), req.method(), ContentType::APP_JSON);
}

StringResponse ApiHandler::RequestPlayersListForUser(const StringRequest& req) {
    // Получаем список всех собак в сессии этого игрока
    std::string token = std::string(req.at(http::field::authorization).substr(7));
    auto player = app_.FindPlayer(token);
    auto dogs = player->GetGameSession().GetDogs();
    return MakeStringResponse(http::status::ok, GetPlayersJsonBody(dogs)
        , req.version(), req.keep_alive(), req.method(), ContentType::APP_JSON);
}

StringResponse ApiHandler::GetGameStateForUser(const StringRequest& req){
    std::string token = std::string(req.at(http::field::authorization).substr(7));
    auto body = json_loader::GetGameSateJsonBody(app_.GetGameSate(token));

    return MakeStringResponse(http::status::ok, body
    , req.version(), req.keep_alive(), req.method(), ContentType::APP_JSON);
}


StringResponse ApiHandler::HandleApiRequest(const StringRequest& req) {
    std::vector<std::string> query_words = GetQueryWords(req);
    // Определяем тип запроса
    auto type_req = GetTypeApiRequest(query_words);
    // Выполняем проверки корректности запроса
    auto error_message = CheckRequest(req, type_req);      
    if(error_message) {
        // Передаем сообщение об ошибке если запрос некорректен
        return *error_message;  
    };                                         
    switch (type_req){ // в зависимости от типа запроса вызываем функцию формирования ответа
    case TypeApiRequest::ListMaps:
        return ListMaps(req);
    case TypeApiRequest::GetMapInfo:
        return GetMapInfo(query_words[3], req);
    case TypeApiRequest::AddPlayer:
        return RequestAddPlayer(req);
    case TypeApiRequest::GetListOfPlayersForUser:
        return RequestPlayersListForUser(req);
    case TypeApiRequest::GameState:
        return GetGameStateForUser(req);
    };
    return ErrorResponseJson(http::status::bad_request, req);
}  




ResponseValue RequestHandler::HandleFileRequest(const StringRequest& req){    
    fs::path abs_path = GetAbsolutePath(static_content_path_, req);
    if (!IsSubPath(abs_path, static_content_path_)) {
        // Ответ, что запрос неверный 
        return MakeStringResponse(http::status::bad_request, "Bad request"s, req.version(), req.keep_alive(), req.method(), ContentType::TEXT_PLAIN);
    }
    if (fs::is_directory(abs_path)){ 
        abs_path /= fs::path{"index.html"};
    }
    // Открываем файл
    http::file_body::value_type file;
    if (sys::error_code ec; file.open(abs_path.c_str(), beast::file_mode::read, ec), ec) {
        // Ответ, что файл не найден
        return MakeStringResponse(http::status::not_found, "File not found"s, req.version(), req.keep_alive(), req.method(), ContentType::TEXT_PLAIN);
    }

    using namespace http;
    response<file_body> res;
    res.version(11);  // HTTP/1.1
    res.result(status::ok);
    res.insert(field::content_type, GetContentType(abs_path.extension().c_str()));
    res.body() = std::move(file);
    // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
    // в зависимости от свойств тела сообщения
    res.prepare_payload();
    return res;
}

RequestHandler::RequestHandler(model::Game& game, app::Application& app, std::filesystem::path path_static_content, Strand api_strand)
: game_{game} 
, static_content_path_{std::move(path_static_content)}
, api_strand_{api_strand}
, api_handler_{game, app}{
}

StringResponse RequestHandler::HandleApiRequest(const StringRequest& req) {
    return api_handler_.HandleApiRequest(req);
}    

bool RequestHandler::IsApiRequest(const StringRequest& req){
    auto target = req.target();
    std::string query(target.substr(1).data(), target.size()-1);
    return query.substr(0,3) == "api"s;
}

StringResponse RequestHandler::ReportServerError(const StringRequest& req){
    return ErrorResponseJson(http::status::bad_request, req);
}
}  // namespace http_handler
