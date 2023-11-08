# MoonLight: Fuzzing Corpus Design and Construction

![moonlight logo](img/moonlight-logo.png "MoonLight logo")

**MoonLight** is a research project looking at efficient methods for
_designing_ a corpus typically for use in fuzzing campaigns. We call this
design process **distillation**.

Corpus distillation is the process by which we take a very large corpus of file
types - e.g. 100,000 PDF files - called **seeds** - and choose a _much smaller
subset_ to fuzz against - say 1,000 PDF files. However, we want this smaller
subset corpus to have the same "_expressive power_" as the original large
corpus. We don't want to lose any information in the distillation process.

Currently "expressive power" means **maximising code coverage**. However, we
are planning other types of measures of interest, such as execution complexity
and usage of specific libraries or system calls.

The project name "MoonLight" is a pun on the word _distillation_. We use the
term distillation in preference to the commonly used term _reduction_. This is
because in fuzzing parlance, "reduction" is a term that is also used in the
crash triage process.

We have included five benchmark corpora in this repo. The following table gives
you an understanding of what level of distillations you should get when you run
the tool.

| File Type | Collection Size | Distilled Size | Improvement Gain |
|:---------:|----------------:|---------------:|:----------------:|
|    PDF    |          42,056 |            664 |               63 |
|    DOC    |           7,836 |            745 |               10 |
|    PNG    |           4,831 |             94 |               51 |
|    TTF    |           5,666 |             27 |              210 |
|   HTML    |          69,991 |            410 |              171 |

Now if you were fuzzing Adobe Acrobat, instead of using 42,000 files you would
only need the much smaller subset of 664 files.

Sometimes what is important is _not_ just the number of seeds in your corpus
but the _total weight_ (or size) of the seeds in bytes. This is particularly
important if your fuzzer is IO bound. In this case the corpus design and
construction is choosing seeds which not only maximise code coverage but also
have the smallest _weight_.

_MoonLight can perform both weighted and unweighted distillations._

For more information on the performance of MoonLight against the current state
of the art see [Results](RESULTS.md) and our paper on
[Arxiv](https://arxiv.org/abs/1905.13055). If you use MoonLight, please cite our
work:

```
@article{2020:Herrera:MoonLight,
  author    = {Adrian Herrera and
               Hendra Gunadi and
               Liam Hayes and
               Shane Magrath and
               Felix Friedlander and
               Maggi Sebastian and
               Michael Norrish and
               Antony L. Hosking},
  title     = {Corpus Distillation for Effective Fuzzing: A Comparative Evaluation},
  journal   = {CoRR},
  volume    = {abs/1905.13055},
  year      = {2020},
  url       = {http://arxiv.org/abs/1905.13055},
  archivePrefix = {arXiv},
  eprint    = {1905.13055},
}
```

## Installation

See [here](INSTALL.md)

## Usage

See [here](USAGE.md)

## Trace Data

For more information on the expected input format of seed trace data see
[here](DATA.md).
