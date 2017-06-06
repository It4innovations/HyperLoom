#!/usr/bin/env python

from distutils.command.build_py import build_py as _build_py
from distutils.core import setup
import os

def read_version():
    with open(os.path.dirname(__file__) + "../version") as f:
        return f.readline().rstrip()

LOOM_VERSION=read_version()
print("LOOM_VERSION =", LOOM_VERSION)


class build_protoc(_build_py):
    """Also calls protoc"""
    def run(self):
        os.system("sh ./generate.sh")
        _build_py.run(self)



setup(name='loom',
      version=LOOM_VERSION,
      description='Python interface for Loom - workflow system',
      author='Loom team',
      url='',
      packages=['loom'],
      cmdclass={'build_py': build_protoc}
      )
