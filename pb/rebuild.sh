#!/bin/sh

BASE_DIR=`dirname $0`/..

LIBLOOM_DIR=${BASE_DIR}/src/libloom
PYTHON_DIR=${BASE_DIR}/python/loom/pb

# LIBLOOM
protoc loomcomm.proto --cpp_out=${LIBLOOM_DIR}
protoc loomplan.proto --cpp_out=${LIBLOOM_DIR}
protoc loomrun.proto --cpp_out=${LIBLOOM_DIR}/tasks

# CLIENT (Python)
protoc loomcomm.proto --python_out=${PYTHON_DIR}
protoc loomplan.proto --python_out=${PYTHON_DIR}
protoc loomrun.proto --python_out=${PYTHON_DIR}
protoc loomreport.proto --python_out=${PYTHON_DIR}
