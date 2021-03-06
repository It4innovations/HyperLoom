from loomenv import loom_env, LOOM_TEST_DATA_DIR
import loom.client.tasks as tasks

import os
import shutil
import pytest

IRIS_DATA = os.path.join(LOOM_TEST_DATA_DIR, "iris.data")

loom_env  # silence flake8

has_libsvm = shutil.which("svm-train") is not None \
             and shutil.which("svm-predict") is not None


@pytest.mark.skipif(not has_libsvm, reason="libsvm not installed")
def test_cv_iris(loom_env):
        CHUNKS = 15
        CHUNK_SIZE = 150 // CHUNKS  # There are 150 irises

        loom_env.start(4, 4)
        loom_env.info = False

        data = tasks.open(IRIS_DATA)
        data = tasks.run(("sort", "--random-sort", "-"), [(data, None)])
        lines = tasks.split(data)

        chunks = [tasks.slice(lines, i * CHUNK_SIZE, (i + 1) * CHUNK_SIZE)
                  for i in range(CHUNKS)]

        trainsets = [tasks.merge(chunks[:i] + chunks[i + 1:])
                     for i in range(CHUNKS)]

        models = []
        for i, ts in enumerate(trainsets):
            model = tasks.run("svm-train data",
                              [(ts, "data")], ["data.model"])
            model.label = "svm-train: {}".format(i)
            models.append(model)

        predict = []
        for chunk, model in zip(chunks, models):
            task = tasks.run("svm-predict testdata model out",
                             [(chunk, "testdata"), (model, "model")])
            task.label = "svm-predict"
            predict.append(task)

        loom_env.set_trace("mytrace")
        results = loom_env.submit_and_gather(predict)

        assert len(results) == CHUNKS
        for line in results:
            assert line.startswith(b"Accuracy = ")
