#ifndef LOOM_SERVER_TASKSTATE_H
#define LOOM_SERVER_TASKSTATE_H

#include <unordered_map>

#include "libloom/types.h"

class WorkerConnection;
template<typename T> using WorkerMap = std::unordered_map<WorkerConnection*, T>;
class ComputationState;
class PlanNode;

class TaskState {

public:
    enum WStatus {
        S_NONE,
        S_READY,
        S_RUNNING,
        S_OWNER,
    };

    TaskState(const PlanNode &node);

    loom::Id get_id() const {
        return id;
    }

    void inc_ref_counter(int count = 1) {
        ref_count += count;
    }

    bool dec_ref_counter() {
        return --ref_count <= 0;
    }

    int get_ref_counter() const {
        return ref_count;
    }

    WorkerMap<WStatus>& get_workers() {
        return workers;
    }

    size_t get_size() const {
        return size;
    }

    void set_size(size_t size) {
        this->size = size;
    }

    size_t get_length() const {
        return length;
    }

    void set_length(size_t length) {
        this->length = length;
    }

    WStatus get_worker_status(WorkerConnection *wc) {
        auto i = workers.find(wc);
        if (i == workers.end()) {
            return S_NONE;
        }
        return i->second;
    }

    WorkerConnection *get_first_owner() {
        for (auto &p : workers) {
            if (p.second == S_OWNER) {
                return p.first;
            }
        }
        return nullptr;
    }

    void set_worker_status(WorkerConnection *wc, WStatus ws) {
        workers[wc] = ws;
    }

    template<typename F> void foreach_owner(F f) {
        for(auto &pair : workers) {
            if (pair.second == S_OWNER) {
                f(pair.first);
            }
        }
    }

private:
    loom::Id id;
    WorkerMap<WStatus> workers;
    int ref_count;
    size_t size;
    size_t length;
};

#endif // LOOM_SERVER_TASKSTATE_H
