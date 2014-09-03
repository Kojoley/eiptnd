#include "connection.hpp"

#include "core.hpp"
#include "plugin_factory.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/log/attributes.hpp>

namespace eiptnd {

connection::connection(core& core)
  : log_(boost::log::keywords::channel = "connection")
  , core_(core)
  , strand_(core_.get_ios())
  , socket_(core_.get_ios())
{
  /// NOTE: There is no real conection here, only waiting for it.
}

connection::~connection()
{
  BOOST_LOG_SEV(log_, logging::info) << "Connection closed";

  //conn_mgr_.on_close(socket_.remote_endpoint());
}

boost::asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}

void
connection::on_connection()
{
  boost::asio::ip::tcp::endpoint local_endpoint = socket_.local_endpoint();
  boost::asio::ip::tcp::endpoint remote_endpoint = socket_.remote_endpoint();

  /// TODO: replace stringstream with anything more perfomance
  std::stringstream remote_addr;
  remote_addr << remote_endpoint;
  boost::log::attributes::constant<std::string> addr(remote_addr.str());
  net_raddr_ = log_.add_attribute("RemoteAddress", addr).first;

  BOOST_LOG_SEV(log_, logging::info) << "Connection accepted";

  BOOST_AUTO(papi, boost::make_shared<plugin_api::api>());
  papi->do_read_until = boost::bind(&connection::do_read_until, this, _1, _2);
  papi->do_read_some = boost::bind(&connection::do_read_some, this, _1);
  papi->do_write = boost::bind(&connection::do_write, this, _1);
  request_router& rr = core_.get_rr();
  papi->authenticate = boost::bind(&request_router::authenticate, boost::ref(rr), remote_endpoint.address(), _1, _2, _3);
  papi->process_data = boost::bind(&request_router::process_data, boost::ref(rr), _1, _2);

  translator_manager& tm = core_.get_pf().get_tm();
  std::string puid;
  BOOST_AUTO(it, tm.list_port(local_endpoint.port()));
  BOOST_FOREACH(translator_manager::port_mapping_t::value_type i, it) {
    BOOST_LOG_SEV(log_, logging::trace) << i.first << ":" << i.second;
    puid = i.second;
  }

  process_handler_ = tm.create(puid);
  process_handler_->setup_api(papi);
  process_handler_->handle_start();

  /// TODO: store pointer to plugin library (for hot unloading plugin)

}

/*void
connection::do_read2(const boost::asio::mutable_buffer& buffers)
{
  boost::asio::async_read(socket_, )
}*/

void
connection::do_read_until(boost::asio::streambuf& sbuf, const std::string& delim)
{
  boost::asio::async_read_until(socket_, sbuf, delim,
      strand_.wrap(
        boost::bind(&connection::handle_read, shared_from_this(), _1, _2)));
}

void
connection::do_read_some(const boost::asio::mutable_buffer& buffers)
{
  socket_.async_read_some(boost::asio::mutable_buffers_1(buffers),
      strand_.wrap(
        boost::bind(&connection::handle_read, shared_from_this(), _1, _2)));
}

void
connection::do_write(const boost::asio::const_buffer& buffers)
{
  boost::asio::async_write(socket_,
      boost::asio::const_buffers_1(buffers),
      strand_.wrap(
        boost::bind(&connection::handle_write, shared_from_this(), _1)));
}

void
connection::handle_read(const boost::system::error_code& ec,
    std::size_t bytes_transferred)
{
  if (!ec) {
    process_handler_->handle_read(bytes_transferred);
  }
  else if (ec == boost::asio::error::eof) {
    BOOST_LOG_SEV(log_, logging::info)
      << "Connection has been closed by the remote endpoint";

    if (bytes_transferred) {
      /// TODO: Is it so?
      BOOST_LOG_SEV(log_, logging::info)
        << "But some data was recieved (" << bytes_transferred << " bytes)";
    }
  }
  else if (ec == boost::asio::error::operation_aborted) {
    BOOST_LOG_SEV(log_, logging::info)
      << "Connection unexpectedly closed";
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << ec.message() << " (" << ec.value() << ")";
  }
}

void
connection::handle_write(const boost::system::error_code& ec)
{
  if (!ec) {
    process_handler_->handle_write();
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << ec.message() << " (" << ec.value() << ")";
  }
}

void
connection::close()
{
  BOOST_LOG_SEV(log_, logging::info) << "Closing connection";

  boost::system::error_code ignored_ec;
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

} // namespace eiptnd
