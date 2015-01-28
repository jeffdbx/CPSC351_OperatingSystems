//==============================================================================================================
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
//	1.  Checks to make sure the file exists.
//	2.  Creates two pipes.
//	3.  Forks all child processes immediately in parallel.
//	4.  The parent transmits the name of the file to each child (over the first pipe).
//	5.  Each child receives the name of a file and computes the hash of the file using either
//      the MD5, SHA1, SHA224, SHA256, SHA384, or SHA512 hashing algorithms.
//	6.  The child transmits the computed hash to the parent (over the second pipe) and
//      terminates.
//	7.  The parent uses the select() function to wait until a pipe is ready to be read. 
//      Then, once a pipe is ready, the parent reads from that pipe the hash value and then
//      prints it.
//	8.  The process repeats until all available pipes in the fd_set have been read.
//	9.  The parent terminates after all hashes have been computed.
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
//==============================================================================================================

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

using namespace std;

int parentToChildPipe[2];           // The pipe for parent-to-child communications.
int childToParentPipe[2];           // The pipe for child-to-parent communications.

#define READ_END 0                  // The read end of the pipe.
#define WRITE_END 1                 // The write end of the pipe.
#define HASH_PROG_ARRAY_SIZE 6      // The maximum size of the array of hash programs.
#define HASH_VALUE_LENGTH 1000      // The maximum length of the hash value.
#define MAX_FILE_NAME_LENGTH 1000   // The maximum length of the file name.

// The array of names of hash programs.
const string hashProgs[] = {"md5sum", "sha1sum", "sha224sum", "sha256sum", "sha384sum", "sha512sum"};

// Simple function to check if a file exists.  It takes 1 parameter: the filename read-in 
// from the command line.
bool file_exists(const char *filename)								
{																	
    if (FILE * file = fopen(filename, "r"))							
    {
        fclose(file);
        return true;
    }
    return false;
}


// The function called by a child, which takes 1 parameter: hashProgName - the name of the
// hash program.
void computeHash(const string &hashProgName)						
{
    // Buffer to store the computed hash value.
	char hashValue[HASH_VALUE_LENGTH];	

    // Buffer to store The received string.
	char fileNameRecv[MAX_FILE_NAME_LENGTH];						

    // Clear the hash value buffer.
	memset(hashValue, (char)NULL, HASH_VALUE_LENGTH);

    // Clear the file name buffer.
	memset(fileNameRecv, (char)NULL, MAX_FILE_NAME_LENGTH);			

    // The child closes his write end of the "Parent to Child" pipe -- he does not need it.
	if(close(parentToChildPipe[WRITE_END]) < 0)						
	{
		perror("closing parentToChild[WRITE]");
		exit(-1);
	}
																	
    // Read the file name sent by the parent.
	if(read(parentToChildPipe[READ_END], fileNameRecv, MAX_FILE_NAME_LENGTH) < 0)
	{
		perror("read");
		exit(-1);
	}

    // The child closes his read end of the "Parent to Child" pipe -- he does not need it anymore.
	if(close(parentToChildPipe[READ_END]) < 0)						
	{ 
		perror("closing parentToChild[READ]");
		exit(-1);
	}

    // Glue together a command line.
	string cmdLine(hashProgName);									
	cmdLine += " ";
	cmdLine += fileNameRecv;

    // Open the pipe to the program (specified in cmdLine) using popen(), save the output into 
    // hashValue. 
	FILE* progOutput = popen(cmdLine.c_str(), "r");					
		
    // Make sure that popen succeeded.     
	if(!progOutput)													
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
    // Close the program output.
	if(pclose(progOutput) < 0)										
	{
		perror("perror");
		exit(-1);
	}

    // This is a temporary char array used to append the name of the hash program to the generated 
    //   hash value. The reason for this is because we do not know which child is going to finish
    //   computing their hash algorithm first.  Therefore, we can not simply print out the names of
    //   the hash program in a loop (inside the main function) like before, because they most likely
    //   will not match-up with the correct hash value.

	char temp[1000];                         
	strcpy(temp, hashProgName.c_str());     
	strcat (temp, " HASH VALUE: ");   
	strcat (temp, hashValue);      
	strcpy(hashValue, temp);   
  
    // Send the parent the name of the hash program along with its correspondng computed hash value.
	if(write(childToParentPipe[WRITE_END], hashValue, strlen(hashValue)) < 0)
	{
		perror("write");
		exit(-1);
	}
    // The child closes his write end of the pipe -- he does not need it anymore.
	if(close(childToParentPipe[WRITE_END]) < 0)						
	{  
		perror("close");
		exit(-1);
	}
}

int main(int argc, char **argv)
{
    // Check for errors.
	if(argc < 2)													
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

    // Save the name of the file.
	string fileName(argv[1]);

    // The process id.
	pid_t pid;														

    // Make sure that the file exists.
	if(file_exists(fileName.c_str()))								
	{
        // Create the Parent-to-child pipe.
		if(pipe(parentToChildPipe) < 0)								
		{
			perror("parent pipe");
			exit(-1);
		}
        // Create the Child-to-parent pipe.
		if(pipe(childToParentPipe) < 0)								
		{
			perror("child pipe");
			exit(-1);
		}

        // Immediately fork all children for each hash algorithm.
		for(int hashAlgNum = 0; hashAlgNum < HASH_PROG_ARRAY_SIZE; ++hashAlgNum)
		{
            // Fork a child process and save the id.
			if((pid = fork()) < 0)									
			{
				perror("fork");
				exit(-1);
			}
            // I am a child.
			else if(pid == 0)										
			{
                // Compute the hash.
				computeHash(hashProgs[hashAlgNum]);	

                // The child terminates.
				exit(0);											
			}
		}

		for(int hashAlgNum = 0; hashAlgNum < HASH_PROG_ARRAY_SIZE; ++hashAlgNum)
		{
            // Buffer to store the hash value received from the child.
			char hashValueRecv[HASH_VALUE_LENGTH];					

            // Clear the 'hash value received' buffer.
			memset(hashValueRecv, (char)NULL, HASH_VALUE_LENGTH);	
					
            // Send the file name to the child.
			if(write(parentToChildPipe[WRITE_END], fileName.c_str(), strlen(fileName.c_str())) < 0)
			{
				perror("write");
				exit(-1);
			}

            // The set to hold our pipes.
			fd_set fds;	

            // A "timeout" that is constructed by this struct "timeval."
			struct timeval tv = { 0 };

            // Allow the children a 8 second time limit to respond.
			tv.tv_sec = 8; 											

            // Zero-out the file descriptor (pipe) set.
			FD_ZERO(&fds); 						

            // Add our pipe to the set.
			FD_SET(childToParentPipe[READ_END], &fds); 				

            // First parameter to select is the HIGHEST fd in our set + 1. The +1 is very important! 
			int retval = select(childToParentPipe[READ_END] + 1, &fds, NULL, NULL, &tv);

            // If select() failed, then return an error.
            if (retval == -1)
            {
                perror("select");
            }
            // If retval == 0, then the child did not respond in time.
            else if (retval == 0)
            {
                printf("Child timed out.\n");
            }
            // Otherwise, the read was successful.
			else													
			{
                // Tests to see the current pipe is part of the set.
				if (FD_ISSET(childToParentPipe[READ_END], &fds))	
				{
                    // If it is, then read from it.
					read(childToParentPipe[READ_END], hashValueRecv, HASH_VALUE_LENGTH);

                    // Make sure the hashValue is valid.
					if (strlen(hashValueRecv) > 0)					
					{
                        // Print out the hash value.
						fprintf(stdout, "%s\n", hashValueRecv);	

                        // Flush the output stream.
						fflush(stdout);								
					}
                    // Remove the pipe that was just read from a set.
					FD_CLR(childToParentPipe[READ_END], &fds);		
				}
			}
		}
        // The parent is done with its pipe, so close both ends. Close the read end.
		if(close(parentToChildPipe[READ_END]) < 0)
		{
			perror("closing parentToChild[READ]");
			exit(-1);
		}
        // Close the write end.
		if(close(parentToChildPipe[WRITE_END]) < 0)					
		{
			perror("closing parentToChild[WRITE]");
			exit(-1);
		}
        // Wait for all children to terminate.
		if(wait(NULL) < 0)											
		{
			perror("wait");
			exit(-1);
		}
	}
    // If no file was found, print an error.
    else
    {
        printf("File not found!\n");
    }

	return 0;
}





