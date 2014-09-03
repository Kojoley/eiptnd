#include "translator_manager.hpp"

#include "plugin_factory.hpp"

namespace eiptnd {

translator_manager::translator_manager()
  : log_(boost::log::keywords::channel = "translator-manager")
{
}

void
translator_manager::add(plugin_info_ptr info)
{
  BOOST_LOG_SEV(log_, logging::info)
    << "Translator was added"
       " (uid='" << info->puid() << "'"
       " name='" << info->name() << "')";
  loaded_translators_.emplace(info->puid(), info);
}

plugin_translator_ptr
translator_manager::create(puid_t uid)
{
  BOOST_AUTO(it, loaded_translators_.find(uid));

  if (it != loaded_translators_.end()) {
    plugin_interface_ptr plugin = (*it->second)();
    return boost::dynamic_pointer_cast<plugin_api::translator>(plugin);
  }

  BOOST_LOG_SEV(log_, logging::error)
    << "Tried to instanciate not loaded translator"
       " (uid='" << uid << "')";

  std::out_of_range e("threre is no loaded translator with such uid");
  boost::throw_exception(e);
}

bool
translator_manager::map_port(unsigned short port_num, const puid_t uid)
{
  BOOST_AUTO(it, loaded_translators_.find(uid));

  if (it == loaded_translators_.end()) {
    BOOST_LOG_SEV(log_, logging::warning)
      << "Tried to map port=" << port_num << " with not loaded plugin"
         " (uid='" << uid << "')";

    return false;
  }
  else if (it->second->ptype() != plugin_api::PLUGIN_TRANSLATOR) {
    BOOST_LOG_SEV(log_, logging::warning)
      << "Tried to map port=" << port_num  << " with plugin"
         " (uid='" << uid << "') which is not a translator";

    return false;
  }

  BOOST_LOG_SEV(log_, logging::info)
    << "port=" << port_num << " has been mapped with plugin"
       " (uid='" << uid << "' name='" << it->second->name() << "')";

  port_mapping_.emplace(port_num, uid);

  return true;
}

translator_manager::iterator_range
translator_manager::list_port(const unsigned short port_num)
{
  /// TODO: Can we use boost::move?
  return boost::make_iterator_range(port_mapping_.lower_bound(port_num),
                                    port_mapping_.upper_bound(port_num));
}

} // namespace eiptnd
