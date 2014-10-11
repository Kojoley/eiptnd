#include "redis_connection.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <boost/log/attributes/constant.hpp>

using namespace boost::asio::ip;

redis_connection::redis_connection(boost::asio::io_service& io_service)
  : log_(boost::log::keywords::channel = "redis-client")
  , io_service_(io_service)
  , resolver_(io_service_)
  , socket_(io_service_)
  , is_connected_(false)
  , out_buf_(new request_queue()), out_tmp_buf_(new request_queue())
{
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
  }
  else {
    BOOST_LOG_SEV(log_, logging::error)
      << "Failed to resolve (" << ec.message() << ")";
  }
}

void
redis_connection::connect(tcp::resolver::iterator endpoint_iterator)
{
  std::string conn_dst("redis@" + endpoint_iterator->host_name() + ":" + endpoint_iterator->service_name());
  BOOST_LOG_CHANNEL_SEV(log_, conn_dst, logging::trace)
    << "Connecting...";

  socket_.async_connect(*endpoint_iterator,
      boost::bind(&redis_connection::handle_connect, this, _1, endpoint_iterator));
}

void
redis_connection::handle_connect(const boost::system::error_code& ec,
                                  tcp::resolver::iterator endpoint_iterator)
{
  if (!ec) {
    BOOST_LOG_SEV(log_, logging::trace)
      << "Connected";

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
redis_connection::publish(const std::string& channel, const std::string& message, bool_callback callback)
{
  if (!is_connected_) {
    callback(false);
    return;
  }

  std::size_t channel_len = channel.size();
  std::size_t message_len = message.size();

  boost::mutex::scoped_lock lock(io_mutex_);

  out_buf_->data.prepare(strlen("*3\r\n") + strlen("$7\r\nPUBLISH\r\n")
      + 1 + 10 + 2  + channel_len + 2
      + 1 + 10 + 2  + message_len + 2);

  std::ostream os(&out_buf_->data);
  os << "*3\r\n" "$7\r\nPUBLISH\r\n"
        "$" << channel_len << "\r\n" << channel << "\r\n"
        "$" << message_len << "\r\n" << message << "\r\n";

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

  if (out_tmp_buf_->callbacks.size() == 0 || out_tmp_buf_->data.size() == 0) {
    /// This sould not happend, but here for safty reasons
    BOOST_LOG_SEV(log_, logging::critical)
      << "Detected unexpected behavior";
    busy_mutex_.unlock();
    return;
  }

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
    is_connected_ = false;
    BOOST_LOG_SEV(log_, logging::error)
      << "Write error: " << ec.message();
    drop();
    /// TODO: Reconnect if connection was closed
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

        if (s.at(0) != ':') {
          BOOST_LOG_SEV(log_, logging::critical)
            << "I can't handle this type of reply (" << s.at(0) << ")";

          is_connected_ = false;
          socket_.close();
          drop();
          /// TODO: Reconnect
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
      BOOST_LOG_SEV(log_, logging::trace)
        << "Whooa";
      if (out_buf_->callbacks.empty()) {
        busy_mutex_.unlock();
      }
      else {
        write(true);
      }
    }
  }
  else {
    is_connected_ = false;
    BOOST_LOG_SEV(log_, logging::error)
      << "Read error: " << ec.message();
    drop();
    /// TODO: Reconnect if connection was closed
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
