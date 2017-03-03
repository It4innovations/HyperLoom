#!/bin/sh
set -e

BASE_DIR=`dirname $0`/..

PROTO_DIR=${BASE_DIR}/pb
PYTHON_DIR=${BASE_DIR}/python/loom/pb

mkdir -p ${PYTHON_DIR}

protoc -I ${PROTO_DIR} ${PROTO_DIR}/comm.proto --python_out=${PYTHON_DIR}
protoc -I ${PROTO_DIR} ${PROTO_DIR}/run.proto --python_out=${PYTHON_DIR}
