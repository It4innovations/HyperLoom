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
