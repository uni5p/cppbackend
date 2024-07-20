#pragma once
#include "http_server.h"
#include "model.h"
#include <filesystem>
#include <variant>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;
using ResponseValue = std::variant<StringResponse, FileResponse>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    constexpr static std::string_view APP_JSON = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                bool keep_alive, http::verb method,
                                std::string_view content_type = ContentType::APP_JSON);


class RequestHandler {
public:

    explicit RequestHandler(model::Game& game, std::filesystem::path path_static_content)
        : game_{game} 
        , static_content_path_{std::move(path_static_content)}{
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto response = HandleRequest(std::move(req));
        if (std::holds_alternative<StringResponse>(response)) {
            send(std::get<StringResponse>(response));
        } else {
            send(std::get<FileResponse>(response));
        }
    }

private:
    model::Game& game_;
    std::filesystem::path static_content_path_;
    ResponseValue HandleRequest(StringRequest&& req);    
};

}  // namespace http_handler
