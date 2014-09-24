#ifndef REQUEST_ROUTER_HPP
#define REQUEST_ROUTER_HPP

#include "log.hpp"
#include "plugin_api.hpp"
#include "plugin_info.hpp"

#include <string>
#include <boost/asio/ip/address.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/vector.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

namespace eiptnd {

class core;

class request_router
  : private boost::noncopyable
{
public:
  request_router(core& core);

  void add(plugin_info_ptr info);

  plugin_dispatcher_ptr create(puid_t uid);

  void authenticate(const boost::asio::ip::address& address, std::string id, std::string password, plugin_api::authenticate_callback callback);

  void process_data(boost::shared_ptr<boost::property_tree::ptree> tree, plugin_api::process_data_callback callback);

  void load_settings(const boost::property_tree::ptree& settings);

private:
  /// Logger instance and attributes.
  logging::logger_mt log_;

  ///
  core& core_;

  boost::container::flat_map<puid_t, plugin_info_ptr> loaded_dispatchers_;
  boost::container::vector<boost::shared_ptr<plugin_api::dispatcher> > dispatchers_;
};

} // namespace eiptnd

#endif // REQUEST_ROUTER_HPP
