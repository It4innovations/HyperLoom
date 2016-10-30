import cloudpickle


def unpack_and_execute(data, inputs):
    obj = cloudpickle.loads(data)
    return obj(*inputs)
