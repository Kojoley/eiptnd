#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include "plugin_factory.hpp"

void
eiptnd::plugin_factory::load(const boost::filesystem::path& path_name)
{
  /// TODO: boost::application::library throws boost::system::system_error
  boost::shared_ptr<boost::application::shared_library> lib
      = boost::make_shared<boost::application::shared_library>(boost::application::library(path_name));

  if (lib->search_symbol(boost::application::symbol("create")) &&
      lib->search_symbol(boost::application::symbol("destroy"))) {

    create_ptr_fn  creator = reinterpret_cast<create_ptr_fn>(lib->get_symbol(boost::application::symbol("create")/*, ec*/));
    destroy_ptr_fn deleter = reinterpret_cast<destroy_ptr_fn>(lib->get_symbol(boost::application::symbol("destroy")/*, ec*/));

    typedef BOOST_TYPEOF(&plugin_api::create_shared) create_shared_fn;
    create_shared_fn create_shared = reinterpret_cast<create_shared_fn>(lib->get_symbol(boost::application::symbol("create_shared")/*, ec*/));
    //create_shared_fn create_shared = lib->get<create_shared_fn>("create_shared"/*, ec*/);

    /// TODO: Check if plugin already loaded
    boost::function<plugin_api_ptr()> plugin_ptr_creator =
        boost::lambda::bind(
          boost::lambda::constructor<plugin_api_ptr>(),
          boost::lambda::bind(creator), deleter );

    //boost::shared_ptr<plugin_api::interface> create_shared;
    //const std::string name = plugin_ptr_creator()->name();
    const std::string name = create_shared()->name();
    if (name == "Echo TCP Plugin") {
      lib_holder_.emplace(3333, lib);
      plugins_.emplace(3333, plugin_ptr_creator);
    }
    else if (name == "Wialon IPS")
    {
      lib_holder_.emplace(4444, lib);
      plugins_.emplace(4444, plugin_ptr_creator);
    }

    BOOST_LOG_SEV(log_, logging::notify)
      << boost::log::add_value("PluginName", name)
      << boost::log::add_value("PluginPath", path_name.string())
      << "Loaded plugin";
  }
}

void
eiptnd::plugin_factory::load_dir(const boost::filesystem::path& path_dir)
{
  BOOST_LOG_SEV(log_, logging::notify)
    << boost::log::add_value("LoadingDir", path_dir)
    << "Loading plugins from directory";

  boost::filesystem::path scan_dir(path_dir);
  std::vector<boost::filesystem::path> files;
  std::copy(boost::filesystem::directory_iterator(scan_dir),
            boost::filesystem::directory_iterator(), back_inserter(files));

  BOOST_FOREACH(boost::filesystem::path path_name, files) {
    const std::string extension
        = boost::algorithm::to_lower_copy(path_name.extension().string());

    if (boost::application::shared_library::suffix() == extension) {
      BOOST_LOG_SEV(log_, logging::info)
        << boost::log::add_value("PluginPath", path_name.string())
        << "Found plugin";
      load(path_name);
    }
  }
}

eiptnd::plugin_api_ptr
eiptnd::plugin_factory::create(puid_t uid)
{
  BOOST_AUTO(it, plugins_.find(uid));

  if (it != plugins_.end()) {
    return it->second();
  }

  return plugin_api_ptr(/*nullptr*/);
}
