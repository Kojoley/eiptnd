#ifndef CONFIG_H
#define CONFIG_H

#define DAEMON_NAME_FULL    "Daemon Name"
#define DAEMON_NAME_SHORT   "daemonname"
#define DAEMON_DESCRIPTION  DAEMON_NAME_FULL " is server software"
#define DAEMON_VERSION      "0.1.0pre-alpha"
#define DAEMON_INFO         DAEMON_NAME_FULL " v" DAEMON_VERSION

#define DEFAULT_PID_PATH    "/var/run/" DAEMON_NAME_SHORT ".pid"
#define DEFAULT_CONFIG_PATH "/etc/" DAEMON_NAME_SHORT ".cfg"
#define DEFAULT_LOG_PATH    "/tmp/" DAEMON_NAME_SHORT ".log"

#endif // CONFIG_H
