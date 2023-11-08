# Benchmark Data Sets

The directory `/data` contains five corpora that can be used to:

* Evaluate and experiment with MoonLight;
* Serve as regression test cases; and
* Serve as benchmarks of performance - both in terms of solution quality but
  also runtime performance.

You may find that the PDF and HTML corpuses will present problems to you if you
have a limited amount of RAM.

The following table gives you an understanding of what level of distillations
you should get when you run the tool:

| File Type | Collection Size | Distilled Size | Improvement Gain |
|:---------:|----------------:|---------------:|:----------------:|
|    PDF    |          42,056 |            664 |               63 |
|    DOC    |           7,836 |            745 |               10 |
|    PNG    |           4,831 |             94 |               51 |
|    TTF    |           5,666 |             27 |              210 |
|   HTML    |          69,991 |            410 |              171 |

# Running the Benchmarks

Extract the files from a benchmark archive:

```console
tar xJf png.tar.xz -C /tmp
```

To run MoonLight on this benchmark:


```console
moonlight -d /tmp/png -r exemplar_ -mm matrix_png.matrix -n png1
```

This tells MoonLight to:

* Find the collection corpus in `/tmp/png`
* Only use trace files with "exemplar_" prefix
* Store the corpus collection matrix in file "matrix_png.matrix". This can save
  a lot of time if you wish to recompute a solution later on and you are using
  a large corpus
* Name this computation "png1"

At runtime MoonLight will display a lot of information to the screen. This same
information is also logged to `moonlight.log` in your current working
directory.

When the program completes you will have three other files in the collection
corpus directory you specified (in this case `/tmp/png`):

* **png1_solution.json**: Contains the file names of the seed traces which are
  in the solution.
* **png1_analytics.csv**: Contains statistics about the individual seed traces.
* **matrix_png.matrix**: A serialization of the corpus matrix. If you
  performing multiple iterations on a corpus that doesn't change then this file
  can be used to save a lot of time.

**If you just care about which seeds to use in your fuzzing corpus then you
only need to examine the file _png1_solution.json_.**

If you are interested in performance statistics then the following information
will be useful to you.

* **Number of Rows**: The number of trace files found that matches the supplied
  regex.
* **Number of Columns**: The total number of basic blocks found in the
  collective trace data.
* **Number of Elements**: The total number basic blocks (edges) actually used
  in the trace data in the sense that the bit vector for that block was set to
  one.
* **Sparsity (density)**: The sparsity of the initial matrix. This is the
  number of elements divided by the size of the matrix (#rows x #columns).
* **Solution size**: The number of seeds in the final solution.
* **Non-optimal choices**: The number of times the algorithm was forced to make
  a non-optimal seed selection choice. In this case all choices were optimum
  and the final solution is also an optimal solution.
* **Init singularities**: A singularity is a column whose sum is zero. This
  means that there are no seeds in the corpus that covers that column (basic
  block).
* **Rows to verify**: MoonLight always verifies that the solution it computed
  is an actual cover of all the non-singular columns of the matrix.

There is quite a bit more information available in the logs as well as the
analytics CSV file.

[Back to main page](README.md)
