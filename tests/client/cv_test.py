from loomenv import loom_env, LOOM_TESTPROG, LOOM_TEST_DATA_DIR  # noqa

import os

IRIS_DATA = os.path.join(LOOM_TEST_DATA_DIR, "iris.data")

loom_env  # silence flake8


def test_cv_iris(loom_env):
        CHUNKS = 5
        CHUNK_SIZE = 150 / CHUNKS  # There are 150 irises

        loom_env.start(2)
        loom_env.info = True

        p = loom_env.plan()
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
            models.append(model)

        predict = []
        for chunk, model in zip(chunks, models):
            task = p.task_run("svm-predict testdata model out",
                              [(chunk, "testdata"), (model, "model")])
            predict.append(task)

        results = loom_env.submit(p, predict)

        assert len(results) == CHUNKS
        for line in results:
            assert line.startswith("Accuracy = ")

        #p.write_dot("test.dot", loom_env.client.info)
