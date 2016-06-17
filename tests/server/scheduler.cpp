#include "catch/catch.hpp"
#include <uv.h>
#include "src/server/server.h"
#include "libloom/loomplan.pb.h"

typedef std::unordered_set<TaskNode*> TaskSet;

typedef
    std::unordered_map<WorkerConnection*, TaskSet>
    DistMap;

DistMap to_distmap(TaskManager::WorkDistribution dist)
{
    DistMap map;
    for (auto &load : dist) {
        assert (map.find(&load.connection) == map.end());
        auto &set = map[&load.connection];
        for (auto tn : load.tasks) {
             set.insert(tn);
        }
    }
    return map;
}

TEST_CASE( "Server scheduling - separate tasks", "[scheduling]" ) {
    Server server(NULL, 0);
    TaskManager &manager = server.get_task_manager();

    loomplan::Plan plan;
    loomplan::Task *t;
    plan.add_tasks();
    plan.add_tasks();
    manager.add_plan(plan, false);


    TaskNode::Vector v;
    v.push_back(&manager.get_task(0));
    v.push_back(&manager.get_task(1));

    auto d = to_distmap(manager.compute_distribution(v));
    CHECK(d.size() == 0);

    std::vector<std::string> tt;

    auto wconn = std::make_unique<WorkerConnection>(server, nullptr, "w1", tt);
    WorkerConnection *w1 = wconn.get();
    server.add_worker_connection(std::move(wconn));

    auto d2 = to_distmap(manager.compute_distribution(v));
    CHECK(d2.size() == 1);

    wconn = std::make_unique<WorkerConnection>(server, nullptr, "w2", tt);
    WorkerConnection *w2 = wconn.get();
    server.add_worker_connection(std::move(wconn));
    CHECK(server.get_connections().size() == 2);

    auto d3 = to_distmap(manager.compute_distribution(v));
    CHECK(d3.size() == 2);

    bool first = d3[w1] == TaskSet{v[0]} && d3[w2] == TaskSet{v[1]};
    bool second = d3[w1] == TaskSet{v[1]} && d3[w2] == TaskSet{v[0]};
    bool result = first | second;
    CHECK(result);
}
