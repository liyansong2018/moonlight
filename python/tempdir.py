# Copyright 2017 The Australian National University
#
# This software is the result of a joint project between the Defence Science
# and Technology Group and the Australian National University. It was enabled
# as part of a Next Generation Technology Fund grant:
# see https://www.dst.defence.gov.au/nextgentechfund
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.

from __future__ import print_function

import shutil
import sys
import tempfile
import warnings


# Adapted from http://stackoverflow.com/a/19299884/5894531
class TemporaryDirectory(object):
    """
    Create and return a temporary directory.  This has the same behavior as
    mkdtemp but can be used as a context manager. For example:

        with TemporaryDirectory() as tmpdir:
            ...

    Upon exiting the context, the directory and everything contained in it are
    removed.
    """

    _warn = warnings.warn

    def __init__(self, suffix='', prefix='tmp', dir_=None):
        self._closed = False
        self.name = None
        self.name = tempfile.mkdtemp(suffix, prefix, dir_)

    def __repr__(self):
        return '<{} {!r}>'.format(self.__class__.__name__, self.name)

    def __enter__(self):
        return self.name

    def __exit__(self, exc, value, tb):
        self.cleanup()

    def __del__(self):
        self.cleanup(warn=True)

    def cleanup(self, warn=False):
        if self.name and not self._closed:
            try:
                shutil.rmtree(self.name)
            except Exception as e:
                print('ERROR: {!r} while cleaning up {!r}'.format(e, self),
                      file=sys.stderr)
                return

            self._closed = True
            if warn:
                self._warn('Implicitly cleaning up {!r}'.format(self),
                           RuntimeWarning)
