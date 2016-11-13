
Installation
============

Loom has two components from the installation perspective:

* Runtime (Server and Worker)
* Python client

Both components resides in the same Git repository,
but their installations are independent.

The main repository is: https://code.it4i.cz/boh126/loom

Runtime
-------

Runtime depends on the following libraries that are not included into the Loom
source codes:

* **libuv** -- Asychronous event notification
* **Protocol buffers** -- Serialization library
* **Python >=3.4** (optional)
* **Clouldpickle** (optional)

(Loom also depends on **spdlog** and **Catch** that are distributed together
with Loom)

In **Debian** based distributions, dependencies can be installed by the
following commands: ::

    apt install libuv-dev libprotobuf-dev
    pip install cloudpickle

When dependencies are installed, Loom itself can be installed by the following
commands: ::

   cd loom
   mkdir _build
   cd _build
   cmake ..
   make
   make install


Python client
-------------

Python client depends on:

* **Protocol buffers**
* **Cloudpickle**

Python client can be installed by the following commands: ::

    cd loom/python
    python setup.py install
