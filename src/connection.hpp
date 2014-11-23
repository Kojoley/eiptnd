#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "log.hpp"
#include "plugin/plugin_translator.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace eiptnd {

class core;

/// Represents a single connection from a client.
class connection
  : public boost::enable_shared_from_this<connection>
  , private boost::noncopyable
{
public:
  explicit connection(core& core);
  ~connection();

  /// Get the socket associated with the connection.
  boost::asio::ip::tcp::socket& socket();

  /// Start the first asynchronous operation for the connection.
  void on_connection();

  /// Initiate graceful connection closure.
  void close();

  /// Reading API.
  void do_read_some(const boost::asio::mutable_buffer& buffer);
  void do_read_until(boost::asio::streambuf& sbuf, const std::string& delim);
  void do_read_at_least(boost::asio::streambuf& sbuf, std::size_t minimum);


  /// Writing API.
  void do_write(const boost::asio::const_buffer& buffer);

private:
  /// Handle completion of a read operation.
  void handle_read(
      boost::shared_ptr<plugin_api::translator> process_handler,
      const boost::system::error_code& ec,
      std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handle_write(
      boost::shared_ptr<plugin_api::translator> process_handler,
      const boost::system::error_code& ec);

  /// Logger instance and attributes.
  logging::logger log_;
  boost::log::attribute_set::iterator net_raddr_;

  ///
  core& core_;

  boost::shared_ptr<boost::asio::io_service> io_service_;

  /// Strand to ensure the connection's handlers are not called concurrently.
  boost::asio::io_service::strand strand_;

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// The handler used to process the data.
  boost::weak_ptr<plugin_api::translator> process_handler_;
};

typedef boost::shared_ptr<connection> connection_ptr;

} // namespace eiptnd

#endif // CONNECTION_HPP
