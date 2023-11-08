# Trace Data Notes

Here we explain the following terms:

* **collection** corpus
* **fuzzing** corpus, or the **distilled** corpus

We also describe the _file format_ for the _seed trace files_ and our  _naming
convention_ for trace files.

## Collection and Fuzzing Corpora

Let's imagine that you have somehow collected a large collection of seeds that
you  potentially want to use in your fuzzing campaign. Let's call this the
**collection corpus**.

For example, in our benchmark data sets included in this project we have the
following five _file type_ corpora:

* Adobe Acrobat (PDF)
* Portable Network Graphics (PNG)
* Microsoft Word (DOC)
* Web HTML (HTML)
* True Type Font (TTF)

Each of these corpora contains a set of **trace files** - one for each seed we
have traced using a dynamic binary analysis tool.

After MoonLight performs a distillation it identifies a __subset__ of the
collection corpus that can be used for fuzzing. This subset is called the
**fuzzing corpus**.

## Trace File Format

The file format of each trace is trivially simple. The contents of the file are
just a bit vector. The **index of each bit** corresponds to a **specific basic
block** in the target application.

For example, the 273rd bit in the trace file corresponds to the 273rd basic
block in the target application. Moreover, the 273rd bit in **every** trace
file in the corpus corresponds to the exact same 273rd basic block in the
target.

Clearly the tracing tool you use must allow you to identify each basic block in
the target application and allow you to create a map from the basic block its
index position into the bit vector.

If the tail (suffix) of the bit vector consists of a run of zeros (bits not
set), then it is permissible to truncate the trace subject to the resulting bit
vector size being a multiple of eight - all traces are an integer multiple of
bytes because they are stored in files. MoonLight will assume that that the
absence of any "higher ranked" bits imply a setting of zero.

In practice, this feature gives substantial file size savings for corpora that
contain a large number of seeds as well as some execution time benefits.

Clearly the largest sized trace file in the corpus defines the maximum number
of basic blocks observed by the tracing tool.

## Notes

Instead of tracing for "basic blocks", some people want to trace for "basic
block _edges_". For example, imagine you have two basic blocks 0 and 1 and two
traces A and B. Trace A goes from BBL 0 --> BBL 1. Trace B goes from BBL 1 -->
BBL 0.

Instead of having two traces with the bit vector "1100 0000" rather with this
encoding you have A:"1000 0000" and B:"0100 0000" to distinguish the two
different edge traversals - the idea being that this captures a different
execution behaviour rather than the simpler basic block coverage.

This is totally fine and MoonLight will work as advertised. However, the
_index_ positions in the bit vector **must uniquely correspond** to an _edge_
between two specific basic blocks and this indexation must be fixed across all
traces. After that the  distillation process is exactly the same.

[Back to main page](README.md)
