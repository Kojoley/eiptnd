#ifndef REQUEST_ROUTER_HPP
#define REQUEST_ROUTER_HPP

#include "log.hpp"
#include "plugin_api.hpp"

#include <string>
#include <boost/asio/ip/address.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

namespace eiptnd {

class request_router
{
public:
  request_router();

  void authenticate(const boost::asio::ip::address& address, std::string id, std::string password, plugin_api::authenticate_callback callback);

  void process_data(boost::shared_ptr<boost::property_tree::ptree> tree, plugin_api::process_data_callback callback);

private:
  /// Logger instance and attributes.
  logging::logger log_;
};

} // namespace eiptnd

#endif // REQUEST_ROUTER_HPP
