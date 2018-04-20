from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_BUILD_DIR  # noqa
import loom.client.tasks as tasks  # noqa

import os

loom_env  # silence flake8


def test_checkpoint_basic(loom_env):
    loom_env.start(1)
    t1 = tasks.const("abcd")
    t2 = tasks.const("XYZ")
    t3 = tasks.merge((t1, t2))
    path = os.path.join(LOOM_TEST_BUILD_DIR, "test.data")
    t3.checkpoint_path = path
    assert b"abcdXYZ" == loom_env.submit_and_gather(t3)
    with open(path, "rb") as f:
        assert f.read() == b"abcdXYZ"
    assert not os.path.isfile(path + ".loom.tmp")


def test_checkpoint_load(loom_env):
    loom_env.start(1)

    path1 = os.path.join(LOOM_TEST_BUILD_DIR, "f1.txt")
    path2 = os.path.join(LOOM_TEST_BUILD_DIR, "f2.txt")
    path3 = os.path.join(LOOM_TEST_BUILD_DIR, "f3.txt")
    path4 = os.path.join(LOOM_TEST_BUILD_DIR, "f4.txt")
    path5 = os.path.join(LOOM_TEST_BUILD_DIR, "nonexisting")

    for i, p in enumerate((path1, path2, path3, path4)):
        with open(p, "w") as f:
            f.write("[{}]".format(i + 1))

    t1 = tasks.const("$t1$")
    t1.checkpoint_path = path1  # This shoud load: [1]

    t2 = tasks.const("$t2$")
    t2.checkpoint_path = path2  # This shoud load: [2]

    t3 = tasks.const("$t3$")
    t4 = tasks.const("$t4$")

    x1 = tasks.merge((t1, t2, t3))  # [1][2]$t3$
    x2 = tasks.merge((t1, x1))
    x2.checkpoint_path = path3 # loaded as [3]

    x3 = tasks.merge((t4, t4))
    x3.checkpoint_path = path4  # loaded as [4]

    x4 = tasks.merge((x3, x1, x2, t1, t2, t3))
    x4.checkpoint_path = path5

    assert loom_env.submit_and_gather(x4, load=True) == b'[4][1][2]$t3$[3][1][2]$t3$'