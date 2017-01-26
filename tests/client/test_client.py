from loomenv import loom_env, LOOM_PYTHON  # noqa
import loom.client.tasks as tasks  # noqa

import time

loom_env  # silence flake8


def test_single_result(loom_env):
    loom_env.start(1)
    a = tasks.const("ABCDE")
    b = tasks.const("123")
    c = tasks.merge((a, b))
    assert b"ABCDE123" == loom_env.submit(c)


def test_more_results(loom_env):
    loom_env.start(1)
    a = tasks.const("ABCDE")
    b = tasks.const("123")
    c = tasks.merge((a, b))
    assert [b"ABCDE123"] * 2 == loom_env.submit([c, c])


def test_reconnect(loom_env):
    loom_env.start(2)
    code = """
import sys
sys.path.insert(0, "{LOOM_PYTHON}")
from loom.client import Client, tasks
client = Client("localhost", {PORT})
a = tasks.run("sleep 1")
b = tasks.run("sleep 1")
c = tasks.run("sleep 1")
d = tasks.run("sleep 1")
print(client.submit((a, b, c, d)))
    """.format(LOOM_PYTHON=LOOM_PYTHON, PORT=loom_env.PORT)

    p = loom_env.independent_python(code)
    time.sleep(0.6)
    assert not p.poll()
    p.kill()

    p = loom_env.independent_python(code)
    time.sleep(0.6)
    assert not p.poll()
    p.kill()

    p = loom_env.independent_python(code)
    time.sleep(0.2)
    assert not p.poll()
    p.kill()

    time.sleep(0.2)

    a = tasks.const("abc")
    b = tasks.const("xyz")
    c = tasks.const("123")
    d = tasks.merge((a, b, c))
    result = loom_env.submit(d)
    assert result == b"abcxyz123"


def test_report_while_killed(loom_env):
    loom_env.start(2, cpus=2)

    code = """
import sys
import time
import datetime
sys.path.insert(0, "{LOOM_PYTHON}")
from loom.client import Client, tasks
client = Client("localhost", {PORT})
@tasks.py_task()
def slow(a):
    time.sleep(1.4)
    return a
t1 = tasks.const("AA")
t2 = slow(t1)
r1 = slow(t2)
r2 = tasks.run("sleep 3")
r3 = tasks.run("sleep 3")
period = datetime.timedelta(seconds=1)
print(client.submit((r1, r2, r3), report="test.report",
      report_save_period=period))
    """.format(LOOM_PYTHON=LOOM_PYTHON, PORT=loom_env.PORT)

    p = loom_env.independent_python(code)
    time.sleep(2.0)
    assert not p.poll()
    p.kill()

    assert len(loom_env.check_files("test.report-*")) == 1
