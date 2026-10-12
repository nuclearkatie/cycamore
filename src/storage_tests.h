#ifndef STORAGE_TESTS_H_
#define STORAGE_TESTS_H_

#include <gtest/gtest.h>

#include "storage.h"

#include "context.h"
#include "facility_tests.h"
#include "agent_tests.h"

namespace cycamore {
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class StorageTest : public ::testing::Test {
 protected:
  cyclus::TestContext tc_;
  Storage* src_facility_;

  virtual void SetUp();
  virtual void TearDown();
  void InitParameters();
  void SetUpStorage();
  void TestInitState(cycamore::Storage* fac);
  void TestAddMat(cycamore::Storage* fac, 
      cyclus::Material::Ptr mat);
  void TestBuffers(cycamore::Storage* fac, double inv, double 

      proc, double ready, double stocks);
  void TestStocks(cycamore::Storage* fac, cyclus::CompMap v);
  void TestReadyTime(cycamore::Storage* fac, int t);
  void TestCurrentCap(cycamore::Storage* fac, double inv);

  /// A resource object traded out of storage, as recorded in the database
  struct TradedObj {
    int time;
    int res_id;
    double qty;
    std::string pkg;
    bool operator<(const TradedObj& o) const {
      return time != o.time ? time < o.time : res_id < o.res_id;
    }
  };

  /// Seeds a discrete-trading storage's inventory with packaged objects of
  /// 9.7, 9.5 and 4.0 kg (oldest first), runs it against one sink of the given
  /// capacity and returns what the sink received, ordered by time. The
  /// original resource ids are returned in ids.
  static std::vector<TradedObj> RunDiscreteStorage(std::string extra_config,
                                                   bool keep_pkg,
                                                   double sink_cap,
                                                   std::vector<int>& ids);

  std::vector<std::string> in_c1, out_c1;
  std::string in_r1;

  int residence_time;
  double throughput, max_inv_size;
  bool discrete_handling;
  std::string package;
};
} // namespace cycamore
#endif // STORAGE_TESTS_H_

