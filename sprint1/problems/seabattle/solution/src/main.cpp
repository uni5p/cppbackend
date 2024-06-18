#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>
#include <cassert>

// ./seabattle 1111 3333
// ./seabattle 2222 127.0.0.1 3333


namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;
using Move = std::optional<std::pair<int, int>>;


void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        PrintFields();
        while (!IsGameEnded()){
            if(my_initiative){
                auto move = InputCoordinates(std::cin);
                if(!move){
                    continue;
                };
                SendMove(move, socket);
                auto shot_res = ReadResult(socket);
                MarkShotResult(shot_res, move);
                my_initiative = shot_res != SeabattleField::ShotResult::MISS;
                PrintFields();
                ShowMessageResult(shot_res);
            } else {
                std::cout << "Waiting for turn..."sv << std::endl;
                auto move = ReadMove(socket);
                auto shot_res = my_field_.Shoot((*move).second, (*move).first);
                SendResult(shot_res, socket);
                my_initiative = shot_res == SeabattleField::ShotResult::MISS;
                PrintFields();
                std::cout << "Shot to "sv << MoveToString(*move) << std::endl;
            };
        };
        if(my_field_.IsLoser()){
            std::cout << "You've lost!"sv << std::endl;
        } else {
            std::cout << "You've won!"sv << std::endl;
        };
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    // TODO: добавьте методы по вашему желанию
    
    static Move InputCoordinates(std::istream& input){
        Move res;
        std::cout << "Your turn:  "sv;

        std::string str_move;
        std::getline(input, str_move);

        res = ParseMove(str_move);
        if(!res){
            std::cout << "Incorrect coordinates!!!  "sv << std::endl;
            return std::nullopt;
        };
        return res;

    }

    bool SendMove(Move move, tcp::socket& socket){
        if (!WriteExact(socket, MoveToString(*move))) {
            std::cout << "Error sending data"sv << std::endl;
            return false;
        }
        return true;
    }

    SeabattleField::ShotResult ReadResult(tcp::socket& socket){
        auto res = ReadExact<1>(socket);
        if (!res){
            std::cout << "Error!!! No answer"sv << std::endl;
            assert(false);
        };
        return {static_cast<SeabattleField::ShotResult>((*res)[0])};
    }

    void ShowMessageResult(SeabattleField::ShotResult shot_res){
        switch (shot_res) {
            case SeabattleField::ShotResult::MISS :
                std::cout << "Miss!"sv << std::endl;
                break;
            case SeabattleField::ShotResult::KILL :
                std::cout << "Kill!"sv << std::endl;
                break;
            case SeabattleField::ShotResult::HIT :
                std::cout << "Hit!"sv << std::endl;
                break;
            default:
                assert(false);
                // throw std::runtime_error("Invalid eabattleField::ShotResult shot_res"s);
        }
    }

    void MarkShotResult(SeabattleField::ShotResult shot_res, Move move){
        size_t x = (*move).second;
        size_t y = (*move).first;
        switch (shot_res) {
            case SeabattleField::ShotResult::MISS :
                other_field_.MarkMiss(x, y);
                break;
            case SeabattleField::ShotResult::KILL :
                other_field_.MarkKill(x, y);
                break;
            case SeabattleField::ShotResult::HIT :
                other_field_.MarkHit(x, y);
                break;
            default:
                assert(false);
                // throw std::runtime_error("Invalid eabattleField::ShotResult shot_res"s);
        }
    }

    Move ReadMove(tcp::socket& socket){
        auto res = ReadExact<2>(socket);
        if (!res){
            std::cout << "Error!!! No answer"sv << std::endl;
            assert(false);
            // return  std::nullopt;
        };
        return ParseMove(*res);
    }

    bool SendResult(SeabattleField::ShotResult shot_res, tcp::socket& socket){
        std::string str(""s + static_cast<char>(shot_res));
        if (!WriteExact(socket, str)) {
            std::cout << "Error sending data"sv << std::endl;
            return false;
        }
        return true;
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);

    if (ec) {
        std::cout << "Can't accept connection"sv << std::endl;
        return;
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    // Создадим endpoint - объект с информацией об адресе и порте.
    // Для разбора IP-адреса пользуемся функцией net::ip::make_address.
    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        return;
    }


    // int main(...)
    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);

    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
        return;
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}


