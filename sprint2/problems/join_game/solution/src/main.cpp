#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"

#include "boost_log.h"   
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
namespace json = boost::json;
namespace logging = boost::log;
// #include <boost/date_time.hpp>
// #include <boost/log/core.hpp>        // для logging::core
// #include <boost/log/expressions.hpp> // для выражения, задающего фильтр

// BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
// BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
// BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

void LogServerStarted(auto port, auto address){
    json::value custom_data{{"port"s, port}, {"address"s, address.to_string()}};
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "server started"sv;
}

void LogExitFailure(auto ex){
    json::value custom_data{
          {"code"s, EXIT_FAILURE}
        , {"exception"s, ex.what()}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "server exited"sv;
}

void LogServerExited(){
    json::value custom_data{{"code"s, 0}};
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "server exited"sv;
}


}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <path-static-files>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        boost_log::InitBoostLogFilter();
        std::string config_file(argv[1]);
        std::string static_content_path(argv[2]);
        // std::string config_file("../../data/config.json");
        // std::string static_content_path("../../static");
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game;
        json_loader::LoadGame(game, config_file);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {ioc.stop();}
        });
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);
        auto handler = std::make_shared<http_handler::RequestHandler>(game, static_content_path, api_strand);
        // http_handler::RequestHandler handler{game, static_content_path, api_strand};
        log_handler::LoggingRequestHandler logging_handler{*handler};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& end_point, auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(end_point)>(end_point), std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        // std::cout << "Server has started..."sv << std::endl;
        LogServerStarted(port, address);

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        // std::cerr << ex.what() << std::endl;
        LogExitFailure(ex); 
        return EXIT_FAILURE;
    }
    LogServerExited();

}
