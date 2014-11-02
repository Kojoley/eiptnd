#include "dispatcher_logger.hpp"

#include "dptree_json.hpp"
#include "relative_timer.hpp"

#include <boost/enable_shared_from_this.hpp>

namespace eiptnd {

class delayed_callback
  : public boost::enable_shared_from_this<delayed_callback>
{
public:
  delayed_callback(
      boost::asio::io_service& io_service,
      plugin_api::process_data_callback callback)
    : delay_timer_(io_service)
    , callback_(boost::move(callback))
  {
  }

  void delay(std::size_t delay_ms)
  {
    delay_timer_.expires_from_now(boost::chrono::milliseconds(delay_ms));
    delay_timer_.async_wait(boost::bind(&delayed_callback::run, shared_from_this()));
  }

  void run()
  {
    callback_(true);
  }

private:
  relative_timer delay_timer_;
  plugin_api::process_data_callback callback_;
};

dispatcher_logger::dispatcher_logger()
  : log_(boost::log::keywords::channel = uid())
{
  BOOST_LOG_SEV(log_, logging::trace) << name() << " created";
}

dispatcher_logger::~dispatcher_logger()
{
  BOOST_LOG_SEV(log_, logging::trace) << name() << " destroyed";

  boost::log::core::get()->flush(); /// FIXME: At reworking logging
}

void
dispatcher_logger::handle_process_data(boost::shared_ptr<dptree> tree, plugin_api::process_data_callback callback)
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, *tree, true);
  BOOST_LOG_SEV(log_, logging::trace)
    << "Data process request json=" << ss.str();

  if (!delay_) {
    callback(true);
  }
  else {
    boost::make_shared<delayed_callback>(
        boost::ref(api_->io_service()), callback)->delay(delay_);
  }
}

void
dispatcher_logger::load_settings(const boost::property_tree::ptree& settings)
{
  delay_ = settings.get<std::size_t>("delay", 0u);
}

} // namespace eiptnd
