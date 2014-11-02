#ifndef DISPATCHER_LOGGER_HPP
#define DISPATCHER_LOGGER_HPP

#include "log.hpp"
#include "plugin_api.hpp"

namespace eiptnd {

class dispatcher_logger
  : public plugin_api::dispatcher
{
public:
  dispatcher_logger();

  ~dispatcher_logger();

  const char* uid() { return "logger-example"; }

  const char* name() { return "Logger Example"; }

  const char* version() { return "0.1"; }

  void handle_process_data(boost::shared_ptr<dptree> tree, plugin_api::process_data_callback callback);

  void load_settings(const boost::property_tree::ptree& settings);

private:
  /// Logger instance and attributes.
  logging::logger log_;

  /// Delay for callback response.
  std::size_t delay_;
};

DECLARE_PLUGIN(dispatcher_logger)

} // namespace eiptnd

#endif // DISPATCHER_LOGGER_HPP
