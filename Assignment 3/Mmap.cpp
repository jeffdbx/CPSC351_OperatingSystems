//========================================================================================================================
//
// 	Jeff Bohlin
// 	December 9th, 2012
// 	CS351 - TTR 5:30
// 	Professor Gofman
// 	Assignment 3 Part II
// 	File copier based on memory-mapped files
//
//	This program completes the following tasks:
//
//	1. The program is invoked with the source file name and the destination file name as parameters.
//  2. The program uses the mmap() system call to map both files into memory.
//  3. The program uses the memory-mapped file memory to to copy the source file to the destination file.
//
// 	Sources used to help complete this assignment:
//
//  1. Professor Gofman's "mcp.cpp" guide.
//  2. Linux System Programming:
//       http://my.safaribooksonline.com/book/operating-systems-and-server-administration/linux/0596009585
//       /advanced-file-i-o/mapping_files_into_memory
//  3. Cprogramming.com forums (5-12-2008):
//       http://cboard.cprogramming.com/c-programming/102991-using-mmap-copying-large-files.html
//
//========================================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

struct stat fileInfo;											// A struct to hold information regarding the input file.

void prepDestFile( char *finFileName, int &finfd, int &foutfd );// This function prepares the destination file for writing. It
																//    creates a destination files (if it does not already exist).
																//    Parameters: The input file name (from the user), the input
																//    file descriptor, and the output file descriptor.

void mapFiles( int finfd, int foutfd );							// This function maps the input and output files into memory. Then
																//    it copies the contents of the input file to the output file.
																//    Parameters: the input and output file descriptors.

int main ( int argc, char *argv[] )								// Main expects 2 arguments from the user: the source (input) filename
{																//    and the destination (output) filename.
	if( argc < 3 ) {
		fprintf(stderr, "\nTo run this program, use the following command:\n"
						"%s <SOURCE FILE> <DESTINATION FILE>\n\n", argv[0]);
		exit( EXIT_FAILURE );
	}

	char *finFileName  = argv[1];								// Create an "easier-to-read" pointer for the input filename.
	char *foutFileName = argv[2];								// Create an "easier-to-read" pointer for the output filename.

	int finfd = open( finFileName, O_RDONLY );					// We need to open the input file here because 'fstat' will need a
																//    valid file descriptor (inside of prepDestFile()).
	if ( finfd < 0 ) {											// Check that we opened the input file successfully.
			perror ( "fin open" );
			exit( EXIT_FAILURE );
	}

	int foutfd  = open( foutFileName, O_RDWR|O_CREAT, 0666 );	// We need to open the output file here because 'fstat' will need a
																//    valid file descriptor (inside of prepDestFile()). If the output
																//    file does not exist, then create one with Read/Write permissions.
	if ( foutfd < 0 ) {											// Check that we opened (or created) the output file successfully.
			perror ( "fout open" );
			exit( EXIT_FAILURE );
	}

	prepDestFile( finFileName, finfd, foutfd );					// Call prepDestFile() passing the input file name, and the input and
																//    output file descriptors as parameters.
	mapFiles( finfd, foutfd );									// Call mapFiles() passing the input and output file descriptors as
																//    parameters.

	if( close ( finfd ) == -1 ) {								// Close the input file.
			perror ( "fin close" );
			exit( EXIT_FAILURE );
	}

	if ( close ( foutfd ) == -1 ) {								// Close the output file.
			perror ( "fout close" );
			exit( EXIT_FAILURE );
	}

	exit( EXIT_SUCCESS );										// If there were no errors, then exit the program successfully.
}

void prepDestFile( char *finFileName, int &finfd, int &foutfd )
{

	if (fstat ( finfd, &fileInfo ) == -1) { 	 				// I am using 'fstat' instead of 'stat'. 'fstat' takes the
			perror ( "fstat" );									//    input file descriptor and the 'fileInfo' struct as
			exit( EXIT_FAILURE );								//    parameters.  Both functions are basically identical except
	}															//    that the file descriptor is used, rather than the file name.
																//    'fstat' populates the 'fileInfo' struct with meaning information
																//    about the specified file. Such as the file size, ownership, etc.

	if ( !S_ISREG ( fileInfo.st_mode ) ) {						// This checks to make sure that we are working with a regular file.
																//    Meaning, that we are not playing with a pipe, directory, etc.
			fprintf ( stderr, "%s is not a file!\n", finFileName );	
			exit( EXIT_FAILURE );
	}

	if( lseek( foutfd, fileInfo.st_size - 1, SEEK_SET ) == -1 ){// I am using 'lseek' instead of 'fseek'. Both are basically
		perror( "lseek" );										//    identical, however 'fseek' works with file pointers and
		exit( EXIT_FAILURE );									//    'lseek' with file descriptors. This sets foutfd file
	}															//    descriptor to the correct offset to match that of the
																//    input file.

	if( write( foutfd, "", 1 ) == -1 ) {						// Make sure that the output file can be written to successfully.
		perror( "write" );
		exit( EXIT_FAILURE );
	}
}

void mapFiles( int finfd, int foutfd )
{
	char *fin = NULL;											// A pointer that will be used for mapping the input file.
	char *fout = NULL;											// A pointer that will be used for mapping the output file.
	long pageSize = sysconf ( _SC_PAGESIZE );					// The default page size.
	int data = fileInfo.st_size;								// The amount of data still to be written.
	off_t offset = 0;											// The offset.

																//----------------------------------------------------------------------
																// Note: this loop is similar to Professor Gofman's, however the
																//    the page sizing logic is different.  This is one of those instances
																//    where there really is only 1 or 2 different ways of accomplishing
																//    this task.  We know we definitely need a loop. We also know that
																//    the page size, the offset, and the mappings are going to be
																//    changing, so this is how it needs to be done.
																//----------------------------------------------------------------------

	while( data > 0 )											// While there is still data to be written...
	{
		if( data < pageSize )									// If the amount of data left is less than the page size...
		{
			pageSize = data;									//  ...Then, set the page size equal to the amount of data left.
			data = 0;											// There is no more data left to write.
		}
		else
		{
			data -= pageSize;									// Else, subtract the page size from the amount of data currently left.
		}

																// Map the page sized input (source) file.
		fin = (char*)mmap ( 0, pageSize, PROT_READ,
						    MAP_SHARED, finfd, offset );

		if ( fin == MAP_FAILED ) {								// Make sure that the mapping was successful.
				perror ( "fin mmap" );
				exit( EXIT_FAILURE );
		}
																// Map the page sized output (destination) file.
		fout = (char*)mmap ( 0, pageSize, PROT_READ|PROT_WRITE,
							 MAP_SHARED, foutfd, offset );

		if ( fout == MAP_FAILED ) {								// Make sure that the mapping was successful.
				perror ( "fout mmap" );
				exit( EXIT_FAILURE );
		}

		memcpy( fout, fin, pageSize );							// Copy the data from the input file (that is currently in
																//    memory) to the output file.


		if ( munmap( (void*)fin, pageSize ) == -1 ) {			// Un-map the input file.
				perror ("fin munmap");
				exit( EXIT_FAILURE );
		}
		if ( munmap( (void*)fout, pageSize ) == -1 ) {			// Un-map the output file.
				perror ( "fout munmap" );
				exit( EXIT_FAILURE );
		}

		offset += pageSize;										// Adjust the offset according to the current page size.
	}
}
