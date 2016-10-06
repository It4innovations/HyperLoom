#ifndef LOOM_SERVER_SOLVER_H
#define LOOM_SERVER_SOLVER_H

#include <vector>

class Solver
{
public:
    Solver(std::size_t variables);
    ~Solver();

    void add_constraint_lq(int index1, int index2);
    void add_constraint_eq(const std::vector<int> &indices, const std::vector<double> &values, double rhs);
    void add_constraint_lq(const std::vector<int> &indices, const std::vector<double> &values, double rhs);
    void set_objective_fn(const std::vector<double> &values);
    std::vector<double> solve();
protected:
    /* lp_solve heavily pollutes namespace with macros,
     * so we do not want to include the library publicaly
     * lp_solver is in fact lprec internaly casted to the
     * the right type */
    void *lp_solver;
    std::size_t variables;
};

#endif // LOOM_SERVER_SOLVER_H
