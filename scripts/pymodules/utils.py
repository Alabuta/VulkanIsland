# coding: utf-8

from __future__ import print_function

__all__ = ['print_fmt', 'err_print_fmt']

import os
import sys
import subprocess

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, 'pymodules'))

import custom_exceptions as exs


def print_fmt(header, msg):
	print(header, msg.replace('\n', '\n\t'), sep='\n\t')

def err_print_fmt(header, msg):
	print(header, msg.replace('\n', '\n\t'), sep='\n\t', file=sys.stderr)