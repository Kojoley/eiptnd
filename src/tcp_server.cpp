#include "tcp_server.hpp"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

eiptnd::tcp_server::tcp_server(boost::asio::io_service& io_service,
               const std::string& address, unsigned short port_num,
               plugin_factory& pf)
  : log_(logging::keywords::channel = "net")
  , io_service_(io_service)
  , acceptor_(io_service)
  , new_connection_()
  , plugin_factory_(pf)
{
  using namespace boost::asio::ip;

  tcp::endpoint endpoint(address::from_string(address), port_num);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();
  start_accept();
}

void
eiptnd::tcp_server::start_accept()
{
  BOOST_LOG_SEV(log_, logging::trace) << "start_accept()";

  /// TODO: Maybe replace with make_shared<T>(...).swap(&T)?
  new_connection_.reset(new connection(io_service_));
  acceptor_.async_accept(new_connection_->socket(),
      boost::bind(&tcp_server::handle_accept, this, _1));
}

void
eiptnd::tcp_server::handle_accept(const boost::system::error_code& ec)
{
  BOOST_LOG_SEV(log_, logging::trace) << "handle_accept()";

  if (!ec) {
    /// TODO: Check for black list
    new_connection_->on_connection(plugin_factory_);
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << ec.message() << " (" << ec.value() << ")";
  }

  start_accept();
}
