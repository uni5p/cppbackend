#include "audio.h"
#include <iostream>
#include <boost/asio.hpp>


namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

static const size_t max_buffer_size = 65000;


void StartClient(const uint16_t port){  
    Recorder recorder(ma_format_u8, 1);

    try {
        net::io_context io_context;

        // Перед отправкой данных нужно открыть сокет. 
        // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
        udp::socket socket(io_context, udp::v4());

        boost::system::error_code ec;
        while (true) {
            std::string str;

            std::cout << "Enter the IP address of the server: " << std::endl;
            std::getline(std::cin, str);

            auto endpoint = udp::endpoint(net::ip::make_address(str, ec), port);

            std::cout << "Press Enter to record message..." << std::endl;
            std::getline(std::cin, str);

            auto rec_result = recorder.Record(max_buffer_size, 1.5s);
            std::cout << "Recording done" << std::endl;
            socket.send_to(net::buffer(rec_result.data), endpoint);
        };

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

void StartServer(const uint16_t port){  
    Player player(ma_format_u8, 1);

    try {
        net::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        // boost::system::error_code ec;
        for (;;) {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
            std::array<char, max_buffer_size> recv_buf;
            udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента
            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            std::cout << "The record has been received "sv << std::endl;
// << std::string_view(recv_buf.data(), size) 
            player.PlayBuffer(recv_buf.data(), size/player.GetFrameSize(), 1.5s);
            std::cout << "Playing done" << std::endl;

        }

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {

    if (argc != 3) {
        std::cout << "Usage: "sv << argv[0] << " client or server and port"sv << std::endl;
        return 1;
    }
    const int port = std::stoi(argv[2]);

    if (argv[1] == "client"sv) {
        StartClient(port);
    } else if (argv[1] == "server"sv) {
        StartServer(port);
    } else {
        std::cout << "Unknown program type: "sv << argv[1] << std::endl;
        return 1;
    }


    return 0;
}
