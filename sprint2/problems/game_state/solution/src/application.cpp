#include "application.h"


namespace app {            
    
    
    Player::Player(Token token, const model::Dog* dog, model::GameSession* session) noexcept
        : token_(std::move(token)) 
        , dog_(dog) 
        , session_(session) 
    {}

    const Player::Token& Player::GetToken() const noexcept{
        return token_;
    }

    const model::Dog& Player::GetDog() const noexcept{
        return *dog_;
    }

    model::GameSession& Player::GetGameSession() const noexcept{
        return *session_;
    }

    const Player* Players::AddPlayer(model::GameSession* session, const model::Dog* dog){
        Player::Token token = GenerateNewToken();
        players_.push_back(std::make_unique<Player>(token, dog, session));
        auto player_ptr = players_.back().get();
        token_to_player_[token] = player_ptr;
        return player_ptr;
    }

    Player* Players::FindByToken(const Player::Token& token) const noexcept {
        if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    Player::Token Players::GenerateNewToken(){
        std::stringstream stream;
        // токен это строка из 32 символов полученная из двух 
        // 64-битных псевдо-случайных чисел в hex-представлении
        // переведенных в строки по 16 символов с ведущими нулями
        stream << std::hex << std::right << std::setfill(ZERO_SYMBOL) 
            << std::setw(LENGHT_HEX_NUMBER) << generator1_() 
            << std::setw(LENGHT_HEX_NUMBER) << generator2_();
        return Player::Token(stream.str());     
    }

    namespace map_info {
        Error::Error(ErrorReason reason)
        : reason_(reason){
        }

        Result::Result(const model::Map& map)
        : id_map(*map.GetId())
        , name_map(map.GetName()){
            const auto buildings = map.GetBuildings();
            size_t buildings_count = buildings.size();
            buildings_.resize(buildings_count);
            for(size_t i=0; i<buildings_count; ++i){
                buildings_[i].bounds = buildings[i].GetBounds();
            }

            const auto roads = map.GetRoads();
            size_t roads_count = roads.size();
            roads_.resize(roads_count);
            for(size_t i=0; i<roads_count; ++i){
                roads_[i].start = roads[i].GetStart();
                roads_[i].end = roads[i].GetEnd();
            }

            const auto offices = map.GetOffices();
            size_t offices_count = offices.size();
            offices_.resize(offices_count);
            for(size_t i=0; i<offices_count; ++i){
                offices_[i].id = *offices[i].GetId();
                offices_[i].offset = offices[i].GetOffset();
                offices_[i].position = offices[i].GetPosition();
            }
        }

        UseCase::UseCase(model::Game& game)
        : game_(game){
        }


        const map_info::Result UseCase::GetMapInfo(const std::string_view map_name){
            std::string map_id = std::string(map_name);
            auto map = game_.FindMap(model::Map::Id{map_id});
    
            if (!map) {
                throw map_info::Error(map_info::ErrorReason::MapNotFound);
            }
            return map_info::Result(*map);
        }

    } // namespace map_info


    namespace join_game {
    
        Error::Error(ErrorReason reason)
        : reason_(reason){
        }

        UseCase::UseCase(model::Game& game, Players& players)
        : game_(game)
        , players_(players){

        }

        const join_game::Result UseCase::AddPlayer(const std::string& user_name, const std::string& map_id) {
            if(user_name.empty()){
                throw join_game::Error{join_game::ErrorReason::InvalidName};
            }
            auto map = game_.FindMap(model::Map::Id{map_id});
            if (!map){
                throw join_game::Error{join_game::ErrorReason::InvalidMap};
            } 
            auto dog = game_.AddDog(user_name); // Создаем собаку для игрока
            auto session = game_.GetSession(map); // находим сессию для карты, которую запросил игрок
            session->AddDog(dog); // добавляем собаку в сессию игры
            auto new_player = players_.AddPlayer(session, dog); // создаем игрока
        
            return {new_player->GetToken(), new_player->GetDog().GetId()};
        }
    
    } // namespace join_game
        
    namespace game_state {

        UseCase::UseCase(model::Game& game) 
        :game_(game){
        }

        const Result UseCase::GetGameSate(const model::GameSession& game_session){
            const auto dogs = game_session.GetDogs();
            Result res;
            size_t dogs_count = dogs.size();
            res.resize(dogs_count);
            for (size_t i = 0; i < dogs_count; ++i){
                auto& rec = res[i];
                const model::Dog& dog = *dogs[i];
                rec.id = std::to_string(*dog.GetId());
                rec.pos = dog.GetPos();
                rec.speed = dog.GetSpeed();
                rec.dir = dog.GetDirSymbol();
            }
            return res;
        }


    }   // namespace game_state 


    Application::Application(model::Game& game) 
    : game_(game)
    , join_game_(game, players_)
    , list_maps_(game)
    , map_info_(game)
    , game_state_(game){
    }
    
    Player* Application::FindPlayer(const std::string_view& token) const noexcept {
        return players_.FindByToken(Player::Token(std::string(token)));
    }

    const list_maps::Result Application::ListMaps() {
        return list_maps_.ListMaps();
    }

    const join_game::Result Application::AddPlayer(const std::string& user_name, const std::string& map_id) {
        return join_game_.AddPlayer(user_name, map_id);
    }

    const map_info::Result Application::GetMapInfo(const std::string_view map_name){
        return map_info_.GetMapInfo(map_name);
    }
        
    const game_state::Result Application::GetGameSate(const std::string_view token){
        auto dogs = FindPlayer(token)->GetGameSession();
        return game_state_.GetGameSate(FindPlayer(token)->GetGameSession());
    }
        
    
} // namespace app 
    