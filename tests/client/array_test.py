from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

loom_env  # silence flake8


def test_make_array(loom_env):
    p = loom_env.plan()
    a = p.task_const("ABC")
    b = p.task_const("123456")
    c = p.task_const("")
    d = p.task_array_make((a, b, c))
    e0 = p.task_array_get(d, 0)
    e1 = p.task_array_get(d, 1)
    e2 = p.task_array_get(d, 2)
    loom_env.start(1)
    result_d, result_e0, result_e1, result_e2 = \
        loom_env.submit(p, (d, e0, e1, e2))
    assert result_d == ["ABC", "123456", ""]
    assert result_e0 == "ABC"
    assert result_e1 == "123456"
    assert result_e2 == ""


def test_make_empty_array(loom_env):
    p = loom_env.plan()
    a = p.task_array_make(())
    loom_env.start(1)
    result_a = loom_env.submit(p, a)
    assert result_a == []


def test_array_of_array(loom_env):
    p = loom_env.plan()

    a = p.task_const("ABC")
    b = p.task_const("123")
    c = p.task_const("+++")

    d = p.task_array_make((a, b))
    e = p.task_array_make((d, c))
    f = p.task_array_get(e, 0)

    loom_env.start(1)
    result_d, result_e, result_f = loom_env.submit(p, (d, e, f))
    assert result_d == ["ABC", "123"]
    assert result_e == [["ABC", "123"], "+++"]
    assert result_f == ["ABC", "123"]


def test_array_same_value(loom_env):
    p = loom_env.plan()
    a = p.task_const("ABC")
    b = p.task_array_make((a, a, a, a))
    loom_env.start(1)
    assert ["ABC"] * 4 == loom_env.submit(p, b)
