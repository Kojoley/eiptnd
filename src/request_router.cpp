#include "request_router.hpp"

#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>

namespace eiptnd {

request_router::request_router()
  : log_(boost::log::keywords::channel = "request-router")
{
}

void
request_router::add(plugin_info_ptr info)
{
  BOOST_LOG_SEV(log_, logging::info)
    << "Dispatcher was added"
       " (uid='" << info->puid() << "'"
       " name='" << info->name() << "')";
  loaded_dispatchers_.emplace(info->puid(), info);
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
  BOOST_LOG_SEV(log_, logging::trace)
    << "Data process request";
  dispatchers_.front()->handle_process_data(tree, callback);
}

void
request_router::load_settings(const boost::property_tree::ptree& settings)
{
  using boost::property_tree::ptree;

  BOOST_FOREACH(plugin_info_ptr info, loaded_dispatchers_ | boost::adaptors::map_values) {
    dispatchers_.push_back(
      boost::dynamic_pointer_cast<plugin_api::dispatcher>(
        (*info)() ));
  }
}

} // namespace eiptnd
