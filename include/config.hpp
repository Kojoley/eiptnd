#ifndef CONFIG_H
#define CONFIG_H

#include <boost/predef.h>

#define DAEMON_NAME_FULL    "Daemon Name"
#define DAEMON_NAME_SHORT   "daemonname"
#define DAEMON_DESCRIPTION  DAEMON_NAME_FULL " is server software"
#define DAEMON_VERSION      "0.1.0pre-alpha"
#define DAEMON_INFO         DAEMON_NAME_FULL " v" DAEMON_VERSION " (" BOOST_COMPILER ")"

#if defined(BOOST_OS_WINDOWS)
#  define DEFAULT_CONFIG_PATH DAEMON_NAME_SHORT ".cfg"
#else
#  define DEFAULT_PID_PATH    "/var/run/" DAEMON_NAME_SHORT ".pid"
#  define DEFAULT_CONFIG_PATH "/etc/" DAEMON_NAME_SHORT ".cfg"
#endif

#endif // CONFIG_H
