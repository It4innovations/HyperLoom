from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

import struct
from datetime import datetime
import os
from loom import client

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
    return map(str, args)


def str2datetime(s):
    parts = s.split('.')
    dt = datetime.strptime(parts[0], "%Y-%m-%d %H:%M:%S")
    return dt.replace(microsecond=int(parts[1]))


def test_single_result(loom_env):
        loom_env.start(1)
        p = loom_env.plan_builder()

        a = p.task_const("ABCDE")
        b = p.task_const("123")
        c = p.task_merge((a, b))
        assert "ABCDE123" == loom_env.submit(p, c)


def test_more_results(loom_env):
        loom_env.start(1)
        p = loom_env.plan_builder()

        a = p.task_const("ABCDE")
        b = p.task_const("123")
        c = p.task_merge((a, b))
        assert ["ABCDE123"] * 2 == loom_env.submit(p, [c, c])


def test_merge_w3(loom_env):
        loom_env.start(3)
        p = loom_env.plan_builder()

        a = p.task_const("ABCDE")
        b = p.task_const("123")
        c = p.task_merge((a, b))
        assert "ABCDE123" == loom_env.submit(p, c)


def test_merge_delimiter(loom_env):
        loom_env.start(1)
        p = loom_env.plan_builder()
        consts = [p.task_const(str(i)) for i in xrange(10)]
        c = p.task_merge(consts, "abc")
        expected = "abc".join(str(i) for i in xrange(10))
        assert expected == loom_env.submit(p, c)


def test_merge_empty_with_delimiter(loom_env):
        loom_env.start(1)
        p = loom_env.plan_builder()
        c = p.task_merge((), "abc")
        assert "" == loom_env.submit(p, c)


def test_run_separated_1_cpu(loom_env):
    loom_env.start(1)
    p = loom_env.plan_builder()

    args = pytestprog(0.3, stamp=True)
    tasks = [p.task_run(args) for i in range(4)]
    results = loom_env.submit(p, tasks)

    starts = []

    for result in results:
        line1, line2 = result.strip().split("\n")
        starts.append(str2datetime(line1))

    for i in range(len(starts) - 1):
        a = starts[i + 1]
        b = starts[i]
        if a > b:
            c = a - b
        else:
            c = b - a
        assert 0.3 < c.total_seconds() < 0.45


def test_run_separated_4_cpu(loom_env):
    loom_env.start(1, cpus=4)
    p = loom_env.plan_builder()

    args = pytestprog(0.3, stamp=True)
    tasks = [p.task_run(args) for i in range(4)]
    results = loom_env.submit(p, tasks)

    starts = []

    for result in results:
        line1, line2 = result.strip().split("\n")
        starts.append(str2datetime(line1))

    for i in range(len(starts) - 1):
        a = starts[i + 1]
        b = starts[i]
        if a > b:
            c = a - b
        else:
            c = b - a
        assert c.total_seconds() < 0.06


def test_run_separated_4cpu_tasks_4_cpu(loom_env):
    loom_env.start(1, cpus=4)
    p = loom_env.plan_builder()

    args = pytestprog(0.3, stamp=True)
    tasks = [p.task_run(args, request=client.cpus(4)) for i in range(4)]
    results = loom_env.submit(p, tasks)

    starts = []

    for result in results:
        line1, line2 = result.strip().split("\n")
        starts.append(str2datetime(line1))

    for i in range(len(starts) - 1):
        a = starts[i + 1]
        b = starts[i]
        if a > b:
            c = a - b
        else:
            c = b - a
        assert 0.3 < c.total_seconds() < 0.45


def test_run_double_lines(loom_env):
    p = loom_env.plan_builder()

    COUNT = 20000

    a1 = p.task_const("afi" * 20000)
    b1 = p.task_run(pytestprog(0.0, "plus1"), stdin=a1)
    c1 = p.task_run(pytestprog(0.0, "plus1"), stdin=b1)

    a2 = p.task_const("kkllmm" * 20000)
    b2 = p.task_run(pytestprog(0.0, "plus1"), stdin=a2)
    c2 = p.task_run(pytestprog(0.0), stdin=b2)

    result = p.task_merge((c1, c2, a2))

    expect = "chk" * COUNT + "llmmnn" * COUNT + "kkllmm" * COUNT

    for i in range(1, 4):
        #  print "Runnig for {}".format(i)
        loom_env.start(i)
        r = loom_env.submit(p, result)
        assert r == expect


def test_run_files(loom_env):
    p = loom_env.plan_builder()

    a1 = p.task_const("abcd" * 100)
    b1 = p.task_run(
            pytestprog(0.0, "plus1", file_out="myfile1"),
            stdin=a1,
            outputs=["myfile1"])

    c1 = p.task_run(pytestprog(
        0.0, "plus1", file_in="input", file_out="output"),
        outputs=["output"],
        inputs=[(b1, "input")])

    loom_env.start(1)
    result = loom_env.submit(p, c1)

    assert result == "cdef" * 100


def test_open_and_merge(loom_env):
    p = loom_env.plan_builder()
    a = p.task_open(FILE1)
    b = p.task_open(FILE2)
    c = p.task_merge((a, b))
    loom_env.start(1)
    result = loom_env.submit(p, c)
    expect = ("This is file 1\n" +
              "\n".join("Line {}".format(i) for i in xrange(1, 13)) +
              "\n")
    assert result == expect


def test_open_and_splitlines(loom_env):
    loom_env.start(1)

    p = loom_env.plan_builder()
    a = p.task_open(FILE2)
    lines = p.task_split(a)
    c1 = p.task_slice(lines, 2, 6)
    c2 = p.task_slice(lines, 0, 6)
    c3 = p.task_slice(lines, 3, 60)
    result1, result2, result3 = loom_env.submit(p, [c1, c2, c3])
    expect1 = "\n".join("Line {}".format(i) for i in xrange(3, 7)) + "\n"
    assert result1 == expect1

    expect2 = "\n".join("Line {}".format(i) for i in xrange(1, 7)) + "\n"
    assert result2 == expect2

    expect3 = "\n".join("Line {}".format(i) for i in xrange(4, 13)) + "\n"
    assert result3 == expect3


def test_split(loom_env):
    loom_env.start(1)
    text = "Line1\nLine2\nLine3\nLine4"
    p = loom_env.plan_builder()

    a = p.task_const(text)
    b = p.task_split(a)
    c = p.task_get(b, 1)
    d = p.task_get(b, 3)
    e = p.task_slice(b, 0, 2)
    f = p.task_slice(b, 10, 20)

    r1, r2, r3, r4 = loom_env.submit(p, (c, d, e, f))

    assert r1 == "Line2\n"
    assert r2 == "Line4"
    assert r3 == "Line1\nLine2\n"
    assert r4 == ""


def test_size_and_length(loom_env):
    loom_env.start(1)

    p = loom_env.plan_builder()
    text = "12345" * 5

    a1 = p.task_const(text)
    a2 = p.task_array_make((a1, a1))
    b1 = p.task_size(a1)
    c1 = p.task_length(a1)
    b2 = p.task_size(a2)
    c2 = p.task_length(a2)

    u64 = struct.Struct("<Q")
    [25, 0, 50, 2] == map(lambda x: u64.unpack(x)[0],
                          loom_env.submit(p, (b1, c1, b2, c2)))
