// Этот файл служит для подключения реализации библиотеки Boost.Json
// #include "boost_json.h"

// #include <boost/json.hpp>



// #include <boost/json/src.hpp>

// #include <filesystem>
// #include <fstream>
// #include <sstream>
// #include <iostream>

// using namespace std;

// void GetJson(const std::filesystem::path& json_path){

//     std::ifstream in_file(json_path);
//     std::ostringstream sstr;
//     sstr << in_file.rdbuf();
//     std::string str_json = sstr.str();
//     boost::json::value j = boost::json::parse(str_json);
//     const auto maps = j.as_object().at("maps").as_array();
//     for(const auto& map : maps){
//         const std::string id =  map.at("id").get_string().c_str();
//         // id.as_string
//         cout << id << endl;
//     };
// }
