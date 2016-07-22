from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

from datetime import datetime
import os
import client
import pytest

loom_env  # silence flake8


def test_invalid_program(loom_env):
    loom_env.start(1)
    p = loom_env.plan()
    a = p.task_run("/usr/bin/non-existing-program")

    with pytest.raises(client.TaskFailed):
        loom_env.submit(p, a)
