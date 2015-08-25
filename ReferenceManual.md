# Introduction #
This document will explain the system requirements necessary for the Poly IR Toolkit (referred to as PolyIRTK from here on), as well as how to compile it, and it's various usage modes.

# System Requirements #
PolyIRTK has been developed with and tested on recent Linux distributions; however, it should run on POSIX compliant operating systems (but perhaps with some tweaks).

# Compiling #
PolyIRTK uses standard C++ as much as possible. It's been compiled with GCC 4.x (recent versions should work OK). It's possible there is some GCC specific code (but not much). It comes with a Makefile, so as long as the following libraries are installed in their default locations, it should compile without changes.

**Dependent Libraries:**
  * zlib
  * POSIX Threads
  * POSIX Asynchronous I/O (AIO)

The Threads and AIO libraries should come with any POSIX compliant OS. zlib has been ported to just about every OS in existence.

# Code Style #
If you would like to hack on PolyIRTK, you'll find that the code style closely follows that of the [Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml). I felt it was very practical and easy to follow.
The main deviation is in the maximum horizontal line size; instead of 80 characters, PolyIRTK uses 160 characters, which I feel takes better advantage of high resolution widescreen monitors.

# Configuration #
PolyIRTK comes with a sample configuration file, 'irtk.conf'. There are many powerful options, which are described in the sample configuration file. Some of the things that can be done through the configuration file (this is, by far, not a complete list):
  * Tune memory usage for the various modes.
  * Define the index coding policies for docIDs, frequencies, positions, and block headers. There are separate coding policies for the indexing and merging steps, allowing you to generate an index with one set of coding policies, and then re-merge with a different set of coding policies.
  * Control whether the index will be indexed/merged with positions (word level index).
  * Control whether the index will remain cached on disk, or loaded fully into main memory.

# Command Line Usage #
PolyIRTK tries to play nice with other UNIX tools; many of the options allow input to come from stdin so that they could be used with pipes and I/O redirection.
Below are some examples of the most common usage modes.

## Indexing ##
**`irtk --index < input`**

The indexer reads the paths to the files it should index from stdin; the above command redirects the standard input to come from a file called 'input'.
The input file is a plain text file that should contain the absolute paths (or relative to the current directory) to the files you need to index, one per line.
PolyIRTK can index either TREC (i.e. Gov2 dataset) or WARC (i.e. ClueWeb dataset) formatted files; however, they must be gzipped. The type of input file is specified in the configuration file.

## Merging ##
**`irtk --merge`**

This command will merge all the index slices from the initial indexing step (with extensions 0.y) into one index, consisting of the files 'index.idx', 'index.lex', 'index.dmap\_basic', 'index.dmap\_extended', and 'index.meta'. Depending on the configuration file, intermediate slices may also be left intact (otherwise, they will be deleted).

**`irtk --merge-input`**

In this command, the merger will read from stdin those files you want to merge followed by the new index slice name. You can specify new groups of files to be merged by leaving an empty new line after the new index slice name.

### Additional Options ###
**`--merge-degree=[value]`**

Specify the merge degree to use (default is 64).

## Querying ##
**`irtk --query [IndexName]`**

Queries the index slice named by 'IndexName'. 'IndexName' can be either of the form 'index\_name' if it's a fully merged index, or 'index\_name:0.1' if it's an index slice that hasn't been fully merged.  In both cases, the document map files 'index.dmap\_basic' and 'index.dmap\_extended' will be used, since the document map is shared across all indices.
If the 'IndexName' is left out, the default is to use the index with the files 'index.idx', 'index.lex', 'index.dmap\_basic', 'index.dmap\_extended', and 'index.meta'.

### Additional Options ###
**`--query-mode=[value]`**

Sets the querying mode. Value is one of 'interactive', 'interactive-single', 'batch', or 'batch-bench'; default is 'interactive'.

**'interactive'**: Will read queries from stdin, displaying a search prompt. User cleanly ends querying with 'Ctrl-D'.

**'interactive-single'**: Will read a single query from stdin, displaying a search prompt, and will exit cleanly after running the single query.

**'batch'**: Will read all queries from stdin and save them in memory. All queries will be timed and query statistics collected; their results will be displayed unless the '--result-format' option is set to 'discard' (the queries themselves will be displayed regardless). This should not be used for benchmarking because the caches are not warmed up.

**'batch-bench'**: Will read all queries from stdin and save them in memory. First, the query log will be executed without timing the queries; then it will be executed a second time, with all queries timed and query statistics collected. Useful when benchmarking with memory mapping the index or using the LRU cache.


---


**`--result-format=[value]`**

Sets the format of the result output. Value is one of 'trec', 'normal', 'compare', and 'discard'; default is 'normal'.

**'trec'**: Will output results in a format suitable for running the 'trec\_eval' program on the output. Document URLs will be replaced with document TREC identifiers.

**'normal'**: Will output the docIDs, scores, and document URLs.

**'compare'**: Will output the docIDs and scores in a compact format. The query will also be output before the results start. Useful for comparing results for different query algorithms.

**'discard'**: Will discard all output. Useful for running large batch queries during benchmarking.


---


**`--query-algorithm=[value]`**

Sets the query algorithm to use. Value is one of 'default', 'daat-and', 'daat-or', 'dual-layered-overlapping-daat', 'dual-layered-overlapping-merge-daat', 'layered-taat-or-early-terminated', 'wand', 'max-score', 'daat-and-top-positions'; 'default' chooses a reasonable algorithm based on the index type (whether it's layered, in-memory, etc.).


---


**`--query-stop-list-file=[value]`**

Provide a stop list of terms that we'll be ignoring while executing queries. We will remove these terms from queries. Value is the path to the stop list file (one term per line).


---


**`--memory-map-index`**

Memory maps the index; this does not load the pages into memory beforehand. The index will be loaded as we execute the queries and request blocks. During the 'batch-bench' query mode, we execute all queries twice to make sure they get loaded into memory before we time the query run time.

## Debug and Performance ##

**`irtk --cat [IndexName]`**

Outputs the specified index in a human readable format.

### Additional Options ###

**`--cat-term=[term]`**

Only output the list for the specified term.


---


**`irtk --diff [IndexName1] [IndexName2]`**

Output the differences between two indices in a human readable format.

### Additional Options ###

**`--diff-term=[term]`**
Only output the differences in the lists for the specified term.


---


**`irtk --test-compression`**

Run compression and decompression tests.


---


**`irtk --test-coder=[coder]`**

Test a particular coder. Coder is one of 'rice', 'turbo-rice', 'pfor', 's9', 's16', 'vbyte', or 'null'.


---


**`--loop-over-index-data=[term]`**

Loops over an inverted list (decompresses but does not do any top-k). Useful for benchmarking decompression coders.


---


**`--retrieve-index-data=[term]`**

Retrieves index data for an inverted list into an in-memory array. See function 'RetrieveIndexData()' in 'ir\_toolkit.cc'.

## Remapping ##

First, generate the remapping file. Currently, PolyIRTK only supports reordering docIDs by their URLs.

**`irtk --generate-url-sorted-doc-mapping=url_sorted_doc_id_mapping < input`**

Note that it's important that the input files are the same ones, and appear in the same order, as those used to generate the index that's going to be remapped. This is because the docIDs are assigned sequentially during the indexing process, and me must remap based on these same sequentially assigned docIDs.
The output of the above command will produce a file named 'url\_sorted\_doc\_id\_mapping'.

This file can be used as an input to the actual docID reordering procedure.

**`irtk --remap=url_sorted_doc_id_mapping`**

The output of the remapping procedure will be a new index with the prefix 'index\_remapped'.

To query, you must specify the name of the remapped index to use, like so:

**`irtk --query index_remapped`**

During querying, the document map will load the mapping file, in order to properly be able to translate the document lengths and URLs.
Note that for querying, currently, the mapping file must be named 'url\_sorted\_doc\_id\_mapping'.

## Layering ##

**`irtk --layerify` `[`InputIndexName`]` `[`OutputIndexName`]`**

This will use 'index' to generate a new layered index named 'index\_layered', by default. You can override these defaults by specifying InputIndexName and OutputIndexName, respectively.

You will want to specify some layering options in the configuration file; alternatively, you can specify them on the command line, like so:

**`irtk --config-options=overlapping_layers=false\;num_layers=8\;layering_strategy=exponentially-increasing --layerify`**

This particular command will not create overlapping layers (each layer will only have unique docIDs), and each list will have at most 8 layers, with an exponential number of docIDs in subsequent layers.

# Index Slices #

An index slice is a complete and queryable index, consisting of the following files:
  * **`[`index\_name`]`.idx`[`.x.y`]`**: The .idx extension is for inverted index files.
  * **`[`index\_name`]`.lex`[`.x.y`]`**: The .lex extension is for lexicon files.
  * **index.dmap\_basic**: The .dmap\_basic and .dmap\_extended extensions are for document map files. A .dmap\_basic file consists of document lengths and offsets into the .dmap\_extended files.
  * **index.dmap\_extended**: The .dmap\_extended file consists of document URLs and document TREC numbers.
  * **`[`index\_name`]`.meta`[`.x.y`]`**: The .meta extension indicates that the file is a meta file, which holds configuration information about how the index was constructed and index statistics.
  * **`[`index\_name`]`.ext`[`.x.y`]`**: The .ext extension is a file generated during the layering process which holds information about the partial BM25 scores of each block and each chunk in the index.

A fully merged index will not have the `[`.x.y`]` extension. The x and y are simply numbers which are assigned by the index merger to distinguish indices. The initial indices, resulting from the indexing step have x equal to 0, and y is just an incrementing index count. Each pass that the merger makes over the index collection will result in an incrementing x, and y is still just an index count within that pass.

Index slices can be merged into larger index slices, as is described in the [merging](Merging.md) section.
