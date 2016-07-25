from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

import os

loom_env  # silence flake8


def test_dslice(loom_env):
    loom_env.start(1)
    p = loom_env.plan()

    consts = []
    for i in xrange(16):
        consts.append(p.task_const("data{}".format(i)))
    a = p.task_array_make(consts)
    ds = p.task_dslice(a)
