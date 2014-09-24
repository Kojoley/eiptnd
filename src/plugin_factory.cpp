#include "plugin_factory.hpp"

#include "core.hpp"
#include "empty_ptree.hpp"

#include <boost/foreach.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/throw_exception.hpp>

namespace eiptnd {

plugin_factory::plugin_factory(core& core)
  : log_(boost::log::keywords::channel = "plugin-factory")
  , core_(core)
  , request_router_(core_)
{
}

void
plugin_factory::load(const boost::filesystem::path& path_name)
{
  /// TODO: catch boost::system::system_error
  BOOST_AUTO(pinfo, boost::make_shared<plugin_info>(path_name));
  const puid_t puid = pinfo->puid();

  BOOST_LOG_SEV(log_, logging::notify)
    << "Plugin loaded uid: " << puid
    << " name: " << pinfo->name()
    << " version: " << pinfo->version()
    << " path: " << path_name.string();

  BOOST_AUTO(it, plugins_.find(puid));
  if (it != plugins_.end()) {
    BOOST_LOG_SEV(log_, logging::error)
      << "Plugin with same uid='" << puid << "' is already loaded"
         " from " << it->second->library().path();
    return;
  }

  switch (pinfo->type()) {
  case plugin_api::PLUGIN_TRANSLATOR:
    translator_manager_.add(pinfo);
    break;

  case plugin_api::PLUGIN_DISPATCHER:
    request_router_.add(pinfo);
    break;

  default:
    BOOST_LOG_SEV(log_, logging::warning)
      << "Unknown plugin type: " << puid << ". Unloading it.";
    return;
  }

  plugins_.emplace(puid, pinfo);
}

void
plugin_factory::load_dir(const boost::filesystem::path& path_dir)
{
  BOOST_LOG_SEV(log_, logging::notify)
    << "Loading plugins from directory: " << path_dir;

  boost::filesystem::path scan_dir(path_dir);
  std::vector<boost::filesystem::path> files;
  std::copy(boost::filesystem::directory_iterator(scan_dir),
            boost::filesystem::directory_iterator(), back_inserter(files));

  BOOST_FOREACH(const boost::filesystem::path& path_name, files) {
    const std::string extension
        = boost::algorithm::to_lower_copy(path_name.extension().string());

    if (boost::plugin::shared_library::suffix() == extension) {
      BOOST_LOG_SEV(log_, logging::info)
        << "Found plugin: " << path_name.string();

      load(path_name);
    }
  }
}

plugin_interface_ptr
plugin_factory::create(puid_t uid)
{
  BOOST_AUTO(it, plugins_.find(uid));

  if (it != plugins_.end()) {
    return (*it->second)();
  }

  BOOST_LOG_SEV(log_, logging::error)
    << "Tried to instanciate not loaded plugin (uid: " << uid << ")";

  std::out_of_range e("threre is no loaded plugin with such uid");
  boost::throw_exception(e);
}

void
plugin_factory::load_settings(const boost::property_tree::ptree& settings)
{
  using boost::property_tree::ptree;

  BOOST_AUTO(path, settings.get_child_optional("path"));
  if (path) {
    BOOST_FOREACH(const ptree::value_type &v, *path) {
      load_dir(boost::filesystem::current_path() / v.second.get<std::string>(""));
    }
  }
  else {
    load_dir(boost::filesystem::current_path());
  }

  translator_manager_.load_settings(settings.get_child("translator", empty_ptree<ptree>()));
  request_router_.load_settings(settings.get_child("dispatcher", empty_ptree<ptree>()));
}

} // namespace eiptnd
