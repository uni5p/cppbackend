#pragma once
#include "http_server.h"
#include "model.h"
#include <filesystem>
#include <variant>
#include <boost/asio/strand.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
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
                                std::string_view content_type = ContentType::APP_JSON,
                                std::string_view allow = ""sv);

struct ResponseParam {
    http::status status = http::status::ok;
    std::string body;
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler>{
public:
    using Strand = net::strand<net::io_context::executor_type>;
    explicit RequestHandler(model::Game& game, std::filesystem::path path_static_content, Strand api_strand)
        : game_{game} 
        , static_content_path_{std::move(path_static_content)}
        , api_strand_{api_strand}{
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto version = req.version();
        auto keep_alive = req.keep_alive();
        if(IsApiRequest(req)){
            auto handle = [self = shared_from_this(), send,
                            req = std::forward<decltype(req)>(req), version, keep_alive] {
                try {
                    // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                    assert(self->api_strand_.running_in_this_thread());
                    return send(self->HandleApiRequest(req));
                } catch (...) {
                    send(self->ReportServerError(req));
                }
            };
            return net::dispatch(api_strand_, handle);
        } else {
            auto response = HandleFileRequest(std::move(req));
            if (std::holds_alternative<StringResponse>(response)) {
                send(std::get<StringResponse>(response));
            } else {
                send(std::get<FileResponse>(response));
            }
        }
        // auto response = HandleRequest(std::move(req));
        // if (std::holds_alternative<StringResponse>(response)) {
        //     send(std::get<StringResponse>(response));
        // } else {
        //     send(std::get<FileResponse>(response));
        // }
    }

private:
    model::Game& game_;
    std::filesystem::path static_content_path_;
    Strand api_strand_;
    // ResponseValue HandleRequest(StringRequest&& req);    
    StringResponse HandleApiRequest(const StringRequest& req);    
    ResponseValue HandleFileRequest(const StringRequest& req);    
    StringResponse GetMapInfo(std::string_view map_name, const StringRequest& req) const;
    StringResponse GetGameInfo(std::string_view command, const StringRequest& req);
    StringResponse RequestAddPlayer(const StringRequest& req);
    StringResponse RequestPlayersList(const StringRequest& req);
    bool IsApiRequest(const StringRequest& req);
    StringResponse ReportServerError(const StringRequest& req);
};

}  // namespace http_handler

/*
class RequestHandler2 : public std::enable_shared_from_this<RequestHandler2> {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    RequestHandler2(fs::path root, Strand api_strand, )  // прочие параметры 
        : root_{std::move(root)}
        , api_strand_{api_strand}
          { // инициализируем остальные поля
    } 

    RequestHandler2(const RequestHandler2&) = delete;
    RequestHandler2& operator=(const RequestHandler2&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(tcp::endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        try {
            if () {  //  req относится к API?
                auto handle = [self = shared_from_this(), send,
                               req = std::forward<decltype(req)>(req), version, keep_alive] {
                    try {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(self->api_strand_.running_in_this_thread());
                        return send(self->HandleApiRequest(req));
                    } catch (...) {
                        send(self->ReportServerError(version, keep_alive));
                    }
                };
                return net::dispatch(api_strand_, handle);
            }
            // Возвращаем результат обработки запроса к файлу
            return std::visit(
                [&send](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                HandleFileRequest(req));
        } catch (...) {
            send(ReportServerError(version, keep_alive));
        }
    }

private:
    using FileRequestResult = std::variant<EmptyResponse, StringResponse, FileResponse>;

    FileRequestResult HandleFileRequest(const StringRequest& req) const;
    StringResponse HandleApiRequest(const StringRequest& request) const;
    StringResponse ReportServerError(unsigned version, bool keep_alive) const;

    fs::path root_;
    Strand api_strand_;
    
};

*/