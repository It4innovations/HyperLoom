from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

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
    return args


def str2datetime(s):
    parts = s.split('.')
    dt = datetime.strptime(parts[0], "%Y-%m-%d %H:%M:%S")
    return dt.replace(microsecond=int(parts[1]))


def test_single_result(loom_env):
        loom_env.start(1)
        p = loom_env.plan()

        a = p.task_const("ABCDE")
        b = p.task_const("123")
        c = p.task_merge((a, b))
        assert "ABCDE123" == loom_env.submit(p, c)


def test_more_results(loom_env):
        loom_env.start(1)
        p = loom_env.plan()

        a = p.task_const("ABCDE")
        b = p.task_const("123")
        c = p.task_merge((a, b))
        assert ["ABCDE123"] * 2 == loom_env.submit(p, [c, c])


def test_merge_w3(loom_env):
        loom_env.start(3)
        p = loom_env.plan()

        a = p.task_const("ABCDE")
        b = p.task_const("123")
        c = p.task_merge((a, b))
        assert "ABCDE123" == loom_env.submit(p, c)


def test_run_separated_1_cpu(loom_env):
    loom_env.start(1)
    p = loom_env.plan()

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
    p = loom_env.plan()

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


def test_run_double_lines(loom_env):
    p = loom_env.plan()

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
    p = loom_env.plan()

    a1 = p.task_const("abcd" * 100)
    b1 = p.task_run(pytestprog(0.0, "plus1", file_out="myfile1"), stdin=a1)
    b1.map_file_out("myfile1")

    c1 = p.task_run(pytestprog(
        0.0, "plus1", file_in="input", file_out="output"))
    c1.map_file_in(b1, "input")
    c1.map_file_out("output")

    loom_env.start(1)
    result = loom_env.submit(p, c1)

    assert result == "cdef" * 100


def test_open_and_merge(loom_env):
    p = loom_env.plan()
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

    p = loom_env.plan()
    a = p.task_open(FILE2)
    c = p.task_split_lines(a, 2, 6)
    result = loom_env.submit(p, c)
    expect = "\n".join("Line {}".format(i) for i in xrange(3, 7)) + "\n"
    assert result == expect

    p = loom_env.plan()
    a = p.task_open(FILE2)
    c = p.task_split_lines(a, 0, 6)
    result = loom_env.submit(p, c)
    expect = "\n".join("Line {}".format(i) for i in xrange(1, 7)) + "\n"
    assert result == expect

    p = loom_env.plan()
    a = p.task_open(FILE2)
    c = p.task_split_lines(a, 3, 60)
    result = loom_env.submit(p, c)
    expect = "\n".join("Line {}".format(i) for i in xrange(4, 13)) + "\n"
    assert result == expect
