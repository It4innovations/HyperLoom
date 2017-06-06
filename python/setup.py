#!/usr/bin/env python

from distutils.core import setup
import os

def read_version():
    with open(os.path.dirname(__file__) + "../version") as f:
        return f.readline().rstrip()

LOOM_VERSION=read_version()
print("LOOM_VERSION =", LOOM_VERSION)

setup(name='loom',
      version=LOOM_VERSION,
      description='Python interface for Loom - workflow system',
      author='Loom team',
      url='',
      packages=['loom'])
