//
// Jeff Bohlin
// September 19th, 2012
// CS351 - TTR 5:30
// Professor Gofman
// Assignment 1B
// Parallel Downloader
//
// This program acts as a parallel downloader; making use of the fork() and exec() commands.  It reads 
// in n URLs from a file and then immediately forks off n children which execute wget.  All children 
// run simultaneously while the parent waits for them all to finish.
//

#include <sys/types.h>  // Needed for pid_t
#include <sys/wait.h>   // Needed for wait().
#include <unistd.h>     // Needed for fork().
#include <iostream>
#include <fstream>
#include <string>

int main()
{
    std::string myURL;      // Temporary variable to hold the current URL.
    pid_t pid;              // The PID of each process.
    int n = 0;              // The number of URLs read-in (for loop control.

    std::ifstream myFile;   // A simple file variable.
    myFile.open("urls.txt");

    if (myFile.is_open())
    {
        // Read one entire URL into myURL.
        while (getline(myFile, myURL))									
        {
            n++;

            // Fork another process.
            pid = fork();  

            // If the PID is less than 0, then the fork failed.
            if (pid < 0) 												
            {
                std::cerr << "Fork failed!" << std::endl;
                return -1;
            }
            // Code executed by the child process.
            if (pid == 0) 												
            {
                // execlp requires char * as an argument, so myURL must be converted.  This command replaces 
                // any executable code that was cloned from the parent with the "wget" executable code.
                execlp("/usr/bin/wget", "wget", myURL.c_str(), NULL);	
            }
        }																   
        myFile.close();
    }
    else
    {
        std::cout << "File not found!" << std::endl;
    }
    // Code executed by parent process.
    if (pid > 0)														
    {
        for (int i = 0; i < n; i++)
        {
            // Parent will wait for child to terminate.
            wait(NULL);													
            std::cout << "\nChild complete!" << std::endl;
        }
    }
    return 0;
}
