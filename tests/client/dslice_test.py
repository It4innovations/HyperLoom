from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

import os

loom_env  # silence flake8


def test_dslice(loom_env):
    loom_env.start(2)
    p = loom_env.plan()

    consts = []
    for i in xrange(16):
        consts.append(p.task_const("data{}".format(i)))
    a = p.task_array_make(consts)
    ds = p.task_dslice(a)
    f = p.task_get(ds, 0)
    r = p.task_array_make((f,))
    result = loom_env.submit(p, r)

    assert len(result) >= 2
    assert sum(result, []) == ["data{}".format(i) for i in xrange(16)]
