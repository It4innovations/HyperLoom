from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR, cleanup  # noqa
import loom.client.tasks as tasks  # noqa

import struct
from datetime import datetime
import os
import pytest
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
    return list(str(a) for a in args)


def str2datetime(data):
    parts = data.decode().split(".")
    dt = datetime.strptime(parts[0], "%Y-%m-%d %H:%M:%S")
    return dt.replace(microsecond=int(parts[1]))


def test_merge_w3(loom_env):
    loom_env.start(3)
    a = tasks.const("ABCDE")
    b = tasks.const("123")
    c = tasks.merge((a, b))
    assert b"ABCDE123" == loom_env.submit_and_gather(c)


def test_merge_delimiter(loom_env):
    loom_env.start(1)
    consts = [tasks.const(str(i)) for i in range(10)]
    c = tasks.merge(consts, "abc")
    expected = bytes("abc".join(str(i) for i in range(10)), "ascii")
    assert expected == loom_env.submit_and_gather(c)


def test_merge_empty_with_delimiter(loom_env):
    loom_env.start(1)
    c = tasks.merge((), "abc")
    assert b"" == loom_env.submit_and_gather(c)


def test_open_and_merge(loom_env):
    a = tasks.open(FILE1)
    b = tasks.open(FILE2)
    c = tasks.merge((a, b))
    loom_env.start(1)
    result = loom_env.submit_and_gather(c)
    expect = bytes("This is file 1\n" +
                   "\n".join("Line {}".format(i) for i in range(1, 13)) +
                   "\n", "ascii")
    assert result == expect


def test_open_and_splitlines(loom_env):
    loom_env.start(1)
    a = tasks.open(FILE2)
    lines = tasks.split(a)
    c1 = tasks.slice(lines, 2, 6)
    c2 = tasks.slice(lines, 0, 6)
    c3 = tasks.slice(lines, 3, 60)
    result1, result2, result3 = loom_env.submit_and_gather([c1, c2, c3])
    expect1 = "\n".join("Line {}".format(i) for i in range(3, 7)) + "\n"
    assert result1.decode() == expect1

    expect2 = "\n".join("Line {}".format(i) for i in range(1, 7)) + "\n"
    assert result2.decode() == expect2

    expect3 = "\n".join("Line {}".format(i) for i in range(4, 13)) + "\n"
    assert result3.decode() == expect3


def test_split(loom_env):
    loom_env.start(1)
    text = "Line1\nLine2\nLine3\nLine4"
    a = tasks.const(text)
    b = tasks.split(a)
    c = tasks.get(b, 1)
    d = tasks.get(b, 3)
    e = tasks.slice(b, 0, 2)
    f = tasks.slice(b, 10, 20)

    r1, r2, r3, r4 = [r.decode()
                      for r in loom_env.submit_and_gather((c, d, e, f))]

    assert r1 == "Line2\n"
    assert r2 == "Line4"
    assert r3 == "Line1\nLine2\n"
    assert r4 == ""


def test_size_and_length(loom_env):
    loom_env.start(1)
    text = "12345" * 5

    a1 = tasks.const(text)
    a2 = tasks.array_make((a1, a1))
    b1 = tasks.size(a1)
    c1 = tasks.length(a1)
    b2 = tasks.size(a2)
    c2 = tasks.length(a2)

    u64 = struct.Struct("<Q")
    [25, 0, 50, 2] == map(lambda x: u64.unpack(x)[0],
                          loom_env.submit_and_gather((b1, c1, b2, c2)))


def test_data_save(loom_env):
    loom_env.start(1)

    a1 = tasks.const("ABC" * 10000)
    a2 = tasks.const(b"")
    a3 = tasks.run("ls")

    with pytest.raises(client.TaskFailed):
        b = tasks.save(a1, "test-file")
        loom_env.submit_and_gather(b)

    with pytest.raises(client.TaskFailed):
        b = tasks.save(a1, "/")
        loom_env.submit_and_gather(b)

    b1 = tasks.save(a1, loom_env.get_filename("file1"))
    b2 = tasks.save(a2, loom_env.get_filename("file2"))
    b3 = tasks.save(a3, loom_env.get_filename("file3"))

    result = loom_env.submit_and_gather((b1, b2, b3))
    assert result == [b"", b"", b""]

    with open(loom_env.get_filename("file1")) as f:
        assert f.read() == "ABC" * 10000

    with open(loom_env.get_filename("file2")) as f:
        assert f.read() == ""

    with open(loom_env.get_filename("file3")) as f:
        content = f.read().split("\n")
        assert "+err" in content
        assert "+out" in content
