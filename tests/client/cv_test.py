from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa
import loom.client.tasks as tasks  # noqa

import os

IRIS_DATA = os.path.join(LOOM_TEST_DATA_DIR, "iris.data")

loom_env  # silence flake8


def test_cv_iris(loom_env):
        CHUNKS = 15
        CHUNK_SIZE = 150 / CHUNKS  # There are 150 irises

        loom_env.start(4, 4)
        loom_env.info = False

        data = tasks.open(IRIS_DATA)
        data = tasks.run(("sort", "--random-sort", "-"), [(data, None)])
        lines = tasks.split(data)

        chunks = [tasks.slice(lines, i * CHUNK_SIZE, (i + 1) * CHUNK_SIZE)
                  for i in xrange(CHUNKS)]

        trainsets = [tasks.merge(chunks[:i] + chunks[i + 1:])
                     for i in xrange(CHUNKS)]

        models = []
        for ts in trainsets:
            model = tasks.run("svm-train data",
                               [(ts, "data")], ["data.model"])
            model.label = "svm-train"
            models.append(model)

        predict = []
        for chunk, model in zip(chunks, models):
            task = tasks.run("svm-predict testdata model out",
                              [(chunk, "testdata"), (model, "model")])
            task.label = "svm-predict"
            predict.append(task)

        loom_env.make_dry_report(predict, "dry.report")

        results = loom_env.submit(predict, report="cv.report")

        assert len(results) == CHUNKS
        for line in results:
            assert line.startswith("Accuracy = ")
