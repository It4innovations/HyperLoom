from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa
import loom.client.tasks as tasks  # noqa
from loom.client.tasks import cpu1  # noqa
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
    p = tasks.py_call(tasks.py_value(f), (c, d))
    q = tasks.py_call(tasks.py_value(g))
    result1, result2 = loom_env.submit_and_gather((p, q))

    assert result1 == b"ABC, 3, 12345, 5"
    assert result2 == b"Test"


def test_py_task(loom_env):

    @tasks.py_task()
    def t1():
        return "ABC"

    @tasks.py_task(label="Merging task")
    def t2(a, b):
        return a.read() + b.read()

    loom_env.start(1)
    a = tasks.const("1234")
    b = t1()
    c = t2(a, b)
    result = loom_env.submit_and_gather(c)
    assert result == b"1234ABC"


def test_py_context_task(loom_env):

    @tasks.py_task(context=True)
    def t1(ctx):
        ctx.log_debug("DEBUG")
        ctx.log_info("INFO")
        ctx.log_warn("WARN")
        ctx.log_error("ERROR")
        ctx.log_critical("CRITICAL")
        return str(ctx.task_id)

    @tasks.py_task(context=True)
    def t2(ctx, a):
        return a.read()

    loom_env.start(1)
    a = tasks.const("1234")
    b = t1()
    c = t2(a)
    ra, rb = loom_env.submit_and_gather((b, c))
    assert 0 <= int(ra)
    assert rb == b"1234"


def test_py_direct_args(loom_env):

    @tasks.py_task(n_direct_args=1)
    def t1(p, a):
        return p + a.read()

    @tasks.py_task(n_direct_args=3)
    def t2(*args):
        return str(len(args))

    @tasks.py_task(n_direct_args=1)
    def t3(x, y):
        return x * y.read()

    loom_env.start(1)

    c = tasks.const("ABC")
    a1 = t1(b"123", c)
    a2 = t2("123", "222", "1231231")
    a3 = t2({"x": 10}, None, True, c, c, c, c)
    a4 = t3(3, c)

    r1, r2, r3, r4 = loom_env.submit_and_gather((a1, a2, a3, a4))
    assert r1 == b"123ABC"
    assert r2 == b"3"
    assert r3 == b"7"
    assert r4 == b"ABCABCABC"


def test_py_redirect1(loom_env):

    def f(a, b):
        return tasks.merge((a, b))

    loom_env.start(1)

    c = tasks.const("ABC")
    d = tasks.const("12345")
    a = tasks.py_call(tasks.py_value(f), (c, d))
    result = loom_env.submit_and_gather(a)
    assert result == b"ABC12345"


def test_py_redirect2(loom_env):

    def f(a, b):
        return tasks.run("/bin/ls $X", [(b, "$X")])

    loom_env.start(1)

    c = tasks.const("abcdef")
    d = tasks.const("/")
    a = tasks.py_call(tasks.py_value(f), (c, d))
    result = loom_env.submit_and_gather(a)
    assert b"bin\n" in result
    assert b"usr\n" in result


def test_py_redirect3(loom_env):

    def f(a):
        return tasks.run("cat X", [(a, "X")])

    loom_env.start(1)

    c = tasks.const("DataData")
    a = tasks.py_call(tasks.py_value(f), (c,))
    result = loom_env.submit_and_gather(a)
    assert b"DataData" in result


def test_py_fail_too_many_args(loom_env):

    def g():
        return "Test"

    loom_env.start(1)
    c = tasks.const("ABC")
    a = tasks.py_call(tasks.py_value(g), (c,))

    with pytest.raises(TaskFailed):
        loom_env.submit_and_gather(a)


def test_py_fail_too_few_args(loom_env):

    def f(a):
        return "ABC"

    loom_env.start(1)

    a = tasks.py_call(tasks.py_value(f), ())

    with pytest.raises(TaskFailed):
        loom_env.submit_and_gather(a)


def test_py_fail_invalid_result(loom_env):

    def f():
        return 42.0

    loom_env.start(1)

    a = tasks.py_call(tasks.py_value(f), ())

    with pytest.raises(TaskFailed):
        loom_env.submit_and_gather(a)


def test_py_multiple_return(loom_env):
    loom_env.start(1)

    @tasks.py_task()
    def t1(a, b):
        return a, "x", b, "yyy", [b, b, "z"]

    a = tasks.const("A")
    b = tasks.const("BBBB")
    c = t1(a, b)

    result = loom_env.submit_and_gather(c)
    assert result == [b"A", b"x", b"BBBB", b"yyy", [b"BBBB", b"BBBB", b"z"]]


def test_py_array(loom_env):
    loom_env.start(1)

    @tasks.py_task()
    def t1(array):
        # len
        assert len(array) == 3

        # indexing
        s = array[1].read()

        # iteration
        for x in array:
            s += x.read()

        return s

    a = tasks.const("A")
    b = tasks.const("BBBB")
    c = tasks.const("CCCCCCC")
    array = tasks.array_make((a, b, c))
    x = t1(array)

    result = loom_env.submit_and_gather(x)
    assert result == b"BBBBABBBBCCCCCCC"


def test_py_value(loom_env):

    @tasks.py_task()
    def to_str(a):
        return str(a.unwrap())

    @tasks.py_task(context=True)
    def to_tuple(ctx, a):
        b = tuple(a.unwrap())
        return ctx.wrap(b)

    @tasks.py_task()
    def join(a, b):
        return [a, b]

    loom_env.start(2)
    v = tasks.py_value([(1, 2)])
    v2 = tasks.py_value(("30", None))

    t = to_str(v)
    result = loom_env.submit_and_gather(t)
    assert result == b"[(1, 2)]"

    t = to_tuple(v)
    result = loom_env.submit_and_gather(t)
    assert result == ((1, 2),)

    v.resource_request = cpu1
    v2.resource_request = cpu1
    t = join(v, v2)
    result = loom_env.submit_and_gather(t)
    assert result == [[(1, 2)], ("30", None)]


def test_py_wrap_wrapped(loom_env):

    @tasks.py_task(context=True)
    def task(ctx, a):
        return ctx.wrap(a)

    loom_env.start(2)
    v = tasks.py_value("ABC")
    t = task(v)
    result = loom_env.submit_and_gather(t)
    assert result == "ABC"


def test_py_task_deserialization(loom_env):

    @tasks.py_task()
    def f():
        return b""

    loom_env.start(2)
    ts = [f(), tasks.run("ls"), f(), f(), f()]
    result = loom_env.submit_and_gather(ts)
    assert len(result) == 5


def test_py_task_deserialization3(loom_env):

    loom_env.start(2)
    objs = tuple(tasks.py_value(str(i + 1000)) for i in range(100))
    x = tasks.array_make(objs)
    loom_env.submit_and_gather(x)


def test_rewrap_test(loom_env):
    @tasks.py_task(context=True, n_direct_args=4)
    def init(ctx):
        content = [1, 2, 3]
        return ctx.wrap(content)

    @tasks.py_task(context=True)
    def center(ctx, train_test):
        train, test = [t.unwrap() for t in train_test]
        return [ctx.wrap(t) for t in (train, test)]

    @tasks.py_task(context=True)
    def remove_empty_rows(ctx, train, test):
        train = list(train.unwrap())
        test = list(test.unwrap())
        return [ctx.wrap(t) for t in (train, test)]

    train = init()
    test  = init()
    mean_task = center(remove_empty_rows(train, test))
    smurff_tasks = [mean_task]

    loom_env.start(1)
    futures = loom_env.client.submit(smurff_tasks)
    results = loom_env.client.gather(futures)