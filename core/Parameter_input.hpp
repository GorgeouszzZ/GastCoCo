#pragma once
#include <gflags/gflags.h>
#include "omp.h"

static bool ValidateApp(const char* flagname, const std::string& value)
{
   if (value == "pagerank" || value == "pr")
        return true;
   if (value == "sssp" || value == "sp")
        return true;
   if (value == "query" || value == "q")
        return true;
   if (value == "cc")
        return true;
   if (value == "bfs")
        return true;
   if (value == "lp")
        return true;
   if (value == "individual_update" || value == "iu")
        return true;
   if (value == "batch_update" || value == "bu")
        return true;
   printf("Please enter the correct graph app name: query(q), pagerank(pr), sssp(sp), cc, bfs, lp, individual_update(iu), batch_update(bu).\n");
   return false; 
}

static bool ValidatePath(const char* flagname, const std::string& value)
{
   if (value != "")
        return true;
   printf("Please enter the graph data path.\n");
   return false; 
}

static bool ValidatePositiveNumber(const char* flagname, int32_t value)
{
   if (value > 0)
   {
      if (flagname == "t" && value > omp_get_max_threads())
         printf("NOTE: The number of threads is currently greater than the number of logical threads of the CPU.\n");
      return true;
   }
   printf("Please enter the positive number.\n");
   return false; 
}

static bool ValidatePositiveNumberAndZero(const char* flagname, int32_t value)
{
   if (value >= 0)
      return true;
   printf("Please enter the positive number or 0.\n");
   return false; 
}

DEFINE_string(app, "", "Graph application.");
DEFINE_string(data, "", "Graph dataset information path.");
DEFINE_int32(t, omp_get_max_threads(), "Thread count, default = Number of logical CPU threads.");
DEFINE_int32(c, 2, "Coroutine count.");
DEFINE_int32(s, 0, "Source in some apps.");
DEFINE_int32(bs, 10000, "Batch size in batch update.");
DEFINE_bool(em, false, "Execute mode: true = Sequential, false = Interleaved.");
DEFINE_bool(cm, false, "Compute mode: true = pull/push, false = pull+push.");

static const bool app_dummy = google::RegisterFlagValidator(&FLAGS_app, &ValidateApp);
static const bool data_dummy = google::RegisterFlagValidator(&FLAGS_data, &ValidatePath);
static const bool t_dummy = google::RegisterFlagValidator(&FLAGS_t, &ValidatePositiveNumber);
static const bool c_dummy = google::RegisterFlagValidator(&FLAGS_c, &ValidatePositiveNumber);
static const bool s_dummy = google::RegisterFlagValidator(&FLAGS_s, &ValidatePositiveNumberAndZero);
static const bool bs_dummy = google::RegisterFlagValidator(&FLAGS_bs, &ValidatePositiveNumber);