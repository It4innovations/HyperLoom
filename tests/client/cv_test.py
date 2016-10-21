from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

import os

IRIS_DATA = os.path.join(LOOM_TEST_DATA_DIR, "iris.data")

loom_env  # silence flake8
import client  # noqa


def test_cv_iris(loom_env):
        CHUNKS = 15
        CHUNK_SIZE = 150 / CHUNKS  # There are 150 irises

        loom_env.start(4, 4)
        loom_env.info = False

        p = loom_env.plan_builder()
        data = p.task_open(IRIS_DATA)
        data = p.task_run(("sort", "--random-sort", "-"), [(data, None)])
        lines = p.task_split(data)

        chunks = [p.task_slice(lines, i * CHUNK_SIZE, (i + 1) * CHUNK_SIZE)
                  for i in xrange(CHUNKS)]

        trainsets = [p.task_merge(chunks[:i] + chunks[i + 1:])
                     for i in xrange(CHUNKS)]

        models = []
        for ts in trainsets:
            model = p.task_run("svm-train data",
                               [(ts, "data")], ["data.model"])
            model.label = "svm-train"
            models.append(model)

        predict = []
        for chunk, model in zip(chunks, models):
            task = p.task_run("svm-predict testdata model out",
                              [(chunk, "testdata"), (model, "model")])
            task.label = "svm-predict"
            predict.append(task)

        loom_env.make_dry_report(p.plan, "dry.report")

        results = loom_env.submit(p, predict, report="cv.report")

        assert len(results) == CHUNKS
        for line in results:
            assert line.startswith("Accuracy = ")

        #p.write_dot("test.dot", loom_env.client.info)
