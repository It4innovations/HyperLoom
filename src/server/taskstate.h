#ifndef LOOM_SERVER_TASKSTATE_H
#define LOOM_SERVER_TASKSTATE_H

#include <unordered_map>

#include "libloom/types.h"

class WorkerConnection;
template<typename T> using WorkerMap = std::unordered_map<WorkerConnection*, T>;
class ComputationState;
class PlanNode;

enum class WStatus {
    NONE,
    READY,
    RUNNING,
    TRANSFER,
    OWNER,
};

inline bool is_planned_owner(WStatus status) {
    return status == WStatus::OWNER || \
           status == WStatus::RUNNING || \
           status == WStatus::TRANSFER;
}


class TaskState {

public:
    TaskState(const PlanNode &node);

    loom::base::Id get_id() const {
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
            return WStatus::NONE;
        }
        return i->second;
    }


    bool has_owner() const {
        return get_first_owner() == nullptr;
    }


    WorkerConnection *get_first_owner() const {
        for (auto &p : workers) {
            if (p.second == WStatus::OWNER) {
                return p.first;
            }
        }
        return nullptr;
    }

    void set_worker_status(WorkerConnection *wc, WStatus ws) {
        workers[wc] = ws;
    }

    bool is_running() const {
        for(auto &pair : workers) {
            if (pair.second == WStatus::RUNNING) {
                return true;
            }
        }
        return false;
    }

    template<typename F> void foreach_planned_owner(const F &f) const {
        for(auto &pair : workers) {
            if (is_planned_owner(pair.second)) {
                f(pair.first);
            }
        }
    }

    template<typename F> void foreach_owner(const F &f) const {
        for(auto &pair : workers) {
            if (pair.second == WStatus::OWNER) {
                f(pair.first);
            }
        }
    }

    template<typename F> void foreach_worker(const F &f) const {
        for(auto &pair : workers) {
            f(pair.first, pair.second);
        }
    }

    std::string get_info() const;

private:
    loom::base::Id id;
    WorkerMap<WStatus> workers;
    int ref_count;
    size_t size;
    size_t length;
};

#endif // LOOM_SERVER_TASKSTATE_H
