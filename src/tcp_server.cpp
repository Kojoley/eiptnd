#include "tcp_server.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include "core.hpp"
#include "plugin_factory.hpp"

namespace eiptnd {

tcp_server::tcp_server(core& core,
               const std::string& address, unsigned short port_num)
  : log_(boost::log::keywords::channel = "net")
  , core_(core)
  , acceptor_(core_.get_ios())
  , new_connection_()
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
tcp_server::start_accept()
{
  BOOST_LOG_SEV(log_, logging::trace) << "start_accept()";

  /// TODO: Maybe replace with make_shared<T>(...).swap(&T)?
  new_connection_.reset(new connection(core_));
  acceptor_.async_accept(new_connection_->socket(),
      boost::bind(&tcp_server::handle_accept, this, _1));
}

void
tcp_server::handle_accept(const boost::system::error_code& ec)
{
  BOOST_LOG_SEV(log_, logging::trace) << "handle_accept()";

  if (!ec) {
    /// TODO: Check for black list
    new_connection_->on_connection();
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << ec.message() << " (" << ec.value() << ")";
  }

  start_accept();
}

} // namespace eiptnd
