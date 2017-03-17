from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR, cleanup  # noqa
import loom.client.tasks as tasks  # noqa


loom_env  # silence flake8


def test_index_transfer(loom_env):
    loom_env.start(2)

    a = tasks.const("AAA\nBBB\n\nC\nDDDDDDDDDDD\nEEE")
    b = tasks.const("E\n\n")
    a2 = tasks.split(a)
    b2 = tasks.split(b)
    d = tasks.array_make((a2, b2))
    x = tasks.get(d, 0)
    r1 = tasks.get(x, 0)
    r2 = tasks.get(x, 1)
    assert loom_env.submit_and_gather((r1, r2)) == [b"AAA\n", b"BBB\n"]
