# Install

## Prerequisites

MoonLight requires the following packages to be installed (assumes Ubuntu):

```console
apt-get install cmake               \
    libboost-filesystem-dev         \
    libboost-log-dev                \
    libboost-program-options-dev    \
    libboost-regex-dev              \
    libboost-serialization-dev      \
    libboost-system-dev             \
    libboost-thread-dev
```

If you wish to build the API documentation, Doxygen must also be installed:

```console
apt-get install doxygen
```

If you wish to run the unit tests and benchmarks, Python must be installed.
Both Python 2.7 and 3.5 should work.

```console
apt-get install python python-pip

# Install the required packages
pip install -r moonlight-code/python/requirements.txt
```

## Building

To build MoonLight:

```console
git clone https://gitlab.com/cybersec/moonlight-code.git
cd moonlight-code
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install/directory ..
make
make install
```

To build the documentation:

```console
make doc
```

[Back to main page](README.md)
