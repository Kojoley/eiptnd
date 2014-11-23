#include "dispatcher_redis.hpp"

#include "dptree_json.hpp"

namespace eiptnd {

dispatcher_redis::dispatcher_redis()
  : log_(boost::log::keywords::channel = uid())
{
  BOOST_LOG_SEV(log_, logging::trace) << name() << " created";
}

dispatcher_redis::~dispatcher_redis()
{
  BOOST_LOG_SEV(log_, logging::trace) << name() << " destroyed";

  boost::log::core::get()->flush(); /// FIXME: At reworking logging
}

void
dispatcher_redis::handle_process_data(boost::shared_ptr<dptree> tree, plugin_api::process_data_callback callback)
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, *tree, pretty_);

  rcon_->publish("foo", ss.str(), callback);
}

void
dispatcher_redis::load_settings(const boost::property_tree::ptree& settings)
{
  host_ = settings.get<std::string>("host", "localhost");
  port_ = settings.get<std::string>("port", "6379");
  pretty_ = settings.get<bool>("pretty", false);
  std::size_t timeout_connect = settings.get<std::size_t>("timeout.connect", 10);
  std::size_t timeout_ping = settings.get<std::size_t>("timeout.ping", 30);

  BOOST_LOG_SEV(log_, logging::trace)
    << name() << " is configured to " << host_ << ":" << port_;

  rcon_ = boost::make_shared<redis_connection>(api_->io_service);
  rcon_->set_timeouts(timeout_connect, timeout_ping);
  rcon_->connect(host_, port_);
}

} // namespace eiptnd
