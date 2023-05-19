# Buffered file transfer

Due to low UART throughput, and the recently appeared necessity to flash
multiple boards in parallel, it is reasonable to store whatever files are to be
transfered to the AP in an intermediate storage, and transfer those in
background using UART leaving ESP working.

# Limitations and ontological constraints

- Plain file structure is implied, no directories;
- A notion of session is employed, i.e. the process of writing to a file is
  stateful: open, write, close;
- No session-sharing mechanism: one user, one file, one session;
- A file is viewed as an array of bytes;
