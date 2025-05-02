#pragma once
#include "model.h"

namespace app {

    class Player {
    public:
        using Token = util::Tagged<std::string, Player>;
        using Id = model::Dog::Id;
        Player(Token token, model::Dog* dog, model::GameSession* session) noexcept;
        const Token& GetToken() const noexcept;
        model::Dog& GetDog() const noexcept;
        model::GameSession& GetGameSession() const noexcept;
    private:
        Token token_;
        model::Dog* dog_;
        model::GameSession* session_;
    };


    class Players {
    public:
        Players() = default;
        // No copy functions.
        Players(const Players&) = delete;
        void operator=(const Players&) = delete;
        
        
        const Player* AddPlayer(model::GameSession* session, model::Dog* dog);
        Player* FindByToken(const Player::Token& token) const noexcept;
    private:
        using ArrPlayersUPtr = std::vector<std::unique_ptr<Player>>;
        using TokenToPlayer = std::unordered_map<Player::Token, Player*, util::TaggedHasher<Player::Token>>;
        ArrPlayersUPtr players_;
        TokenToPlayer token_to_player_;
    
        Player::Token GenerateNewToken();
    
        static const char ZERO_SYMBOL = '0';
        static const unsigned LENGHT_HEX_NUMBER = 16;
    
    
        std::random_device random_device_;
        // std::random_device random_device2_;
        std::mt19937_64 generator1_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
        std::mt19937_64 generator2_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
    
    };

    namespace list_maps {

        
        struct MapInfo {
            std::string id;
            std::string name;
        };
        
        using Result = std::vector<MapInfo>;

        class UseCase {
        public:
            UseCase(model::Game& game)
            : game_(game){
            }
            
            const list_maps::Result ListMaps() {
                auto maps = game_.GetMaps();
                list_maps::Result res(maps.size());
                size_t i = 0;
                for(const auto& map : maps){
                    res[i].id = *map.GetId();
                    res[i].name = map.GetName();
                    ++i;
                };
                return res;           
            }
        private:
            model::Game& game_;
        };

    } // namespace list_maps

    namespace map_info {

        struct Road {
            model::Point start;
            model::Point end;
        };

        struct Building {
            model::Rectangle bounds;
        };

        struct Office {
            std::string id;
            model::Point position;
            model::Offset offset;
        };

        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        struct Result {
            Result(const model::Map& map);
            std::string name_map;
            std::string id_map;
            Roads roads_;
            Buildings buildings_;        
            Offices offices_;        
        };

        enum class ErrorReason {
            MapNotFound
        };

        class Error : public std::exception {
        public:
            Error(ErrorReason reason);
            ErrorReason reason_;
        };

        class UseCase {
        public:
            UseCase(model::Game& game);
            const map_info::Result GetMapInfo(const std::string_view map_name);
        private:
            model::Game& game_;
        };

    } // namespace map_info

    namespace join_game {

        struct Result {
            Player::Token token;
            Player::Id player_id;
        };

        enum class ErrorReason {
            InvalidName,
            InvalidMap
        };

        class Error : public std::exception {
        public:
            Error(ErrorReason reason);
            ErrorReason reason_;
        };

        class UseCase {
        public:
            UseCase(model::Game& game, Players& players);
            const Result AddPlayer(const std::string& user_name, const std::string& map_id);
        private:
            model::Game& game_;
            Players& players_;
        };

    } // namespace join_game

    namespace game_state {

        struct Dog {
            std::string id;
            model::Dog::Pos pos;
            model::Dog::Speed speed;
            std::string dir;
        };

        using Result = std::vector<Dog>;

        class UseCase {
        public:
            UseCase();
            const Result GetGameSate(const model::GameSession::Dogs& dogs);
        };

    } // namespace game_state

    namespace players_list {

        struct Dog {
            std::string id;
            std::string name;
        };

        using Result = std::vector<Dog>;

        class UseCase {
        public:
            UseCase();
            const Result GetPlayersListForUser(const model::GameSession::Dogs& dogs);
        };

    } // namespace players_list

    class Application {
    public:
        Application(model::Game& game);
        // No copy functions.
        Application(const Application&) = delete;
        void operator=(const Application&) = delete;
    
        Player* FindPlayer(const std::string_view& token) const noexcept ;
        const list_maps::Result ListMaps();
        const join_game::Result AddPlayer(const std::string& user_name, const std::string& map_id);
        const map_info::Result GetMapInfo(const std::string_view map_name);
        const game_state::Result GetGameSate(const std::string_view map_name);
        players_list::Result GetPlayersListForUser(const std::string_view& token);
        void SetDogDirect(const std::string_view& token, const std::string direct);
        void CheckDogDirect(const std::string direct);

    private:
        Players players_;
        model::Game& game_;
        join_game::UseCase join_game_;
        list_maps::UseCase list_maps_;
        map_info::UseCase map_info_;
        game_state::UseCase game_state_;
        players_list::UseCase players_list_;

        const model::GameSession::Dogs& GetDogsGameSessionForToken(const std::string_view& token);
    };
    
    
} // namespace app 
    