/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : HttpServer.cpp
// Author      : partio
// Revision    : $Revision$
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "HttpServer.hpp"
#include "ClientInvoker.hpp"
#include <boost/filesystem.hpp>
#include "Child.hpp"

#ifdef ECF_OPENSSL
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include "HttpMethod.hpp"
#include "HttpServerException.hpp"
#include <httplib.h>
#include "Base64.hpp"

static bool verbose_ = false;
static bool no_ssl = false;
static int port = 8080;

namespace {
struct command {
   std::string name;
   std::string host;
   std::string port;
   std::string ssl;
   std::string username;
   std::string password;
   std::string argument1;
   std::string argument2;
   std::string argument3;
   std::string argument4;
   std::string argument5;
   std::string argument6;
   std::string format;
   HTTPMethod method;
};

struct child_env {
   // Mandatory
   std::string task_path;     // ECF_NAME:     The name of this current task
   std::string job_password;  // ECF_PASS:     A unique password
   std::string rid;           // ECF_RID:      Process id. Also used for zombie detection
   std::string try_no;        // ECF_TRYNO:    Current try number of the task
   // Optional
   std::string timeout;       // ECF_TIMEOUT:  Max time in *seconds* for client to deliver message
   std::string hostfile;      // ECF_HOSTFILE: File that lists alternate hosts to try
   std::string denied;        // ECF_DENIED:   Provides a way for child to exit with an error, if server denies connection
   std::string no_ecf;        // NO_ECF:       Exits ecflow_client immediately with success
   std::string zombie_timeout; // ECF_ZOMBIE_TIMEOUT:
};

std::vector<std::string> Split(const std::string& str, const std::string delim) {
   std::regex rgx(delim);
   std::sregex_token_iterator first{begin(str), end(str), rgx, -1}, last;

   return {first, last};
}

std::string get_value(const httplib::Request& request, const std::string& key, const std::string& default_value = "NONE") {
   const std::string val = request.get_param_value(key.c_str());
   if (val.empty()) {
      if (default_value == "NONE") {
         throw HttpServerException(400, "Missing argument '" + key + "'");
      }
      return default_value;
   }
   return val;
}

}  // namespace

HttpServer::HttpServer(int argc, char** argv) {
   parse_args(argc, argv);
}

void HttpServer::parse_args(int argc, char** argv) {
   namespace po = boost::program_options;

   po::options_description desc("Allowed options", 100);

   // clang-format off

   desc.add_options()
       ("help,h", "print help message")
       ("port,p", po::value(&port), "port to listen (default: 8080)")
       ("no_ssl", po::bool_switch(&no_ssl), "disable ssl (default: false)")
       ("verbose,v", po::bool_switch(&verbose_), "enable verbose mode");

   // clang-format on

   po::variables_map opt;
   po::store(po::command_line_parser(argc, argv).options(desc).run(), opt);
   po::notify(opt);

   if (opt.count("help")) {
      std::cout << desc << std::endl;
      exit(1);
   }
}

std::vector<std::string> create_argv(const command& cmd) {
   std::vector<std::string> argv;
   argv.reserve(5);

   argv.push_back("ecflow_http");

   if (cmd.username.empty() == false) argv.insert(argv.end(), { "--user", cmd.username });
   if (cmd.host.empty() == false) argv.insert(argv.end(), { "--host", cmd.host });
   if (cmd.port.empty() == false) argv.insert(argv.end(), { "--port", cmd.port });
   if (cmd.ssl.empty() == false) argv.push_back("--ssl");
   argv.push_back("--" + cmd.name);
   if (cmd.argument1.empty() == false) argv.push_back(cmd.argument1);
   if (cmd.argument2.empty() == false) argv.push_back(cmd.argument2);
   if (cmd.argument3.empty() == false) argv.push_back(cmd.argument3);
   if (cmd.argument4.empty() == false) argv.push_back(cmd.argument4);
   if (cmd.argument5.empty() == false) argv.push_back(cmd.argument5);
   if (cmd.argument6.empty() == false) argv.push_back(cmd.argument6);

   return argv;
}

std::pair<int, std::string> call_ecflow(const command& cmd, const child_env& cenv) {
   std::vector<std::string> argv = create_argv(cmd);

   std::stringstream buffer;

   if (verbose_) {
      std::cout << "args:\n";
      for (int i = 0; i < argv.size(); i++) {
         std::cout << i << "/" << argv.size() << ": " << argv[i] << "\n";
      }
      if (cenv.task_path.empty() == false) {
         std::cout << "child env:\n"
                   << "0/4 task_path: " << cenv.task_path << "\n1/4 job_password: " << cenv.job_password
                   << "\n2/4 rid: " << cenv.rid << "\n3/4 try_no: " << cenv.try_no << "\n";
      }
   }

   std::streambuf* prevcoutbuf = nullptr;

   int status_code = 200;

   if (cmd.method == HTTPMethod::POST) status_code = 201;

   try {
      ClientInvoker client;
      client.set_password(cmd.password);

      if (cenv.task_path.empty() == false) {
         client.set_child_path(cenv.task_path);
         client.set_child_password(cenv.job_password);
         client.set_child_pid(cenv.rid);
         client.set_child_try_no(std::stoi(cenv.try_no));
         client.set_child_timeout(std::stoi(cenv.timeout));
         client.set_zombie_child_timeout(std::stoi(cenv.zombie_timeout));
      }

      // Redirect std::cout to buffer
      prevcoutbuf = std::cout.rdbuf(buffer.rdbuf());
      client.set_cli(true);
      client.invoke(argv);
      std::cout.rdbuf(prevcoutbuf);
   } catch (std::exception& e) {
      buffer << "client_error: " << e.what() << std::endl;
      std::cout.rdbuf(prevcoutbuf);

      // Try to guess a bit more suitable return values
      // based on client output

      status_code = 400;

      const std::string err(e.what());

      if (err.find("authentication failed") != std::string::npos) {
         status_code = 401;
      } else if (err.find("Could not find") != std::string::npos) {
         status_code = 404;
      } else if (err.find("Cannot find") != std::string::npos) {
         status_code = 404;
      } else if (err.find("Add Suite failed: A Suite of name") != std::string::npos) {
         status_code = 409;
      }
   } catch (...) {
      status_code = 500;
      buffer << "Unexpected error\n";
      std::cout.rdbuf(prevcoutbuf);
   }

   return std::make_pair(status_code, buffer.str());
}

std::pair<std::string, std::string> get_credentials(const httplib::Headers& header) {
   auto auth = header.find("Authorization");

   if (auth == header.end()) {
      return std::make_pair("", "");
   }

   const auto tokens = ::Split(auth->second, " ");

   const std::string auth_type = tokens[0];

   if (auth_type == "Basic") {
      const std::string decoded = base64_decode(tokens[1]);
      const auto creds = ::Split(decoded, ":");

      return std::make_pair(creds[0], creds[1]);
   } else {
      throw HttpServerException(405, "Authentication method not supported: " + auth_type);
   }
}

child_env parse_child_env(const httplib::Request& request, const std::string& command) {

   child_env cenv;

   if (ecf::Child::valid_child_cmd(command)) {
      cenv.task_path = get_value(request, "task_path");
      cenv.job_password = get_value(request, "job_password");
      cenv.rid = get_value(request, "rid");
      cenv.try_no = get_value(request, "try_no");
      // optional
      cenv.timeout = get_value(request, "timeout", "86400");
      cenv.zombie_timeout = get_value(request, "timeout", "43200");
      cenv.hostfile = get_value(request, "hostfile", "");
      cenv.denied = get_value(request, "denied", "false");
      cenv.no_ecf = get_value(request, "no_ecf", "false");
   }

   return cenv;
}

command parse_command(const httplib::Request& request) {
   command cmd;

   cmd.name = get_value(request, "command");
   cmd.host = get_value(request, "host", "");
   cmd.port = get_value(request, "port", "");
   cmd.ssl = get_value(request, "ssl", "");

   cmd.argument1 = get_value(request, "argument", "");

   if (cmd.argument1.empty()) { 
      cmd.argument1 = get_value(request, "argument1", "");
   }

   cmd.argument2 = get_value(request, "argument2", "");
   cmd.argument3 = get_value(request, "argument3", "");
   cmd.argument4 = get_value(request, "argument4", "");
   cmd.argument5 = get_value(request, "argument5", "");
   cmd.argument6 = get_value(request, "argument6", "");
   cmd.format = get_value(request, "format", "text");

   if (cmd.format != "text") {
      throw HttpServerException(400, "Only text format is supported");
   }

   const auto creds = get_credentials(request.headers);

   if (creds.first.empty() == false) {
      cmd.username = creds.first;
      cmd.password = crypt(creds.second.c_str(), creds.first.c_str());
   }

   return cmd;
}

std::pair<command, child_env> parse_query_string(const httplib::Request& request) {

   command cmd = parse_command(request);
   cmd.method = method(cmd.name);

   child_env cenv = parse_child_env(request, cmd.name);

   const HTTPMethod given_method = string_to_method(request.method);

   // Match HTTP method and operation type

   if (given_method != cmd.method) {
      throw HttpServerException(400, "Invalid HTTP method '" + request.method + "' for command '" + cmd.name + "'");
   }

   if (given_method != HTTPMethod::GET && no_ssl == true) {
      throw HttpServerException(400, "Method " + request.method + " only allowed with SSL");
   }

   if (given_method != HTTPMethod::GET && ecf::Child::valid_child_cmd(cmd.name) == false && cmd.password.empty()) {
      throw HttpServerException(401, "Missing authentication details");
   }

   return std::make_pair(cmd, cenv);
}

void handle_query(const httplib::Request& request, httplib::Response& response) {
   try {
      const auto args = parse_query_string(request);
      auto resp = call_ecflow(args.first, args.second);
      response.status = resp.first;
      response.set_content(resp.second, "text/plain");
   } catch (const HttpServerException& e) {
      response.status = e.code();
      response.set_content(e.what(), "text/plain");
   } catch (const std::exception& e) {
      response.status = 400;
      response.set_content(e.what(), "text/plain");
   }
}

void ApplyListeners(httplib::Server& http_server) {
   auto dump_headers = [](const httplib::Headers& headers) {
      std::stringstream ss;
      for (const auto& m : headers) {
         ss << m.first << ": " << m.second << "\n";
      }
      return ss.str();
   };
   auto format = [&dump_headers](const httplib::Request& req, const httplib::Response& res) {
      std::stringstream ss;

      ss << req.method << " " << req.version << " " << req.path;

      char sep = '?';

      for (const auto& p : req.params) {
         ss << sep << p.first << "=" << p.second;
         sep = '&';
      }

      ss << "\n";

      if (verbose_) {
         ss << dump_headers(req.headers);

         ss << "\nresponse: ";
         ss << res.status << " " << res.version << "\n";
         ss << dump_headers(res.headers) << "\n";

         if (!res.body.empty()) {
            ss << res.body << "\n";
         }
      }
      return ss.str();
   };

   http_server.Get(
       "/query", [](const httplib::Request& request, httplib::Response& response) { handle_query(request, response); });

   http_server.Post(
       "/query", [](const httplib::Request& request, httplib::Response& response) { handle_query(request, response); });

   http_server.Put(
       "/query", [](const httplib::Request& request, httplib::Response& response) { handle_query(request, response); });

   http_server.Delete(
       "/query", [](const httplib::Request& request, httplib::Response& response) { handle_query(request, response); });

   http_server.set_exception_handler([](const httplib::Request& req, httplib::Response& res, std::exception& e) {
      std::cout << "Exception: Error 500: " << e.what() << "\n";
      res.status = 500;
      res.set_content(e.what(), "text/plain");
   });

   http_server.set_error_handler([&format](const httplib::Request& req, httplib::Response& res) {
      std::cout << "Error: " << format(req, res);
   });

   http_server.set_logger(
       [&format](const httplib::Request& req, const httplib::Response& res) { std::cout << format(req, res); });
}

void StartServer(httplib::Server& http_server, int port) {
   if (verbose_) {
      const char* chost = getenv("ECF_HOST");
      const char* cport = getenv("ECF_PORT");

      std::string host, port;

      if (chost == nullptr) {
         host = "localhost";
      } else {
         host = std::string(chost);
      }

      if (cport == nullptr) {
         port = "3141";
      } else {
         port = std::string(cport);
      }

      std::cout << "Default location for ecFlow server is " << host << ":" << port << std::endl;
   }

   const std::string proto = (no_ssl ? "http" : "https");

   std::cout << proto << " server listening on port " << port << std::endl;

   http_server.listen("0.0.0.0", port);
}

void HttpServer::run() {
#ifdef ECF_OPENSSL
   if (no_ssl == false) {
      const std::string path_to_cert = std::string(getenv("HOME")) + "/.ecflowrc/ssl/";
      namespace fs = boost::filesystem;

      if (fs::exists(path_to_cert + "server.crt") == false || fs::exists(path_to_cert + "server.key") == false) {
         throw std::runtime_error("Directory " + path_to_cert + " does not contain server.crt and/or server.key");
      }

      const std::string cert = path_to_cert + "server.crt";
      const std::string key = path_to_cert + "server.key";

      httplib::SSLServer http_server(cert.c_str(), key.c_str());
      ApplyListeners(dynamic_cast<httplib::Server&>(http_server));
      StartServer(http_server, port);
   }
#endif

   httplib::Server http_server;

   ApplyListeners(http_server);
   StartServer(http_server, port);
}
