//========================================================================================================================
//
// Jeff Bohlin
// October 22nd, 2012
// CS351 - TTR 5:30
// Professor Gofman
// Assignment 2
// Hashing Algorithms using Pipes
//
//	This assignment implements a program for computing a hash of a file using MD5,
//	SHA1, SHA224, SHA256, SHA384, and SHA512 hashing algorithms. The program takes the
//	name of the target file as a command line argument, and then does the following:
//
//	1. 	Checks to make sure the file exists.
//	2. 	Creates two pipes.
//	3. 	Forks all child processes immediately in parallel.
//	4. 	The parent transmits the name of the file to each child (over the first pipe).
//	5. 	Each child receives the name of a file and computes the hash of the file using either
//		    the MD5, SHA1, SHA224, SHA256, SHA384, or SHA512 hashing algorithms.
//	6. 	The child transmits the computed hash to the parent (over the second pipe) and
//		    terminates.
//	7.	The parent uses the select() function to wait until a pipe is ready to be read. 
//          Then, once a pipe is ready, the parent reads from that pipe the hash value and then
//			prints it.
//	8. 	The process repeats until all available pipes in the fd_set have been read.
//	9. 	The parent terminates after all hashes have been computed.
//
// Note: This file is best viewed in gedit with "tab width" set to 4 (Edit > Preferences > Editor).
//
// Credits: Professor Gofman's templates were used as the skeleton for this program. Also, the following
// webpages were used as reference to complete this assignment:
//
//	http://linux.die.net/man/2/write
//	http://stackoverflow.com/questions/1856624/having-trouble-with-pipes-and-select
//	http://www.dreamincode.net/forums/topic/170242-blocked-reading-from-a-pipe-in-c/
//
//========================================================================================================================

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

using namespace std;

int parentToChildPipe[2];											// The pipe for parent-to-child communications.
int childToParentPipe[2];											// The pipe for child-to-parent communications.

#define READ_END 0													// The read end of the pipe.
#define WRITE_END 1													// The write end of the pipe.
#define HASH_PROG_ARRAY_SIZE 6										// The maximum size of the array of hash programs.
#define HASH_VALUE_LENGTH 1000										// The maximum length of the hash value.
#define MAX_FILE_NAME_LENGTH 1000									// The maximum length of the file name.

const string hashProgs[] = {"md5sum", "sha1sum", "sha224sum", 		// The array of names of hash programs.
							"sha256sum", "sha384sum", "sha512sum"};

bool file_exists(const char * filename)								// Simple function to check if a file exists.  It
{																	//    takes 1 parameter: the filename read-in from
    if (FILE * file = fopen(filename, "r"))							//    the command line.
    {
        fclose(file);
        return true;
    }
    return false;
}

void computeHash(const string& hashProgName)						// The function called by a child, which takes 1
{																	//     parameter: hashProgName - the name of the
																	//     hash program.
	char hashValue[HASH_VALUE_LENGTH];								// Buffer to store the computed hash value.
	char fileNameRecv[MAX_FILE_NAME_LENGTH];						// Buffer to store The received string.

	memset(hashValue, (char)NULL, HASH_VALUE_LENGTH);				// Clear the hash value buffer.
	memset(fileNameRecv, (char)NULL, MAX_FILE_NAME_LENGTH);			// Clear the file name buffer.

	//sleep(4);														// Used for testing the timeval.
	if(close(parentToChildPipe[WRITE_END]) < 0)						// The child closes his write end of the "Parent
	{																//     to Child" pipe -- he does not need it.
		perror("closing parentToChild[WRITE]");
		exit(-1);
	}
																	// Read the file name sent by the parent.
	if(read(parentToChildPipe[READ_END], fileNameRecv, MAX_FILE_NAME_LENGTH) < 0)
	{
		perror("read");
		exit(-1);
	}

	if(close(parentToChildPipe[READ_END]) < 0)						// The child closes his read end of the "Parent
	{																//     to Child" pipe -- he does not need it anymore.
		perror("closing parentToChild[READ]");
		exit(-1);
	}

	string cmdLine(hashProgName);									// Glue together a command line.
	cmdLine += " ";
	cmdLine += fileNameRecv;

	FILE* progOutput = popen(cmdLine.c_str(), "r");					// Open the pipe to the program (specified in cmdLine)
																	//     using popen(), save the output into hashValue.
	if(!progOutput)													// Make sure that popen succeeded.
	{
		perror("popen");
		exit(-1);
	}
																	// Read the program output into the hash value buffer.
	if(fread(hashValue, sizeof(char), sizeof(char) * HASH_VALUE_LENGTH, progOutput) < 0)
	{
		perror("fread");
		exit(-1);
	}
	if(pclose(progOutput) < 0)										// Close the program output.
	{
		perror("perror");
		exit(-1);
	}

	char temp[1000];												// This is a temporary char array used to append the name 
	strcpy(temp, hashProgName.c_str());								//    of the hash program to the generated hash value.  The 
	strcat (temp, " HASH VALUE: ");									//    reason for this is because we do not know which child 
	strcat (temp, hashValue);										//    is going to finish computing their hash algorithm first.  
	strcpy(hashValue, temp);										//    Therefore, we can not simply print out the names of 
																	//    the hash program in a loop (inside the main function) 
																	//    like before, because they most likely will not match-up 
																	//    with the correct hash value!

																	// Send the parent the name of the hash program along with
																	//    its correspondng computed hash value.
	if(write(childToParentPipe[WRITE_END], hashValue, strlen(hashValue)) < 0)
	{
		perror("write");
		exit(-1);
	}

	if(close(childToParentPipe[WRITE_END]) < 0)						// The child closes his write end of the pipe --
	{																//     he does not need it anymore.
		perror("close");
		exit(-1);
	}
}

int main(int argc, char** argv)
{
	if(argc < 2)													// Check for errors.
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	string fileName(argv[1]);										// Save the name of the file.
	pid_t pid;														// The process id.

	if(file_exists(fileName.c_str()))								// Make sure that the file exists.
	{

		if(pipe(parentToChildPipe) < 0)								// Create the Parent-to-child pipe.
		{
			perror("parent pipe");
			exit(-1);
		}
		if(pipe(childToParentPipe) < 0)								// Create the Child-to-parent pipe.
		{
			perror("child pipe");
			exit(-1);
		}

																	// Immediately fork all children for each hash algorithm.
		for(int hashAlgNum = 0; hashAlgNum < HASH_PROG_ARRAY_SIZE; ++hashAlgNum)
		{
			if((pid = fork()) < 0)									// Fork a child process and save the id.
			{
				perror("fork");
				exit(-1);
			}
			else if(pid == 0)										// I am a child.
			{
				computeHash(hashProgs[hashAlgNum]);					// Compute the hash.
				exit(0);											// The child terminates.
			}
		}

		for(int hashAlgNum = 0; hashAlgNum < HASH_PROG_ARRAY_SIZE; ++hashAlgNum)
		{
			char hashValueRecv[HASH_VALUE_LENGTH];					// Buffer to store the hash value received
																	//    from the child.
			memset(hashValueRecv, (char)NULL, HASH_VALUE_LENGTH);	// Clear the 'hash value received' buffer.

																	// Send the file name to the child.
			if(write(parentToChildPipe[WRITE_END], fileName.c_str(), strlen(fileName.c_str())) < 0)
			{
				perror("write");
				exit(-1);
			}

			fd_set fds;												// The set to hold our pipes.
			struct timeval tv = { 0 };								// A "timeout" that is constructed by this struct "timeval."
			tv.tv_sec = 8; 											// Allow the children a 8 second time limit to respond.

			FD_ZERO(&fds); 											// Zero-out the file descriptor (pipe) set.
			FD_SET(childToParentPipe[READ_END], &fds); 				// Add our pipe to the set.

																	// First parameter to select is the HIGHEST fd in our 
																	//     set + 1. The +1 is very important!
			int retval = select(childToParentPipe[READ_END] + 1, &fds, NULL, NULL, &tv);

			if (retval == -1)										// If select() failed, then return an error.
				perror("select");
			else if (retval == 0)									// If retval == 0, then the child did not respond in time.
				printf("Child timed out.\n");
			else													// Otherwise, the read was successful.
			{
				if (FD_ISSET(childToParentPipe[READ_END], &fds))	// Tests to see the current pipe is part of the set.
				{
																	// If it is, then read from it.
					read(childToParentPipe[READ_END], hashValueRecv, HASH_VALUE_LENGTH);

					if (strlen(hashValueRecv) > 0)					// Make sure the hashValue is valid.
					{
						fprintf(stdout, "%s\n", hashValueRecv);		// Print out the hash value.
						fflush(stdout);								// Flush the output stream.
					}

					FD_CLR(childToParentPipe[READ_END], &fds);		// Remove the pipe that was just read from a set.
				}
			}
		}
																	// The parent is done with its pipe, so close both ends.
		if(close(parentToChildPipe[READ_END]) < 0)					// Close the read end.
		{
			perror("closing parentToChild[READ]");
			exit(-1);
		}
		if(close(parentToChildPipe[WRITE_END]) < 0)					// Close the write end.
		{
			perror("closing parentToChild[WRITE]");
			exit(-1);
		}

		if(wait(NULL) < 0)											// Wait for all children to terminate.
		{
			perror("wait");
			exit(-1);
		}
	}
	else
		printf("File not found!\n");								// If no file was found, print an error.

	return 0;
}





