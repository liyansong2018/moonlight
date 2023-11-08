# Moonlight Usage

## Example
A typical run of Moonlight can be called like this:

```console
moonlight -d /tmp/PNG -r exemplar_ -m matrix_png.matrix -n png1
```

See this [Example](BENCHMARKS.md) for understanding the output of the tool in
more detail. However, the most important file to look at in this case is
**png1_solution.json** - which contains the file names of the seed traces which
form the solution.

## Useful Command Line Switches

Moonlight has a number of command line options:

- `--directory, -d <directory>`
  The directory location of the corpus exemplar files.

- `--pattern -r <regex>`
  The regular expression pattern to select exemplar files in the corpus directory.

- `--matrix, -m  <matrix data file>`
  File name to load and/or save matrix data to file. It helps avoid the perhaps
  expensive step of re-creating a matrix from exemplar file data if that's been
  done before.

- `--name, -n  <run name>`
  User defined name for the Moonlight run. Useful for some logging messages
  and data files that are created during the run.
  default: `moonlight`

- `--ignore-matrix, -i`
  Ignore an existing matrix data file (if it exists) and load the raw data.

- `--large-data, -l`
  Data will be too large in sparse matrix format.
  Uses less memory but triples data read in time.

- `--weighted, -w`
  Path to text file containing corpus weights. Each line in this file contains
  the name of a file in the corpus followed by a float representing its weight.
  The file name and weight are separated by a space.

- `--help`
  Produce a nice help message.

[Back to main page](README.md)
