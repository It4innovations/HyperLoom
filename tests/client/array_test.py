from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa
import loom.client.tasks as tasks  # noqa

loom_env  # silence flake8


def test_make_array(loom_env):
    loom_env.start(1)

    a = tasks.const("ABC")
    b = tasks.const("123456")
    c = tasks.const("")
    d = tasks.array_make((a, b, c))

    e0 = tasks.get(d, 0)
    e1 = tasks.get(d, 1)
    e2 = tasks.get(d, 2)

    result_d, result_e0, result_e1, result_e2 = \
        loom_env.submit((d, e0, e1, e2))
    assert result_d == [b"ABC", b"123456", b""]
    assert result_e0 == b"ABC"
    assert result_e1 == b"123456"
    assert result_e2 == b""


def test_slice_array(loom_env):
    loom_env.start(1)

    items = [tasks.const(str(i)) for i in range(20)]
    a = tasks.array_make(items)

    e0 = tasks.slice(a, 0, 100)
    e1 = tasks.slice(a, 50, 100)
    e2 = tasks.slice(a, 2, 4)
    e3 = tasks.slice(a, 1, 0)
    e4 = tasks.slice(a, 4, 100)

    r0, r1, r2, r3, r4 = \
        loom_env.submit((e0, e1, e2, e3, e4))
    assert r0 == [bytes(str(i), "ascii") for i in range(0, 20)]
    assert r1 == []
    assert r2 == [b'2', b'3']
    assert r3 == []
    assert r4 == [bytes(str(i), "ascii") for i in range(4, 20)]


def test_make_empty_array(loom_env):
    a = tasks.array_make(())
    loom_env.start(1)
    result_a = loom_env.submit(a)
    assert result_a == []


def test_array_of_array(loom_env):
    a = tasks.const("ABC")
    b = tasks.const("123")
    c = tasks.const("+++")

    d = tasks.array_make((a, b))
    e = tasks.array_make((d, c))
    f = tasks.get(e, 0)

    loom_env.start(1)
    result_d, result_e, result_f = loom_env.submit((d, e, f))
    assert result_d == [b"ABC", b"123"]
    assert result_e == [[b"ABC", b"123"], b"+++"]
    assert result_f == [b"ABC", b"123"]


def test_array_same_value(loom_env):
    a = tasks.const("ABC")
    b = tasks.array_make((a, a, a, a))
    loom_env.start(1)
    assert [b"ABC"] * 4 == loom_env.submit(b)
