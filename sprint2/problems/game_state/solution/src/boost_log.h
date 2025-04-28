#pragma once

#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
// #include <boost/log/core.hpp>        // для logging::core
// #include <boost/log/expressions.hpp> // для выражения, задающего фильтр
// #include <boost/date_time.hpp>
// #include <boost/log/utility/setup/console.hpp>
// #include <boost/log/utility/setup/common_attributes.hpp>
// #include <boost/log/utility/manipulators/add_value.hpp>
// #include <boost/json/src.hpp>
// namespace json = boost::json;


namespace logging = boost::log;
// namespace sinks = boost::log::sinks;
// namespace keywords = boost::log::keywords;
// namespace expr = boost::log::expressions;
// namespace attrs = boost::log::attributes;

namespace boost_log {

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

void InitBoostLogFilter();

} // namespace boost_log