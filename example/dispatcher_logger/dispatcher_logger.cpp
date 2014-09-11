#include "dispatcher_logger.hpp"

#include <boost/property_tree/json_parser.hpp>

namespace eiptnd {

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
dispatcher_logger::handle_process_data(boost::shared_ptr<boost::property_tree::ptree> tree, plugin_api::process_data_callback callback)
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, *tree, true);
  BOOST_LOG_SEV(log_, logging::trace)
    << "Data process request json=" << ss.str();

  callback(true);
}

} // namespace eiptnd
