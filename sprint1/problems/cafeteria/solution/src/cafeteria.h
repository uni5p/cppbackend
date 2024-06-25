#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <chrono>

#include "hotdog.h"
#include "result.h"
#include "ingredients.h"

namespace net = boost::asio;
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Logger {
public:
    explicit Logger(std::string id)
        : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(int id, net::io_context& io, Store& store_, GasCooker& gas_cooker, HotDogHandler handler)
        : id_{id}
        , io_{io}
        , hot_dog_handler_{std::move(handler)} 
        , bread_(store_.GetBread())
        , sausage_(store_.GetSausage())
        , gas_cooker_(gas_cooker)
    {}

    // Запускает асинхронное выполнение заказа
    void Execute() {
        logger_.LogMessage("Order has been started."sv);
        FrySausage();
        BakeBread();
    }
private:
    net::io_context& io_;
    int id_;
    HotDogHandler hot_dog_handler_;
    Logger logger_{std::to_string(id_)};
    using Strand = net::strand<net::io_context::executor_type>;
    Strand strand_{net::make_strand(io_)};
    Timer bread_timer_{io_, 100s};  
    Timer sausage_timer_{io_, 200s};  
    // Bread bread_;
    std::shared_ptr<Bread> bread_;
    std::shared_ptr<Sausage> sausage_;
    GasCooker& gas_cooker_;
    bool end_order = false;

    // Sausage sausage_;
    // bool onion_marinaded_ = false;
    // bool delivered_ = false; // Заказ доставлен?

    void BakeBread() {
        logger_.LogMessage("Start baking Bread"sv);
        bread_->StartBake(gas_cooker_, [order = shared_from_this()](){
            order->StartBakeTimer();
        });
    }

    void StartBakeTimer(){
        bread_timer_.expires_after(1s);
        bread_timer_.async_wait([order = shared_from_this()](sys::error_code ec) {
            order->OnBaking(ec);
        });
    }

    void OnBaking(sys::error_code ec) {
        if (ec) {
            logger_.LogMessage("Bake error : "s + ec.what());
        } else {
            logger_.LogMessage("Bread has been baked."sv);
            bread_->StopBaking();
        }
        net::post(strand_, [order = shared_from_this(), ec] {     
            order->CheckReadiness(ec);
        });
    }

    void FrySausage() {
        logger_.LogMessage("Start fry susage"sv);
        sausage_->StartFry(gas_cooker_, [order = shared_from_this()](){
            order->StartFryTimer();
        });
    }

    void StartFryTimer(){
        sausage_timer_.expires_after(1500ms);
        sausage_timer_.async_wait([order = shared_from_this()](sys::error_code ec) {
            order->OnFrySausage(ec);
        });
    }

    void OnFrySausage(sys::error_code ec) {
        if (ec) {
            logger_.LogMessage("Fry error : "s + ec.what());
        } else {
            logger_.LogMessage("Sausage has been fried."sv);
            sausage_->StopFry();
        }
        net::post(strand_, [order = shared_from_this(), ec] {     
            order->CheckReadiness(ec);
        });
    }

    void CheckReadiness(sys::error_code ec) {
        // if (ec) {
        //     // В случае ошибки уведомляем клиента о невозможности выполнить заказ
        //     hot_dog_handler_(Result{ec});
        // }
        if(end_order){
            return;
        };

        if (bread_->IsCooked() && sausage_->IsCooked()) {
            hot_dog_handler_(Result{HotDog{id_, sausage_, bread_}});
            end_order = true;
        }
    }


}; 

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }
/*
            cafeteria.OrderHotDog([&hotdogs, &mut, start_time](Result<HotDog> result) {
                const auto duration = Clock::now() - start_time;
                PrintHotDogResult(result, duration);
                if (result.HasValue()) {
                    // Защищаем доступ к hotdogs с помощью мьютекса
                    std::lock_guard lk{mut};
                    hotdogs.emplace_back(std::move(result).GetValue());
                }
            });
*/
    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(int id, HotDogHandler handler) {
        // TODO: Реализуйте метод самостоятельно
        // При необходимости реализуйте дополнительные классы
        std::make_shared<Order>(id, io_, store_, *gas_cooker_,  std::move(handler))->Execute();
        // Order order(io_);
        // auto sausage = store_.GetSausage();
        // sausage->StartFry(*gas_cooker_, [](){

        // });
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
