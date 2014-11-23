#ifndef REDIS_CONNECTION_HPP
#define REDIS_CONNECTION_HPP

#include "log.hpp"

#define BOOST_CHRONO_HEADER_ONLY
#include "relative_timer.hpp"
#undef BOOST_CHRONO_HEADER_ONLY

#include <queue>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>

using namespace eiptnd;

class redis_connection
{
  typedef boost::function<void(bool)> bool_callback;

public:
  redis_connection(boost::shared_ptr<boost::asio::io_service> io_service);

  /// Configure timeouts.
  void set_timeouts(std::size_t connect, std::size_t ping);

  /// Connect to Redis server.
  void connect(const std::string& host, const std::string& port);

  /// Close connection or pending connection.
  void close();

  /// Try to connect to last connected address.
  void reconnect();

  /// Send ping (calls callback with tru if reply was got).
  void ping(bool_callback callback);

  /// Publish message on channel.
  void publish(const std::string& channel, const std::string& message, bool_callback callback);

private:
  void timeout_connect(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
  void timeout_no_ping(bool ok);
  void timeout_ping(const boost::system::error_code& ec);

  void handle_resolve(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
  void connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
  void handle_connect(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
  void write(bool already_locked=false);
  void handle_write(const boost::system::error_code& ec);
  void read();
  void handle_read(const boost::system::error_code& ec);
  void drop();

  /// Logger instance and attributes.
  logging::logger_mt log_;

  /// Boost.Asio Proactor.
  boost::shared_ptr<boost::asio::io_service> io_service_;

  /// Resolver unit
  boost::asio::ip::tcp::resolver resolver_;

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// Buffer for incoming data.
  boost::asio::streambuf in_buf_;

  /// Timeout timer for reconnect and ping.
  relative_timer timeout_timer_;

  /// Timeouts duration
  std::size_t timeout_connect_;
  std::size_t timeout_ping_;

  /// Remote endpoint host and service name.
  std::string host_;
  std::string port_;

  bool is_connected_;

  /// Buffers for outgoing data.
  struct request_queue {
    boost::asio::streambuf data;
    std::queue<bool_callback> callbacks;
  };
  boost::scoped_ptr<request_queue> out_buf_, out_tmp_buf_;

  /// Synchronization.
  boost::mutex io_mutex_, busy_mutex_;
};

#endif // REDIS_CONNECTION_HPP
