# Buffered file transfer

Due to low UART throughput, and the recently appeared necessity to flash
multiple boards in parallel, it is reasonable to store whatever files are to be
transfered to the AP in an intermediate storage, and transfer those in
background using UART leaving ESP working.

It has some FS-related abstraction layers, because it will probably be needed
to store files in external RAM or flash.

# Stages

BFT component can be conceptualized in the following way. The entire process
consists of two stages:

1. Buffering
2. Transfer

Each of those stages notify respective components over the appropriate event
key (see "buffered_file_transfer/Sub.hpp" for the list of topics).

# Limitations and ontological constraints

- Plain file structure is implied, no directories;
- A notion of session is employed, i.e. the process of writing to a file is
  stateful: open, write, close;
- A file is viewed as an array of bytes;

