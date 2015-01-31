//=========================================================================================================
//
//  Jeff Bohlin
//  December 9th, 2012
//  CS351 - TTR 5:30
//  Professor Gofman
//  Assignment 3 Part II
//  File copier based on memory-mapped files
//
//  This program completes the following tasks:
//
//  1. The program is invoked with the source file name and the destination file name as parameters.
//  2. The program uses the mmap() system call to map both files into memory.
//  3. The program uses the memory-mapped file memory to to copy the source file to the destination file.
//
//  Sources used to help complete this assignment:
//
//  1. Professor Gofman's "mcp.cpp" guide.
//  2. Linux System Programming:
//       http://my.safaribooksonline.com/book/operating-systems-and-server-administration/linux/0596009585
//       /advanced-file-i-o/mapping_files_into_memory
//  3. Cprogramming.com forums (5-12-2008):
//       http://cboard.cprogramming.com/c-programming/102991-using-mmap-copying-large-files.html
//
//=========================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

// A struct to hold information regarding the input file.
struct stat fileInfo;

// This function prepares the destination file for writing. It creates a destination file
// (if it does not already exist). Parameters: The input file name (from the user), the 
// input file descriptor, and the output file descriptor.
void prepDestFile(char *finFileName, int &finfd, int &foutfd);

// This function maps the input and output files into memory. Then it copies the contents of 
// the input file to the output file. Parameters: the input and output file descriptors.
void mapFiles(int finfd, int foutfd);

// Main expects 2 arguments from the user: the source (input) filename and the destination 
// (output) filename.
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "\nTo run this program, use the following command:\n"
            "%s <SOURCE FILE> <DESTINATION FILE>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create an "easier-to-read" pointer for the input filename.
    char *finFileName = argv[1];

    // Create an "easier-to-read" pointer for the output filename.
    char *foutFileName = argv[2];

    // We need to open the input file here because 'fstat' will need a valid file 
    // descriptor (inside of prepDestFile()).
    int finfd = open(finFileName, O_RDONLY);

    // Check that we opened the input file successfully.
    if (finfd < 0)
    {
        perror("fin open");
        exit(EXIT_FAILURE);
    }

    // We need to open the output file here because 'fstat' will need a valid file 
    // descriptor (inside of prepDestFile()). If the output file does not exist, 
    // then create one with Read/Write permissions.
    int foutfd = open(foutFileName, O_RDWR | O_CREAT, 0666);

    // Check that we opened (or created) the output file successfully.
    if (foutfd < 0)
    {
        perror("fout open");
        exit(EXIT_FAILURE);
    }

    prepDestFile(finFileName, finfd, foutfd);
    mapFiles(finfd, foutfd);

    // Close the input file.
    if (close(finfd) == -1)
    {
        perror("fin close");
        exit(EXIT_FAILURE);
    }
    // Close the output file.
    if (close(foutfd) == -1)
    {
        perror("fout close");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void prepDestFile(char *finFileName, int &finfd, int &foutfd)
{
    // I am using 'fstat' instead of 'stat'. 'fstat' takes the input file descriptor and the 
    // 'fileInfo' struct as parameters.  Both functions are basically identical except that the 
    // file descriptor is used, rather than the file name. 'fstat' populates the 'fileInfo' struct 
    // with meaning information about the specified file. Such as the file size, ownership, etc.
    if (fstat(finfd, &fileInfo) == -1)
    {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    // This checks to make sure that we are working with a regular file. Meaning, that we are not 
    // playing with a pipe, directory, etc.
    if (!S_ISREG(fileInfo.st_mode))
    {
        fprintf(stderr, "%s is not a file!\n", finFileName);
        exit(EXIT_FAILURE);
    }

    // I am using 'lseek' instead of 'fseek'. Both are basically identical, however 'fseek' works 
    // with file pointers and 'lseek' with file descriptors. This sets foutfd file descriptor to
    // the correct offset to match that of the input file.
    if (lseek(foutfd, fileInfo.st_size - 1, SEEK_SET) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }

    // Make sure that the output file can be written to successfully.
    if (write(foutfd, "", 1) == -1)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

void mapFiles(int finfd, int foutfd)
{
    char *fin = NULL;                       // A pointer that will be used for mapping the input file.
    char *fout = NULL;                      // A pointer that will be used for mapping the output file.
    long pageSize = sysconf(_SC_PAGESIZE);  // The default page size.
    int data = fileInfo.st_size;            // The amount of data still to be written.
    off_t offset = 0;                       // The offset.


    // Note: this loop is similar to Professor Gofman's, however the the page sizing logic is different.  
    // This is one of those instances where there really is only 1 or 2 different ways of accomplishing
    // this task.  We know we definitely need a loop. We also know that the page size, the offset, and 
    // the mappings are going to be changing, so this is how it needs to be done.

    // While there is still data to be written...
    while (data > 0)
    {
        // If the amount of data left is less than the page size...
        if (data < pageSize)
        {
            //  ...Then, set the page size equal to the amount of data left.
            pageSize = data;

            // There is no more data left to write.
            data = 0;
        }
        else
        {
            // Else, subtract the page size from the amount of data currently left.
            data -= pageSize;
        }

        // Map the page sized input (source) file.
        fin = (char*)mmap(0, pageSize, PROT_READ, MAP_SHARED, finfd, offset);

        // Make sure that the mapping was successful.
        if (fin == MAP_FAILED)
        {
            perror("fin mmap");
            exit(EXIT_FAILURE);
        }
        // Map the page sized output (destination) file.
        fout = (char*)mmap(0, pageSize, PROT_READ | PROT_WRITE, MAP_SHARED, foutfd, offset);

        // Make sure that the mapping was successful.
        if (fout == MAP_FAILED)
        {
            perror("fout mmap");
            exit(EXIT_FAILURE);
        }

        // Copy the data from the input file (that is currently in memory) to the output file.
        memcpy(fout, fin, pageSize);

        // Un-map the input file.
        if (munmap((void*)fin, pageSize) == -1)
        {
            perror("fin munmap");
            exit(EXIT_FAILURE);
        }

        // Un-map the output file.
        if (munmap((void*)fout, pageSize) == -1)
        {
            perror("fout munmap");
            exit(EXIT_FAILURE);
        }

        // Adjust the offset according to the current page size.
        offset += pageSize;
    }
}
