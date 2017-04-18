
Installation
============

HyperLoom has two components from the installation perspective:

* Runtime - the HyperLoom infrastructure (Server and Worker)
* Python client

Both components resides in the same Git repository,
but their installations are independent.

The main repository is: https://code.it4i.cz/ADAS/loom

Runtime
-------

The HyperLoom infrastructural components depend on the following libraries that are not included in the HyperLoom
source code:

* **libuv** -- Asychronous event notification
* **Protocol buffers** -- Serialization library
* **Python >=3.4** (optional)
* **Clouldpickle** (optional)

(HyperLoom also depends on **spdlog** and **Catch** that are distributed together
with HyperLoom)

In **Debian** based distributions, dependencies can be installed by the
following commands: ::

    apt install libuv-dev libprotobuf-dev
    pip install cloudpickle

.. Note::
   If you are going to create plans with many tasks, you can obtain a
   significant speedup by using PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION="cpp"
   feature.


When dependencies are installed, HyperLoom itself can be installed by the following
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
    sh generate.sh
    python setup.py install
