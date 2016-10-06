#include "catch/catch.hpp"

#include "src/server/server.h"

#include "libloom/compat.h"
#include "libloom/loomplan.pb.h"

#include <set>

#include <uv.h>
#include <iostream>

template<typename T> std::set<T> v_to_s(std::vector<T> &v) {
   return std::set<T>(v.begin(), v.end());
}

typedef std::unordered_set<PlanNode*> TaskSet;

typedef
std::unordered_map<WorkerConnection*, TaskSet>
DistMap;

static std::unique_ptr<WorkerConnection>
simple_worker(Server &server, const std::string &name, int cpus=1)
{
   std::vector<loom::Id> tt, dt;
   return std::make_unique<WorkerConnection>(server, nullptr, name, tt, dt, cpus);
}

template<typename T>
bool check_uvector(const std::vector<T> &v, const std::set<T> &s)
{
   std::set<T> s2(v.begin() , v.end());
   return s == s2;
}

/* Simple plan

  n1  n2
   \  /
    n3

*/
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

/* Plan2

  n0  n1  n2  n3  n4
   \  / \ |  / \  | \
    \/   \| /   \ /  \
    n5    n6     n7  n8

*/
static loomplan::Plan make_plan2()
{
   loomplan::Plan plan;
   plan.add_tasks(); // n0
   plan.add_tasks(); // n1
   plan.add_tasks(); // n2
   plan.add_tasks(); // n3
   plan.add_tasks(); // n4

   loomplan::Task *n5 = plan.add_tasks();
   loomplan::Task *n6 = plan.add_tasks();
   loomplan::Task *n7 = plan.add_tasks();
   loomplan::Task *n8 = plan.add_tasks();

   n5->add_input_ids(0);
   n5->add_input_ids(1);
   n6->add_input_ids(1);
   n6->add_input_ids(2);
   n6->add_input_ids(3);
   n7->add_input_ids(3);
   n7->add_input_ids(4);
   n8->add_input_ids(4);

   return plan;
}

static loomplan::Plan make_big_plan(size_t plan_size)
{
   loomplan::Plan plan;

   for (size_t i = 0; i < plan_size; i++) {
      plan.add_tasks();
   }
   loomplan::Task *m = plan.add_tasks();
   for (size_t i = 1; i < plan_size; i++) {
      loomplan::Task *n = plan.add_tasks();
      n->add_input_ids(i);
      n->add_input_ids(0);
      n->add_input_ids(i - 1);
      if (i % 2 == 0) {
         m->add_input_ids(i);
      }
   }

   return plan;
}

static loomplan::Plan make_big_trivial_plan(size_t plan_size)
{
   loomplan::Plan plan;

   for (size_t i = 0; i < plan_size; i++) {
      plan.add_tasks();
   }
   for (size_t i = 0; i < plan_size; i++) {
      loomplan::Task *n = plan.add_tasks();
      n->add_input_ids(i);
   }

   return plan;
}


void dump_dist(const TaskDistribution &d)
{
   for (auto& pair : d) {
      std::cout << pair.first << " " << pair.first->get_address() << ":";
      for (auto &i : pair.second) {
         std::cout << " " << i;
      }
      std::cout << std::endl;
   }
}


static void finish(ComputationState &cstate, loom::Id id, size_t size, size_t length, WorkerConnection *wc)
{
   std::vector<loom::Id> ids = {id};
   cstate.add_ready_nodes(ids);
   cstate.set_running_task(wc, id);
   cstate.set_task_finished(id, size, length, wc);
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

TEST_CASE("Basic plan scheduling", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   s.set_plan(Plan(make_simple_plan(), 10));


   auto w1 = simple_worker(server, "w1");
   s.add_worker(w1.get());

   auto w2 = simple_worker(server, "w2");
   s.add_worker(w2.get());


   SECTION("Scheduling first two tasks") {
      s.add_ready_nodes(std::vector<loom::Id>{10, 11});

      TaskDistribution d = s.compute_distribution();
      CHECK(d[w1.get()].size() == 1);
      CHECK(d[w2.get()].size() == 1);
      loom::Id id1 = d[w1.get()][0];
      loom::Id id2 = d[w2.get()][0];
      CHECK(id1 != id2);
      CHECK((id1 == 10 || id1 == 11));
      CHECK((id2 == 10 || id2 == 11));
   }

   SECTION("Scheduling third task - first dep is bigger") {
      finish(s, 10, 200, 0, w1.get());
      finish(s, 11, 100, 0, w2.get());

      s.add_ready_nodes(std::vector<loom::Id>{12});
      TaskDistribution d = s.compute_distribution();

      CHECK((d[w1.get()] == std::vector<loom::Id>{12}));
      CHECK((d[w2.get()] == std::vector<loom::Id>{}));
   }

   SECTION("Scheduling third task - second dep is bigger") {
      finish(s, 10, 100, 0, w1.get());
      finish(s, 11, 200, 0, w2.get());

      s.add_ready_nodes(std::vector<loom::Id>{12});
      TaskDistribution d = s.compute_distribution();

      CHECK((d[w1.get()] == std::vector<loom::Id>{}));
      CHECK((d[w2.get()] == std::vector<loom::Id>{12}));
   }

   SECTION("Scheduling third task - all on first") {
      finish(s, 10, 100, 0, w1.get());
      finish(s, 11, 200, 0, w1.get());

      s.add_ready_nodes(std::vector<loom::Id>{12});
      TaskDistribution d = s.compute_distribution();

      CHECK((d[w1.get()] == std::vector<loom::Id>{12}));
      CHECK((d[w2.get()] == std::vector<loom::Id>{}));
   }

   SECTION("Scheduling third task - all on second") {
      finish(s, 10, 100, 0, w2.get());
      finish(s, 11, 200, 0, w2.get());

      s.add_ready_nodes(std::vector<loom::Id>{12});
      TaskDistribution d = s.compute_distribution();

      CHECK((d[w1.get()] == std::vector<loom::Id>{}));
      CHECK((d[w2.get()] == std::vector<loom::Id>{12}));
   }
}

TEST_CASE("Plan2 scheduling", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   s.set_plan(Plan(make_plan2(), 0));

   SECTION("Two simple workers") {
      auto w1 = simple_worker(server, "w1");
      s.add_worker(w1.get());

      auto w2 = simple_worker(server, "w2");
      s.add_worker(w2.get());

      SECTION("Each one has own") {
         finish(s, 0, 100, 0, w2.get());
         finish(s, 1, 100, 0, w1.get());
         finish(s, 2, 100, 0, w1.get());
         finish(s, 3, 100, 0, w1.get());
         finish(s, 4, 100, 0, w2.get());

         s.add_ready_nodes(std::vector<loom::Id>{5, 6, 7, 8});
         TaskDistribution d = s.compute_distribution();
         //dump_dist(d);

         CHECK((d[w1.get()] == std::vector<loom::Id>{6}));
         CHECK((d[w2.get()] == std::vector<loom::Id>{8}));
      }
   }

   SECTION("Three simple workers - 2-2-0 cpus") {
      auto w1 = simple_worker(server, "w1", 2);
      s.add_worker(w1.get());

      auto w2 = simple_worker(server, "w2", 2);
      s.add_worker(w2.get());

      auto w3 = simple_worker(server, "w3", 0);
      s.add_worker(w3.get());

      SECTION("Each one has own") {
         finish(s, 0, 100, 0, w1.get());
         finish(s, 1, 100, 0, w1.get());
         finish(s, 2, 100, 0, w1.get());
         finish(s, 3, 100, 0, w1.get());
         finish(s, 4, 100, 0, w2.get());

         s.add_ready_nodes(std::vector<loom::Id>{5, 6, 7, 8});
         TaskDistribution d = s.compute_distribution();
         //dump_dist(d);

         CHECK(check_uvector(d[w1.get()], {5, 6}));
         CHECK(check_uvector(d[w2.get()], {7, 8}));
         CHECK(check_uvector(d[w3.get()], {}));
      }

      SECTION("Two task with common deps") {
         finish(s, 0, 100, 0, w3.get());
         finish(s, 1, 200, 0, w3.get());
         finish(s, 2, 100, 0, w3.get());
         finish(s, 3, 100, 0, w3.get());
         finish(s, 4, 100, 0, w3.get());

         s.add_ready_nodes(std::vector<loom::Id>{5, 6, 8});
         TaskDistribution d = s.compute_distribution();

         if (d[w1.get()].size() > d[w2.get()].size()) {
            CHECK(check_uvector(d[w1.get()], {5, 6}));
            CHECK(check_uvector(d[w2.get()], {8}));
         } else {
            CHECK(check_uvector(d[w1.get()], {8}));
            CHECK(check_uvector(d[w2.get()], {5, 6}));
         }
      }

      SECTION("Two task with common deps") {
         finish(s, 0, 100, 0, w3.get());
         finish(s, 1, 200, 0, w3.get());
         finish(s, 2, 100, 0, w3.get());
         finish(s, 3, 100, 0, w3.get());
         finish(s, 4, 100, 0, w3.get());

         s.add_ready_nodes(std::vector<loom::Id>{5, 6, 8});
         TaskDistribution d = s.compute_distribution();
         //dump_dist(d);

         if (d[w1.get()].size() > d[w2.get()].size()) {
            CHECK(check_uvector(d[w1.get()], {5, 6}));
            CHECK(check_uvector(d[w2.get()], {8}));
         } else {
            CHECK(check_uvector(d[w1.get()], {8}));
            CHECK(check_uvector(d[w2.get()], {5, 6}));
         }
      }

      SECTION("Better one big than two smaller with bigger sum") {
         finish(s, 0, 800, 0, w2.get());
         finish(s, 1, 1100, 0, w1.get());
         finish(s, 2, 800, 0, w2.get());
         finish(s, 3, 0, 0, w3.get());
         //finish(s, 4, 100, 0, w3.get());

         s.add_ready_nodes(std::vector<loom::Id>{5, 6});
         TaskDistribution d = s.compute_distribution();
         //dump_dist(d);

         CHECK(check_uvector(d[w2.get()], {5, 6}));
         CHECK(check_uvector(d[w1.get()], {}));
      }
   }
}



TEST_CASE("Big plan", "[scheduling]") {
   const size_t BIG_PLAN_SIZE = 60;
   const size_t BIG_PLAN_WORKERS = 5;

   Server server(NULL, 0);
   ComputationState s(server);
   s.set_plan(Plan(make_big_plan(BIG_PLAN_SIZE), 0));

   std::vector<std::unique_ptr<WorkerConnection>> ws;
   ws.reserve(BIG_PLAN_WORKERS);
   for (size_t i = 0; i < BIG_PLAN_WORKERS; i++) {
      ws.push_back(simple_worker(server, "w", 2));
      s.add_worker(ws[i].get());
   }

   std::vector<loom::Id> ready;
   for (size_t i = 0; i < BIG_PLAN_SIZE; i++) {
      finish(s, i, 10 + i * 10, 0, ws[i % BIG_PLAN_WORKERS].get());
      ready.push_back(i + BIG_PLAN_SIZE);
   }
   s.add_ready_nodes(ready);
   TaskDistribution d = s.compute_distribution();
   //dump_dist(d);
   CHECK(d.size() == BIG_PLAN_WORKERS);
   size_t sum = 0;
   for (auto& pair : d) {
      sum += pair.second.size();
   }
   CHECK(sum == BIG_PLAN_WORKERS * 2);
}

TEST_CASE("Big simple plan", "[scheduling]") {
   const size_t BIG_PLAN_SIZE = 200;
   const size_t BIG_PLAN_WORKERS = 20;

   Server server(NULL, 0);
   ComputationState s(server);
   s.set_plan(Plan(make_big_trivial_plan(BIG_PLAN_SIZE), 0));

   std::vector<std::unique_ptr<WorkerConnection>> ws;
   ws.reserve(BIG_PLAN_WORKERS);
   for (size_t i = 0; i < BIG_PLAN_WORKERS; i++) {
      ws.push_back(simple_worker(server, "w", 5));
      s.add_worker(ws[i].get());
   }

   std::vector<loom::Id> ready;
   for (size_t i = 0; i < BIG_PLAN_SIZE; i++) {
      finish(s, i, 10 + i * 10, 0, ws[i % BIG_PLAN_WORKERS].get());
      ready.push_back(i + BIG_PLAN_SIZE);
   }
   s.add_ready_nodes(ready);
   TaskDistribution d = s.compute_distribution();
   //dump_dist(d);
   CHECK(d.size() == BIG_PLAN_WORKERS);
   size_t sum = 0;
   for (auto& pair : d) {
      sum += pair.second.size();
   }
   CHECK(sum == BIG_PLAN_WORKERS * 5);
}
