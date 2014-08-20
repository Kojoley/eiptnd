#ifndef WIALON_PLUGIN_HPP
#define WIALON_PLUGIN_HPP

#include "plugin_api.hpp"

#include <boost/array.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/tuple/tuple.hpp>

class wialon_plugin
  : public plugin_api::interface
{
public:
  wialon_plugin();

  ~wialon_plugin();

  const char* name() { return "Wialon IPS"; };

  const char* version() { return "0.1.0"; };

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
  void store_data();
  void commit_command();
  void answer(const std::string& msg);

  /// Buffer for incoming data.
  /*boost::array<char, 8192> buffer_;*/

  /// Buffer for outgoing data.
  boost::array<char, 32> answer_buffer_;

  boost::asio::streambuf sbuf_;

  /// Estimate tokens to read.
  //std::size_t estimate_tokens_;

  boost::shared_ptr<boost::property_tree::ptree> tree_;
  std::size_t multicommand_;
  std::size_t estimate_;
  std::string imei_;
  std::string cmd_;
  bool authenticated_;

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

  typedef const boost::container::flat_map<std::string, boost::tuple<std::size_t, command_t> > estimates_t;
  static estimates_t estimates_;


  //typedef const char* fields_type;
  typedef std::string fields_type;
  typedef std::vector<fields_type> fields_subtype;
  typedef const boost::array<fields_subtype, COMMAND_COUNT> fields_t;
  static fields_t fields_;
};

DECLARE_PLUGIN(wialon_plugin)

#endif // WIALON_PLUGIN_HPP
