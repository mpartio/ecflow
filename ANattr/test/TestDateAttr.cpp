
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <stdexcept>
#include<iostream>

#include <boost/test/unit_test.hpp>

#include "DateAttr.hpp"
#include "Str.hpp"
#include "PrintStyle.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_date )
{
   cout << "ANattr:: ...test_date\n";
   {
      DateAttr empty;
      DateAttr empty2;
      BOOST_CHECK_MESSAGE(empty == empty2,"Equality failed");
      BOOST_CHECK_MESSAGE(empty.day() == 0 && empty.month() == 0 && empty.year() == 0,"");
   }
   {
      for(int day = 0; day < 28; day++) {
         for(int month = 0; month < 13; month++) {
            int year = 2017;
            if (month == 0) year = 0;
            std::stringstream ss;
            if (day == 0) ss << "*";
            else          ss << day;
            ss << ".";
            if (month == 0) ss << "*";
            else            ss << month;
            ss << ".";
            if (year == 0) ss << "*";
            else           ss << year;

            DateAttr date1(day,month,year);
            DateAttr date2(ss.str());
            BOOST_CHECK_MESSAGE(date1 == date2,"Equality failed");
            BOOST_CHECK_MESSAGE(date1.name() == date2.name(),"name failed");
         }
      }
   }
}

static DateAttr print_and_parse_attr(DateAttr & date){

   PrintStyle style(PrintStyle::MIGRATE);
   std::string output;
   date.print(output);
   output.erase( output.begin() +  output.size()-1); // remove trailing newline

   std::vector<std::string> tokens;
   Str::split_orig(output,tokens);

   return DateAttr::create(tokens,true/*read state*/);
}


BOOST_AUTO_TEST_CASE( test_date_parsing ) {

   cout << "ANattr:: ...test_date_parsing\n";
   {
      DateAttr date("12.12.2019");
      date.setFree();
      DateAttr parsed_date  = print_and_parse_attr(date);

      BOOST_CHECK_MESSAGE(date == parsed_date,"Parse failed expected " << date.dump() << " but found " << parsed_date.dump());
   }
   {
      DateAttr date("12.12.2019");
      date.setFree();
      DateAttr parsed_date  = print_and_parse_attr(date);

      BOOST_CHECK_MESSAGE(date == parsed_date,"Parse failed expected " << date.dump() << " but found " << parsed_date.dump());
   }
   {
      DateAttr date("12.12.2019");
      DateAttr parsed_date  = print_and_parse_attr(date);

      BOOST_CHECK_MESSAGE(date == parsed_date,"Parse failed expected " << date.dump() << " but found " << parsed_date.dump());
   }
}

BOOST_AUTO_TEST_CASE( test_date_errors )
{
   cout << "ANattr:: ...test_date_errors\n";
   {
      BOOST_REQUIRE_THROW(DateAttr("-1.2.*"), std::runtime_error);
      BOOST_REQUIRE_THROW(DateAttr("32.2.*"), std::runtime_error );
      BOOST_REQUIRE_THROW(DateAttr("1.-1.*"), std::runtime_error );
      BOOST_REQUIRE_THROW(DateAttr("1.13.*"), std::runtime_error );
      BOOST_REQUIRE_THROW(DateAttr("1.13.-1"), std::runtime_error );
      BOOST_REQUIRE_THROW(DateAttr("1.13.99999999"), std::runtime_error );
   }
}
BOOST_AUTO_TEST_SUITE_END()

