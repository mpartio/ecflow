//============================================================================
// Name        : ClientOptions
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Delegates argument parsing to the registered commands
//============================================================================
#include <stdexcept>
#include <iostream>
#include <iomanip>

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "ClientOptions.hpp"
#include "ClientEnvironment.hpp"
#include "Version.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Child.hpp"
#include "TaskApi.hpp"

using namespace std;
using namespace ecf;
using namespace boost;
namespace po = boost::program_options;

static const char* client_env_description();
static const char* client_task_env_description();
static std::string print_variable_map(const boost::program_options::variables_map& vm);


ClientOptions::ClientOptions()
{
   // This could have been moved to parse(). However since the same ClienttInvoker can be
   // be used for multiple commands. We have separated out the parts the need only be done once.
   // hence improving the performance:
   std::string title_help = "Client options, ";
   title_help += Version::description();
   title_help += "   ";
   desc_ = new po::options_description( title_help , po::options_description::m_default_line_length + 80 );

   // This will iterate over all the registered client to server commands and
   // Each command will add to the option description, its required arguments
   cmdRegistry_.addAllOptions(*desc_);

   // Allow the host,port and rid to be  overridden by the command line
   // This allows the jobs, which make other calls to ecflow_client from interfering with each other
   desc_->add_options()("rid",po::value< string >()->implicit_value( string("") ),
            "rid: If specified will override the environment variable ECF_RID, Can only be used for child commands");
   desc_->add_options()("port",po::value< string >()->implicit_value( string("") ),
            "port: If specified will override the environment variable ECF_PORT and default port number of 3141");
   desc_->add_options()("host",po::value< string >()->implicit_value( string("") ),
            "host: If specified will override the environment variable ECF_HOST and default host, localhost");
   desc_->add_options()("user",po::value< string >()->implicit_value( string("") ),
            "user: The user name to be used when contacting the server. Can only be used when password is also specified");
   desc_->add_options()("password",po::value< string >()->implicit_value( string("") ),
            "password: The password to be used when contacting the server");
   desc_->add_options()("token",po::value< string >()->implicit_value( string("") ),
            "token: The authentication token to be used when contacting the server");
#ifdef ECF_OPENSSL
   desc_->add_options()("ssl","ssl: If specified will override the environment variable ECF_SSL");
#endif
}

ClientOptions::~ClientOptions()
{
   delete desc_;
}

Cmd_ptr ClientOptions::parse(int argc, char* argv[],ClientEnvironment* env) const
{
   // We expect two hyphen/minus, However sometime we get a weird/rogue kind of hyphen
   // This rogue hyphen can mess up the parsing.
   // # ecflow_client ––group="halt=yes; check_pt; terminate=yes"  // *BAD* hyphens 2 of them
   // # ecflow_client –-group="halt=yes; check_pt; terminate=yes"  // *BAD* hyphens 1 of them, i.e. first
   // # ecflow_client --group="halt=yes; check_pt; terminate=yes"  // *GOOD*
   //
   //   dec:  -30 ffffffe2 37777777742 \342
   //   hex: -128 ffffff80 37777777600 \200
   //   oct: -109 ffffff93 37777777623 \223
   //
   // The correct hyphen has:
   //   dec:45 hex:2D oct:55 -
   if (env->debug()) {
      cout <<  "  ClientOptions::parse argc=" << argc;
      for(int i = 0; i < argc; i++) { cout << "  arg" << i << "=" << argv[i];}
      cout << "\n";
      std::cout << "  help column width = " << po::options_description::m_default_line_length + 80 << "\n";
   }

   // parse arguments into 'vm'.
   //       --alter delete cron -w 0,1 10:00 /s1     # -w treated as option
   //       --alter=/s1 change meter name -1         # -1 treated as option
   // Note: negative numbers get treated as options: i.e trying to change meter value to a negative number
   //       To avoid negative numbers from being treated as option use, we need to change command line style:
   //       po::command_line_style::unix_style ^ po::command_line_style::allow_short
   boost::program_options::variables_map vm;
   po::store( po::parse_command_line( argc, argv, *desc_ ,po::command_line_style::unix_style ^ po::command_line_style::allow_short), vm );
   po::notify( vm );


   // Check to see if host or port, specified. This will override the environment variables
   std::string host,port;
   if ( vm.count( "port" ) ) {
      port = vm[ "port" ].as< std::string > ();
      if (env->debug())  std::cout << "  port " << port << " overridden at the command line\n";
      try { boost::lexical_cast< int >( port );}
      catch ( boost::bad_lexical_cast& e ) {
         std::stringstream ss; ss << "ClientOptions::parse: The specified port(" << port << ") must be convertible to an integer";
         throw std::runtime_error( ss.str() );
      }
   }
   if ( vm.count( "host" ) ) {
      host = vm[ "host" ].as< std::string > ();
      if (env->debug())  std::cout << "   host " << host << " overridden at the command line\n";
   }
   if (!host.empty() || !port.empty()) {
      if (host.empty()) host = env->hostSpecified();       // get the environment variable ECF_HOST
      if (port.empty()) port = env->portSpecified();       // get the environment variable ECF_PORT || Str::DEFAULT_PORT_NUMBER()
      if (host.empty()) host = Str::LOCALHOST();           // if ECF_HOST not specified default to localhost
      if (port.empty()) port = Str::DEFAULT_PORT_NUMBER(); // if ECF_PORT not specified use default
      env->set_host_port(host,port);
   }
   if ( vm.count( "rid" ) ) {
      std::string rid = vm[ "rid" ].as< std::string > ();
      if (env->debug())  std::cout << "  rid " << rid << " overridden at the command line\n";
      env->set_remote_id(rid);
   }
   if (( vm.count( "user" ) || vm.count( "password" )) && vm.count( "token" )) {
      throw std::runtime_error("Both user&password and token cannot be specified at the same time");
   }
   if ( vm.count( "user" ) ) {
       std::string user = vm[ "user" ].as< std::string > ();
       if (env->debug())  std::cout << "  user " << user << " overridden at the command line\n";
       env->set_user_name(user);
    }
#if 0
   if ( vm.count( "password" ) ) {
       std::string password = vm[ "password" ].as< std::string > ();
       if (env->debug())  std::cout << "  password overridden at the command line\n";
       env->set_password(crypt(password.c_str(), env->get_user_name().c_str()));
    }
#endif
   if ( vm.count( "token" ) ) {
       std::string token = vm[ "token" ].as< std::string > ();
       if (env->debug())  std::cout << "  token given at the command line\n";
       env->set_token(token);
    }

#ifdef ECF_OPENSSL
   if ( vm.count( "ssl" )) {
       if (env->debug()) std::cout << "  ssl set via command line\n";
       env->enable_ssl();
   }
#endif

   // Defer the parsing of the command , to the command. This allows
   // all cmd functionality to be centralised with the command
   // This can throw std::runtime_error if arg's don't parse
   Cmd_ptr client_request;
   if ( ! cmdRegistry_.parse( client_request, vm, env) ) {

      // The arguments did *NOT* match with any of the registered command.
      // Hence if arguments don't match help, debug or version its an error
      // Note: we did *NOT* check for a NULL client_request since *NOT* all
      //       request need to create it. Some commands are client specific.
      //       For example:
      //         --server_load         // this is sent to server
      //         --server_load=<path>  // no command returned, command executed by client
      if ( vm.count( "help" ) )   {
         string help_cmd = vm[ "help" ].as< std::string > ();
         show_help(help_cmd);
         return client_request;
      }

      if ( vm.count( "debug" ) )  {
         cout << env->toString() << "\n";
         return client_request;
      }

      if ( vm.count( "version" ) )  {
         cout << Version::description()  << "\n";
         return nullptr;
      }

      std::stringstream ss;
      ss << print_variable_map(vm) << "\n";
      ss << "ClientOptions::parse: Arguments did not match any commands.\n";
      ss << "  argc=" << argc << "\n";
      for(int i = 0; i < argc; i++) {
         ss << "  arg" << i << "=" << argv[i];

         std::string str = argv[i];
         for(size_t s=0; s < str.size(); s++) {
            if (static_cast<int>(str[s]) < 0 || static_cast<int>(str[s]) > 127) {
               ss << "\nUnrecognised character not in ASCII range(0-127) " << std::dec << "dec(" << static_cast<int>(str[s]) << ") char:" << str[s];
               ss << " found at index " << s << " for string '" << str << "'\n";
               if ( static_cast<int>(str[s]) == -30) ss << "check for bad hyphen/minus";
               throw std::runtime_error(ss.str());
            }
         }
      }
      ss << "\nUse --help to see all the available commands\n";
      throw std::runtime_error(ss.str());
   }

   return client_request;
}

void ClientOptions::show_help(const std::string & help_cmd) const
{
   // WARNING: This assumes that there are no user/child commands with name 'summary','all','child','user'
   if (help_cmd.empty()) {

      cout << "\nClient/server based work flow package:\n\n";
      cout << Version::description() << "\n\n";
      cout << Ecf::CLIENT_NAME() << " provides the command line interface, for interacting with the server:\n";

      cout << "Try:\n\n";
      cout << "   " << Ecf::CLIENT_NAME() << " --help=all       # List all commands, verbosely\n";
      cout << "   " << Ecf::CLIENT_NAME() << " --help=summary   # One line summary of all commands\n";
      cout << "   " << Ecf::CLIENT_NAME() << " --help=child     # One line summary of child commands\n";
      cout << "   " << Ecf::CLIENT_NAME() << " --help=user      # One line summary of user command\n";
      cout << "   " << Ecf::CLIENT_NAME() << " --help=<cmd>     # Detailed help on each command\n\n";

      show_all_commands("Commands:");
    }
    else {
       if (help_cmd == "all") cout << *desc_ << "\n";
       else if (help_cmd == "summary") show_cmd_summary("\nEcflow client commands:\n");
       else if (help_cmd == "child")   show_cmd_summary("\nEcflow child client commands:\n","child");
       else if (help_cmd == "user")    show_cmd_summary("\nEcflow user client commands:\n","user");
       else {
          // Help on individual command
          const po::option_description* od = desc_->find_nothrow(help_cmd,
                                                   true,  /*approx, will find nearest match*/
                                                   false, /*long_ignore_case = false*/
                                                   false  /*short_ignore_case = false*/
                                                   );
//               cout << "long_name = " << od.long_name() << "\n";
//               cout << "format_name = " << od.format_name() << "\n";
//               cout << "format_parameter = " << od.format_parameter() << "\n";
          if (od) {
             cout << "\n";
             cout << od->long_name() << "\n";
             for(size_t i =0; i< od->long_name().size(); i++)  cout << "-";
             cout << "\n\n";
             cout << od->description() << "\n\n";
             cout << client_env_description();
             if ( od->long_name() == TaskApi::initArg()     ||
                  od->long_name() == TaskApi::completeArg() ||
                  od->long_name() == TaskApi::abortArg()    ||
                  od->long_name() == TaskApi::waitArg()     ||
                  od->long_name() == TaskApi::eventArg()    ||
                  od->long_name() == TaskApi::labelArg()    ||
                  od->long_name() == TaskApi::queue_arg()   ||
                  od->long_name() == TaskApi::meterArg()) {
                cout << "\n";
                cout << client_task_env_description();
             }
          }
          else {
             show_all_commands("No matching command found, please choose from:");
          }
       }
    }
}

void ClientOptions::show_all_commands(const char* title) const
{
   cout << title << "\n";
   // take a copy, since we need to sort
   std::vector< boost::shared_ptr<po::option_description> > options = desc_->options();

   // sort using long_name
   using opt_desc = boost::shared_ptr<po::option_description>;
   std::sort(options.begin(),options.end(),
             [](const opt_desc& a,const opt_desc& b) { return a->long_name() < b->long_name();}
   );


   size_t vec_size = options.size();
   size_t max_width = 0;
   for(size_t i = 0; i < vec_size; i++) { max_width = std::max(max_width,options[i]->long_name().size()); }
   max_width += 1;
   for(size_t i = 0; i < vec_size; i++) {
      if (i == 0 || i % 5 == 0) cout << "\n   ";
      cout << left << std::setw(max_width) << options[i]->long_name();
   }
   cout << "\n";
}

void ClientOptions::show_cmd_summary(const char *title,const std::string& user_or_child) const
{
   assert(user_or_child.empty() || user_or_child == "child" || user_or_child == "user");
   cout << title << "\n";

   // take a copy, since we need to sort
   std::vector< boost::shared_ptr<po::option_description> > options = desc_->options();

   // sort using long_name
   using opt_desc = boost::shared_ptr<po::option_description>;
   std::sort(options.begin(),options.end(),
             [](const opt_desc& a,const opt_desc& b) { return a->long_name() < b->long_name();}
   );

   size_t vec_size = options.size();
   size_t max_width = 0;
   for(size_t i = 0; i < vec_size; i++) { max_width = std::max(max_width,options[i]->long_name().size()); }
   max_width += 1;
   for(size_t i = 0; i < vec_size; i++) {

      if (user_or_child == "child" && Child::valid_child_cmd(options[i]->long_name())) {
         std::vector< std::string > lines;
         Str::split(options[i]->description(),lines,"\n");
         if (!lines.empty()) {
            cout << "  " << left << std::setw(max_width) << options[i]->long_name() << " ";
            cout << "child  ";
            cout << lines[0] << "\n";
         }
      }
      else if (user_or_child == "user" && !Child::valid_child_cmd(options[i]->long_name())) {
         std::vector< std::string > lines;
         Str::split(options[i]->description(),lines,"\n");
         if (!lines.empty()) {
            cout << "  " << left << std::setw(max_width) << options[i]->long_name() << " ";
            cout << "user   ";
            cout << lines[0] << "\n";
         }
      }
      else if (user_or_child .empty()) {
         std::vector< std::string > lines;
         Str::split(options[i]->description(),lines,"\n");
         if (!lines.empty()) {
            cout << "  " << left << std::setw(max_width) << options[i]->long_name() << " ";
            if (Child::valid_child_cmd(options[i]->long_name())) cout << "child  ";
            else cout << "user   ";
            cout << lines[0] << "\n";
         }
      }
   }
   cout << "\n";
}

const char* client_env_description() {
   return
            "The client reads in the following environment variables. These are read by user and child command\n\n"
            "|----------|----------|------------|-------------------------------------------------------------------|\n"
            "| Name     |  Type    | Required   | Description                                                       |\n"
            "|----------|----------|------------|-------------------------------------------------------------------|\n"
            "| ECF_HOST | <string> | Mandatory* | The host name of the main server. defaults to 'localhost'         |\n"
            "| ECF_PORT |  <int>   | Mandatory* | The TCP/IP port to call on the server. Must be unique to a server |\n"
#ifdef ECF_OPENSSL
            "| ECF_SSL  |  <any>   | Optional*  | Enable encrypted comms with SSL enabled server.                   |\n"
#endif
            "|----------|----------|------------|-------------------------------------------------------------------|\n\n"
            "* The host and port must be specified in order for the client to communicate with the server, this can \n"
            "  be done by setting ECF_HOST, ECF_PORT or by specifying --host=<host> --port=<int> on the command line\n"
            ;
}

const char* client_task_env_description()
{
   return
            "The following environment variables are specific to child commands.\n"
            "The scripts should export the mandatory variables. Typically defined in the head/tail includes files\n\n"
            "|--------------|----------|-----------|---------------------------------------------------------------|\n"
            "| Name         |  Type    | Required  | Description                                                   |\n"
            "|--------------|----------|-----------|---------------------------------------------------------------|\n"
            "| ECF_NAME     | <string> | Mandatory | Full path name to the task                                    |\n"
            "| ECF_PASS     | <string> | Mandatory | The jobs password, allocated by server, then used by server to|\n"
            "|              |          |           | authenticate client request                                   |\n"
            "| ECF_TRYNO    |  <int>   | Mandatory | The number of times the job has run. This is allocated by the |\n"
            "|              |          |           | server, and used in job/output file name generation.          |\n"
            "| ECF_RID      | <string> | Mandatory | The process identifier. Helps zombies identification and      |\n"
            "|              |          |           | automated killing of running jobs                             |\n"
            "| ECF_TIMEOUT  |  <int>   | optional  | Max time in *seconds* for client to deliver message to main   |\n"
            "|              |          |           | server. The default is 24 hours                               |\n"
            "| ECF_HOSTFILE | <string> | optional  | File that lists alternate hosts to try, if connection to main |\n"
            "|              |          |           | host fails                                                    |\n"
            "| ECF_DENIED   |  <any>   | optional  | Provides a way for child to exit with an error, if server     |\n"
            "|              |          |           | denies connection. Avoids 24hr wait. Note: when you have      |\n"
            "|              |          |           | hundreds of tasks, using this approach requires a lot of      |\n"
            "|              |          |           | manual intervention to determine job status                   |\n"
            "| NO_ECF       |  <any>   | optional  | If set exit's ecflow_client immediately with success. This    |\n"
            "|              |          |           | allows the scripts to be tested independent of the server     |\n"
            "|--------------|----------|-----------|---------------------------------------------------------------|\n"
            ;
}


static std::string print_variable_map(const boost::program_options::variables_map& vm) {
   std::stringstream ss; ss << "boost::program_options::variables_map:    vm.size() " << vm.size() << "\n";
    for (po::variables_map::const_iterator it = vm.begin(); it != vm.end(); it++) {
        std::cout << "> " << it->first;
        if (((boost::any)it->second.value()).empty())            ss << "(empty)";
        if (vm[it->first].defaulted() || it->second.defaulted()) ss << "(default)";

        ss << "=";

        bool is_char;
        try {
            boost::any_cast<const char *>(it->second.value());
            is_char = true;
        } catch (const boost::bad_any_cast &) {
            is_char = false;
        }
        bool is_str;
        try {
            boost::any_cast<std::string>(it->second.value());
            is_str = true;
        } catch (const boost::bad_any_cast &) {
            is_str = false;
        }

        if (((boost::any)it->second.value()).type() == typeid(int)) {
            ss << vm[it->first].as<int>() << std::endl;
        } else if (((boost::any)it->second.value()).type() == typeid(bool)) {
            ss << vm[it->first].as<bool>() << std::endl;
        } else if (((boost::any)it->second.value()).type() == typeid(double)) {
            ss << vm[it->first].as<double>() << std::endl;
        } else if (is_char) {
            ss << vm[it->first].as<const char * >() << std::endl;
        } else if (is_str) {
            std::string temp = vm[it->first].as<std::string>();
            if (temp.size()) {
                ss << temp << std::endl;
            } else {
                ss << "true" << std::endl;
            }
        } else { // Assumes that the only remainder is vector<string>
            try {
                std::vector<std::string> vect = vm[it->first].as<std::vector<std::string> >();
                size_t i = 0;
                for (std::vector<std::string>::iterator oit=vect.begin(); oit != vect.end(); oit++, ++i) {
                    ss << "\r> " << it->first << "[" << i << "]=" << (*oit) << std::endl;
                }
            } catch (const boost::bad_any_cast &) {
                ss << "UnknownType(" << ((boost::any)it->second.value()).type().name() << ")" << std::endl;
            }
        }
    }
    return ss.str();
}
