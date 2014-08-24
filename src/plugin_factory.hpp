#ifndef PLUGIN_FACTORY_HPP
#define PLUGIN_FACTORY_HPP

#include <boost/application.hpp>
#include <boost/function.hpp>
#include <boost/container/flat_map.hpp>

#include "plugin_api.hpp"
#include "plugin_info.hpp"
#include "log.hpp"

namespace eiptnd {

class plugin_factory
{
public:
  plugin_factory()
    : log_(boost::log::keywords::channel = "plugin-factory")
  {};

  /// Load plugin from specified path to it.
  void load(const boost::filesystem::path& path_name);

  /// Load all founded plugins from specified directory.
  void load_dir(const boost::filesystem::path& path_dir);

  /// Create plugin instance.
  plugin_interface_ptr create(puid_t puid);

private:
  /// Logger instance and channels.
  logging::logger log_;

  ///
  boost::container::flat_map<puid_t, plugin_info_ptr> plugins_;
  boost::container::flat_multimap<unsigned short, puid_t> plugin_ports_;
};

} // namespace eiptnd

#endif // PLUGIN_FACTORY_HPP
