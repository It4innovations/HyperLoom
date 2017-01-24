from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa
import loom.client.tasks as tasks  # noqa

from loom import client
import pytest

loom_env  # silence flake8


def test_invalid_program(loom_env):
    loom_env.start(1)
    a = tasks.run("/usr/bin/non-existing-program")

    with pytest.raises(client.TaskFailed):
        loom_env.submit(a)


def test_program_failed(loom_env):
    loom_env.start(1)
    a = tasks.run("ls /non-existing-dictionary")

    with pytest.raises(client.TaskFailed):
        loom_env.submit(a)


def test_output_failed(loom_env):
    loom_env.start(1)
    a = tasks.run("ls /", outputs=["xxx"])

    with pytest.raises(client.TaskFailed):
        loom_env.submit(a)


def test_get_failed(loom_env):
    loom_env.start(1)
    a = tasks.const("A")
    b = tasks.const("BB")
    c = tasks.const("CCC")
    array = tasks.array_make((a, b, c))
    x = tasks.get(array, 3)
    with pytest.raises(client.TaskFailed):
        loom_env.submit(x)


def test_recover1(loom_env):
    loom_env.start(1)
    for i in range(10):
        a = tasks.run("ls /non-existing-dictionary")
        with pytest.raises(client.TaskFailed):
            loom_env.submit(a)
        b = tasks.const("XYZ")
        c = tasks.const("12345")
        d = tasks.merge((b, c))
        assert loom_env.submit(d) == b"XYZ12345"


def recover2(loom_env, n):
    @tasks.py_task()
    def slow_task1():
        import time
        time.sleep(0.4)
        raise Exception("FAIL_2")

    @tasks.py_task()
    def slow_task2():
        import time
        time.sleep(0.4)
        return b"ABC"

    @tasks.py_task()
    def fail_task():
        import time
        time.sleep(0.10)
        raise Exception("FAIL")

    loom_env.start(n)
    x1 = slow_task1()
    x2 = slow_task2()
    x3 = fail_task()
    x4 = tasks.merge((x1, x2, x3))

    b = tasks.const("XYZ")
    c = tasks.const("12345")
    d = tasks.merge((b, c))

    for i in range(6):
        with pytest.raises(client.TaskFailed):
            loom_env.submit(x4)
        assert loom_env.submit(d) == b"XYZ12345"


def test_recover2_1(loom_env):
    recover2(loom_env, 1)


def test_recover2_2(loom_env):
    recover2(loom_env, 2)


def test_recover2_3(loom_env):
    recover2(loom_env, 3)
