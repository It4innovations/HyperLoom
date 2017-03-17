from loomenv import loom_env, LOOM_PYTHON  # noqa
import loom.client.tasks as tasks  # noqa

import time

loom_env  # silence flake8


def test_single_result(loom_env):
    loom_env.start(1)
    a = tasks.const("ABCDE")
    b = tasks.const("123")
    c = tasks.merge((a, b))
    assert b"ABCDE123" == loom_env.submit_and_gather(c)
    loom_env.check_final_state()


def test_more_same_results(loom_env):
    loom_env.start(1)
    a = tasks.const("ABCDE")
    b = tasks.const("123")
    c = tasks.merge((a, b))
    assert [b"ABCDE123", b"ABCDE123"] == loom_env.submit_and_gather([c, c])
    loom_env.check_final_state()


def test_more_results(loom_env):
    loom_env.start(1)
    a = tasks.const("ABCDE")
    b = tasks.const("123")
    c = tasks.merge((a, b))
    assert [b"ABCDE", b"123", b"ABCDE123"] == \
        loom_env.submit_and_gather([a, b, c])
    loom_env.check_final_state()


def test_future_untouched(loom_env):
    loom_env.start(1)
    a = tasks.const("ABCDE")
    loom_env.client.submit_one(a)
    import time
    time.sleep(0.2)


def test_future_wait(loom_env):
    loom_env.start(1)
    a = tasks.const("abcde")
    f = loom_env.client.submit_one(a)
    f.wait()
    assert f.finished()
    f.release()
    loom_env.check_final_state()


def test_two_futures(loom_env):
    loom_env.start(1)
    a = tasks.const("abcde")
    fa = loom_env.client.submit_one(a)
    b = tasks.const("xyz")
    fb = loom_env.client.submit_one(b)

    loom_env.client.wait((fa, fb))
    assert fa.finished()
    assert fb.finished()
    assert fa.fetch() == b"abcde"
    assert fb.fetch() == b"xyz"
    fa.release()
    fb.release()
    loom_env.check_final_state()


def test_fetch_all(loom_env):
    loom_env.start(1)
    a = tasks.const("abcde")
    fa = loom_env.client.submit_one(a)
    b = tasks.const("xyz")
    fb = loom_env.client.submit_one(b)

    assert [b"abcde", b"xyz"] == loom_env.client.fetch((fa, fb))
    assert not fa.released()
    assert not fb.released()
    fa.release()
    fb.release()
    loom_env.check_final_state()


def test_gather_all(loom_env):
    loom_env.start(1)
    a = tasks.const("abcde")
    fa = loom_env.client.submit_one(a)
    b = tasks.const("xyz")
    fb = loom_env.client.submit_one(b)

    assert [b"abcde", b"xyz"] == loom_env.client.gather((fa, fb))
    assert fa.released()
    assert fb.released()
    fa.release()
    fb.release()
    loom_env.check_final_state()


def test_future_fetch(loom_env):
    loom_env.start(1)
    a = tasks.const("ABCDE")
    f = loom_env.client.submit_one(a)
    assert f.fetch() == b"ABCDE"
    f.release()
    loom_env.check_final_state()


def test_future_double_release(loom_env):
    loom_env.start(1)
    a = tasks.const("ABCDE")
    f = loom_env.client.submit_one(a)
    assert f.fetch() == b"ABCDE"
    f.release()
    f.release()
    assert f.released()
    loom_env.check_final_state()


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
fs = client.submit((a, b, c, d))
print(client.gather(fs))
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

    a = tasks.run("ls")
    result = loom_env.submit_and_gather(a)
    assert result
    loom_env.check_final_state()

    a = tasks.const("abc")
    b = tasks.const("xyz")
    c = tasks.const("123")
    d = tasks.merge((a, b, c))
    result = loom_env.submit_and_gather(d)
    assert result == b"abcxyz123"

    a = tasks.run("ls")
    result = loom_env.submit_and_gather(a)
    assert result
    loom_env.check_final_state()


def test_reconnect2(loom_env):
    loom_env.start(1)

    a = tasks.run("sleep 1")
    loom_env.client.submit_one(a)
    loom_env.close_client()

    a = tasks.const("abc")
    b = tasks.const("xyz")
    c = tasks.const("123")
    d = tasks.merge((a, b, c))
    result = loom_env.submit_and_gather(d)
    assert result == b"abcxyz123"

    loom_env.check_final_state()
