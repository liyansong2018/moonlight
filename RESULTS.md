# MoonLight Performance Results

MoonLight can perform both **weighted and unweighted** distillations of a
corpus.

## What does this mean?

In **unweighted** distillation we only care about minimising the **number of
seeds** in the fuzzing corpus.

In a **weighted** distillation we care about minimising the **total size** (in
bytes) of the fuzzing corpus. We usually end up with more seeds compared to the
unweighted solution but the total byte weight will be smaller.

## Why does this matter?

Some fuzzers are IO bound so you not only want a small sized fuzzing corpus but
you also want the total corpus size in bytes to be as small as possible.

Other corpus distillation tools typically use a greedy reduction algorithm.
This actually works pretty well in practice because you do get very good
distillation gains. Of course why have "good enough" when you can have the best
for free?

Let's benchmark the MoonLight tool with the greedy algorithm...

## Benchmark Corpus Performance Results

We will use our benchmark corpora under `/data`.

### Unweighted Distillations

The following table shows MoonLight outperforms greedy in all cases:

| File Type | Greedy | MoonLight | Optimal Size |
|:---------:|:------:|:---------:|:------------:|
|    PDF    |  724   |    664    |     663      |
|    DOC    |  766   |    745    |     744      |
|    PNG    |   97   |    94     |      94      |
|    TTF    |   28   |    27     |      27      |
|   HTML    |  454   |    410    |     409      |

The other important feature of the MoonLight tool is that it can also tell you
**a bound** on how far its solution could be from an optimum solution. The last
column in the above table gives a lower bound on the size of an optimal corpus.
That is, we can be sure than any optimal solution can't be smaller than this
size. As you can MoonLight produces near optimal distillations of a corpus. For
PNG and TTF we can be sure that our solution is also an optimal cover for the
corpus. Having processed over 300 corpora we have never seen a MoonLight
solution be more than 3 seeds away from an optimal lower bound.

### Why can't we just compute an optimal solution if we can measure how far away we are from optimality?

Computing an optimal solution is NP Hard because the corpus distillation
problem is an instance of the [minimal (weighted) set cover decision
problem](https://en.wikipedia.org/wiki/Set_cover_problem).

Whilst our algorithm as a side effect of the computation allows us to count the
non-optimal choices we make it doesn't tell you how to make a better one. Put
another way, we can measure our distance from optimality but we don't know what
direction it is in.

### Weighted Distillations

As was noted on the main page sometimes you must care about the **total
weight** of your solution (measured in bytes).

The following table compares MoonLight to the greedy algorithm for the weighted
case. Weights are in bytes. The size is the number of seeds in the solution.

| File Type | Greedy Size | MoonLight Size | Greedy Weight | MoonLight Weight |
|:---------:|:-----------:|:--------------:|--------------:|-----------------:|
|    PDF    |    1240     |      855       |   648,260,319 |      606,841,839 |
|    DOC    |    1009     |      777       | 1,126,046,335 |    1,071,298,008 |
|    PNG    |     138     |      104       |    13,730,370 |       13,251,301 |
|    TTF    |     31      |       27       |     1,156,709 |        1,121,558 |
|   HTML    |     756     |      530       |    90,062,979 |       84,925,558 |

As you can see MoonLight not only has a smaller weighted corpus than the greedy
case but the number of seeds in each corpus is smaller. All good for IO
optimisation. You can also see the number of seeds in the solution is usually
larger than in the unweighted case.

At first glance perhaps the weight benefits of MoonLight over greedy don't look
that impressive. However, over a long fuzzing campaign those benefits earn
compound interest in the form of higher iteration rates delivered through
better IOPS, cache hits and test throughput.

[Back to main page](README.md)
