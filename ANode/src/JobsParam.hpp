#ifndef JOBSPARAM_HPP_
#define JOBSPARAM_HPP_
//============================================================================
// Name        : time
// Author      : Avi
// Revision    : $Revision: #14 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/noncopyable.hpp>
#include "NodeFwd.hpp"

// Used as a utility class for controlling job creation.
// Collates data during the node tree traversal
// Note: For testing purposes we do not always want to create jobs or spawn jobs
class JobsParam : private boost::noncopyable {
public:
   // This constructor is used in test
	JobsParam(bool createJobs = false)
      : timed_out_of_job_generation_(false),
        createJobs_(createJobs), spawnJobs_(false), submitJobsInterval_(60){}

	JobsParam(int submitJobsInterval, bool createJobs, bool spawn_jobs = true)
	   : timed_out_of_job_generation_(false),
	     createJobs_(createJobs),spawnJobs_(spawn_jobs), submitJobsInterval_(submitJobsInterval)
	   { if (!createJobs_) spawnJobs_ = false;}

	std::string& errorMsg() { return errorMsg_;}
	const std::string& getErrorMsg() const { return errorMsg_;}

	void push_back_submittable(Submittable* t) { submitted_.push_back(t); }
	const std::vector<Submittable*>& submitted() const { return submitted_;}

	bool createJobs() const { return createJobs_;}
	bool spawnJobs() const { return spawnJobs_;}

	/// returns the number of seconds at which we should check time dependencies
	/// this includes evaluating trigger dependencies and submit the corresponding jobs.
	/// This is set at 60 seconds. But will vary for debug purposes only.
	int submitJobsInterval() const { return submitJobsInterval_;}

	/// Allow user to set the debug message that appears in log file when job submission starts
	void logDebugMessage(const std::string& s) { debugMsg_ = s;}
	const std::string& logDebugMessage() const { return debugMsg_;}

	void set_user_edit_variables(const NameValueMap& v) { user_edit_variables_ = v;}
	const NameValueMap& user_edit_variables() const { return user_edit_variables_;}

	void set_user_edit_file(const std::vector<std::string>& file) { user_edit_file_ = file;}
	const std::vector<std::string>& user_edit_file() const { return user_edit_file_; }

	// Functions to aid timing of job generation
   void set_next_poll_time(const boost::posix_time::ptime& next_poll_time) { next_poll_time_ = next_poll_time;}
   const boost::posix_time::ptime&  next_poll_time() const { return next_poll_time_;}
   const boost::posix_time::ptime&  time_out_time() const { return time_out_time_;}
   void set_timed_out_of_job_generation(const boost::posix_time::ptime& t) { time_out_time_ = t; timed_out_of_job_generation_ = true;}
   bool timed_out_of_job_generation() const { return timed_out_of_job_generation_; }

   // ensure that we avoid job generation close the server poll time.
   bool check_for_job_generation_timeout();

private:
   bool timed_out_of_job_generation_;
	bool createJobs_;
	bool spawnJobs_;
	int  submitJobsInterval_;
	std::string errorMsg_;
	std::string debugMsg_;
	std::vector<Submittable*> submitted_;
	std::vector<std::string> user_edit_file_;
	NameValueMap user_edit_variables_;          // Used for User edit
	boost::posix_time::ptime next_poll_time_;   // Aid early exit from job generation, if it takes to long
	boost::posix_time::ptime time_out_time_;    // When we actually timed out must >= next_poll_time_
};
#endif
