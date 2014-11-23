#include "tcp_server.hpp"

#include "core.hpp"
#include "plugin_factory.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

namespace eiptnd {

tcp_server::tcp_server(core& core,
               const std::string& bind_addr, unsigned short bind_port)
  : log_(boost::log::keywords::channel = "net")
  , core_(core)
  , io_service_(core_.get_ios())
  , acceptor_(*io_service_)
{
  using namespace boost::asio::ip;

  tcp::endpoint endpoint(address::from_string(bind_addr), bind_port);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();
}

void
tcp_server::start_accept()
{
  BOOST_LOG_SEV(log_, logging::trace) << "start_accept()";

  new_connection_ = boost::make_shared<connection>(boost::ref(core_));
  acceptor_.async_accept(new_connection_->socket(),
      boost::bind(&tcp_server::handle_accept, shared_from_this(), _1));
}

void
tcp_server::handle_accept(const boost::system::error_code& ec)
{
  BOOST_LOG_SEV(log_, logging::trace) << "handle_accept()";

  if (!ec) {
    /// TODO: Check for black list
    new_connection_->on_connection();

    start_accept();
  }
  else if (ec != boost::asio::error::operation_aborted) {
    BOOST_LOG_SEV(log_, logging::error)
      << "Accept failed: " << ec.message() << " (" << ec.value() << ")";
  }
}

void
tcp_server::cancel()
{
  boost::system::error_code ec;
  acceptor_.cancel(ec);
  if (ec) {
    BOOST_LOG_SEV(log_, logging::error)
      << ec.message() << " (" << ec.value() << ")";
  }
}

} // namespace eiptnd
