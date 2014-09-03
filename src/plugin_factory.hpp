#ifndef PLUGIN_FACTORY_HPP
#define PLUGIN_FACTORY_HPP

#include "plugin_api.hpp"
#include "plugin_info.hpp"
#include "translator_manager.hpp"
#include "log.hpp"

#include <boost/application.hpp>
#include <boost/function.hpp>
#include <boost/container/flat_map.hpp>

namespace eiptnd {

class translator_manager;

class plugin_factory
{
public:
  plugin_factory();

  /// Load plugin from specified path to it.
  void load(const boost::filesystem::path& path_name);

  /// Load all founded plugins from specified directory.
  void load_dir(const boost::filesystem::path& path_dir);

  /// Create plugin instance.
  plugin_interface_ptr create(puid_t puid);

  translator_manager& get_tm() { return translator_manager_; }

private:
  /// Logger instance and channels.
  logging::logger log_;

  ///
  translator_manager translator_manager_;

  ///
  boost::container::flat_map<puid_t, plugin_info_ptr> plugins_;
};

} // namespace eiptnd

#endif // PLUGIN_FACTORY_HPP
