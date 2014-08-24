#include <boost/foreach.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include "plugin_factory.hpp"

namespace eiptnd {

void
plugin_factory::load(const boost::filesystem::path& path_name)
{
  /// TODO: catch boost::system::system_error
  BOOST_AUTO(pinfo, boost::make_shared<plugin_info>(path_name));
  /// TODO: Check if plugin already loaded
  const puid_t puid = pinfo->puid;
  plugins_.emplace(puid, pinfo);

  BOOST_LOG_SEV(log_, logging::notify)
    << "Plugin loaded uid: " << puid
    << " name: " << pinfo->name
    << " version: " << pinfo->version
    << " path: " << path_name.string();
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

  return plugin_interface_ptr(/*nullptr*/);
}

} // namespace eiptnd
