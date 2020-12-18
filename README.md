Compress / decompress files using Huffman encoding.

Compression and decompression defined in huffmantree.c, predefined in common.h. These functions each take two file streams.

Decompression information is stored at the front of the compressed file.

Works with any file input, compression stats in the html folder.

Metadata to restore compressed files could be read from somewhere else (rather than just using one file).

Can compile other code with huffmantree.c and common.h or use as a dynamic library.

The curses utility was a trial. It works to compress and decompress and it will display the huffman tree's values in a table
