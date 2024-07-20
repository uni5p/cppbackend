#include "boost_log.h"   
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>




using namespace std::literals;



//---------------------------------------------------------------
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace json = boost::json;

namespace boost_log {

// BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // Момент времени     
    auto ts = *rec[timestamp];
    json::value log_record{{"timestamp"s, to_iso_extended_string(ts)}};

    // Добавляем данные в формате json, если есть
    auto data = rec[additional_data];
    if (data){
        log_record.as_object().emplace("data"s, *data);
    }

    // Добавляем само сообщение.
    log_record.as_object().emplace("message", *rec[logging::expressions::smessage]);

    // Выводим запись лога как объект json
    strm << log_record;
}

void InitBoostLogFilter() {
    logging::add_common_attributes();

    logging::add_console_log(
        std::clog,
        keywords::format = &MyFormatter
    );
}

} // namespace boost_log