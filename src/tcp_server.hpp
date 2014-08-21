#ifndef SERVER_HPP
#define SERVER_HPP

/*#include <boost/move/move.hpp>*/
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "connection.hpp"
#include "plugin_factory.hpp"
#include "log.hpp"

namespace eiptnd {

class tcp_server
  : private boost::noncopyable
{
public:
  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit tcp_server(boost::asio::io_service& io_service,
                  const std::string& address, unsigned short port_num,
                  plugin_factory& pf);

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

  /// The io_service used to perform asynchronous operations.
  boost::asio::io_service& io_service_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The next connection to be accepted.
  connection_ptr new_connection_;

  plugin_factory& plugin_factory_;
};

} // namespace eiptnd

#endif // SERVER_HPP
