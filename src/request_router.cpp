#include "request_router.hpp"

#include <boost/property_tree/json_parser.hpp>

namespace eiptnd {

request_router::request_router()
  : log_(boost::log::keywords::channel = "request-router")
{
}

void
request_router::authenticate(const boost::asio::ip::address& address, std::string id, std::string password, plugin_api::authenticate_callback callback)
{
  BOOST_LOG_SEV(log_, logging::trace)
    << "Check credentials request for id: " << id << " pass: " << password << " from: " << address;

  /// TODO: Check for brute-force attack!?

  bool ok = true;

  callback(ok);
}

void
request_router::process_data(boost::shared_ptr<boost::property_tree::ptree> tree, plugin_api::process_data_callback callback)
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, *tree, true);
  BOOST_LOG_SEV(log_, logging::trace)
    << "Data process request json: " << ss.str();

  bool ok = true;

  callback(ok);
}

} // namespace eiptnd
