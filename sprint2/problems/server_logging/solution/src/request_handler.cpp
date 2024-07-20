#include "request_handler.h"
#include "json_loader.h"
#include <boost/beast.hpp>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <sstream>
// #include <cctype>
// #include <algorithm>
#include <boost/algorithm/string.hpp>  

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace http_handler {
    

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                bool keep_alive, http::verb method,
                                std::string_view content_type /*= ContentType::TEXT_HTML указал в request_handler.h*/ ) { 
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    // auto test_base = response.base().empty();
    // auto test_content_type = response.base().at(http::field::content_type);
    if (method != http::verb::head) {
        response.body() = body;
    }
    if (method != http::verb::get && method != http::verb::head) {
        response.set(http::field::allow, "GET, HEAD");
    }
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    // std::string(response.base().at(http::field::content_type));
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
    std::string res;
    std::unordered_map<std::string, std::string> content_types = {
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
        , {""s, "application/octet-stream"s}
    };
    // std::transform(extension.begin(), extension.end(), extension.begin(), std::tolower );
    boost::to_lower(extension);
    res = content_types.at(extension.c_str());
    return res;
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


ResponseValue MakeFileResponse(std::string query, const fs::path& static_content_path, const StringRequest& req){
    using namespace http;
    using namespace std::literals;
    // Проверка пути в запросе
    fs::path rel_path = {UriPercentDecoding(query)};
    fs::path abs_path = fs::weakly_canonical(static_content_path / rel_path);
    if (!IsSubPath(abs_path, static_content_path)) {
        // Ответ, что запрос неверный 
        return MakeStringResponse(http::status::bad_request, "Bad request"s, req.version(), req.keep_alive(), req.method(), ContentType::TEXT_PLAIN);
    }
    if (fs::is_directory(abs_path)){ 
        abs_path /= fs::path{"index.html"};
    }
    // Открываем файл
    file_body::value_type file;
    if (sys::error_code ec; file.open(abs_path.c_str(), beast::file_mode::read, ec), ec) {
        // Ответ, что файл не найден
        return MakeStringResponse(http::status::not_found, "File not found"s, req.version(), req.keep_alive(), req.method(), ContentType::TEXT_PLAIN);
    }

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

ResponseValue RequestHandler::HandleRequest(StringRequest&& req) {
    const auto text_response = [&req](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method()); //  ContentType::TEXT_HTML
    };

    std::string body;
    http::verb method = req.method();

    if (method != http::verb::get && method != http::verb::head) {
        body = "Invalid method";
        return {text_response(http::status::method_not_allowed, body)};
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
                    return {text_response(http::status::not_found, body)};
                }
            }
        } else { // если не удается обработать запрос
            body = json_loader::GetErrorMes("badRequest", "Bad request");
            return {text_response(http::status::bad_request, body)};
        }

    } else {
        return MakeFileResponse(query, static_content_path_, req);
    }  
    // body = "<strong>Hello, "+str+"</strong>";
    return {text_response(http::status::ok, body)};
}    

// FileResponse RequestHandler::HandleRequestFile(StringRequest&& req){
//     using namespace http;
//     using namespace std::literals;


//     // Проверка пути в запросе
//     fs::path rel_path = {UriPercentDecoding(query)};
//     fs::path abs_path = fs::weakly_canonical(static_content_path / rel_path);
//     if (rel_path.empty() || !IsSubPath(abs_path, static_content_path)) {
//         // Ответ, что запрос неверный 
//         return MakeStringResponse(http::status::bad_request, "Bad request"s, req.version(), req.keep_alive(), req.method(), ContentType::TEXT_PLAIN);
//     }

//     // Открываем файл
//     file_body::value_type file;
//     if (sys::error_code ec; file.open(abs_path.c_str(), beast::file_mode::read, ec), ec) {
//         // Ответ что файл не найден
//         return MakeStringResponse(http::status::not_found, "File not found"s, req.version(), req.keep_alive(), req.method(), ContentType::TEXT_PLAIN);
//     }

//     response<file_body> res;
//     res.version(11);  // HTTP/1.1
//     res.result(status::ok);
//     res.insert(field::content_type, GetContentType(abs_path.extension().c_str()));
//     res.body() = std::move(file);
//     // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
//     // в зависимости от свойств тела сообщения
//     res.prepare_payload();

//     return res;

// }    


}  // namespace http_handler
