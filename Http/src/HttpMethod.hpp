#ifndef HTTP_METHOD
#define HTTP_METHOD

#include "HttpServerException.hpp"
#include <map>
#include <string>

enum class HTTPMethod { Unknown = 0, GET, POST, PUT, DELETE };

HTTPMethod string_to_method(const std::string& str) {
   if (str == "GET")
      return HTTPMethod::GET;
   if (str == "PUT")
      return HTTPMethod::PUT;
   if (str == "POST")
      return HTTPMethod::POST;
   if (str == "DELETE")
      return HTTPMethod::DELETE;
   return HTTPMethod::Unknown;
}

HTTPMethod method(const std::string& cmd) {

   // clang-format off

   const static std::map<std::string, HttpServerException> unsupported_cmds = {
      { "add", HttpServerException(405, "'add' is not a command but an argument to 'init'") },
      { "host", HttpServerException(405, "'host' should not be passed as command") },
      { "password", HttpServerException(405, "'password' should be passed with header") },
      { "port", HttpServerException(405, "'port' should not be passed as command") },
      { "remove", HttpServerException(405, "'remove' is not a command but an argument to 'init'") },
      { "rid", HttpServerException(405, "'rid' should not be passed as command") },
      { "server_load", HttpServerException(501, "'server_load' not implemented") },
      { "show", HttpServerException(405, "'show' is not a command but an argument to 'group'") },
      { "sync", HttpServerException(501, "'sync' should be used with c++/python interface only") },
      { "sync_clock", HttpServerException(501, "'sync_clock' should be used with c++/python interface only") },
      { "sync_full", HttpServerException(501, "'sync_full' should be used with c++/python interface only") },
      { "user", HttpServerException(405, "'user' should be passed with header") }
   };

   const static std::map<std::string, HTTPMethod> supported_cmds = {
      { "abort", HTTPMethod::PUT },
      { "alter", HTTPMethod::PUT },
      { "archive", HTTPMethod::PUT },
      { "begin", HTTPMethod::PUT },
      { "ch_add", HTTPMethod::POST },
      { "ch_auto_add", HTTPMethod::POST },
      { "ch_drop", HTTPMethod::DELETE },
      { "ch_drop_user", HTTPMethod::DELETE },
      { "ch_register", HTTPMethod::PUT },
      { "ch_rem", HTTPMethod::DELETE },
      { "ch_suites", HTTPMethod::GET },
      { "check", HTTPMethod::GET },
      { "checkJobGenOnly", HTTPMethod::GET },
      { "check_pt", HTTPMethod::PUT },
      { "complete", HTTPMethod::PUT },
      { "debug", HTTPMethod::GET },
      { "debug_server_off", HTTPMethod::PUT },
      { "debug_server_on", HTTPMethod::PUT },
      { "delete", HTTPMethod::DELETE },
      { "edit_history", HTTPMethod::PUT },
      { "edit_script", HTTPMethod::PUT },
      { "event", HTTPMethod::PUT },
      { "file", HTTPMethod::GET },
      { "force", HTTPMethod::PUT },
      { "force-dep-eval", HTTPMethod::PUT },
      { "free-dep", HTTPMethod::PUT },
      { "get", HTTPMethod::GET },
      { "get_state", HTTPMethod::GET },
      { "group", HTTPMethod::GET },
      { "halt", HTTPMethod::PUT },
      { "help", HTTPMethod::GET },
      { "init", HTTPMethod::PUT },
      { "job_gen", HTTPMethod::PUT },
      { "kill", HTTPMethod::PUT },
      { "label", HTTPMethod::PUT },
      { "load", HTTPMethod::POST },
      { "log", HTTPMethod::PUT },
      { "meter", HTTPMethod::PUT },
      { "migrate", HTTPMethod::GET },
      { "msg", HTTPMethod::PUT },
      { "news", HTTPMethod::GET },
      { "order", HTTPMethod::PUT },
      { "ping", HTTPMethod::GET },
      { "plug", HTTPMethod::PUT },
      { "query", HTTPMethod::GET },
      { "queue", HTTPMethod::PUT },
      { "reloadcustompasswdfile", HTTPMethod::PUT },
      { "reloadpasswdfile", HTTPMethod::PUT },
      { "reloadwsfile", HTTPMethod::PUT },
      { "replace", HTTPMethod::PUT },
      { "requeue", HTTPMethod::PUT },
      { "restart", HTTPMethod::PUT },
      { "restore", HTTPMethod::PUT },
      { "restore_from_checkpt", HTTPMethod::PUT },
      { "resume", HTTPMethod::PUT },
      { "run", HTTPMethod::PUT },
      { "server_version", HTTPMethod::GET },
      { "shutdown", HTTPMethod::PUT },
      { "stats", HTTPMethod::GET },
      { "stats_reset", HTTPMethod::PUT },
      { "stats_server", HTTPMethod::GET },
      { "status", HTTPMethod::GET },
      { "suites", HTTPMethod::GET },
      { "suspend", HTTPMethod::PUT },
      { "terminate", HTTPMethod::PUT },
      { "version", HTTPMethod::GET },
      { "wait", HTTPMethod::GET },
      { "why", HTTPMethod::GET },
      { "zombie_adopt", HTTPMethod::PUT },
      { "zombie_block", HTTPMethod::PUT },
      { "zombie_fail", HTTPMethod::PUT },
      { "zombie_fob", HTTPMethod::PUT },
      { "zombie_get", HTTPMethod::GET },
      { "zombie_kill", HTTPMethod::PUT },
      { "zombie_remove", HTTPMethod::DELETE }
   };

   // clang-format on

   try {
      return supported_cmds.at(cmd);
   } catch (const std::out_of_range& e) {
      auto it = unsupported_cmds.find(cmd);

      if (it != unsupported_cmds.end()) {
         throw it->second;
      }

      throw HttpServerException(501, "Not implemented: '" + cmd + "'");
   }
}

#endif
