#include "request_router.hpp"

#include "core.hpp"
#include "empty_ptree.hpp"
#include "plugin/plugin_translator.hpp"

#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>

namespace eiptnd {

template <
    class CallbackT
  , typename ResultT
  , ResultT InitialResult
  , ResultT CompareResult = InitialResult
>
class combiner
  : private boost::noncopyable
{
public:
  typedef combiner<CallbackT, ResultT, InitialResult, CompareResult> type;
  typedef boost::shared_ptr<type> ptr_type;

  combiner(CallbackT callback)
    : callback_(callback)
    , result_(InitialResult)
  {
  }

  ~combiner()
  {
    callback_(result_);
  }

  void combine(ResultT result)
  {
    ResultT old = CompareResult;
    result_.compare_exchange_strong(old, result, boost::memory_order_relaxed);
  }

private:
  CallbackT callback_;
  boost::atomic<ResultT> result_;
};

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
    api->io_service = core_.get_ios();
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
request_router::process_data(dispatchers_vector_ptr dispatch_targets, boost::shared_ptr<dptree> tree, plugin_api::process_data_callback callback)
{
  using namespace plugin_api;

  boost::asio::io_service& io_service = *core_.get_ios();
  std::size_t n = dispatch_targets->size();
  if (n > 1) {
    typedef combiner<process_data_callback, bool, true> combiner_t;
    combiner_t::ptr_type comb = boost::make_shared<combiner_t::type>(boost::move(callback));
    process_data_callback comb_cb = boost::bind(&combiner_t::combine, comb, _1);

    BOOST_FOREACH(plugin_dispatcher_ptr target, *dispatch_targets) {
      io_service.post(boost::bind(&dispatcher::handle_process_data, target, tree, comb_cb));
    }
  }
  else if (n > 0) {
    io_service.post(boost::bind(&dispatcher::handle_process_data, dispatch_targets->front(), tree, callback));
  }
  else {
    io_service.post(boost::bind(callback, false));
  }
}

void
request_router::load_settings(const boost::property_tree::ptree& settings)
{
  using boost::property_tree::ptree;

  BOOST_FOREACH(const ptree::value_type &dispatcher, settings) {
    const std::string& dispatcher_uid = dispatcher.first;

    /// TODO: catch no loaded plugin
    plugin_dispatcher_ptr dispatcher_ptr = create(dispatcher_uid);
    dispatcher_ptr->load_settings(dispatcher.second);
    dispatchers_.emplace(dispatcher_uid, dispatcher_ptr);

    BOOST_AUTO(it, routes_.end());
    BOOST_FOREACH(const ptree::value_type &translator, dispatcher.second.get_child("source", empty_ptree<ptree>())) {
      BOOST_FOREACH(const ptree::value_type &arr, translator.second.get_child("tcp", empty_ptree<ptree>())) {
        unsigned short port_num = arr.second.get<unsigned short>("");
        route_key key = std::make_pair(port_num, translator.first);

        if (it == routes_.end() || it->first != key) {
          it = routes_.find(key);
        }
        if (it == routes_.end()) {
          it = routes_.emplace(key, boost::make_shared<dispatchers_vector_ptr::element_type>(1, dispatcher_ptr)).first;
        }
        else {
          it->second->push_back(dispatcher_ptr);
        }

        BOOST_LOG_SEV(log_, logging::trace)
          << "add route for port=" << port_num << " uid=" << translator.first
          << " to " << dispatcher_ptr->uid();
      }
    }
  }
}

inline void helper_callback_false(boost::shared_ptr<boost::asio::io_service> io_service, plugin_api::process_data_callback callback)
{
  io_service->post(boost::bind(callback, false));
}

void
request_router::setup_connection_routes(puid_t uid, plugin_api::api_translator& papi, unsigned short port_num, const boost::asio::ip::address& remote_address)
{
  papi.authenticate = boost::bind(&request_router::authenticate, this, remote_address, _1, _2, _3);
  try {
    dispatchers_vector_ptr dispatch_targets = routes_.at(std::make_pair(port_num, uid));
    papi.process_data = boost::bind(&request_router::process_data, this, boost::move(dispatch_targets), _1, _2);
  }
  catch (const std::out_of_range&) {
    BOOST_LOG_SEV(log_, logging::warning)
      << "No process_data route founded for port=" << port_num << " uid=" << uid;
#if 0
    boost::asio::io_service& io_service = core_.get_ios();
    papi.process_data = boost::bind(
      &boost::asio::io_service::post< boost::function<void()> >,
      boost::ref(io_service),
      boost::protect(boost::bind(_2, false)));
#else
    papi.process_data = boost::bind(helper_callback_false, core_.get_ios(), _2);
#endif
  }
}

} // namespace eiptnd
