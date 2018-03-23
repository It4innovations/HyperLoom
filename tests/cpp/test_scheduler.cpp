#include "catch/catch.hpp"

#include "src/server/server.h"

#include "libloom/compat.h"
#include "pb/comm.pb.h"

#include <set>
#include <chrono>

#include <uv.h>
#include <iostream>

template<typename T> std::set<T> v_to_s(std::vector<T> &v) {
   return std::set<T>(v.begin(), v.end());
}

static WorkerConnection *
simple_worker(Server &server, const std::string &name, int cpus=1)
{
   std::vector<loom::base::Id> tt, dt;
   auto wc = std::make_unique<WorkerConnection>(server, nullptr, name, tt, dt, cpus, 0);
   WorkerConnection *r = wc.get();
   server.add_worker_connection(std::move(wc));
   return r;
}

template<typename T>
static bool check_uvector(const std::vector<T> &v, const std::vector<T> &s)
{
   std::set<T> s1(v.begin() , v.end());
   std::set<T> s2(s.begin() , s.end());
   return s1 == s2;
}

static void add_cpu_request(Server &server, loom::pb::comm::Plan &plan, int value)
{
    using namespace loom::pb;
    comm::ResourceRequest *rr = plan.add_resource_requests();
    comm::Resource *r = rr->add_resources();
    r->set_resource_type(server.get_dictionary().find_or_create("loom/resource/cpus"));
    r->set_value(value);
}

loom::pb::comm::Task* new_task(loom::pb::comm::Plan &plan, int rr_index=-1)
{
    loom::pb::comm::Task *t = plan.add_tasks();
    if (rr_index >= 0) {
        t->set_resource_request_index(rr_index);
    }
    return t;
}

/* Simple plan

  n1  n2
   \  /
    n3

*/
static loom::pb::comm::Plan make_simple_plan(Server &server)
{
   using namespace loom::pb::comm;
   Plan plan;
   plan.set_id_base(0);
   add_cpu_request(server, plan, 1);
   Task *n1 = new_task(plan, 0);
   Task *n2 = new_task(plan, 0);
   Task *n3 = new_task(plan, 0);
   n3->add_input_ids(0);
   n3->add_input_ids(1);
   return plan;
}

/* Plan2

  n0  n1  n2  n3  n4
   \  / \ |  / \  | \
    \/   \| /   \ /  \
    n5    n6     n7  n8

*/
static loom::pb::comm::Plan make_plan2(Server &server)
{
   using namespace loom::pb::comm;
   Plan plan;
   plan.set_id_base(0);
   add_cpu_request(server, plan, 1);

   new_task(plan, 0); // n0
   new_task(plan, 0); // n1
   new_task(plan, 0); // n2
   new_task(plan, 0); // n3
   new_task(plan, 0); // n4

   Task *n5 = new_task(plan, 0);
   Task *n6 = new_task(plan, 0);
   Task *n7 = new_task(plan, 0);
   Task *n8 = new_task(plan, 0);

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

/* Plan3

      n0    n1     n2
     / \   / \    / \
    /   \ /   \  /   \
   n3    n5    n4   n6 <-- Changed order!
    \      \ /      /
     \      X      /
      \    / \    /
        n7     n8

*/
static loom::pb::comm::Plan make_plan3(Server &server)
{
   using namespace loom::pb::comm;
   Plan plan;
   plan.set_id_base(0);
   add_cpu_request(server, plan, 1);

   new_task(plan, 0); // n0
   new_task(plan, 0); // n1
   new_task(plan, 0); // n2

   Task *n3 = new_task(plan, 0);
   Task *n4 = new_task(plan, 0);
   Task *n5 = new_task(plan, 0);
   Task *n6 = new_task(plan, 0);

   n3->add_input_ids(0);
   n4->add_input_ids(1);
   n4->add_input_ids(2);
   n5->add_input_ids(0);
   n5->add_input_ids(1);
   n6->add_input_ids(2);

   Task *n7 = new_task(plan, 0);
   Task *n8 = new_task(plan, 0);

   n7->add_input_ids(3);
   n7->add_input_ids(4);
   n8->add_input_ids(5);
   n8->add_input_ids(6);

   return plan;
}


/* Plan4

      n0  n1  n2    n4  n5  n6
       \  |  /       \  |  /
        \ | /         \ | /
          n3            n7      n8  n9

*/
static loom::pb::comm::Plan make_plan4(Server &server)
{
   using namespace loom::pb::comm;
   Plan plan;
   plan.set_id_base(0);
   add_cpu_request(server, plan, 1);

   new_task(plan, 0); // n0
   new_task(plan, 0); // n1
   new_task(plan, 0); // n2

   Task *n3 = new_task(plan, 0);

   n3->add_input_ids(0);
   n3->add_input_ids(1);
   n3->add_input_ids(2);


   new_task(plan, 0); // n4
   new_task(plan, 0); // n5
   new_task(plan, 0); // n6

   Task *n7 = new_task(plan, 0);

   n7->add_input_ids(4);
   n7->add_input_ids(5);
   n7->add_input_ids(6);

   new_task(plan, 0); // n8
   new_task(plan, 0); // n9

   return plan;
}



static loom::pb::comm::Plan make_big_plan(Server &server, size_t plan_size)
{
   using namespace loom::pb::comm;
   Plan plan;
   plan.set_id_base(0);
   add_cpu_request(server, plan, 1);

   for (size_t i = 0; i < plan_size; i++) {
      new_task(plan, 0);
   }
   Task *m = new_task(plan, 0);
   for (size_t i = 1; i < plan_size; i++) {
      Task *n = new_task(plan, 0);
      n->add_input_ids(i);
      n->add_input_ids(0);
      n->add_input_ids(i - 1);
      if (i % 2 == 0) {
         m->add_input_ids(i - 2);
      }
   }
   return plan;
}


/* make_big_trivial_plan

      n0    n1     n2
      |     |      |    ...
      n0+s  n1+s   n2+s
*/
static loom::pb::comm::Plan make_big_trivial_plan(Server &server, size_t plan_size)
{
   using namespace loom::pb::comm;
   Plan plan;
   plan.set_id_base(0);
   add_cpu_request(server, plan, 1);

   for (size_t i = 0; i < plan_size; i++) {
      new_task(plan, 0);
   }
   for (size_t i = 0; i < plan_size; i++) {
      Task *n = new_task(plan, 0);
      n->add_input_ids(i);
   }

   return plan;
}

static loom::pb::comm::Plan make_request_plan(Server &server)
{
   using namespace loom::pb::comm;
   Plan plan;
   plan.set_id_base(0);
   add_cpu_request(server, plan, 1); // 0
   add_cpu_request(server, plan, 2); // 1
   add_cpu_request(server, plan, 3); // 2
   add_cpu_request(server, plan, 4); // 3

   new_task(plan, 0); // n0 (1 cpu)
   new_task(plan, 0); // n1 (1 cpu)

   new_task(plan, 1); // n2 (2 cpu)
   new_task(plan, 1); // n3 (2 cpu)

   new_task(plan, 2); // n4 (3 cpu)

   new_task(plan, 3); // n5 (4 cpu)
   new_task(plan, 3); // n6 (4 cpu)

   new_task(plan, -1); // n7 (0 cpu)

   return plan;
}

std::vector<loom::base::Id> range(size_t limit)
{
    std::vector<loom::base::Id> result;
    result.reserve(limit);
    for (size_t i = 0; i < limit; i++) {
        result.push_back(i);
    }
    return result;
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

static void start(ComputationState &cstate, loom::base::Id id, WorkerConnection *wc)
{
    auto &node = cstate.get_node(id);
    node.set_as_running(wc);
}

static void finish(ComputationState &cstate, loom::base::Id id, size_t size, size_t length, WorkerConnection *wc)
{
    auto &node = cstate.get_node(id);
    node.set_as_finished_no_check(wc, size, length);
}

static std::vector<TaskNode*> nodes(ComputationState &s, std::vector<loom::base::Id> ids)
{
    std::vector<TaskNode*> result;
    for (auto id : ids) {
        result.push_back(&s.get_node(id));
    }
    return result;
}

static void add_plan(ComputationState &s, const loom::pb::comm::Plan &plan) {
    std::vector<TaskNode*> to_load;
    s.add_plan(plan, to_load);
    assert(to_load.empty());
}

TEST_CASE("basic-plan", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   add_plan(s, make_simple_plan(server));

   auto w1 = simple_worker(server, "w1");
   auto w2 = simple_worker(server, "w2");

   SECTION("Scheduling first two tasks") {
      s.test_ready_nodes({0, 1});

      TaskDistribution d = schedule(s);
      //dump_dist(d);
      REQUIRE(d[w1].size() == 1);
      REQUIRE(d[w2].size() == 1);
      TaskNode *t1 = d[w1][0];
      TaskNode *t2 = d[w2][0];
      REQUIRE(t1 != t2);
      REQUIRE((t1->get_id() == 0 || t1->get_id() == 1));
      REQUIRE((t2->get_id() == 0 || t2->get_id() == 1));
   }

   SECTION("Scheduling third task - first dep is bigger") {
      finish(s, 0, 200, 0, w1);
      finish(s, 1, 100, 0, w2);

      s.test_ready_nodes({2});
      TaskDistribution d = schedule(s);
      //dump_dist(d);

      REQUIRE((d[w1] == nodes(s, {2})));
      REQUIRE((d[w2] == nodes(s, {})));
   }

   SECTION("Scheduling third task - second dep is bigger") {
      finish(s, 0, 100, 0, w1);
      finish(s, 1, 200, 0, w2);

      s.test_ready_nodes({2});
      TaskDistribution d = schedule(s);

      REQUIRE((d[w1] == nodes(s, {})));
      REQUIRE((d[w2] == nodes(s, {2})));
   }

   SECTION("Scheduling third task - all on first") {
      finish(s, 0, 100, 0, w1);
      finish(s, 1, 200, 0, w1);

      s.test_ready_nodes(std::vector<loom::base::Id>{2});
      TaskDistribution d = schedule(s);

      REQUIRE((d[w1] == nodes(s, {2})));
      REQUIRE((d[w2] == nodes(s, {})));
   }

   SECTION("Scheduling third task - all on second") {
      finish(s, 0, 100, 0, w2);
      finish(s, 1, 200, 0, w2);

      s.test_ready_nodes(std::vector<loom::base::Id>{2});
      TaskDistribution d = schedule(s);

      REQUIRE((d[w1] == nodes(s, {})));
      REQUIRE((d[w2] == nodes(s, {2})));
   }
}

TEST_CASE("plan4", "[scheduling]") {
    Server server(NULL, 0);
    ComputationState s(server);
    add_plan(s, make_plan4(server));

    SECTION("More narrow") {
        auto w1 = simple_worker(server, "w1", 1);
        auto w2 = simple_worker(server, "w2", 1);
        auto w3 = simple_worker(server, "w2", 1);

        SECTION("w2 is busy") {
           s.test_ready_nodes({3, 7});

           SECTION("Case 1") {
               finish(s, 0, 1000, 0, w1);
               finish(s, 1, 1000, 0, w2);
               finish(s, 2, 1000, 0, w3);

               finish(s, 4, 1000, 0, w1);
               finish(s, 5, 1000, 0, w2);
               finish(s, 6,  500, 0, w3);
               start(s, 8, w2);

               TaskDistribution d = schedule(s);
               //dump_dist(d);
               REQUIRE((d[w1] == nodes(s, {7})));
           }

           SECTION("Case 2") {
               finish(s, 0, 1000, 0, w1);
               finish(s, 1, 1000, 0, w2);
               finish(s, 2,  100, 0, w3);

               finish(s, 4, 1000, 0, w1);
               finish(s, 5, 1000, 0, w2);
               finish(s, 6,  500, 0, w3);
               start(s, 8, w2);

               TaskDistribution d = schedule(s);
               //dump_dist(d);
               REQUIRE((d[w1] == nodes(s, {3})));
           }
        }

        SECTION("w2+w3 are busy") {
           s.test_ready_nodes({3, 7});

           SECTION("Case 1") {
               finish(s, 0, 1000, 0, w1);
               finish(s, 1, 1000, 0, w2);
               finish(s, 2, 1000, 0, w3);

               finish(s, 4, 1000, 0, w1);
               finish(s, 5, 1000, 0, w2);
               finish(s, 6,  500, 0, w3);

               start(s, 8, w2);
               start(s, 9, w3);

               TaskDistribution d = schedule(s);
               //dump_dist(d);
               REQUIRE((d[w1] == nodes(s,{7})));
           }

           SECTION("Case 2") {
               finish(s, 0, 1000, 0, w1);
               finish(s, 1, 1000, 0, w2);
               finish(s, 2,  100, 0, w3);

               finish(s, 4, 1000, 0, w1);
               finish(s, 5, 1000, 0, w2);
               finish(s, 6,  500, 0, w3);

               start(s, 8, w2);
               start(s, 9, w3);

               TaskDistribution d = schedule(s);
               //dump_dist(d);
               REQUIRE((d[w1] == nodes(s, {3})));
           }
        }
    }
}


TEST_CASE("Plan2", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   add_plan(s, make_plan2(server));

   SECTION("Two simple workers") {
      auto w1 = simple_worker(server, "w1");
      auto w2 = simple_worker(server, "w2");

      SECTION("Each one has own") {
         finish(s, 0, 10 << 20, 0, w2);
         finish(s, 1, 10 << 20, 0, w1);
         finish(s, 2, 10 << 20, 0, w1);
         finish(s, 3, 10 << 20, 0, w1);
         finish(s, 4, 10 << 20, 0, w2);

         s.test_ready_nodes(std::vector<loom::base::Id>{5, 6, 7, 8});
         TaskDistribution d = schedule(s);
         //dump_dist(d);

         REQUIRE((d[w1] == nodes(s, {6})));
         REQUIRE((d[w2] == nodes(s, {8})));
      }
   }

   SECTION("Three simple workers - 2-2-0 cpus") {
      auto w1 = simple_worker(server, "w1", 2);
      auto w2 = simple_worker(server, "w2", 2);
      auto w3 = simple_worker(server, "w3", 0);

      SECTION("Each one has own") {
         finish(s, 0, 100, 0, w1);
         finish(s, 1, 100, 0, w1);
         finish(s, 2, 100, 0, w1);
         finish(s, 3, 100, 0, w1);
         finish(s, 4, 100, 0, w2);

         s.test_ready_nodes(std::vector<loom::base::Id>{5, 6, 7, 8});
         TaskDistribution d = schedule(s);
         //dump_dist(d);

         REQUIRE(check_uvector(d[w1], nodes(s, {5, 6})));
         REQUIRE(check_uvector(d[w2], nodes(s, {7, 8})));
         REQUIRE(check_uvector(d[w3], nodes(s, {})));
      }

      /* Good example but not solved now
      SECTION("Two task with shared deps") {
         finish(s, 0, 10 << 20, 0, w3);
         finish(s, 1, 20 << 20, 0, w3);
         finish(s, 2, 10 << 20, 0, w3);
         finish(s, 3, 10 << 20, 0, w3);
         finish(s, 4, 10 << 20, 0, w3);

         s.test_ready_nodes(std::vector<loom::base::Id>{5, 6, 8});
         TaskDistribution d = schedule(s);
         dump_dist(d);

         if (d[w1].size() > d[w2].size()) {
            REQUIRE(check_uvector(d[w1], {5, 6}));
            REQUIRE(check_uvector(d[w2], {8}));
         } else {
            REQUIRE(check_uvector(d[w1], {8}));
            REQUIRE(check_uvector(d[w2], {5, 6}));
         }
      }*/

      /* Good example, but not solved now
      SECTION("Better one big than two smaller with bigger sum") {
         finish(s, 0, 800, 0, w2);
         finish(s, 1, 1100, 0, w1);
         finish(s, 2, 800, 0, w2);
         finish(s, 3, 0, 0, w3);
         //finish(s, 4, 100, 0, w3);

         s.test_ready_nodes(std::vector<loom::base::Id>{5, 6});
         TaskDistribution d = schedule(s);
         //dump_dist(d);

         REQUIRE(check_uvector(d[w2], {5, 6}));
         REQUIRE(check_uvector(d[w1], {}));
      }*/
   }
}

TEST_CASE("big-plan", "[scheduling]") {
   const size_t BIG_PLAN_SIZE = 200000;
   const size_t BIG_PLAN_WORKERS = 8;
   const size_t CPUS = 24;

   Server server(NULL, 0);
   ComputationState s(server);
   add_plan(s, make_big_plan(server, BIG_PLAN_SIZE));

   std::vector<WorkerConnection*> ws;
   ws.reserve(BIG_PLAN_WORKERS);
   for (size_t i = 0; i < BIG_PLAN_WORKERS; i++) {
      ws.push_back(simple_worker(server, std::string("w") + std::to_string(i), CPUS));
   }

   std::vector<loom::base::Id> ready;
   for (size_t i = 0; i < BIG_PLAN_SIZE; i++) {
      finish(s, i, 10 + i * 10, 0, ws[i % BIG_PLAN_WORKERS]);
      ready.push_back(i + BIG_PLAN_SIZE);
   }
   s.test_ready_nodes(ready);
   TaskDistribution d = schedule(s);
   //dump_dist(d);
   REQUIRE(d.size() == BIG_PLAN_WORKERS);
   size_t sum = 0;
   for (auto& pair : d) {
      sum += pair.second.size();
   }
   REQUIRE(sum == BIG_PLAN_WORKERS * CPUS * 3);
}

TEST_CASE("big-simple-plan", "[scheduling]") {
   const size_t BIG_PLAN_SIZE = 600000;
   const size_t BIG_PLAN_WORKERS = 100;
   const size_t CPUS = 24;

   Server server(NULL, 0);
   ComputationState s(server);
   add_plan(s, make_big_trivial_plan(server, BIG_PLAN_SIZE));

   std::vector<WorkerConnection*> ws;
   ws.reserve(BIG_PLAN_WORKERS);
   for (size_t i = 0; i < BIG_PLAN_WORKERS; i++) {
      ws.push_back(simple_worker(server, "w", CPUS));
   }

   std::vector<loom::base::Id> ready;
   for (size_t i = 0; i < BIG_PLAN_SIZE; i++) {
      finish(s, i, 10 + i * 10, 0, ws[i % BIG_PLAN_WORKERS]);
      ready.push_back(i + BIG_PLAN_SIZE);
   }
   s.test_ready_nodes(ready);
   TaskDistribution d = schedule(s);
   //dump_dist(d);
   REQUIRE(d.size() == BIG_PLAN_WORKERS);
   size_t sum = 0;
   for (auto& pair : d) {
      sum += pair.second.size();
   }
   REQUIRE(sum == BIG_PLAN_WORKERS * CPUS * 3);
}

TEST_CASE("request-plan", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   add_plan(s, make_request_plan(server));

   SECTION("0 cpus - include free tasks") {
       auto w1 = simple_worker(server, "w1", 0);
       s.test_ready_nodes(range(8));
       TaskDistribution d = schedule(s);
       REQUIRE(d[w1].size() == 1);
       REQUIRE(d[w1][0] == &s.get_node(7));
   }

   SECTION("0 cpus") {
       auto w1 = simple_worker(server, "w1", 0);
       s.test_ready_nodes(range(7));
       TaskDistribution d = schedule(s);
       REQUIRE(d[w1].empty());
   }

   SECTION("1/2 cpus") {
       auto w1 = simple_worker(server, "w1", 1);
       auto w2 = simple_worker(server, "w2", 2);
       s.test_ready_nodes(range(7));
       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE((d[w1] == nodes(s, {0}) ||
                d[w1] == nodes(s, {1})));
       REQUIRE((d[w2] == nodes(s, {2}) ||
                d[w2] == nodes(s, {3})));
   }

   SECTION("2/2 cpus") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       s.test_ready_nodes(range(7));
       TaskDistribution d = schedule(s);
       REQUIRE((d[w1] == nodes(s, {2}) ||
                d[w1] == nodes(s, {3})));
       REQUIRE((d[w2] == nodes(s, {2}) ||
                d[w2] == nodes(s, {3})));
   }

   SECTION("5 cpus") {
       auto w1 = simple_worker(server, "w1", 5);
       s.test_ready_nodes(range(6));
       TaskDistribution d = schedule(s);

       REQUIRE((check_uvector(d[w1], nodes(s, {0, 5})) ||
                check_uvector(d[w1], nodes(s, {1, 5}))));
   }

   SECTION("5/1/1 cpus") {
       auto w1 = simple_worker(server, "w1", 5);
       auto w2 = simple_worker(server, "w2", 1);
       auto w3 = simple_worker(server, "w3", 1);

       s.test_ready_nodes(range(6));
       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE(d[w1].size() + d[w2].size() + d[w3].size() >= 3);
   }

   SECTION("5 cpus (3-4-4-0 jobs)") {
       auto w1 = simple_worker(server, "w1", 5);

       s.test_ready_nodes(std::vector<loom::base::Id>{4, 5, 6, 7});
       TaskDistribution d = schedule(s);
       REQUIRE((check_uvector(d[w1], nodes(s, {6, 7})) || check_uvector(d[w1], nodes(s, {5, 7}))));
   }
}


TEST_CASE("continuation2", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   add_plan(s, make_plan2(server));

   /*SECTION("Stick together") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       s.test_ready_nodes({0, 1});
       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE((d[w1].size() == 2 || d[w2].size() == 2));
   }*/

   SECTION("Stick to gether - 2") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 1);
       s.test_ready_nodes({0, 3, 4});
       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE(check_uvector(d[w1], nodes(s, {3, 4})));
       REQUIRE(check_uvector(d[w2], nodes(s, {0})));
   }
}


TEST_CASE("continuation", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   add_plan(s, make_plan3(server));

   SECTION("Stick together - inputs dominant") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       auto w3 = simple_worker(server, "w3", 0);

       finish(s, 0, 800 << 20, 0, w3);
       finish(s, 1, 30 << 20, 0, w3);
       finish(s, 2, 800 << 20, 0, w3);

       s.test_ready_nodes({3, 4, 5, 6});

       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE((check_uvector(d[w1], nodes(s, {3, 5})) || (check_uvector(d[w1], nodes(s, {4, 6})))));
       REQUIRE((check_uvector(d[w2], nodes(s, {3, 5})) || (check_uvector(d[w2], nodes(s, {4, 6})))));
   }

   SECTION("Stick together - inputs dominant2") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 0);
       auto w3 = simple_worker(server, "w3", 2);

       finish(s, 0, 800 << 20, 0, w1);
       finish(s, 1, 30 << 20, 0, w2);
       finish(s, 2, 800 << 20, 0, w3);

       s.test_ready_nodes({3, 4, 5, 6});

       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE((check_uvector(d[w1], nodes(s, {3, 5}))));
       REQUIRE((check_uvector(d[w3], nodes(s, {4, 6}))));
   }

   SECTION("Stick together - follows are dominant") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       auto w3 = simple_worker(server, "w3", 0);

       finish(s, 0, 20000, 0, w3);
       finish(s, 1, 20000, 0, w3);
       finish(s, 2, 20000, 0, w3);

       s.test_ready_nodes({3, 4, 5, 6});

       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE((check_uvector(d[w1], nodes(s, {3, 4})) || (check_uvector(d[w1], nodes(s, {5, 6})))));
       REQUIRE((check_uvector(d[w2], nodes(s, {3, 4})) || (check_uvector(d[w2], nodes(s, {5, 6})))));
   }

   SECTION("Stick together - small neighbour tasks already exists") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       auto w3 = simple_worker(server, "w3", 0);

       finish(s, 1, 20000, 0, w1);
       finish(s, 2, 200000, 0, w2);
       finish(s, 3, 200, 0, w1);

       s.test_ready_nodes({4});

       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE(check_uvector(d[w2], nodes(s, {4})));
       REQUIRE(check_uvector(d[w1], nodes(s, {})));
       REQUIRE(check_uvector(d[w3], nodes(s, {})));
   }

   SECTION("Stick together - big neighbour tasks already exists") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       auto w3 = simple_worker(server, "w3", 0);

       finish(s, 1, 2000, 0, w1);
       finish(s, 2, 20000, 0, w2);
       finish(s, 3, 9000000, 0, w1);

       s.test_ready_nodes({4});

       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE(check_uvector(d[w1], nodes(s, {4})));
       REQUIRE(check_uvector(d[w2], {}));
       REQUIRE(check_uvector(d[w3], {}));
   }
}


TEST_CASE("benchmark1", "[benchmark][!hide]") {
    using namespace std::chrono;
    const size_t CPUS = 24;

    for (size_t plan_size = 100; plan_size < 2000000; plan_size *= 2) {
        Server server(NULL, 0);
        auto plan = make_big_trivial_plan(server, plan_size);
        size_t tasks_size = plan.tasks_size();
        for (size_t n_workers = 10; n_workers < 600; n_workers *= 2) {
            Server server(NULL, 0);
            ComputationState s(server);
            add_plan(s, plan);

            std::vector<WorkerConnection*> ws;
            ws.reserve(n_workers);
            for (size_t i = 0; i < n_workers; i++) {
               ws.push_back(simple_worker(server, "w", CPUS));
            }

            std::vector<loom::base::Id> ready;
            for (size_t i = 0; i < plan_size; i++) {
               finish(s, i, 100 << 20, 0, ws[i % n_workers]);
               ready.push_back(i + plan_size);
            }
            s.test_ready_nodes(ready);

            auto start = steady_clock::now();
            TaskDistribution d = schedule(s);
            auto end = steady_clock::now();
            auto ds = duration_cast<duration<double>>(end - start);
            std::cout << "!! " << n_workers << " " << tasks_size << " " << ds.count() << std::endl;
        }
    }
}

TEST_CASE("benchmark2", "[benchmark][!hide]") {
    using namespace std::chrono;
    const size_t CPUS = 24;

    for (size_t plan_size = 100; plan_size <= 2000000; plan_size *= 2) {
        Server server(NULL, 0);
        auto plan = make_big_trivial_plan(server, plan_size);
        size_t tasks_size = plan.tasks_size();
        for (size_t n_workers = 10; n_workers <= 160; n_workers *= 2) {
            Server server(NULL, 0);
            ComputationState s(server);
            add_plan(s, plan);
            std::vector<WorkerConnection*> ws;
            ws.reserve(n_workers);
            for (size_t i = 0; i < n_workers; i++) {
               ws.push_back(simple_worker(server, "w", CPUS));
            }
            std::vector<loom::base::Id> ready;
            for (size_t i = 0; i < plan_size; i++) {
               finish(s, i, 10 + i * 10, 0, ws[i % n_workers]);
               ready.push_back(i + plan_size);
            }
            s.test_ready_nodes(ready);

            auto start = steady_clock::now();
            TaskDistribution d = schedule(s);
            auto end = steady_clock::now();
            auto ds = duration_cast<duration<double>>(end - start);
            std::cout << "!! " << n_workers << " " << tasks_size << " " << ds.count() << std::endl;
        }
    }
}
