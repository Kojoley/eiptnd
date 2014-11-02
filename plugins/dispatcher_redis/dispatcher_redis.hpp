#ifndef DISPATCHER_REDIS_HPP
#define DISPATCHER_REDIS_HPP

#include "log.hpp"
#include "plugin_api.hpp"

#include "redis_connection.hpp"

namespace eiptnd {

class dispatcher_redis
  : public plugin_api::dispatcher
{
public:
  dispatcher_redis();

  ~dispatcher_redis();

  const char* uid() { return "redis"; }

  const char* name() { return "Redis"; }

  const char* version() { return "0.1"; }

  void handle_process_data(boost::shared_ptr<dptree> tree, plugin_api::process_data_callback callback);

  void load_settings(const boost::property_tree::ptree& settings);

  void connect();
  bool is_connected();
  bool ping();

private:
  /// Logger instance and attributes.
  logging::logger_mt log_;

  boost::shared_ptr<redis_connection> rcon_;

  std::string host_;
  std::string port_;
  bool pretty_;
};

DECLARE_PLUGIN(dispatcher_redis)

} // namespace eiptnd

#endif // DISPATCHER_REDIS_HPP
