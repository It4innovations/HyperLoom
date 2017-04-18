# Loom

Loom is a framework for distributed computation, mainly focused on scientific
pipelines.

  * BSD license
  * High-level Python interface, back-end written in C++
  * Peer-to-peer data sharing between workers
  * Low latency & low task overhead to process hundred thousands of tasks

## Quickstart

Execute a Loom pipeline in 4 easy steps:

### 1. Deploy virtualized Loom infrastructure

```
docker-compose up
```

Note that before re-running `docker-compose up` you need to run `docker-compose down` to delete containers state.

### 2. Install Loom client (virtualenv)

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
