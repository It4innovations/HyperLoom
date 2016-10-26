from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa
import loom.client.tasks as tasks  # noqa

loom_env  # silence flake8


def test_dslice(loom_env):
    loom_env.start(2)
    consts = []
    for i in xrange(16):
        consts.append(tasks.const("data{}".format(i)))
    a = tasks.array_make(consts)
    ds = tasks.dslice(a)
    f = tasks.get(ds, 0)
    r = tasks.array_make((f,))
    result = loom_env.submit(r)

    assert len(result) >= 2
    assert result == ["data{}".format(i) for i in xrange(0, 16, 2)]


def test_dget(loom_env):
    loom_env.start(2)
    consts = []
    for i in xrange(16):
        consts.append(tasks.const("data{}".format(i)))
    a = tasks.array_make(consts)
    ds = tasks.dget(a)
    f = tasks.run("/bin/cat", stdin=ds)
    r = tasks.array_make((f,))
    result = loom_env.submit(r)
    assert result == ["data{}".format(i) for i in xrange(16)]
