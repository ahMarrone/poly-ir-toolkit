# General Notes #
  * There are some duplicate docIDs in GOV2, and their scores will be the same.

# Index Organization Ideas #
  * Can we store all the block headers together at the front of each list? This should result in somewhat better processor cache utilization, since chunks will be closer together as well as the skipping information; but might not be worth it. In this case, the lexicon would need a pointer (offset) to the start of the appropriate block headers, plus a pointer (offset) to the start of the actual list data (for OR/single word queries) for when we don't need to decode block headers.

  * Another idea to eliminate the "wasted space" at the end of each block. This means that a chunk can span block headers (assuming it can span a maximum of 2 blocks only). Since we know the size of each chunk (stored in the block header), when we start decompressing a chunk, we know if it'll spill over to the next block. The PROBLEM with this scheme is that when a chunk spans across a block, we need to get the complete chunk data in a single array. However, our caching scheme (LRU) prevents this because consecutive blocks are not guaranteed to be placed consecutively in the cache (although it is likely they are). When using this caching scheme, we'd have to check whether the blocks that the chunk spans are consecutively placed, and if not we need to copy the chunk data into a new array and use it as input to the compression function. Note that this does not affect us when the index is completely in main memory, since the blocks are always consecutively placed in main memory.

# Parallel Indexing Capability #
  * Have command line options to specify the number of processes we want and fork off several irtk processes. Each process should create a unique folder for outputting index files (perhaps using the pid for the folder name). The parent that sets this up can split the files to index for each child process and pipe them into each child (through I/O redirection) and then die. Things like merging can be done independently within each folder. The final merge would have to be done by specifying the indices in each folder. Note that each process would need to record the docID offset for it's index. The final merge would have to take the offsets into account.
  * Use Hadoop (via their C/C++ pipes API) to generate indices in parallel.

# Experiments to Run #
  * Benchmark indexing speed and query speed against zettair. Can also index with varbyte to make comparison fair.
  * To test decompression speeds, can use the loop over list function. This would ignore scoring function and other overheads.

# Planned Features #
  * **Unicode Support**: This would require major changes to the parser and lexicon
  * **Parser Improvements**: Aside from unicode support (should be able to parse UTF-8 documents, possibly other character sets), need to allow user to specify which bytes to tokenize on. Also, need a custom XML format for defining documents and their features (title, body, custom fields); this should have the capability to be piped into our program, uncompressed.
  * **B Tree Lexicon**: This is essential to conserving memory, especially on embedded systems. Instead of loading the lexicon into main memory into a hash table, we will have an additional file that is a B tree representation of the lexicon.  The lexicon itself will remain mostly the same (but it would be a good time to front code the terms to save some space). In order to efficiently (as in, don't require the whole thing to be read into main memory) build the B tree, we would need to either split the lexicon into fixed size blocks, or let each term contain a sentinel value, so that we can seek to some position in the lexicon, and figure out where the start of the closest term is. The B tree itself could either be loaded into main memory (it should be fairly small), or some levels can be cached, or not at all. Each internal node will simply contain n terms, front coded, and pointers to the next level; the last level will contain offsets into the actual lexicon file.
  * **Custom Ranking Functions**: Aside from BM25, users should be able to supply weights based on term or document attributes (i.e. PageRank would be a document attribute, whether a term appears in the title or body would be a term attribute). Unclear at this point of how we can utilize this along with our query optimization algorithms based on BM25. One possibility is to get many documents only based on BM25, in an optimized way, and then further rank these using the user's custom ranking.
  * **Split Code Into Library and Command Line Client**: After the split, we could also develop a search daemon and a socket based interface to it.
  * **Support for Mixed Boolean Filtering**: Instead of pure AND / OR queries, need to be able to mix AND, OR, NOT, as well as positional filters (i.e. all terms must be within 10 words of each other).
  * **Improved Document Map**: Need to store additional metadata about each document, so we can support more features in the future. This includes document last fetch date, last modified date, MD5 or SHA1 sum to detect exact duplicates, document language.

# Commands to Keep in Mind #
  * **Valgrind**:
    * **Test cache and branching**: `valgrind --tool=cachegrind --branch-sim=yes --cachegrind-out-file=cachegrind.out ./irtk --loop-over-index-data=gov index`
    * **Test for memory leaks and other invalid uses**: `valgrind --tool=memcheck --leak-check=yes ./irtk --loop-over-index-data=gov index`
  * **Drop caches in Linux (must be root)**:
    * **Free page cache**: `echo 1 > /proc/sys/vm/drop_caches`
    * **Free dentries and inodes**: `echo 2 > /proc/sys/vm/drop_caches`
    * **Free pagecache, dentries and inodes**: `echo 3 > /proc/sys/vm/drop_caches`