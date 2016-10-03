#include "catch/catch.hpp"

#include "src/server/server.h"

#include "libloom/compat.h"
#include "libloom/loomplan.pb.h"

#include <set>

#include <uv.h>

template<typename T> std::set<T> v_to_s(std::vector<T> &v) {
    return std::set<T>(v.begin(), v.end());
}

typedef std::unordered_set<TaskNode*> TaskSet;

typedef
std::unordered_map<WorkerConnection*, TaskSet>
DistMap;

static std::unique_ptr<WorkerConnection>
simple_worker(Server &server, const std::string &name, int cpus=1)
{
    std::vector<loom::Id> tt, dt;
    return std::make_unique<WorkerConnection>(server, nullptr, name, tt, dt, cpus);
}

static loomplan::Plan make_simple_plan()
{
    loomplan::Plan plan;
    loomplan::Task *n1 = plan.add_tasks();
    loomplan::Task *n2 = plan.add_tasks();
    loomplan::Task *n3 = plan.add_tasks();
    n3->add_input_ids(0);
    n3->add_input_ids(1);
    n3->add_input_ids(0);
    return plan;
}

TEST_CASE( "Server scheduling - plan construction", "[scheduling]" ) {

   Plan p(make_simple_plan(), 100);

   CHECK(p.get_size() == 3);

   auto& d1 = p.get_node(100);
   CHECK(d1.get_id() == 100);
   CHECK(d1.get_client_id() == 0);
   CHECK(d1.get_inputs().empty());
   CHECK((d1.get_nexts() == std::vector<loom::Id>{102}));

   auto& d2 = p.get_node(101);
   CHECK(d2.get_id() == 101);
   CHECK(d2.get_client_id() == 1);
   CHECK(d2.get_inputs().empty());
   CHECK((d2.get_nexts() == std::vector<loom::Id>{102}));

   auto& d3 = p.get_node(102);
   CHECK(d3.get_id() == 102);
   CHECK(d3.get_client_id() == 2);
   CHECK((d3.get_inputs() == std::vector<loom::Id>{100, 101, 100}));
   CHECK((d3.get_nexts() == std::vector<loom::Id>{}));
}

TEST_CASE("Server scheduling - init tasks", "[scheduling]") {
   Plan p(make_simple_plan(), 10);
   auto st = p.get_init_tasks();
   CHECK((v_to_s(st) == std::set<loom::Id>{10, 11}));
}

TEST_CASE("Server scheduling - distribute tasks", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   Plan p(make_simple_plan(), 10);
   s.set_plan(Plan(make_simple_plan(), 10));


   auto w1 = simple_worker(server, "w1");
   s.add_worker(w1.get());

   auto w2 = simple_worker(server, "w2");
   s.add_worker(w2.get());


   SECTION("Two tasks") {
      std::vector<int> ready{10, 11};
      s.add_ready_nodes(ready);

      TaskDistribution d = s.compute_distribution();
      CHECK(d[w1.get()].size() == 1);
      CHECK(d[w2.get()].size() == 1);
      loom::Id id1 = d[w1.get()][0];
      loom::Id id2 = d[w2.get()][0];
      CHECK(id1 != id2);
      CHECK((id1 == 10 || id1 == 11));
      CHECK((id2 == 10 || id2 == 11));
   }

   SECTION("Two tasks") {
      std::vector<int> ready{12};
      s.add_ready_nodes(ready);

      TaskDistribution d = s.compute_distribution();

      CHECK(d[w1.get()].size() != d[w2.get()].size());
      WorkerConnection *w = d[w1.get()].size() > d[w2.get()].size() ? w1.get() : w2.get();
      CHECK(d[w][0] == 12);
   }
}
