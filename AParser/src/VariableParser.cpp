//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "VariableParser.hpp"
#include "Node.hpp"
#include "Str.hpp"
#include "Defs.hpp"

using namespace ecf;
using namespace std;

bool VariableParser::doParse(
         const std::string& line,
         std::vector<std::string >& lineTokens )
{
   // Note: For migrate the defs can have variables
   Node* node =  NULL;
   if ( nodeStack().empty()) {
      if (!parsing_defs_) throw std::runtime_error("VariableParser::doParse: Could not add variable, as node stack is empty at line: " + line );
   }
   else node = nodeStack_top();

   size_t line_tokens_size = lineTokens.size();
   if ( line_tokens_size < 3 ) {
      std::stringstream ss;
      ss << "VariableParser::doParse: expected at least 3 tokens, found " << line_tokens_size << " on line:" << line << "\n";
      if (node) ss << "At node: " << node->debugNodePath() << "\n";
      throw std::runtime_error(ss.str());
   }

   // There is no need check for '#' comment character in variable name
   // since the variable constructor will check for this
   // i.e  edit #var value
   //      edit var# value

   // Make sure value does not begin with '#' comment character
   if ( lineTokens[2][0] == '#') {
      // edit fred #comment
      // edit fred #
      std::stringstream ss;
      ss << "VariableParser::doParse: Expected value but found comment at line:" << line << "\n";
      if (node) ss << "At node: " << node->debugNodePath() << "\n";
      throw std::runtime_error(ss.str());
   }


   // ** For aliases, the variables may be **different** to normal variables in that they may contain a ":" & $
   // ** This is **not** allowed in normal variables.
   // ** i.e it allows for  %A:1%, %A:2%, %A:3%
   // ** This is not really recommended but its what the old system supported.
   // ** Hence the variable construction by-passes variable name checking ***

   // Note:
   // edit OWNER 'fred'               => value = fred
   // edit OWNER 'fred and "ginger"'  => value = fred and "ginger"
   // edit OWNER ""                   => value =
   // edit OWNER ''                   => value =
   // edit OWNER '"fred"'             => value = "fred"   * quotes are preserved *
   // edit OWNER "'fred'"             => value = fred     * tick are not preserved *
   if ( line_tokens_size == 3 ) {
      // The order of removing double quotes and then single quotes is significant here
      Str::removeQuotes(lineTokens[2]);       // if first *and* last character is "
      Str::removeSingleQuotes(lineTokens[2]); // if first *and* last character is '
      if (node) {
         if (node->isAlias()) node->addVariable( Variable( lineTokens[1], lineTokens[2],false )); // bypass name checking
         else node->addVariable( Variable( lineTokens[1], lineTokens[2] ));
      }
      else defsfile()->set_server().add_or_update_user_variables(lineTokens[1], lineTokens[2]);
      return true;
   }

   // i.e
   //  0     1         2
   // edit var_name "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%"  #fred
   std::string value; value.reserve(line.size()-4);
   for (size_t i = 2; i < line_tokens_size; ++i) {
      if ( lineTokens[i].at( 0 ) == '#' ) break;
      if ( i != 2 ) value += " ";
      value += lineTokens[i];
   }

   Str::removeQuotes(value);
   Str::removeSingleQuotes(value);
   if (node) {
      if (node->isAlias()) node->addVariable( Variable( lineTokens[1], value, false )); // bypass name checking
      else                 node->addVariable( Variable( lineTokens[1], value )) ;
   }
   else defsfile()->set_server().add_or_update_user_variables(lineTokens[1], value);

   return true;
}
