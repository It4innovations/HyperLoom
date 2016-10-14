#include "solver.h"

#include <libloom/log.h>
#include <libloom/utils.h>

#include <lpsolve/lp_lib.h>

#define LP_CHECK(call) \
{ \
    if (unlikely((!call))) { \
    report_lp_error(__LINE__, __FILE__); \
    } \
    }

static void report_lp_error(int line, const char* filename)
{
    loom::llog->critical("lp_solve error at {}:{}", filename, line);
    exit(1);
}

Solver::Solver(size_t variables)
    : lp_solver(nullptr)
{
    assert(variables > 0);
    assert(sizeof(REAL) == sizeof(double));

    lprec *lp;
    lp = make_lp(0, variables);
    LP_CHECK(lp);

    for (size_t i = 1; i <= variables; i++) {
        set_binary(lp, i, TRUE);
    }

    set_add_rowmode(lp, TRUE);


    lp_solver = lp;
    this->variables = variables;
}

Solver::~Solver()
{
    lprec *lp = static_cast<lprec*>(lp_solver);
    if (lp) {
        delete_lp(lp);
    }
}

void Solver::add_constraint_lq(int index1, int index2)
{
    if (index1 == index2) {
        return;
    }
    double v[2] = { 1.0, -1.0 };
    int i[2];
    i[0] = index1;
    i[1] = index2;
    lprec *lp = static_cast<lprec*>(lp_solver);
    LP_CHECK(add_constraintex(lp, 2, v, i, LE, 0.0));
}

void Solver::add_constraint_eq(const std::vector<int> &indices, const std::vector<double> &values, double rhs)
{
    lprec *lp = static_cast<lprec*>(lp_solver);
    size_t size = indices.size();
    assert(values.size() >= size);
    LP_CHECK(add_constraintex(
                 lp, size,
                 const_cast<double*>(&values[0]),
             const_cast<int*>(&indices[0]),
            EQ, rhs));
}

void Solver::add_constraint_lq(const std::vector<int> &indices, const std::vector<double> &values, double rhs)
{
    lprec *lp = static_cast<lprec*>(lp_solver);
    size_t size = indices.size();
    assert(values.size() >= size);
    LP_CHECK(add_constraintex(lp, size,
                              const_cast<double*>(&values[0]),
             const_cast<int*>(&indices[0]),
            LE, rhs));
}

void Solver::set_objective_fn(const std::vector<double> &values)
{
    assert(values.size() == variables + 1);
    lprec *lp = static_cast<lprec*>(lp_solver);
    set_obj_fn(lp, const_cast<double*>(&values[0]));
}

std::vector<double> Solver::solve()
{
    constexpr bool debug = false;

    lprec *lp = static_cast<lprec*>(lp_solver);
    set_add_rowmode(lp, FALSE);
    if (debug) {
        write_LP(lp, stdout);
    }
    set_verbose(lp, IMPORTANT);

    set_presolve(lp, /* PRESOLVE_ROWS | */ PRESOLVE_REDUCEMIP + PRESOLVE_KNAPSACK + PRESOLVE_COLS + PRESOLVE_LINDEP, get_presolveloops(lp));

    int ret = ::solve(lp);
    assert(ret == OPTIMAL || ret == SUBOPTIMAL);

    std::vector<double> result(variables);
    get_variables(lp, &result[0]);

    if (debug) {
        for (size_t i = 0; i < variables; i++) {
            loom::llog->alert("{}: {}", i + 1, result[i]);
        }
    }
    return result;
}
