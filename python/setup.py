#!/usr/bin/env python

from setuptools.command.build_py import build_py as _build_py
from setuptools import setup, find_packages


import os


def read_version():
    with open(os.path.join(os.path.dirname(__file__), "..", "version")) as f:
        return f.readline().rstrip()


LOOM_VERSION = read_version()
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
      packages=find_packages(),
      cmdclass={'build_py': build_protoc}
      )
