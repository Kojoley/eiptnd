#include "request_router.hpp"

#include "core.hpp"

#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>

namespace eiptnd {

request_router::request_router(core& core)
  : log_(boost::log::keywords::channel = "request-router")
  , core_(core)
{
}

void
request_router::add(plugin_info_ptr info)
{
  BOOST_LOG_SEV(log_, logging::info)
    << "Dispatcher was added"
       " (uid='" << info->puid() << "'"
       " name='" << info->name() << "')";
  loaded_dispatchers_.emplace(info->puid(), info);
}

plugin_dispatcher_ptr
request_router::create(puid_t uid)
{
  BOOST_AUTO(it, loaded_dispatchers_.find(uid));

  if (it != loaded_dispatchers_.end()) {
    plugin_interface_ptr plugin = (*it->second)();
    plugin_dispatcher_ptr dispatcher = boost::dynamic_pointer_cast<plugin_api::dispatcher>(plugin);

    boost::shared_ptr<plugin_api::api_dispatcher> api = boost::make_shared<plugin_api::api_dispatcher>();
    api->io_service = boost::bind(&core::get_ios, &core_);
    dispatcher->setup_api(api);
    return dispatcher;
  }

  BOOST_LOG_SEV(log_, logging::error)
    << "Tried to instanciate not loaded dispatcher"
       " (uid='" << uid << "')";

  std::out_of_range e("threre is no loaded dispatcher with such uid");
  boost::throw_exception(e);
}

void
request_router::authenticate(const boost::asio::ip::address& address, std::string id, std::string password, plugin_api::authenticate_callback callback)
{
  BOOST_LOG_SEV(log_, logging::trace)
    << "Check credentials request for id: " << id << " pass: " << password << " from: " << address;

  /// TODO: Check for brute-force attack!?

  bool ok = true;

  callback(ok);
}

void
request_router::process_data(boost::shared_ptr<boost::property_tree::ptree> tree, plugin_api::process_data_callback callback)
{
  BOOST_LOG_SEV(log_, logging::trace)
    << "Data process request";
  dispatchers_.front()->handle_process_data(tree, callback);
}

void
request_router::load_settings(const boost::property_tree::ptree& settings)
{
  using boost::property_tree::ptree;

  BOOST_FOREACH(const ptree::value_type &v, settings) {
    /// TODO: catch no loaded plugin
    plugin_dispatcher_ptr plugin = create(v.first);
    plugin->load_settings(v.second);
    dispatchers_.push_back(plugin);
  }
}

} // namespace eiptnd
