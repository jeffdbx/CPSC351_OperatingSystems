//
// Jeff Bohlin
// September 19th, 2012
// CS351 - TTR 5:30
// Professor Gofman
// Assignment 1A
// Serial Downloader
//
// This program acts as a serial downloader; making use of the fork() and exec() commands.  It  
// reads in one URL at a time from an input file. Once a URL is read-in, a child is forked to 
// execute wget on that URL.  The parent waits until the current child is finished before forking 
// a new child with the next URL.  This continues until the end of the input file.
//

#include <sys/types.h>  // Needed for pid_t datatype.
#include <sys/wait.h>   // Needed for wait().
#include <unistd.h>     // Needed for fork().
#include <iostream>
#include <fstream>
#include <string>

int main()
{
    std::string myURL;      // Temporary variable to hold the current URL.
    pid_t pid;              // The PID of each process.

    std::ifstream myFile;    // A simple file variable.
    myFile.open("urls.txt");

    if (myFile.is_open())
    {
        // Read one entire URL into myURL.
        while (getline(myFile, myURL))
        {
            // Fork another process.
            pid = fork();

            // If the PID is less than 0, then the fork failed.
            if (pid < 0)
            {
                std::cerr << "Fork failed!" << std::endl;
                return -1;
            }
            // Code executed by the child process.
            else if (pid == 0)
            {
                // execlp requires char * as an argument, so myURL must be converted.  This command replaces 
                // any executable code that was cloned from the parent with the "wget" executable code.
                execlp("/usr/bin/wget", "wget", myURL.c_str(), NULL);
            }
            // Code executed by parent process.
            else
            {
                // Parent will wait for child to terminate.
                wait(NULL);
                std::cout << "\nChild complete!" << std::endl;
            }
        }
        myFile.close();
    }
    else
    {
        std::cout << "File not found!" << std::endl;
    }

    return 0;
}
