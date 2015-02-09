 This assignment implements a program for computing a hash of a file using MD5,
 SHA1, SHA224, SHA256, SHA384, and SHA512 hashing algorithms. The program takes the
 name of the target file as a command line argument, and then does the following:

 1. Checks to make sure the file exists.
 2. Creates two pipes.
 3. Forks all child processes immediately in parallel.
 4. The parent transmits the name of the file to each child (over the first pipe).
 5. Each child receives the name of a file and computes the hash of the file using either
 the MD5, SHA1, SHA224, SHA256, SHA384, or SHA512 hashing algorithms.
 6. The child transmits the computed hash to the parent (over the second pipe) and
 terminates.
 7. The parent uses the select() function to wait until a pipe is ready to be read.
 Then, once a pipe is ready, the parent reads from that pipe the hash value and then
 prints it.
 8. The process repeats until all available pipes in the fd_set have been read.
 9. The parent terminates after all hashes have been computed.