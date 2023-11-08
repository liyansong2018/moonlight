# Python Scripts

This directory contains a number of useful Python scripts for testing and
verifying MoonLight on the corpora under `data/`.


## Benchmarks

See [here](DATA.md) and [here](BENCHMARKS.md) for more information.

### Running

A user can run all or some of the benchmarks in `data/`. To run all of the
benchmarks:

```console
cd moonlight-code
mkdir build && cd build
cmake ..
make
make benchmarks
```

Benchmarks can also be run individually by running the Python script directly:

```console
cd moonlight-code/python
./run_benchmarks.py --moonight-path=/path/to/moonlight --benchmarks png
```

## Unit tests

The unit tests simulate different workloads for MoonLight. They are described
in JSON format. The exepcted solution is given in the JSON under the `solution`
key.

### Running

```console
cd moonlight-code
mkdir build && cd build
cmake ..
make
make unit_tests
```
