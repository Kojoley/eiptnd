#ifndef REDIS_CONNECTION_HPP
#define REDIS_CONNECTION_HPP

#include "log.hpp"

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
  redis_connection(boost::asio::io_service& io_service);

  /// Connect to Redis server.
  void connect(const std::string& host, const std::string& port);

  /// Publish message.
  void publish(const std::string& channel, const std::string& message, bool_callback callback);

private:
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
  boost::asio::io_service& io_service_;

  /// Resolver unit
  boost::asio::ip::tcp::resolver resolver_;

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// Buffer for incoming data.
  boost::asio::streambuf in_buf_;

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
