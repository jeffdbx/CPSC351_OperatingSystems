//==========================================================================================================
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
//==========================================================================================================

#include <sys/types.h>                                                  // Needed for pid_t
#include <sys/wait.h>                                                   // Needed for wait().
#include <unistd.h>                                                     // Needed for fork().
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main()
{
    string myURL;                                                       // Temporary variable to hold the current URL.
    pid_t pid;                                                          // The PID of each process.
    int n = 0;                                                          // The number of URLs read-in (for loop control.

    ifstream myFile;                                                    // A simple file variable.
    myFile.open("urls.txt");

    if(myFile.is_open())
    {
        while(getline(myFile, myURL))                                   // Read one entire URL into myURL.
        {
            n++;
            pid = fork();                                               // Fork another process.
            if(pid < 0)                                                 // If the PID is less than 0, then the fork failed.
            {
                cerr << "Fork failed!" << endl;
                return -1;
            }
            if(pid == 0)                                                 // Code executed by the child process.
            {
                execlp("/usr/bin/wget", "wget", myURL.c_str(), NULL);    // execlp requires char * as an argument, so myURL
            }                                                            //    must be converted.  This command replaces any 
        }                                                                //    executable code that was cloned from the parent
                                                                         //    with the "wget" executable code.
        myFile.close();
    }
    else
    {
        cout << "File not found!" << endl;
    }

    if(pid > 0)                                                          // Code executed by parent process.
    {
        for(int i = 0; i < n; i++)
        {
            wait(NULL);                                                  // Parent will wait for child to terminate.
            cout << "\nChild complete!" << endl;
        }
    }

    return 0;
}
