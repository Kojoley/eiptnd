#include "redis_connection.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/log/attributes/constant.hpp>

using namespace boost::asio::ip;

redis_connection::redis_connection(boost::asio::io_service& io_service)
  : log_(boost::log::keywords::channel = "redis-client")
  , io_service_(io_service)
  , resolver_(io_service_)
  , socket_(io_service_)
  , timeout_timer_(io_service_)
  , timeout_connect_(10)
  , timeout_ping_(30)
  , is_connected_(false)
  , out_buf_(new request_queue()), out_tmp_buf_(new request_queue())
{
}

void
redis_connection::set_timeouts(std::size_t connect, std::size_t ping)
{
  timeout_connect_ = connect;
  timeout_ping_ = ping;
}

void
redis_connection::connect(const std::string& host, const std::string& port)
{
  BOOST_LOG_SEV(log_, logging::trace)
    << "Resolving " << host << ":" << port;

  tcp::resolver::query query(host, port);
  resolver_.async_resolve(query,
      boost::bind(&redis_connection::handle_resolve, this, _1, _2));
}

void
redis_connection::handle_resolve(const boost::system::error_code& ec,
                                  tcp::resolver::iterator endpoint_iterator)
{
  if (!ec) {
    BOOST_LOG_SEV(log_, logging::trace)
      << "Resolved to " << endpoint_iterator->endpoint();

    connect(endpoint_iterator);
    host_ = endpoint_iterator->host_name();
    port_ = endpoint_iterator->service_name();
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << "Failed to resolve (" << ec.message() << "). Retrying...";

    connect(endpoint_iterator->host_name(), endpoint_iterator->service_name());
  }
}

void
redis_connection::connect(tcp::resolver::iterator endpoint_iterator)
{
  std::string conn_dst("redis@" + endpoint_iterator->host_name() + ":" + endpoint_iterator->service_name());
  BOOST_LOG_CHANNEL_SEV(log_, conn_dst, logging::trace)
    << "Connecting...";

  timeout_timer_.expires_from_now(boost::chrono::seconds(timeout_connect_));
  timeout_timer_.async_wait(boost::bind(&redis_connection::timeout_connect, this, _1, endpoint_iterator));

  socket_.async_connect(*endpoint_iterator,
      boost::bind(&redis_connection::handle_connect, this, _1, endpoint_iterator));
}

void
redis_connection::timeout_connect(const boost::system::error_code& ec, tcp::resolver::iterator endpoint_iterator)
{
  if (ec != boost::asio::error::operation_aborted) {
    BOOST_LOG_SEV(log_, logging::warning)
      << "Server does not respond. Retrying...";

    socket_.cancel();
    connect(endpoint_iterator);
  }
}

void
redis_connection::timeout_no_ping(bool ok)
{
  if (ok) {
    BOOST_LOG_SEV(log_, logging::trace)
      << "Got Pong!";

    timeout_timer_.cancel();
    timeout_timer_.expires_from_now(boost::chrono::seconds(timeout_ping_));
    timeout_timer_.async_wait(boost::bind(&redis_connection::timeout_ping, this, _1));
  }
  else {
    BOOST_LOG_SEV(log_, logging::trace)
      << "Connection timeout. Reconnecting...";

    /// Actually reconnect will be called in read/write handler
    /// as read/write ended with fail
  }
}

void
redis_connection::timeout_ping(const boost::system::error_code& ec)
{
  if (ec != boost::asio::error::operation_aborted) {
    BOOST_LOG_SEV(log_, logging::trace)
      << "Ping...";

    ping(boost::bind(&redis_connection::timeout_no_ping, this, _1));
  }
}


void
redis_connection::handle_connect(const boost::system::error_code& ec,
                                  tcp::resolver::iterator endpoint_iterator)
{
  if (!ec) {
    BOOST_LOG_SEV(log_, logging::trace)
      << "Connected";

    timeout_no_ping(true);
    is_connected_ = true;

    if (!out_buf_->callbacks.empty()) {
      write();
    }
  }
  else if (++endpoint_iterator != tcp::resolver::iterator()) {
    connect(endpoint_iterator);
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << "Failed to connect (" << ec.message() << ")";
  }
}

void
redis_connection::publish(const std::string& channel,
                          const std::string& message, bool_callback callback)
{
  if (!is_connected_) {
    callback(false);
    return;
  }

  const std::size_t channel_len = channel.size();
  const std::size_t message_len = message.size();
  const std::size_t buf_size =
      std::strlen("*3\r\n")
    + std::strlen("$7\r\nPUBLISH\r\n")
    + std::strlen("$\r\n\r\n") + 10 + channel_len
    + std::strlen("$\r\n\r\n") + 10 + message_len;

  boost::mutex::scoped_lock lock(io_mutex_);

  boost::asio::mutable_buffer buf = out_buf_->data.prepare(buf_size);

  char* buf_endp = boost::asio::buffer_cast<char*>(buf);
  const char* buf_beginp = buf_endp;

  namespace karma = boost::spirit::karma;
  karma::generate(buf_endp
    , "*3\r\n"
      "$7\r\nPUBLISH\r\n"
      "$" << karma::uint_ << "\r\n" << boost::spirit::ascii::string << "\r\n"
      "$" << karma::uint_ << "\r\n" << boost::spirit::ascii::string << "\r\n"
    , channel_len, channel, message_len, message);

  BOOST_ASSERT(buf_size >= static_cast<std::size_t>(buf_endp - buf_beginp));
  out_buf_->data.commit(buf_endp - buf_beginp);

  out_buf_->callbacks.push(callback);

  write();
}

void
redis_connection::ping(bool_callback callback)
{
  if (!is_connected_) {
    callback(false);
    return;
  }

  boost::mutex::scoped_lock lock(io_mutex_);

  static const char bytes[] = "*1\r\n" "$4\r\nPING\r\n";
  static const std::size_t buf_size = std::strlen(bytes);
  static const boost::asio::const_buffer data(bytes, buf_size);

  boost::asio::mutable_buffer buf = out_buf_->data.prepare(buf_size);
  boost::asio::buffer_copy(buf, data);
  out_buf_->data.commit(buf_size);

  out_buf_->callbacks.push(callback);

  write();
}

void
redis_connection::write(bool already_locked)
{
  if (!already_locked && !busy_mutex_.try_lock()) {
    return;
  }

  boost::swap(out_buf_, out_tmp_buf_);

  BOOST_ASSERT(out_tmp_buf_->callbacks.size() > 0);
  BOOST_ASSERT(out_tmp_buf_->data.size() > 0);

  boost::asio::async_write(socket_, out_tmp_buf_->data,
      boost::bind(&redis_connection::handle_write, this, _1));
}

void
redis_connection::handle_write(const boost::system::error_code& ec)
{
  if (!ec) {
    read();
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << "Write error: " << ec.message();

    reconnect();
  }
}

void
redis_connection::read()
{
  boost::asio::async_read(socket_, in_buf_, boost::asio::transfer_at_least(1),
      boost::bind(&redis_connection::handle_read, this, _1));
}

void
redis_connection::handle_read(const boost::system::error_code& ec)
{
  if (!ec) {
    std::size_t i = out_tmp_buf_->callbacks.size();
    std::size_t left = in_buf_.size();
    try {
      std::string s;
      std::istream is(&in_buf_);
      for (; i > 0 && left >= 4; --i) {
        std::getline(is, s);
        std::size_t len = s.size();

        if (s.at(len - 1) != '\r') {
          break;
        }

        if (s.at(0) != ':' && s != "+PONG\r") {
          BOOST_LOG_SEV(log_, logging::critical)
            << "I can't handle this type of reply (" << s.at(0) << ")";

          reconnect();
          return;
        }

        out_tmp_buf_->callbacks.front()(true);
        out_tmp_buf_->callbacks.pop();
        left -= len + 1;
      }
    }
    catch (std::ios_base::failure) {
      // getline failed
    }

    in_buf_.consume(in_buf_.size() - left);

    if (i > 0) {
      BOOST_LOG_SEV(log_, logging::trace)
        << "Left unreaded: " << i;
      read();
    }
    else {
      boost::mutex::scoped_lock lock(io_mutex_);
      if (out_buf_->callbacks.empty()) {
        busy_mutex_.unlock();
      }
      else {
        write(true);
      }
    }
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << "Read error: " << ec.message();

    reconnect();
  }
}

void
redis_connection::drop()
{
  std::size_t len = out_tmp_buf_->callbacks.size();
  BOOST_LOG_SEV(log_, logging::trace)
    << "Dropping " << len << " requests";

  for (; len > 0; --len) {
    out_tmp_buf_->callbacks.front()(false);
    out_tmp_buf_->callbacks.pop();
  }
  busy_mutex_.unlock();
}

void
redis_connection::close()
{
  is_connected_ = false;
  timeout_timer_.cancel();
  if (socket_.is_open()) {
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_.close();
  }
}

void
redis_connection::reconnect()
{
  close();
  drop();
  connect(host_, port_);
}
