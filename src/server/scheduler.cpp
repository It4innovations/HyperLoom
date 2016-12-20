
#include "scheduler.h"
#include "workerconn.h"

#include "libloom/compat.h"
#include "libloom/log.h"

#include <sstream>

static constexpr size_t DEFAULT_EXPECTED_SIZE = 5 * 1024 * 1024; // 5 MB


template< typename Iter1, typename Iter2 >
static bool intersects(Iter1 iter1, Iter1 iter1_end, Iter2 iter2, Iter2 iter2_end)
{
   while(iter1 != iter1_end && iter2 != iter2_end) {
      if(*iter1 < *iter2) {
         ++iter1;
      } else if (*iter2 < *iter1) {
         ++iter2;
      } else {
         return true;
      }
   }
   return false;
}

template< typename T>
static bool intersects(const std::vector<T> &v2, const std::vector<T> &v1)
{
   return intersects(v1.begin(), v1.end(), v2.begin(), v2.end());
}

template<typename T>
static std::vector<T>
merge_vectors(const std::vector<T> &in1, const std::vector<T> in2)
{
   std::vector<T> out;
   out.reserve(in1.size() + in2.size());
   out.insert(out.end(), in1.begin(), in1.end());
   out.insert(out.end(), in2.begin(), in2.end());
   return out;
}

template<typename T>
static void
erase_duplicates(std::vector<T> &v)
{
   std::sort(v.begin(), v.end());
   v.erase(std::unique(v.begin(), v.end()), v.end());
}

Scheduler::Scheduler(ComputationState &cstate)
{
   if (cstate.get_worker_info().size() == 0 ||
       cstate.get_pending_tasks().size() == 0) {
      return;
   }

   // Gather info about workers
   workers.reserve(cstate.get_worker_info().size());
   std::unordered_map<WorkerConnection*, size_t> worker_map;
   worker_map.reserve(cstate.get_worker_info().size());

   int max_free_cpus = 0;
   for (auto pair : cstate.get_worker_info()) {
      int free_cpus = pair.second.free_cpus;
      if (free_cpus > max_free_cpus) {
         max_free_cpus = free_cpus;
      }
      worker_map[pair.first] = workers.size();
      Worker w;
      w.free_cpus = pair.second.free_cpus;
      w.max_cpus = pair.first->get_resource_cpus();
      w.wc = pair.first;
      workers.push_back(std::move(w));
   }

   // Gather info about pending tasks
   std::unordered_map<loom::base::Id, size_t> inputs;
   std::unordered_map<loom::base::Id, size_t> nexts;
   size_t input_i = 0;

   // * 2 because units is expanded in solve()
   s_units.reserve(cstate.get_pending_tasks().size() * 2);

   for (loom::base::Id id : cstate.get_pending_tasks()) {
      const PlanNode &node = cstate.get_node(id);

      SUnit s_unit;
      s_unit.n_cpus = node.get_n_cpus();

      if (s_unit.n_cpus > max_free_cpus) {
         continue;
      }
      s_unit.bonus_score = s_unit.n_cpus;
      s_unit.expected_size = DEFAULT_EXPECTED_SIZE;

      s_unit.ids.push_back(id);

      for (loom::base::Id id2 : node.get_inputs()) {
         auto it = inputs.find(id2);
         if (it == inputs.end()) {
            s_unit.inputs.push_back(input_i);
            inputs[id2] = input_i++;
         } else {
            s_unit.inputs.push_back(it->second);
         }
      }

      for (loom::base::Id id2 : node.get_nexts()) {
         const PlanNode &node2 = cstate.get_node(id2);
         for (loom::base::Id id3 : node2.get_inputs()) {
            const TaskState *state = cstate.get_state_ptr(id3);
            if (state) {
                  s_unit.nexts.push_back(id2);
                  auto it = inputs.find(id3);
                  if (it == inputs.end()) {
                     s_unit.next_inputs.push_back(input_i);
                     inputs[id3] = input_i++;
                  } else {
                     s_unit.next_inputs.push_back(it->second);
                  }
            }
         }
      }

      erase_duplicates(s_unit.inputs);
      erase_duplicates(s_unit.next_inputs);
      erase_duplicates(s_unit.next_inputs);
      s_units.push_back(std::move(s_unit));


   }

/*   for (auto &pair : nexts)
   {
      const PlanNode &node = cstate.get_node(pair.first);
      NUnit n_unit;
      n_unit.n_cpus = node.get_n_cpus();
      for (loom::base::Id id : node.get_inputs()) {
         const TaskState *state = cstate.get_state_ptr(id);
         if (state) {
            state->foreach_source([&input_i, id, &n_unit, &inputs](WorkerConnection *wc) {
               auto it = inputs.find(id);
               if (it == inputs.end()) {
                  n_unit.inputs.push_back(input_i);
                  inputs[id] = input_i++;
               } else {
                  n_unit.inputs.push_back(it->second);
               }
            });
         }
      }
      n_units[pair.second] = std::move(n_unit);
   }*/
   assert(input_i == inputs.size());
   data.resize(inputs.size());

   for (auto &pair : inputs)
   {
      const TaskState &state = cstate.get_state(pair.first);
      DataObj obj;
      obj.size = state.get_size();
      state.foreach_source([&obj, &worker_map](WorkerConnection *wc) {
         obj.owners.push_back(worker_map[wc]);
      });
      data[pair.second] = std::move(obj);
   }
}

TaskDistribution Scheduler::schedule()
{
   TaskDistribution result;

   if (workers.size() == 0 || s_units.size() == 0) {
      return result;
   }

   create_derived_units();

   for (;;) {
      UW uw;
      if (!find_best(uw)) {
         break;
      }
      apply_uw(uw);

      auto &unit = s_units[uw.unit_index];

      /* DEBUG
      std::stringstream s;
      for (auto id : unit.ids) {
         s << " " << id;
      }
      loom::logger->alert("PLANNED UNIT = {} / {}", s.str(), uw.worker_index);
      */

      auto ids = unit.ids; // we need a copy
      auto &v = result[workers[uw.worker_index].wc];
      v.insert(v.end(), unit.ids.begin(), unit.ids.end());

      s_units.erase(std::remove_if(s_units.begin(),
                                   s_units.end(),
                                   [&ids](const SUnit &u){return intersects(u.ids, ids);}),
                    s_units.end());

   };

   return result;
}

bool Scheduler::find_best(UW &uw)
{
   bool found = false;
   size_t best_unit_i, best_worker_i;
   double best_score = INT64_MIN;
   int64_t n_workers = workers.size();

   std::vector<int64_t> sizes(workers.size(), 0);
   std::vector<int64_t> next_sizes(workers.size(), 0);

   for (size_t unit_i = 0; unit_i < s_units.size(); unit_i++) {
      const SUnit &unit = s_units[unit_i];
      int n_cpus = unit.n_cpus;
      int64_t size_sum = 0;
      int64_t next_sum = 0;

      // Reset sizes
      for (int64_t worker_i = 0; worker_i < n_workers; worker_i++) {
         sizes[worker_i] = 0;
      }

      // Compose sizes
      for (size_t input_i : unit.inputs) {
         assert (input_i < data.size());
         const DataObj &obj = data[input_i];
         size_sum += obj.size;
         for (size_t worker_i : obj.owners) {
            sizes[worker_i] += obj.size;
         }
      }

      // Reset next sizes
      for (int64_t worker_i = 0; worker_i < n_workers; worker_i++) {
         next_sizes[worker_i] = 0;
      }

      // Compose next sizes
      for (size_t input_i : unit.next_inputs) {
         const DataObj &obj = data[input_i];
         for (size_t worker_i : obj.owners) {
            next_sum += obj.size;
            next_sizes[worker_i] += obj.size;
         }
      }


      for (int64_t worker_i = 0; worker_i < n_workers; worker_i++) {
         if (workers[worker_i].free_cpus < n_cpus) {
            continue;
         }

         int64_t score2 = (next_sizes[worker_i] - next_sum / n_workers) / 10;
         if (score2 > unit.expected_size) {
            score2 = unit.expected_size;
         } else if (score2 < -unit.expected_size) {
            score2 = -unit.expected_size;
         }

         int n_cpus = unit.n_cpus;
         if (n_cpus == 0) {
            n_cpus = 1;
         }
         double score = (sizes[worker_i] - size_sum) / n_cpus;
         score += score2 / 10.0 + unit.bonus_score;

         /*DEBUG
         std::stringstream s;
         for (auto id : unit.ids) {
            s << " " << id;
         }
         loom::logger->alert("SCORE {} / {} : {} [size={}, size_sum={}, {}]", s.str(), worker_i, score, sizes[worker_i], size_sum, n_workers);
         */

         if (best_score < score) {
            found = true;
            best_score = score;
            best_unit_i = unit_i;
            best_worker_i = worker_i;
         }
      }
   }

   if (found) {
      uw.unit_index = best_unit_i;
      uw.worker_index = best_worker_i;
      return true;
   } else {
      return false;
   }
}

void Scheduler::apply_uw(const Scheduler::UW &uw)
{
   SUnit &unit = s_units[uw.unit_index];
   size_t worker_index = uw.worker_index;

   workers[worker_index].free_cpus -= unit.n_cpus;

   for (size_t input_i : unit.inputs) {
      DataObj &obj = data[input_i];
      if (std::find(obj.owners.begin(), obj.owners.end(), worker_index) == obj.owners.end()) {
         obj.owners.push_back(worker_index);
      }
   }
}

Scheduler::SUnit Scheduler::merge_units(const SUnit &u1, const SUnit &u2)
{
   SUnit unit;
   unit.n_cpus = u1.n_cpus + u2.n_cpus;
   unit.bonus_score = unit.n_cpus / (u1.ids.size() + u2.ids.size());

   if (intersects(u1.nexts.begin(), u1.nexts.end(),
                  u2.nexts.begin(), u2.nexts.end())) {
      unit.bonus_score += std::min(u1.expected_size, u2.expected_size);
   }
   unit.expected_size = u1.expected_size + u2.expected_size;
   unit.ids = merge_vectors(u1.ids, u2.ids);
   unit.inputs = merge_vectors(u1.inputs, u2.inputs);
   erase_duplicates(unit.inputs);

   unit.nexts = merge_vectors(u1.nexts, u2.nexts);
   // Do remove
   return unit;
}

void Scheduler::create_derived_units()
{
   const size_t initial_size = s_units.size();

   // Sort basic units by id
   std::sort(s_units.begin(), s_units.end(),
             [](const SUnit &u1, const SUnit &u2) -> bool {
      return u1.ids[0] < u2.ids[0];
   });

   // Create pairs
   for (size_t i = 0; i < initial_size; i++) {
      for (size_t j = i + 1; j < initial_size; j++) {
         const SUnit &u1 = s_units[i];
         const SUnit &u2 = s_units[j];
         if (intersects(u1.inputs.begin(), u1.inputs.end(),
                        u2.inputs.begin(), u2.inputs.end())) {
            s_units.push_back(merge_units(u1, u2));
         } else if (intersects(u1.nexts.begin(), u1.nexts.end(),
                        u2.nexts.begin(), u2.nexts.end())) {
            s_units.push_back(merge_units(u1, u2));
         }
      }
   }
}


/*TaskDistribution Scheduler::single_worker_schedule()
{
    assert(workers.size() == 1);

    std::sort(units.begin(), units.end(),
        [](const SUnit & a, const SUnit & b) -> bool
    {
        return a.n_cpus > b.n_cpus;
    });

    Worker w = workers[0];
    std::vector<loom::base::Id> result;

    for (auto &unit : units) {
        if (unit.n_cpus <= w.free_cpus) {
            w.free_cpus -= unit.n_cpus;
            result.push_back(unit.ids[0]);
        }
    }

    TaskDistribution d;
    d[w.wc] = std::move(result);
    return d;
}
*/
