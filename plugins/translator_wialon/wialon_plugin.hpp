#ifndef WIALON_PLUGIN_HPP
#define WIALON_PLUGIN_HPP

#include "log.hpp"
#include "plugin/plugin_translator.hpp"

#include <iostream>
#include <boost/array.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/tuple/tuple.hpp>

namespace eiptnd {

class wialon_plugin
  : public boost::enable_shared_from_this<wialon_plugin>
  , public plugin_api::translator
{
public:
  wialon_plugin();

  ~wialon_plugin();

  const char* uid() { return "wialon"; }

  const char* name() { return "Wialon IPS"; }

  const char* version() { return "1.1"; }

  /// Handle completion of a start operation.
  void handle_start();

  /// Handle completion of a read operation.
  void handle_read(std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handle_write();

private:
  void parse_string(const std::string& msg);
  void consume_token(const std::string& tok);
  void unexpected_token(const std::string& expected, const std::string& got);
  void parser_error(const std::string& message);
  void commit_command();
  void store_data();
  void answer(const std::string& msg);
  void handle_authenticate(bool ok);
  void handle_process_data(bool ok);

  /// Logger instance and attributes.
  logging::logger log_;

  /// Buffer for incoming data.
  /*boost::array<char, 8192> buffer_;*/
  boost::asio::streambuf sbuf_;

  /// Buffer for outgoing data.
  boost::array<char, 32> answer_buffer_;

  /// Estimate tokens/bytes to read.
  std::size_t estimate_;

  /// Tree for storing data by parser that will be processed by dispatcher
  boost::shared_ptr<dptree> tree_;

  /// COMMAND_BLACKBOX parsed (and stored) data blocks
  boost::atomic<std::size_t> multicommand_;

  /// Authenticatiin status
  bool authenticated_;

  /// Flag of the error that occurred during the parsing
  bool is_parser_error_;

  /// Parser state
  enum state_t {
    STATE_INITIAL,
    STATE_IMEI,
    STATE_COMMAND,
    STATE_BODY,
    STATE_HASH_FIRST,
    STATE_HASH_SECOND,
    STATE_BAR,
    STATE_END,
    STATE_SKIP_TO_BAR_OR_END,
    STATE_SKIP_TO_END,
    STATE_BINARY_DATA
  } state_;

  /// Parsed command
  std::string cmd_;
  enum command_t {
    COMMAND_LOGIN,
    COMMAND_DATA,
    COMMAND_PING,
    COMMAND_SHORTDATA,
    COMMAND_BLACKBOX,
    COMMAND_MESSAGE,
    COMMAND_IMAGE,

    COMMAND_COUNT
  } current_cmd_;

  friend inline std::ostream& operator<<(std::ostream &os, const wialon_plugin::state_t state);

  /// Estimate tokens to consume in STATE_BODY
  typedef const boost::container::flat_map<std::string, boost::tuple<std::size_t, command_t> > estimates_t;
  static estimates_t estimates_;

  /// Names fo tokens consumed in STATE_BODY
  typedef std::string fields_type;
  typedef std::vector<fields_type> fields_subtype;
  typedef const boost::array<fields_subtype, COMMAND_COUNT> fields_t;
  static fields_t fields_;
};

DECLARE_PLUGIN(wialon_plugin)

} // namespace eiptnd

#endif // WIALON_PLUGIN_HPP
