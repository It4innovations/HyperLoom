
import os
import subprocess
import sys
import time
import pytest
import shutil


LOOM_TESTDIR = os.path.dirname(os.path.abspath(__file__))
LOOM_ROOT = os.path.dirname(os.path.dirname(LOOM_TESTDIR))
LOOM_BUILD = os.path.join(LOOM_ROOT, "_build")

LOOM_SERVER_BIN = os.path.join(LOOM_BUILD, "src", "server", "loom-server")
LOOM_WORKER_BIN = os.path.join(LOOM_BUILD, "src", "worker", "loom-worker")
LOOM_PYTHON = os.path.join(LOOM_ROOT, "python")
LOOM_TEST_BUILD_DIR = os.path.join(LOOM_TESTDIR, "build")

LOOM_TESTPROG = os.path.join(LOOM_TESTDIR, "testprog.py")
LOOM_TEST_DATA_DIR = os.path.join(LOOM_TESTDIR, "testdata")

sys.path.insert(0, LOOM_PYTHON)

import loom.client as client  # noqa

VALGRIND = False


class Env():

    def __init__(self):
        self.processes = []

    def start_process(self, name, args):
        fname = os.path.join(LOOM_TEST_BUILD_DIR, name)
        with open(fname + ".out", "w") as out:
            with open(fname + ".err", "w") as err:
                p = subprocess.Popen(args, stdout=out, stderr=err)
        self.processes.append((name, p))
        return p

    def kill_all(self):
        for n, p in self.processes:
            p.kill()


class LoomEnv(Env):

    PORT = 19010
    _client = None

    def start(self, workers_count, cpus=1):
        if self.processes:
            self._client = None
            self.kill_all()
            time.sleep(0.2)
        server_args = (LOOM_SERVER_BIN,
                       "--debug",
                       "--port=" + str(self.PORT))
        valgrind_args = ("valgrind", "--num-callers=40")
        if VALGRIND:
            server_args = valgrind_args + server_args
        server = self.start_process("server", server_args)
        time.sleep(0.1)
        assert not server.poll()
        workers = []
        worker_args = (LOOM_WORKER_BIN,
                       "--debug",
                       "--wdir=" + LOOM_TEST_BUILD_DIR,
                       "--cpus=" + str(cpus),
                       "127.0.0.1", str(self.PORT))
        if VALGRIND:
            time.sleep(2)
            worker_args = valgrind_args + worker_args
        for i in range(workers_count):
            w = self.start_process("worker{}".format(i), worker_args)
            workers.append(w)
        time.sleep(0.1)
        if VALGRIND:
            time.sleep(2)
        assert not server.poll()
        assert not any(w.poll() for w in workers)

    @property
    def client(self):
        if self._client is None:
            self._client = client.Client("localhost", self.PORT)
        return self._client

    def submit(self, results, report=None):
        if report:
            report = os.path.join(LOOM_TEST_BUILD_DIR, report)
        return self.client.submit(results, report)

    def make_dry_report(self, tasks, filename):
        filename = os.path.join(LOOM_TEST_BUILD_DIR, filename)
        return client.make_dry_report(tasks, filename)


@pytest.yield_fixture(autouse=True, scope="function")
def loom_env():
    if os.path.isdir(LOOM_TEST_BUILD_DIR):
        for item in os.listdir(LOOM_TEST_BUILD_DIR):
            path = os.path.join(LOOM_TEST_BUILD_DIR, item)
            if os.path.isfile(path):
                os.unlink(path)
            else:
                shutil.rmtree(path)
    else:
        os.makedirs(LOOM_TEST_BUILD_DIR)
    env = LoomEnv()
    yield env
    env.kill_all()
