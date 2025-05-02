#pragma once
#include "http_server.h"
// #include "model.h"
#include "application.h"
#include <filesystem>
#include <variant>
#include <boost/asio/strand.hpp>
#include <optional>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using std::literals::string_literals::operator""s;
using std::literals::string_view_literals::operator""sv;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;
using ResponseValue = std::variant<StringResponse, FileResponse>;


enum class WaitingMethod {GET_HEAD,POST};
enum class CheckToken {No, Yes};

struct ChekParam {
    WaitingMethod wiating_method; 
    CheckToken check_token;
};

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    constexpr static std::string_view APP_JSON = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

enum class TypeApiRequest {
    Unknown
    , ListMaps
    , GetMapInfo
    , AddPlayer
    , GetListOfPlayersForUser
    , GameState
    , MovePlayers
};

const std::unordered_map<TypeApiRequest, ChekParam> CHECK_LIST_REQUEST{
    {TypeApiRequest::ListMaps,           {WaitingMethod::GET_HEAD, CheckToken::No}}
    , {TypeApiRequest::GetMapInfo,              {WaitingMethod::GET_HEAD, CheckToken::No}}
    , {TypeApiRequest::AddPlayer,               {WaitingMethod::POST,     CheckToken::No}}
    , {TypeApiRequest::GetListOfPlayersForUser, {WaitingMethod::GET_HEAD, CheckToken::Yes}}
    , {TypeApiRequest::GameState, {WaitingMethod::GET_HEAD, CheckToken::Yes}}
    , {TypeApiRequest::MovePlayers, {WaitingMethod::POST, CheckToken::Yes}}
};

struct ResponseParam {
    http::status status = http::status::ok;
    std::string body;
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
    bool keep_alive, http::verb method,
    std::string_view content_type = ContentType::APP_JSON,
    std::string_view allow = ""sv);

class ApiHandler {
public:
    explicit ApiHandler(app::Application& app);
    StringResponse HandleApiRequest(const StringRequest& req);    
    StringResponse ListMaps(const StringRequest& req) const ;
    StringResponse GetMapInfo(std::string_view map_name, const StringRequest& req) const;
    StringResponse RequestAddPlayer(const StringRequest& req);
    StringResponse RequestPlayersListForUser(const StringRequest& req);
    StringResponse GetGameStateForUser(const StringRequest& req);
    StringResponse RequestMovePlayers(const StringRequest& req);
private:
    app::Application& app_;
    TypeApiRequest GetTypeApiRequest(const std::vector<std::string>& query_words);
    std::optional<StringResponse> CheckMethodRequest(const StringRequest& req, WaitingMethod waiting_method);
    std::optional<StringResponse> CheckPlayerToken(const StringRequest& req);
    std::optional<StringResponse>  CheckRequest(const StringRequest& req, TypeApiRequest type_rec);
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler>{
public:
    using Strand = net::strand<net::io_context::executor_type>;
    explicit RequestHandler(app::Application& app, std::filesystem::path path_static_content, Strand api_strand);
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto version = req.version();
        auto keep_alive = req.keep_alive();
        try {
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
        } catch (...) {
            send(ReportServerError(req));
        }
    }

private:
    std::filesystem::path static_content_path_;
    Strand api_strand_;
    ApiHandler api_handler_;
    StringResponse HandleApiRequest(const StringRequest& req);    
    ResponseValue HandleFileRequest(const StringRequest& req);    
    bool IsApiRequest(const StringRequest& req);
    StringResponse ReportServerError(const StringRequest& req);
};

}  // namespace http_handler

