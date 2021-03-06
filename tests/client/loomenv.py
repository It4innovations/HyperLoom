
import os
import subprocess
import sys
import time
import pytest
import shutil
import glob


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
from loom.client import Task  # noqa
from loom.lore.report import Report  # noqa
from loom.lore.html import create_html  # noqa

VALGRIND = False


class Env():

    def __init__(self):
        self.processes = []
        self.cleanups = []

    def start_process(self, name, args, env=None, catch_io=True):
        fname = os.path.join(LOOM_TEST_BUILD_DIR, name)
        if catch_io:
            with open(fname + ".out", "w") as out:
                p = subprocess.Popen(args,
                                     stdout=out,
                                     stderr=subprocess.STDOUT,
                                     cwd=LOOM_TEST_BUILD_DIR,
                                     env=env)
        else:
            p = subprocess.Popen(args,
                                 cwd=LOOM_TEST_BUILD_DIR,
                                 env=env)
        self.processes.append((name, p))
        return p

    def kill_all(self):
        for fn in self.cleanups:
            fn()
        for _, p in self.processes:
            p.kill()

    def kill(self, name):
        for n, p in self.processes:
            if n == name:
                p.kill()
                return
        raise Exception("Unknown processes")


class LoomEnv(Env):

    PORT = 19010
    _client = None

    def start(self, workers_count, cpus=1):
        self.workers_count = workers_count
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
                       "--nopin",
                       "--wdir=" + LOOM_TEST_BUILD_DIR,
                       "--cpus=" + str(cpus),
                       "127.0.0.1", str(self.PORT))
        if VALGRIND:
            time.sleep(2)
            worker_args = valgrind_args + worker_args
        env = os.environ.copy()
        if "PYTHONPATH" in env:
            env["PYTHONPATH"] = LOOM_PYTHON + ":" + env["PYTHONPATH"]
        else:
            env["PYTHONPATH"] = LOOM_PYTHON
        for i in range(workers_count):
            w = self.start_process("worker{}".format(i), worker_args, env)
            workers.append(w)
        time.sleep(0.25)
        if VALGRIND:
            time.sleep(5)
        assert not server.poll()
        assert not any(w.poll() for w in workers)

    def get_filename(self, filename):
        return os.path.join(LOOM_TEST_BUILD_DIR, filename)

    def check_files(self, pattern):
        return glob.glob(os.path.join(LOOM_TEST_BUILD_DIR, pattern),
                         recursive=True)

    def check_stats(self):
        stats = self._client.get_stats()
        assert stats["n_workers"] == self.workers_count
        assert stats["n_data_objects"] == 0

    def kill_worker(self, id):
        assert self.workers_count > 0
        self.kill("worker{}".format(id))
        self.workers_count -= 1
        time.sleep(0.02)

    def check_final_state(self):
        time.sleep(0.25)
        self.check_stats()
        filenames = glob.glob(
            os.path.join(LOOM_TEST_BUILD_DIR, "worker-*/**/*"), recursive=True)
        runs = 0
        data = 0
        for filename in filenames:
            if filename.endswith("run"):
                runs += 1
            elif filename.endswith("data"):
                data += 1
            else:
                assert 0, "Invalid file/directory remains: " + filename

        assert runs <= self.workers_count
        assert data <= self.workers_count

    @property
    def client(self):
        if self._client is None:
            self._client = client.Client("localhost", self.PORT)
            self.check_stats()
        return self._client

    def submit_and_gather(self, tasks, check=True, load=False):
        if isinstance(tasks, Task):
            future = self.client.submit_one(tasks, load=load)
            return self.client.gather_one(future)
        else:
            futures = self.client.submit(tasks, load=load)
            return self.client.gather(futures)
        if check:
            self.check_final_state()

    def set_trace(self, trace_path):
        self.client.set_trace(self.get_filename(trace_path))

    def independent_python(self, code):
        return self.start_process("python3",
                                  (sys.executable, "-c", code),
                                  catch_io=False)

    def make_dry_report(self, tasks, filename):
        filename = os.path.join(LOOM_TEST_BUILD_DIR, filename)
        return client.make_dry_report(tasks, filename)

    def timeout(self, sleep_time, fn):
        import threading
        thread = threading.Timer(sleep_time, fn)
        thread.start()
        self.cleanups.append(lambda: thread.cancel())

    def close_client(self):
        self._client.close()
        self._client = None

    def terminate(self):
        if self._client:
            self._client.terminate()
            time.sleep(0.15)
            self._client = None

    def lore(self, dirname):
        dirname = self.get_filename(dirname)
        report = Report(dirname)
        create_html(report, self.get_filename("output.html"), True)


def cleanup():
    if os.path.isdir(LOOM_TEST_BUILD_DIR):
        for item in os.listdir(LOOM_TEST_BUILD_DIR):
            path = os.path.join(LOOM_TEST_BUILD_DIR, item)
            if os.path.isfile(path):
                os.unlink(path)
            else:
                shutil.rmtree(path)
    else:
        os.makedirs(LOOM_TEST_BUILD_DIR)


@pytest.yield_fixture(autouse=True, scope="function")
def loom_env():
    cleanup()
    env = LoomEnv()
    yield env
    env.terminate()
    env.kill_all()
