from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa
import loom.client.tasks as tasks  # noqa
from loom.client import TaskFailed
import pytest

loom_env  # silence flake8


def test_py_call(loom_env):

    def f(a, b):
        return "{}, {}, {}, {}".format(
            str(a.read(), encoding="ascii"), a.size(),
            str(b.read(), encoding="ascii"), b.size())

    def g():
        return "Test"

    loom_env.start(1)
    c = tasks.const("ABC")
    d = tasks.const("12345")
    p = tasks.py_call(f, (c, d))
    q = tasks.py_call(g)
    result1, result2 = loom_env.submit((p, q))

    assert result1 == b"ABC, 3, 12345, 5"
    assert result2 == b"Test"


def test_py_redirect1(loom_env):

    def f(a, b):
        return tasks.merge((a, b))

    loom_env.start(1)

    c = tasks.const("ABC")
    d = tasks.const("12345")
    a = tasks.py_call(f, (c, d))
    result = loom_env.submit(a)
    assert result == b"ABC12345"


def test_py_redirect2(loom_env):

    def f(a, b):
        return tasks.run("/bin/ls $X", [(b, "$X")])

    loom_env.start(1)

    c = tasks.const("abcdef")
    d = tasks.const("/")
    a = tasks.py_call(f, (c, d))
    result = loom_env.submit(a)
    assert b"bin\n" in result
    assert b"usr\n" in result


def test_py_fail_too_many_args(loom_env):

    def g():
        return "Test"

    loom_env.start(1)
    c = tasks.const("ABC")
    a = tasks.py_call(g, (c,))

    with pytest.raises(TaskFailed):
        loom_env.submit(a)


def test_py_fail_too_few_args(loom_env):

    def f(a):
        return "ABC"

    loom_env.start(1)

    a = tasks.py_call(f, ())

    with pytest.raises(TaskFailed):
        loom_env.submit(a)


def test_py_fail_invalid_result(loom_env):

    def f():
        return 42.0

    loom_env.start(1)

    a = tasks.py_call(f, ())

    with pytest.raises(TaskFailed):
        loom_env.submit(a)
