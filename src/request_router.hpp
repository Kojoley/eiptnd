#ifndef REQUEST_ROUTER_HPP
#define REQUEST_ROUTER_HPP

#include "log.hpp"
#include "plugin_api.hpp"
#include "plugin_info.hpp"

#include <string>
#include <boost/container/flat_map.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

namespace boost { namespace asio { namespace ip { class address; }}}

namespace eiptnd {

class core;

class request_router
  : private boost::noncopyable
{
  typedef std::pair<unsigned short, puid_t> route_key;
  typedef boost::shared_ptr<std::vector<plugin_dispatcher_ptr> > dispatchers_vector_ptr;

public:
  request_router(core& core);

  void add(plugin_info_ptr info);

  plugin_dispatcher_ptr create(puid_t uid);

  void authenticate(const boost::asio::ip::address& address, std::string id, std::string password, plugin_api::authenticate_callback callback);

  void process_data(dispatchers_vector_ptr dispatch_targets, boost::shared_ptr<boost::property_tree::ptree> tree, plugin_api::process_data_callback callback);

  void load_settings(const boost::property_tree::ptree& settings);

  void setup_connection_routes(puid_t uid, plugin_api::api_translator& papi, unsigned short port_num, const boost::asio::ip::address& remote_address);

private:
  /// Logger instance and attributes.
  logging::logger_mt log_;

  ///
  core& core_;

  boost::container::flat_map<puid_t, plugin_info_ptr> loaded_dispatchers_;
  boost::container::flat_map<puid_t, plugin_dispatcher_ptr> dispatchers_;
  boost::container::flat_map<route_key, dispatchers_vector_ptr> routes_;
};

} // namespace eiptnd

#endif // REQUEST_ROUTER_HPP
