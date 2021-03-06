// MapReduce library
// Copyright (C) 2009-2013 Craig Henderson.
// cdm.henderson@gmail.com

#ifndef MAPREDUCE_HPP
#define MAPREDUCE_HPP

#ifdef BOOST_MSVC
#   if !defined(__SGI_STL_PORT)
#       pragma message("warning: using STLPort is recommended to avoid STL container performance problems in MSVC supplied libraries.")
#       if _SECURE_SCL
#           pragma message("warning: using MSVC with _SECURE_SCL=1 defined can cause serious runtime performance degradation.")
#       endif
#   endif
#endif

#include <string>
#include <vector>
#include <thread>
#include <cstdint>
#include <boost/config.hpp>

namespace mapreduce {
namespace detail {

class noncopyable {
#if defined(_MSC_VER) && _MSC_VER < 1800
  protected:
    noncopyable() {}
    ~noncopyable() {}

  private:
    noncopyable(noncopyable const &);
    noncopyable(noncopyable &&);
    noncopyable &operator=(noncopyable const &);
    noncopyable &operator=(noncopyable &&);
#else
  protected:
    noncopyable()                               = default;
    ~noncopyable()                              = default;
    noncopyable(noncopyable const &)            = delete;
    noncopyable(noncopyable &&)                 = delete;
    noncopyable &operator=(noncopyable const &) = delete;
    noncopyable &operator=(noncopyable &&)      = delete;
#endif
};

class joined_thread_group : public std::vector<std::thread>
{
  public:
    ~joined_thread_group()
    {
        join_all();
    }

    void join_all(void)
    {
        for (auto &thread : *this)
            thread.join();
    }
};


}   // namespace detail
}   // namespace mapreduce

namespace mapreduce {

struct specification
{
    unsigned       map_tasks;             // ideal number of map tasks to use
    unsigned       reduce_tasks;          // ideal number of reduce tasks to use
    std::uintmax_t max_file_segment_size; // ideal maximum number of bytes in each input file segment
    std::string    output_filespec;       // filespec of the output files - can contain a directory path if required
    std::string    input_directory;       // directory path to scan for input files

    specification()
      : map_tasks(0),                   
        reduce_tasks(1),
        max_file_segment_size(1048576L),    // default 1Mb
        output_filespec("mapreduce_")   
    {
    }
};

struct results
{
    struct tag_counters
    {
        unsigned actual_map_tasks;      // number of map tasks actually used
        unsigned actual_reduce_tasks;   // number of reduce tasks actually used

        // counters for map key processing
        unsigned map_keys_executed;
        unsigned map_key_errors;
        unsigned map_keys_completed;

        // counters for reduce key processing
        unsigned reduce_keys_executed;
        unsigned reduce_key_errors;
        unsigned reduce_keys_completed;

        unsigned num_result_files;      // number of result files created

        tag_counters()
          : actual_map_tasks(0),
            actual_reduce_tasks(0),
            map_keys_executed(0),
            map_key_errors(0),
            map_keys_completed(0),
            reduce_keys_executed(0),
            reduce_key_errors(0),
            reduce_keys_completed(0),
            num_result_files(0)
        {
        }
    } counters;

    std::chrono::duration<double>              job_runtime;
    std::chrono::duration<double>              map_runtime;
    std::chrono::duration<double>              shuffle_runtime;
    std::chrono::duration<double>              reduce_runtime;
    std::vector<std::chrono::duration<double>> map_times;
    std::vector<std::chrono::duration<double>> shuffle_times;
    std::vector<std::chrono::duration<double>> reduce_times;
};

}   // namespace mapreduce

#include <boost/throw_exception.hpp>
#include "detail/platform.hpp"
#include "detail/mergesort.hpp"
#include "detail/intermediates.hpp"
#include "detail/schedule_policy.hpp"
#include "detail/datasource.hpp"
#include "detail/null_combiner.hpp"
#include "detail/job.hpp"

namespace mapreduce {

template<typename Job>
void run(mapreduce::specification &spec, mapreduce::results &result)
{
    typename Job::datasource_type datasource(spec);
    Job job(datasource, spec);
    job.run<mapreduce::schedule_policy::cpu_parallel<Job> >(result);
}

}   // namespace mapreduce

#endif  // MAPREDUCE_HPP
