from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

import os

loom_env  # silence flake8


def test_dslice(loom_env):
    loom_env.start(2)
    p = loom_env.plan_builder()

    consts = []
    for i in xrange(16):
        consts.append(p.task_const("data{}".format(i)))
    a = p.task_array_make(consts)
    ds = p.task_dslice(a)
    f = p.task_get(ds, 0)
    r = p.task_array_make((f,))
    result = loom_env.submit(p, r)

    assert len(result) >= 2
    assert result == ["data{}".format(i) for i in xrange(0, 16, 2)]


def test_dget(loom_env):
    loom_env.start(2)
    p = loom_env.plan_builder()

    consts = []
    for i in xrange(16):
        consts.append(p.task_const("data{}".format(i)))
    a = p.task_array_make(consts)
    ds = p.task_dget(a)
    f = p.task_run("/bin/cat", stdin=ds)
    r = p.task_array_make((f,))
    result = loom_env.submit(p, r)
    assert result == ["data{}".format(i) for i in xrange(16)]
