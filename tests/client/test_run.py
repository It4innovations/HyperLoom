from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR, cleanup  # noqa
import loom.client.tasks as tasks  # noqa

from datetime import datetime
import os

FILE1 = os.path.join(LOOM_TEST_DATA_DIR, "file1")
FILE2 = os.path.join(LOOM_TEST_DATA_DIR, "file2")

loom_env  # silence flake8


def pytestprog(sleep, op="copy", stamp=False, file_out=None, file_in=None):
    args = ["/usr/bin/python", LOOM_TESTPROG]
    if stamp:
        args.append("--stamp")
    if file_in:
        args.append("--input=" + file_in)
    if file_out:
        args.append("--out=" + file_out)
    args.append(op)
    args.append(sleep)
    return list(str(a) for a in args)


def str2datetime(data):
    parts = data.decode().split(".")
    dt = datetime.strptime(parts[0], "%Y-%m-%d %H:%M:%S")
    return dt.replace(microsecond=int(parts[1]))


def test_run_separated_1_cpu(loom_env):
    loom_env.start(1)
    args = pytestprog(0.3, stamp=True)
    ts = [tasks.run(args) for i in range(3)]
    results = loom_env.submit(ts)

    starts = []

    for result in results:
        line1, line2 = result.strip().split(b"\n")
        starts.append(str2datetime(line1))

    for i in range(len(starts)):
        for j in range(len(starts)):
            if i == j:
                continue
            a = starts[i]
            b = starts[j]
            c = abs(a - b)
            assert 0.3 < c.total_seconds() < 1.40


def test_run_separated_4_cpu(loom_env):
    loom_env.start(1, cpus=4)
    args = pytestprog(0.3, stamp=True)
    ts = [tasks.run(args) for i in range(4)]
    results = loom_env.submit(ts)

    starts = []

    for result in results:
        line1, line2 = result.strip().split(b"\n")
        starts.append(str2datetime(line1))

    for i in range(len(starts)):
        for j in range(len(starts)):
            if i == j:
                continue
            a = starts[i]
            b = starts[j]
            c = abs(a - b)
            assert c.total_seconds() < 0.55


def test_run_separated_4cpu_tasks_4_cpu(loom_env):
    loom_env.start(1, cpus=4)
    args = pytestprog(0.3, stamp=True)
    ts = [tasks.run(args, request=tasks.cpus(4)) for i in range(4)]
    results = loom_env.submit(ts)

    starts = []

    for result in results:
        line1, line2 = result.strip().split(b"\n")
        starts.append(str2datetime(line1))

    for i in range(len(starts)):
        for j in range(len(starts)):
            if i == j:
                continue
            a = starts[i]
            b = starts[j]
            c = abs(a - b)
            assert 0.3 < c.total_seconds() < 1.40


def test_run_double_lines(loom_env):
    COUNT = 20000

    a1 = tasks.const("afi" * 20000)
    b1 = tasks.run(pytestprog(0.0, "plus1"), stdin=a1)
    c1 = tasks.run(pytestprog(0.0, "plus1"), stdin=b1)

    a2 = tasks.const("kkllmm" * 20000)
    b2 = tasks.run(pytestprog(0.0, "plus1"), stdin=a2)
    c2 = tasks.run(pytestprog(0.0), stdin=b2)

    result = tasks.merge((c1, c2, a2))

    expect = b"chk" * COUNT + b"llmmnn" * COUNT + b"kkllmm" * COUNT

    for i in range(1, 4):
        #  print "Runnig for {}".format(i)
        cleanup()
        loom_env.start(i)
        r = loom_env.submit(result)
        assert r == expect


def test_run_files(loom_env):
    a1 = tasks.const("abcd" * 100)
    b1 = tasks.run(
            pytestprog(0.0, "plus1", file_out="myfile1"),
            stdin=a1,
            outputs=["myfile1"])

    c1 = tasks.run(pytestprog(
        0.0, "plus1", file_in="input", file_out="output"),
        outputs=["output"],
        inputs=[(b1, "input")])

    loom_env.start(1)
    result = loom_env.submit(c1)

    assert result == b"cdef" * 100


def test_run_variable2(loom_env):
    a = tasks.const("123")
    b = tasks.const("456")
    c = tasks.run("/bin/echo $xyz xyz $ab $c", inputs=[(a, "$xyz"), (b, "$c")])
    loom_env.start(1)
    result = loom_env.submit(c)
    assert result == b"123 xyz $ab 456\n"


def test_run_hostname(loom_env):
    loom_env.start(1)

    TASKS_COUNT = 100

    ts = [tasks.run("hostname") for i in range(TASKS_COUNT)]
    array = tasks.array_make(ts)

    loom_env.submit(array)


def test_run_chain(loom_env):
    const = b"ABC" * 10000000
    a = tasks.const(const)

    b1 = tasks.run(pytestprog(0.01, file_out="test"), stdin=a, outputs=["test"])
    b2 = tasks.run(pytestprog(0.01, file_in="in", file_out="test"),
                   inputs=[(b1, "in")], outputs=["test"])
    b3 = tasks.run(pytestprog(0.01, file_in="in", file_out="test"),
                   inputs=[(b2, "in")], outputs=["test"])
    b4 = tasks.run(pytestprog(0.01, file_in="in", file_out="test"),
                   inputs=[(b3, "in")], outputs=["test"])
    loom_env.start(1)
    result = loom_env.submit(b4)
    assert result == const
