shadowtrace
===========

Instructions
============

1. Copy trace/trace.c into your directory tree and build your excecutable with -finstrument-functions flag and trace.c. Do not forget -g option to preserve symbols. Exclude as many shared
libraries as possible. Link them statically. If you are interested in the library calls, also build that library with -finstrument-functions and -g.

2. In the running directory do the following:

touch TRACE

3. Run yout executable. When it is done, the output will be written into TRACE file.

4. Build trace package:

make

5. Generate call tree:

src/post-process <input file> <output file>

Input file here is TRACE file.

6. Address to symbol mapping:

scripts/get-symbols.pl -e <executable file> -i <optpus of pre-process step> -o <symbol mapping file>

7. Generate tree file:

scripts/print-call-tree.pl -i <optpus of pre-process step> -s <symbol mapping file> -o <trace tree file>
