#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
/*#include <boost/move/move.hpp>*/
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "connection.hpp"
#include "log.hpp"

namespace eiptnd {

class core;

class tcp_server
  : private boost::noncopyable
{
public:
  explicit tcp_server(core& core,
                  const std::string& address, unsigned short port_num);

  /*server(BOOST_RV_REF(server) x)            /// Move ctor
     : thread_pool_size_(x.thread_pool_size_)
     , io_service_(io_service_)
     , signals_(boost::move(signals_))
     , acceptor_(boost::move(acceptor_))
  {
  }

  server& operator=(BOOST_RV_REF(server) x) /// Move assign
  {
     return *this;
  }*/

private:
  /*BOOST_MOVABLE_BUT_NOT_COPYABLE(server);*/

  /// Initiate an asynchronous accept operation.
  void start_accept();

  /// Handle completion of an asynchronous accept operation.
  void handle_accept(const boost::system::error_code& ec);

  /// Logger channels and attributes.
  logging::logger log_;

  ///
  core& core_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The next connection to be accepted.
  connection_ptr new_connection_;
};

} // namespace eiptnd

#endif // SERVER_HPP
