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
bool check_uvector(const std::vector<T> &v, const std::set<T> &s)
{
   std::set<T> s2(v.begin() , v.end());
   return s == s2;
}

static void add_cpu_request(Server &server, loomplan::Plan &plan, int value)
{
    loomplan::ResourceRequest *rr = plan.add_resource_requests();
    loomplan::Resource *r = rr->add_resources();
    r->set_resource_type(server.get_dictionary().find_or_create("loom/resource/cpus"));
    r->set_value(value);
}

static TaskDistribution schedule(ComputationState &cstate)
{
    return Scheduler(cstate).schedule();
}

loomplan::Task* new_task(loomplan::Plan &plan, int rr_index=-1)
{
    loomplan::Task *t = plan.add_tasks();
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
static loomplan::Plan make_simple_plan(Server &server)
{
   loomplan::Plan plan;
   add_cpu_request(server, plan, 1);
   loomplan::Task *n1 = new_task(plan, 0);
   loomplan::Task *n2 = new_task(plan, 0);
   loomplan::Task *n3 = new_task(plan, 0);
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
static loomplan::Plan make_plan2(Server &server)
{
   loomplan::Plan plan;
   add_cpu_request(server, plan, 1);

   new_task(plan, 0); // n0
   new_task(plan, 0); // n1
   new_task(plan, 0); // n2
   new_task(plan, 0); // n3
   new_task(plan, 0); // n4

   loomplan::Task *n5 = new_task(plan, 0);
   loomplan::Task *n6 = new_task(plan, 0);
   loomplan::Task *n7 = new_task(plan, 0);
   loomplan::Task *n8 = new_task(plan, 0);

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
static loomplan::Plan make_plan3(Server &server)
{
   loomplan::Plan plan;
   add_cpu_request(server, plan, 1);

   new_task(plan, 0); // n0
   new_task(plan, 0); // n1
   new_task(plan, 0); // n2

   loomplan::Task *n3 = new_task(plan, 0);
   loomplan::Task *n4 = new_task(plan, 0);
   loomplan::Task *n5 = new_task(plan, 0);
   loomplan::Task *n6 = new_task(plan, 0);

   n3->add_input_ids(0);
   n4->add_input_ids(1);
   n4->add_input_ids(2);
   n5->add_input_ids(0);
   n5->add_input_ids(1);
   n6->add_input_ids(2);

   loomplan::Task *n7 = new_task(plan, 0);
   loomplan::Task *n8 = new_task(plan, 0);

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
static loomplan::Plan make_plan4(Server &server)
{
   loomplan::Plan plan;
   add_cpu_request(server, plan, 1);

   new_task(plan, 0); // n0
   new_task(plan, 0); // n1
   new_task(plan, 0); // n2

   loomplan::Task *n3 = new_task(plan, 0);

   n3->add_input_ids(0);
   n3->add_input_ids(1);
   n3->add_input_ids(2);


   new_task(plan, 0); // n4
   new_task(plan, 0); // n5
   new_task(plan, 0); // n6

   loomplan::Task *n7 = new_task(plan, 0);

   n7->add_input_ids(4);
   n7->add_input_ids(5);
   n7->add_input_ids(6);

   new_task(plan, 0); // n8
   new_task(plan, 0); // n9

   return plan;
}



static loomplan::Plan make_big_plan(Server &server, size_t plan_size)
{
   loomplan::Plan plan;
   add_cpu_request(server, plan, 1);

   for (size_t i = 0; i < plan_size; i++) {
      new_task(plan, 0);
   }
   loomplan::Task *m = new_task(plan, 0);
   for (size_t i = 1; i < plan_size; i++) {
      loomplan::Task *n = new_task(plan, 0);
      n->add_input_ids(i);
      n->add_input_ids(0);
      n->add_input_ids(i - 1);
      if (i % 2 == 0) {
         m->add_input_ids(i);
      }
   }

   return plan;
}


/* make_big_trivial_plan

      n0    n1     n2
      |     |      |    ...
      n0+s  n1+s   n2+s
*/
static loomplan::Plan make_big_trivial_plan(Server &server, size_t plan_size)
{
   loomplan::Plan plan;
   add_cpu_request(server, plan, 1);

   for (size_t i = 0; i < plan_size; i++) {
      new_task(plan, 0);
   }
   for (size_t i = 0; i < plan_size; i++) {
      loomplan::Task *n = new_task(plan, 0);
      n->add_input_ids(i);
   }

   return plan;
}

static loomplan::Plan make_request_plan(Server &server)
{
   loomplan::Plan plan;
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

TEST_CASE("Basic plan scheduling", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   s.add_plan(make_simple_plan(server));


   auto w1 = simple_worker(server, "w1");
   auto w2 = simple_worker(server, "w2");

   SECTION("Scheduling first two tasks") {
      s.test_ready_nodes({0, 1});

      TaskDistribution d = schedule(s);
      //dump_dist(d);
      REQUIRE(d[w1].size() == 1);
      REQUIRE(d[w2].size() == 1);
      loom::base::Id id1 = d[w1][0];
      loom::base::Id id2 = d[w2][0];
      REQUIRE(id1 != id2);
      REQUIRE((id1 == 0 || id1 == 1));
      REQUIRE((id2 == 0 || id2 == 1));
   }

   SECTION("Scheduling third task - first dep is bigger") {
      finish(s, 0, 200, 0, w1);
      finish(s, 1, 100, 0, w2);

      s.test_ready_nodes({2});
      TaskDistribution d = schedule(s);
      //dump_dist(d);

      REQUIRE((d[w1] == std::vector<loom::base::Id>{2}));
      REQUIRE((d[w2] == std::vector<loom::base::Id>{}));
   }

   SECTION("Scheduling third task - second dep is bigger") {
      finish(s, 0, 100, 0, w1);
      finish(s, 1, 200, 0, w2);

      s.test_ready_nodes({2});
      TaskDistribution d = schedule(s);

      REQUIRE((d[w1] == std::vector<loom::base::Id>{}));
      REQUIRE((d[w2] == std::vector<loom::base::Id>{2}));
   }

   SECTION("Scheduling third task - all on first") {
      finish(s, 0, 100, 0, w1);
      finish(s, 1, 200, 0, w1);

      s.test_ready_nodes(std::vector<loom::base::Id>{2});
      TaskDistribution d = schedule(s);

      REQUIRE((d[w1] == std::vector<loom::base::Id>{2}));
      REQUIRE((d[w2] == std::vector<loom::base::Id>{}));
   }

   SECTION("Scheduling third task - all on second") {
      finish(s, 0, 100, 0, w2);
      finish(s, 1, 200, 0, w2);

      s.test_ready_nodes(std::vector<loom::base::Id>{2});
      TaskDistribution d = schedule(s);

      REQUIRE((d[w1] == std::vector<loom::base::Id>{}));
      REQUIRE((d[w2] == std::vector<loom::base::Id>{2}));
   }
}

TEST_CASE("Plan 4", "[scheduling]") {
    Server server(NULL, 0);
    ComputationState s(server);
    s.add_plan(make_plan4(server));

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
               REQUIRE((d[w1] == std::vector<loom::base::Id>{7}));
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
               REQUIRE((d[w1] == std::vector<loom::base::Id>{3}));
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
               REQUIRE((d[w1] == std::vector<loom::base::Id>{7}));
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
               REQUIRE((d[w1] == std::vector<loom::base::Id>{3}));
           }
        }
    }
}


TEST_CASE("Plan2 scheduling", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   s.add_plan(make_plan2(server));

   SECTION("Two simple workers") {
      auto w1 = simple_worker(server, "w1");      
      auto w2 = simple_worker(server, "w2");

      SECTION("Each one has own") {
         finish(s, 0, 100, 0, w2);
         finish(s, 1, 100, 0, w1);
         finish(s, 2, 100, 0, w1);
         finish(s, 3, 100, 0, w1);
         finish(s, 4, 100, 0, w2);

         s.test_ready_nodes(std::vector<loom::base::Id>{5, 6, 7, 8});
         TaskDistribution d = schedule(s);
         //dump_dist(d);

         REQUIRE((d[w1] == std::vector<loom::base::Id>{6}));
         REQUIRE((d[w2] == std::vector<loom::base::Id>{8}));
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

         REQUIRE(check_uvector(d[w1], {5, 6}));
         REQUIRE(check_uvector(d[w2], {7, 8}));
         REQUIRE(check_uvector(d[w3], {}));
      }

      SECTION("Two task with shared deps") {
         finish(s, 0, 100, 0, w3);
         finish(s, 1, 200, 0, w3);
         finish(s, 2, 100, 0, w3);
         finish(s, 3, 100, 0, w3);
         finish(s, 4, 100, 0, w3);

         s.test_ready_nodes(std::vector<loom::base::Id>{5, 6, 8});
         TaskDistribution d = schedule(s);
         //dump_dist(d);

         if (d[w1].size() > d[w2].size()) {
            REQUIRE(check_uvector(d[w1], {5, 6}));
            REQUIRE(check_uvector(d[w2], {8}));
         } else {
            REQUIRE(check_uvector(d[w1], {8}));
            REQUIRE(check_uvector(d[w2], {5, 6}));
         }
      }

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
      }
   }
}



TEST_CASE("Big plan", "[scheduling]") {
   const size_t BIG_PLAN_SIZE = 60;
   const size_t BIG_PLAN_WORKERS = 5;

   Server server(NULL, 0);
   ComputationState s(server);
   s.add_plan(make_big_plan(server, BIG_PLAN_SIZE));

   std::vector<WorkerConnection*> ws;
   ws.reserve(BIG_PLAN_WORKERS);
   for (size_t i = 0; i < BIG_PLAN_WORKERS; i++) {
      ws.push_back(simple_worker(server, "w", 2));      
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
   REQUIRE(sum == BIG_PLAN_WORKERS * 2);
}

TEST_CASE("Big simple plan", "[scheduling]") {
   const size_t BIG_PLAN_SIZE = 200;
   const size_t BIG_PLAN_WORKERS = 20;

   Server server(NULL, 0);
   ComputationState s(server);
   s.add_plan(make_big_trivial_plan(server, BIG_PLAN_SIZE));

   std::vector<WorkerConnection*> ws;
   ws.reserve(BIG_PLAN_WORKERS);
   for (size_t i = 0; i < BIG_PLAN_WORKERS; i++) {
      ws.push_back(simple_worker(server, "w", 5));      
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
   REQUIRE(sum == BIG_PLAN_WORKERS * 5);
}

TEST_CASE("Request plan", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   s.add_plan(make_request_plan(server));

   SECTION("0 cpus - include free tasks") {
       auto w1 = simple_worker(server, "w1", 0);
       s.test_ready_nodes(range(8));
       TaskDistribution d = schedule(s);
       REQUIRE(d[w1].size() == 1);
       REQUIRE(d[w1][0] == 7);
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
       REQUIRE((d[w1] == std::vector<loom::base::Id>{0} ||
                d[w1] == std::vector<loom::base::Id>{1}));
       REQUIRE((d[w2] == std::vector<loom::base::Id>{2} ||
                d[w2] == std::vector<loom::base::Id>{3}));
   }

   SECTION("2/2 cpus") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       s.test_ready_nodes(range(7));
       TaskDistribution d = schedule(s);
       REQUIRE((d[w1] == std::vector<loom::base::Id>{2} ||
                d[w1] == std::vector<loom::base::Id>{3}));
       REQUIRE((d[w2] == std::vector<loom::base::Id>{2} ||
                d[w2] == std::vector<loom::base::Id>{3}));
   }

   SECTION("5 cpus") {
       auto w1 = simple_worker(server, "w1", 5);
       s.test_ready_nodes(range(6));
       TaskDistribution d = schedule(s);

       REQUIRE((check_uvector(d[w1], {0, 5}) ||
                check_uvector(d[w1], {1, 5})));
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
       REQUIRE((check_uvector(d[w1], {6, 7}) || check_uvector(d[w1], {5, 7})));
   }
}


TEST_CASE("Plan continution (plan2)", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   s.add_plan(make_plan2(server));

   SECTION("Stick together") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       s.test_ready_nodes({0, 1});
       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE((d[w1].size() == 2 || d[w2].size() == 2));
   }

   SECTION("Stick to gether - 2") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 1);
       s.test_ready_nodes({0, 3, 4});
       TaskDistribution d = schedule(s);
       dump_dist(d);
       REQUIRE(check_uvector(d[w1], {3, 4}));
       REQUIRE(check_uvector(d[w2], {0}));
   }
}


TEST_CASE("Plan continution (plan3)", "[scheduling]") {
   Server server(NULL, 0);
   ComputationState s(server);
   s.add_plan(make_plan3(server));

   SECTION("Stick together - inputs dominant ") {
       auto w1 = simple_worker(server, "w1", 2);
       auto w2 = simple_worker(server, "w2", 2);
       auto w3 = simple_worker(server, "w3", 0);

       finish(s, 0, 800000000, 0, w3);
       finish(s, 1, 800000, 0, w3);
       finish(s, 2, 800000000, 0, w3);

       s.test_ready_nodes({3, 4, 5, 6});

       TaskDistribution d = schedule(s);
       //dump_dist(d);
       REQUIRE((check_uvector(d[w1], {3, 5}) || (check_uvector(d[w1], {4, 6}))));
       REQUIRE((check_uvector(d[w2], {3, 5}) || (check_uvector(d[w2], {4, 6}))));
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
       REQUIRE((check_uvector(d[w1], {3, 4}) || (check_uvector(d[w1], {5, 6}))));
       REQUIRE((check_uvector(d[w2], {3, 4}) || (check_uvector(d[w2], {5, 6}))));
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
       REQUIRE(check_uvector(d[w2], {4}));
       REQUIRE(check_uvector(d[w1], {}));
       REQUIRE(check_uvector(d[w3], {}));
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
       REQUIRE(check_uvector(d[w1], {4}));
       REQUIRE(check_uvector(d[w2], {}));
       REQUIRE(check_uvector(d[w3], {}));
   }
}
