#ifndef ECHO_PLUGIN_HPP
#define ECHO_PLUGIN_HPP

#include "log.hpp"
#include "plugin_api.hpp"

#include <iostream>
#include <boost/array.hpp>

namespace eiptnd {

class echo_plugin
  : public plugin_api::interface
{
public:
  echo_plugin();

  ~echo_plugin();

  const char* name() { return "Echo Example"; };

  const char* version() { return "0.1.0"; };

  /// Handle completion of a start operation.
  void handle_start();

  /// Handle completion of a read operation.
  void handle_read(std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handle_write();

private:
  /// Logger instance and attributes.
  logging::logger log_;

  /// Buffer for incoming data.
  boost::array<char, 8192> buffer_;
};

DECLARE_PLUGIN(echo_plugin)

} // namespace eiptnd

#endif // ECHO_PLUGIN_HPP
