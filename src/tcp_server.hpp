#ifndef SERVER_HPP
#define SERVER_HPP

#include "connection.hpp"
#include "log.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
/*#include <boost/move/move.hpp>*/
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace eiptnd {

class core;

class tcp_server
  : public boost::enable_shared_from_this<tcp_server>
  , private boost::noncopyable
{
public:
  explicit tcp_server(core& core,
                  const std::string& address, unsigned short port_num);

  /// Initiate an asynchronous accept operation.
  void start_accept();

  /// Cancel accepting new connections
  void cancel();

private:
  /// Handle completion of an asynchronous accept operation.
  void handle_accept(const boost::system::error_code& ec);

  /// Logger channels and attributes.
  logging::logger log_;

  ///
  core& core_;

  boost::shared_ptr<boost::asio::io_service> io_service_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The next connection to be accepted.
  connection_ptr new_connection_;
};

} // namespace eiptnd

#endif // SERVER_HPP
