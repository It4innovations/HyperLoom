
#include "scheduler.h"
#include "workerconn.h"
#include "server.h"

#include "libloom/compat.h"
#include "libloom/log.h"

#include <sstream>


using Score = int64_t;
static constexpr Score SCORE_MIN = INT64_MIN;
static constexpr Score UNIT_RECOMPUTE = INT64_MAX;

static constexpr size_t MIN_SCHEDULED_TASKS_LIMIT = 128;

static constexpr size_t OVERBOOKING_LIMIT = 8;
static constexpr size_t OVERBOOKING_FACTOR = 3;

static constexpr size_t INPUT_UPDATE_LIMIT = 32;

static constexpr size_t DEFAULT_EXPECTED_SIZE = 5 << 20; // 5 MB
static constexpr size_t NEXT_EXPLORE_LIMIT = 8;
static constexpr size_t NEXT_UPDATE_LIMIT = 12;

static constexpr Score NEXT_SIZE_FACTOR = 8;
static constexpr Score NEXT_SIZE_LIMIT = 5 << 20; // 5 MB

static constexpr Score BONUS_PER_EXTRA_CPU = 150 << 20; // 1MB
static constexpr Score BONUS_PER_NEXT = 250000;

struct Worker {
    WorkerConnection *wc;
    int free_cpus;
};

struct WSPair {
    WorkerConnection *wc;
    Score score;
};

struct SUnit {
    TaskNode *node;
    WorkerConnection *wc;
    Score score;
    int index;
};

struct SContext {
    size_t worker_size;
    std::vector<WorkerConnection*> workers;
    std::unordered_map<loom::base::Id, SUnit> units;
    std::unique_ptr<Score[]> score_table;
};

static inline WSPair find_best(int n_cpus, Score *table, SContext &context)
{
    WorkerConnection *wc = nullptr;
    Score score = SCORE_MIN;
    size_t worker_size = context.worker_size;
    for (size_t i = 0; i < worker_size; i++) {
        //loom::base::logger->alert("worker={} score={}", workers[i]->get_address(), table[i]);
        if (table[i] > score && n_cpus <= context.workers[i]->get_scheduler_free_cpus()) {
            score = table[i];
            wc = context.workers[i];
        }
    }
    WSPair p;
    p.wc = wc;
    p.score = score;
    return p;
}

static inline Score score_from_next_size(Score size)
{
    Score score = size / NEXT_SIZE_FACTOR;
    if (score > NEXT_SIZE_LIMIT) {
        score = NEXT_SIZE_LIMIT;
    }
    return score;
}

static void compute_table(const TaskNode *node,
                          Score *table,
                          size_t worker_size)
{
    // In this function we deliberately ignore a situation when a same id
    // is used as input on more positions
    // It is quite expensive to detect duplication, and since it is quite
    // rare ... and moreover this is just an heuristic at the end.

    // Init variables
    //Score input_size = 0;
    std::fill(table, table + worker_size, 0);
    Score total_size = 0;

    // Collect sizes
    for (const TaskNode *input_node : node->get_inputs()) {
        Score size = input_node->get_size();
        //input_size += size;
        for (const auto &pair : input_node->get_workers()) {
            WorkerConnection *wc = pair.first;
            table[wc->get_scheduler_index()] += size;
            total_size += size;
        }
    }

    // Score bonus
    Score score_bonus = -total_size / static_cast<Score>(worker_size);
    score_bonus += node->get_nexts().size() * BONUS_PER_NEXT;
    const int n_cpus = node->get_n_cpus();
    if (n_cpus > 1) {
        score_bonus += ((n_cpus - 1) * BONUS_PER_EXTRA_CPU);
    }

    for (size_t i = 0; i < worker_size; i++) {
        table[i] += score_bonus;
    }

    // Compute next score
    if (node->get_nexts().size() <= NEXT_EXPLORE_LIMIT) {
        for (TaskNode *next_node : node->get_nexts()) {
            if (next_node->get_inputs().size() > NEXT_EXPLORE_LIMIT) {
                continue;
            }
            for (TaskNode *input_node : next_node->get_inputs()) {
                if (node == input_node) {
                    continue;
                }
                if (input_node->has_state()) {
                    Score score = score_from_next_size(input_node->get_size());
                    for (const auto &pair : input_node->get_workers()) {
                        WorkerConnection *wc = pair.first;
                        table[wc->get_scheduler_index()] += score;
                    }
                }
            }
        }
    }
}

static inline void init_unit(int index,
                             TaskNode *node,
                             SContext &context)
{
    size_t worker_size = context.worker_size;
    Score *table = context.score_table.get() + index * worker_size;
    compute_table(node, table, worker_size);
    /*loom::base::logger->alert("SUNIT {}", node->get_id());
    for (size_t i = 0; i < worker_size; i++) {
        loom::base::logger->alert("INIT id={} worker={} score={}", node->get_id(), context.workers[i]->get_address(), table[i]);
    }*/
    WSPair ws_pair = find_best(node->get_n_cpus(), table, context);
    if (ws_pair.score != SCORE_MIN) {
        SUnit unit;
        unit.node = node;
        unit.wc = ws_pair.wc;
        unit.score = ws_pair.score;
        unit.index = index;
        context.units.emplace(std::make_pair(node->get_id(), std::move(unit)));
    }
}


TaskDistribution schedule(const ComputationState &cstate)
{
    TaskDistribution result;

    // Init workers
    auto &worker_conns = cstate.get_server().get_workers();
    size_t worker_size = worker_conns.size();
    if (worker_size == 0 ||
            cstate.get_pending_tasks().size() == 0) {
        return result;
    }

    SContext context;
    context.workers.reserve(worker_size);

    int index = 0;
    size_t total_free_cpus = 0;
    size_t total_cpus = 0;

    for (auto &wc : worker_conns) {
       if (unlikely(wc->is_blocked())) {
          worker_size--;
          continue;
       }
       total_cpus += wc->get_resource_cpus();
       int free_cpus = wc->get_free_cpus();
       total_free_cpus += free_cpus;
       wc->set_scheduler_index(index++);
       wc->set_scheduler_free_cpus(free_cpus);
       context.workers.push_back(wc.get());
    }

    if (worker_size == 0) {
       return result;
    }

    context.worker_size = worker_size;

    size_t ptasks_size = cstate.get_pending_tasks().size();

    if (ptasks_size > (total_cpus + 1) * OVERBOOKING_LIMIT) {
        for (auto &wc : context.workers) {
            auto free_cpus = wc->get_scheduler_free_cpus();
            free_cpus += wc->get_resource_cpus() * (OVERBOOKING_FACTOR - 1);
            wc->set_scheduler_free_cpus(free_cpus);
        }
        total_free_cpus += (OVERBOOKING_FACTOR - 1) * total_cpus;
    }

    size_t limit = total_free_cpus * 5 + total_cpus;
    if (limit < MIN_SCHEDULED_TASKS_LIMIT) {
        limit = MIN_SCHEDULED_TASKS_LIMIT;
    }

    std::vector<std::unordered_set<loom::base::Id>> scheduled_moves(worker_size);

    loom::base::logger->debug("Scheduler: {} pending task(s) on {} worker(s) / free_cpus={}",
                              cstate.get_pending_tasks().size(),
                              worker_size, total_free_cpus);

    // Init units


    if (ptasks_size <= limit) {
        context.units.reserve(ptasks_size);
        context.score_table = std::make_unique<Score[]>(worker_size * ptasks_size);
        index = 0;
        for (TaskNode* node: cstate.get_pending_tasks()) {
            init_unit(index, node, context);
            index++;
        }
    } else { // pick limit number of tasks with lowest ids
        std::vector<TaskNode*> nodes;
        nodes.reserve(ptasks_size);
        for (TaskNode *node : cstate.get_pending_tasks()) {
            nodes.push_back(node);
        }
        std::nth_element(nodes.begin(), nodes.begin() + limit, nodes.end(),
                         [](const TaskNode *a, const TaskNode *b) { return a->get_id() < b->get_id(); });

        context.units.reserve(limit);
        context.score_table = std::make_unique<Score[]>(worker_size * limit);
        for (index = 0; index < static_cast<int>(limit); index++) {
            init_unit(index, nodes[index], context);
        }
    }

    // Just create an pointer that does not definitely point to a valid WorkerConnection
    WorkerConnection *invalid_ptr = reinterpret_cast<WorkerConnection*>(&invalid_ptr);
    WorkerConnection *last_changed = invalid_ptr;

    for(;;) {
        Score best_score = SCORE_MIN;
        WorkerConnection *best_wc = nullptr;
        TaskNode *best_node = nullptr;

        for (auto &pair : context.units) {
            SUnit &unit = pair.second;
            TaskNode *node = unit.node;

            if (unit.score == UNIT_RECOMPUTE ||
                    (last_changed == unit.wc && unit.wc->get_scheduler_free_cpus() < node->get_n_cpus())) {
                Score *table = context.score_table.get() + unit.index * worker_size;
                WSPair ws_pair = find_best(node->get_n_cpus(), table, context);
                unit.wc = ws_pair.wc;
                unit.score = ws_pair.score;
            }

            //loom::base::logger->alert("UNIT id={} worker={} score={}", unit.node->get_id(), unit.wc?unit.wc->get_address():"none", unit.score);
            if (unit.score > best_score) {
                best_score = unit.score;
                best_wc = unit.wc;
                best_node = node;
            }
        }

        if (best_score != SCORE_MIN) {
            auto n_cpus = best_node->get_n_cpus();
            auto id = best_node->get_id();

            if (best_wc == nullptr) {
                std::sort(context.workers.begin(), context.workers.end(),
                          [](WorkerConnection *a, WorkerConnection *b)
                            { return a->get_scheduler_free_cpus() > b->get_scheduler_free_cpus(); });
                if (context.workers[0]->get_scheduler_free_cpus() < n_cpus) {
                    auto it = context.units.find(id);
                    assert(it != context.units.end());
                    context.units.erase(it);
                    continue;
                }
                best_wc = context.workers[0];
            }
            //loom::base::logger->alert(">> SELECTED id={} worker={} score={}", id, best_wc->get_address(), best_score);
            result[best_wc].push_back(best_node);
            best_wc->set_scheduler_free_cpus(best_wc->get_scheduler_free_cpus() - n_cpus);
            if (n_cpus > 0) {
                last_changed = best_wc;
            } else {
                last_changed = invalid_ptr;
            }

            if (best_node->get_inputs().size() <= INPUT_UPDATE_LIMIT)
            {
                for (TaskNode *input_node : best_node->get_inputs()) {
                    if (// Check update limit
                        input_node->get_nexts().size() > INPUT_UPDATE_LIMIT ||
                        // Check that the input was ok
                        input_node->get_worker_status(best_wc) != TaskStatus::NONE||
                        // Check that we did not already planned the node
                        scheduled_moves[best_wc->get_scheduler_index()].find(input_node->get_id()) \
                            != scheduled_moves[best_wc->get_scheduler_index()].end()) {
                        continue;
                    }
                    scheduled_moves[best_wc->get_scheduler_index()].insert(input_node->get_id());
                    Score size = input_node->get_size();
                    for (TaskNode *next_node : input_node->get_nexts()) {
                        auto it = context.units.find(next_node->get_id());
                        if (it == context.units.end()) {
                            continue;
                        }
                        SUnit &unit = it->second;
                        unit.score = UNIT_RECOMPUTE;
                        Score *table = context.score_table.get() + unit.index * worker_size;
                        //loom::base::logger->alert("BOOSTING id={} worker={}", unit.node->get_id(), best_wc->get_address());
                        table[best_wc->get_scheduler_index()] += size;
                    }
                }
            }


            if (best_node->get_nexts().size() <= NEXT_UPDATE_LIMIT)
            {
                for (TaskNode *next_node : best_node->get_nexts()) {
                    if (next_node->get_inputs().size() > NEXT_UPDATE_LIMIT) {
                        continue;
                    }
                    for (TaskNode *input_node : next_node->get_inputs()) {
                        if (input_node == best_node || input_node->has_state()) {
                            continue;
                        }
                        auto it = context.units.find(input_node->get_id());
                        if (it == context.units.end()) {
                            continue;
                        }
                        SUnit &unit = it->second;
                        unit.score = UNIT_RECOMPUTE;
                        Score *table = context.score_table.get() + unit.index * worker_size;
                        //loom::base::logger->alert("BOOSTING id={} worker={}", unit.node->get_id(), best_wc->get_address());
                        table[best_wc->get_scheduler_index()] += NEXT_SIZE_LIMIT;
                    }
                }
            }

            auto it = context.units.find(id);
            assert(it != context.units.end());
            context.units.erase(it);
        } else {
            break;
        }
    }
    return result;
}
