
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
LOOM_CLIENT = os.path.join(LOOM_ROOT, "src")
LOOM_TEST_BUILD_DIR = os.path.join(LOOM_TESTDIR, "build")

LOOM_TESTPROG = os.path.join(LOOM_TESTDIR, "testprog.py")
LOOM_TEST_DATA_DIR = os.path.join(LOOM_TESTDIR, "testdata")

sys.path.insert(0, LOOM_CLIENT)

import client  # noqa

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
    info = False
    _client = None

    def start(self, workers_count, cpus=1):
        if self.processes:
            self._client = None
            self.kill_all()
            time.sleep(0.2)
        server_args = (LOOM_SERVER_BIN,
                       "--debug",
                       "--port=" + str(self.PORT))
        if VALGRIND:
            server_args = ("valgrind",) + server_args
        server = self.start_process("server", server_args)
        time.sleep(0.1)
        assert not server.poll()
        workers = []
        worker_args = (LOOM_WORKER_BIN,
                       "--debug",
                       "--wdir=" + LOOM_TEST_BUILD_DIR,
                       "--cpus=" + str(cpus),
                       "localhost", str(self.PORT))
        if VALGRIND:
            time.sleep(2)
            worker_args = ("valgrind",) + worker_args
        for i in xrange(workers_count):
            w = self.start_process("worker{}".format(i), worker_args)
            workers.append(w)
        time.sleep(0.1)
        if VALGRIND:
            time.sleep(2)
        assert not server.poll()
        assert not any(w.poll() for w in workers)

    def plan(self):
        return client.Plan()

    @property
    def client(self):
        if self._client is None:
            self._client = client.Client("localhost", self.PORT, self.info)
        return self._client

    def submit(self, plan, results):
        return self.client.submit(plan, results)


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
