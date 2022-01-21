#include "deploy_manager_inst_tests.h"
// make sure that the deployed agent's prototype name is identical to the
// originally specified prototype name - this is important to test because
// DeployManagerInst does some mucking around with registering name-modded prototypes
// in order to deal with lifetime setting.

using cyclus::QueryResult;

void DeployManagerInstTests::SetUp() {
  ctx_ = new cyclus::Context(&ti_, &rec_);
  src_inst = new cycamore::DeployManagerInst(ctx_);
  producer = new TestProducer(ctx_);
  commodity = cyclus::toolkit::Commodity("commod");
  capacity = 5;
  producer->cyclus::toolkit::CommodityProducer::Add(commodity);
  producer->SetCapacity(commodity, capacity);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeployManagerInstTests::TearDown() {
  delete producer;
  delete src_inst;
  delete ctx_;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(DeployManagerInstTests, ProtoNames) {
  std::string config =
     "<prototypes>  <val>foobar</val> </prototypes>"
     "<build_times> <val>1</val>      </build_times>"
     "<n_build>     <val>3</val>      </n_build>"
     "<lifetimes>   <val>2</val>      </lifetimes>"
     ;

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cycamore:DeployManagerInst"), config, simdur);
  sim.DummyProto("foobar");
  int id = sim.Run();

  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'foobar';"
      );
  stmt->Step();
  EXPECT_EQ(3, stmt->GetInt(0));
}

TEST_F(DeployManagerInstTests, BuildTimes) {
  std::string config =
     "<prototypes>  <val>foobar</val> <val>foobar</val> </prototypes>"
     "<build_times> <val>1</val>      <val>3</val>      </build_times>"
     "<n_build>     <val>1</val>      <val>7</val>      </n_build>"
     ;

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cycamore:DeployManagerInst"), config, simdur);
  sim.DummyProto("foobar");
  int id = sim.Run();

  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'foobar' AND EnterTime = 1;"
      );
  stmt->Step();
  EXPECT_EQ(1, stmt->GetInt(0));

  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'foobar' AND EnterTime = 3;"
      );
  stmt->Step();
  EXPECT_EQ(7, stmt->GetInt(0));
}

// make sure that specified lifetimes are honored both in agent's table record
// and in decommissioning.
TEST_F(DeployManagerInstTests, FiniteLifetimes) {
  std::string config =
     "<prototypes>  <val>foobar</val> <val>foobar</val> <val>foobar</val> </prototypes>"
     "<build_times> <val>1</val>      <val>1</val>      <val>2</val>      </build_times>"
     "<n_build>     <val>1</val>      <val>7</val>      <val>3</val>      </n_build>"
     "<lifetimes>   <val>1</val>      <val>2</val>      <val>-1</val>     </lifetimes>"
     ;

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cycamore:DeployManagerInst"), config, simdur);
  sim.DummyProto("foobar");
  int id = sim.Run();

  // check agent deployment
  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'foobar' AND EnterTime = 1 AND Lifetime = 1;"
      );
  stmt->Step();
  EXPECT_EQ(1, stmt->GetInt(0));

  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'foobar' AND EnterTime = 1 AND Lifetime = 2;"
      );
  stmt->Step();
  EXPECT_EQ(7, stmt->GetInt(0));

  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'foobar' AND EnterTime = 2 AND Lifetime = -1;"
      );
  stmt->Step();
  EXPECT_EQ(3, stmt->GetInt(0));

  // check decommissioning
  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry As e JOIN AgentExit AS x ON x.AgentId = e.AgentId WHERE e.Prototype = 'foobar' AND x.ExitTime = 1;"
      );
  stmt->Step();
  EXPECT_EQ(1, stmt->GetInt(0));

  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry As e JOIN AgentExit AS x ON x.AgentId = e.AgentId WHERE e.Prototype = 'foobar' AND x.ExitTime = 2;"
      );
  stmt->Step();
  EXPECT_EQ(7, stmt->GetInt(0));

  // agent with -1 lifetime should not be in AgentExit table
  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentExit;"
      );
  stmt->Step();
  EXPECT_EQ(8, stmt->GetInt(0));
}

TEST_F(DeployManagerInstTests, NoDupProtos) {
  std::string config =
     "<prototypes>  <val>foobar</val> <val>foobar</val> <val>foobar</val> </prototypes>"
     "<build_times> <val>1</val>      <val>1</val>      <val>2</val>      </build_times>"
     "<n_build>     <val>1</val>      <val>7</val>      <val>3</val>      </n_build>"
     "<lifetimes>   <val>1</val>      <val>1</val>      <val>-1</val>     </lifetimes>"
     ;

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cycamore:DeployManagerInst"), config, simdur);
  sim.DummyProto("foobar");
  int id = sim.Run();

  // don't duplicate same prototypes with same custom lifetime
  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM Prototypes WHERE Prototype = 'foobar_life_1';"
      );
  stmt->Step();
  EXPECT_EQ(1, stmt->GetInt(0));

  // don't duplicate custom lifetimes that are identical to original prototype
  // lifetime.
  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM Prototypes WHERE Prototype = 'foobar';"
      );
  stmt->Step();
  EXPECT_EQ(1, stmt->GetInt(0));
}

TEST_F(DeployManagerInstTests, PositionInitialize) {
  std::string config =
     "<prototypes>  <val>foobar</val> </prototypes>"
     "<build_times> <val>1</val>      </build_times>"
     "<n_build>     <val>3</val>      </n_build>"
     "<lifetimes>   <val>2</val>      </lifetimes>";

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cycamore:DeployManagerInst"), config, simdur);
  sim.DummyProto("foobar");
  int id = sim.Run();

  QueryResult qr = sim.db().Query("AgentPosition", NULL);
  EXPECT_EQ(qr.GetVal<double>("Latitude"), 0.0);
  EXPECT_EQ(qr.GetVal<double>("Longitude"), 0.0);
}

TEST_F(DeployManagerInstTests, PositionInitialize2) {
  std::string config =
     "<prototypes>  <val>foobar</val> </prototypes>"
     "<longitude>   -20.0             </longitude>"
     "<latitude>    2.0               </latitude>"
     "<build_times> <val>1</val>      </build_times>"
     "<n_build>     <val>3</val>      </n_build>"
     "<lifetimes>   <val>2</val>      </lifetimes>";

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cycamore:DeployManagerInst"), config, simdur);
  sim.DummyProto("foobar");
  int id = sim.Run();

  QueryResult qr = sim.db().Query("AgentPosition", NULL);
  EXPECT_EQ(qr.GetVal<double>("Latitude"), 2.0);
  EXPECT_EQ(qr.GetVal<double>("Longitude"), -20.0);
}

TEST_F(DeployManagerInstTests, producerexists) {
  using std::set;
  ctx_->AddPrototype("foop", producer);
  set<cyclus::toolkit::CommodityProducer*>::iterator it;
  for (it = src_inst->cyclus::toolkit::CommodityProducerManager::
          producers().begin();
       it != src_inst->cyclus::toolkit::CommodityProducerManager::
          producers().end();
       it++) {
    EXPECT_EQ(dynamic_cast<TestProducer*>(*it)->prototype(),
              producer->prototype());
  }
}

TEST_F(DeployManagerInstTests, productioncapacity) {
  EXPECT_EQ(src_inst->TotalCapacity(commodity), 0);
  src_inst->BuildNotify(producer);
  EXPECT_EQ(src_inst->TotalCapacity(commodity), capacity);
  src_inst->DecomNotify(producer);
  EXPECT_EQ(src_inst->TotalCapacity(commodity), 0);
}

TEST_F(DeployManagerInstTests, cornercase){
  std::string dmi_config =
     "<prototypes>  <val>foobar1</val> <val>foobar2</val> <val>foobar1</val> </prototypes>"
     "<build_times> <val>1</val>      <val>1</val>      <val>2</val>      </build_times>"
     "<n_build>     <val>1</val>      <val>7</val>      <val>3</val>      </n_build>"
     "<lifetimes>   <val>1</val>      <val>2</val>      <val>-1</val>     </lifetimes>"
     ;

  std::string gr_config =
    "<growth> <item> <commod>commod1</commod>"
    "<piecewise_function> <piece> <start>2</start>" 
    "<function> <type>linear</type> <params>0 5</params> </function> </piece> </piecewise_function>"
    "</item> </growth>"
    ;
  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cycamore:DeployManagerInst"), dmi_config, simdur);
  sim.DummyProto("foobar1");
  sim.DummyProto("foobar2");
  int id = sim.Run();

  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'foobar1' AND EnterTime = 1 AND Lifetime = 1;"
      );
  stmt->Step();
  EXPECT_EQ(1, stmt->GetInt(0));
}

// required to get functionality in cyclus agent unit tests library
cyclus::Agent* DeployManagerInstitutionConstructor(cyclus::Context* ctx) {
  return new cycamore::DeployManagerInst(ctx);
}

#ifndef CYCLUS_AGENT_TESTS_CONNECTED
int ConnectAgentTests();
static int cyclus_agent_tests_connected = ConnectAgentTests();
#define CYCLUS_AGENT_TESTS_CONNECTED cyclus_agent_tests_connected
#endif  // CYCLUS_AGENT_TESTS_CONNECTED

INSTANTIATE_TEST_CASE_P(DeployManagerInst, InstitutionTests,
                        Values(&DeployManagerInstitutionConstructor));
INSTANTIATE_TEST_CASE_P(DeployManagerInst, AgentTests,
                        Values(&DeployManagerInstitutionConstructor));
