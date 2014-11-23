#ifndef PLUGIN_DISPATCHER_HPP
#define PLUGIN_DISPATCHER_HPP

#include "plugin/plugin_api.hpp"
#include "dptree.hpp"

#include <boost/asio/io_service.hpp>

namespace eiptnd {
namespace plugin_api {

struct api_dispatcher
{
  boost::shared_ptr<boost::asio::io_service> io_service;
};

class dispatcher
  : public interface
{
public:
  virtual ~dispatcher() { api_.reset(); }
  virtual plugin_type type() { return PLUGIN_DISPATCHER; }
  virtual void handle_process_data(boost::shared_ptr<dptree> tree, process_data_callback callback) = 0;
  void setup_api(boost::shared_ptr<api_dispatcher> p) { api_ = p; }

protected:
  boost::shared_ptr<api_dispatcher> api_;
};

} // plugin_api

typedef boost::shared_ptr<plugin_api::dispatcher> plugin_dispatcher_ptr;

} // namespace eiptnd

#endif // PLUGIN_DISPATCHER_HPP
