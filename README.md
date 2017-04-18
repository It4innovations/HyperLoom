# HyperLoom

HyperLoom is a platform for defining and executing workflow pipelines in a distributed environment. HyperLoom aims to be a highly scalable framework that is able to efficiently execute millions of interconnected tasks on hundreds of computational nodes.

HyperLoom features:

  * Optimized dynamic scheduling with low overhead.
  * In-memory data storage with a direct access over the network with a low I/O footprint.
  * Direct worker-to-worker data transfer for low server overhead.
  * Third party application support.
  * Data-location aware scheduling reducing inter-node network traffic.
  * C++ core with a Python client enabling high performance available through a simple API.
  * High scalability and native HPC support.
  * BSD license.

## Quickstart

Execute your first HyperLoom pipeline in 4 easy steps using [Docker](https://docs.docker.com/):

### 1. Deploy virtualized HyperLoom infrastructure

```
docker-compose up
```

Note that before re-running `docker-compose up` you need to run `docker-compose down` to delete containers state.

### 2. Install HyperLoom client (virtualenv)

```
virtualenv -p python3 loom_client_env
source loom_client_env/bin/activate
pip3 install cloudpickle protobuf
cd ./python
chmod +x generate.sh
./generate.sh
python3 setup.py install
```

### 3. Define a pipeline

Create a python file `pipeline.py` with the following content:

```
from loom.client import Client, tasks

task1 = tasks.const("Hello ")        # Create a plain object
task2 = tasks.const("world!")        # Create a plain object
task3 = tasks.merge((task1, task2))  # Merge two data objects together

client = Client("localhost", 9010)   # Create a client object
future = client.submit_one(task3)    # Submit task

result = future.gather()             # Gather result
print(result)                        # Prints b"Hello world!"
```

### 4. Execute the pipeline

```
python3 pipeline.py
```

## Documentation

You can build the full documentation from the [doc](./doc) subdirectory by running `make html`.

## Acknowledgements

This project has received funding from the European Union’s Horizon 2020 Research and Innovation programme under Grant Agreement No. 671555. This work was also supported by The Ministry of Education, Youth and Sports from the National Programme of Sustainability (NPU II) project „IT4Innovations excellence in science - LQ1602“ and by the IT4Innovations infrastructure which is supported from the Large Infrastructures for Research, Experimental Development and Innovations project „IT4Innovations National Supercomputing Center – LM2015070“.

## License

See the [LICENSE](./LICENSE) file.
