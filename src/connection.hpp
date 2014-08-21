#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <boost/asio/io_service.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "plugin_api.hpp"
#include "plugin_factory.hpp"
#include "log.hpp"

namespace eiptnd {

/// Represents a single connection from a client.
class connection
  : public boost::enable_shared_from_this<connection>
  , private boost::noncopyable
{
public:
  explicit connection(boost::asio::io_service& io_service);
  ~connection();

  /// Get the socket associated with the connection.
  boost::asio::ip::tcp::socket& socket();

  /// Start the first asynchronous operation for the connection.
  void on_connection(plugin_factory& plugin_factory_);

  /// Initiate graceful connection closure.
  void close();

  /// Reading API.
  void do_read_some(const boost::asio::mutable_buffer& buffer);
  void do_read_until(boost::asio::streambuf& sbuf, const std::string& delim);

  /// Writing API.
  void do_write(const boost::asio::const_buffer& buffer);

private:
  /// Handle completion of a read operation.
  void handle_read(const boost::system::error_code& ec,
      std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handle_write(const boost::system::error_code& ec);

  /// Logger instance and attributes.
  logging::logger log_;
  boost::log::attribute_set::iterator net_raddr_;

  /// Strand to ensure the connection's handlers are not called concurrently.
  boost::asio::io_service::strand strand_;

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// The handler used to process the data.
  plugin_api_ptr process_handler_;

};

typedef boost::shared_ptr<connection> connection_ptr;

} // namespace eiptnd

#endif // CONNECTION_HPP
