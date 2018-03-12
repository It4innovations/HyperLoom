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
