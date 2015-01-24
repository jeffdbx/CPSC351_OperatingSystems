//==========================================================================================================
//
// Jeff Bohlin
// September 19th, 2012
// CS351 - TTR 5:30
// Professor Gofman
// Assignment 1 Bonus
// Multi-Process Linear Search Tool
//
// This program searches an input file for a string specified by the user.  It does this by branching
// off 'n' child processes (again, specified by the user).  These parameters are entered on the command
// line: "./multi-search <FILENAME> <KEY> <NUMBER OF PROCESSES>." For example, "./multi-search animals.txt
// penguin 5" will search an input file for 'penguin' using 5 child processes.  If the string is found,
// then the program outputs a success message.
//
//==========================================================================================================

#include <sys/wait.h>                                           // Needed for wait().
#include <stdlib.h>                                             // Needed for general utilities (atoi, etc.)
#include <signal.h>                                             // Needed for process events.
#include <unistd.h>                                             // Needed for fork().
#include <iostream>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

void getData(ifstream &myFile, vector<string> &myStrings,       // This function opens a file and populates a vector with strings. It
             char *filename, string key)                        //    takes the input file variable, myStrings vector, file name, and
{                                                               //    string to search for, as parameters.
    string temp;
    myFile.open(filename);

    if(myFile.is_open())
    {
       while(myFile >> temp)
       {
          myStrings.push_back(temp);                            // Put the string into the vector.
       }
       myFile.close();
    }
    else
    {
       perror("\nFile");
       exit(EXIT_FAILURE);
    }

}

bool compareString(string x, string y)                          // This function simply compares 2 strings to see if they
{                                                               //    are a match and then returns true or false.
    return (x == y) ? true : false;
}

void murderChildren(int *childPIDs, int numChildren)            // This function kills all of the remaining children,
{                                                               //    according to their PID number.
    for(int i = 0; i < numChildren; i++)
    {
        kill(childPIDs[i], SIGKILL);
    }
}

int main(int argc, char* argv[] )
{
    if (argc != 4)                                              // There should be 4 parameters for correct execution.
    {                                                           //    (3 entered by the user, and argv[0] which is simply
                                                                //    the path where this program is stored).
       cout << "\nRun the program with the following command: ./multi-search <FILENAME> <KEY> <NUMBER OF PROCESSES>.\n"
              "   <KEY> being the string to search for.\n"
              "   For example: \'./multi-search urls.txt abcdef 10\' will search the file for \'abcdef\' using 10 child\n"
              "   processes.\n\n";

       exit(EXIT_FAILURE);
    }

    unsigned int numChildren = atoi(argv[3]);                   // The number of child processes to be created.
    int exit_status;                                            // The exit_status returned by each child.
    int *childPIDs;                                             // An array to hold each child' PID.
    string key(argv[2]);                                        // The string to search for (convert from a char*).
    pid_t pid;                                                  // The current PID.
    vector<string> myStrings;                                   // A dynamic vector to hold all of the input strings.
    ifstream myFile;                                            // File variable.

    getData(myFile, myStrings, argv[1], key);                   // Call function to retrieve input data.

    unsigned int const arraySize = myStrings.size();            // Store the size of the vector in a variable to reduce overhead.
    childPIDs = new int[arraySize];                             // Create a new array to hold the PID of each child.

    cout << endl;

    if(numChildren < 1 || numChildren > arraySize)              // Check for valid input.
    {
        cout << arraySize << " strings were read-in from the input file." << endl;
        cout << "The number of child processes must be between 1 and " << arraySize << ".\n" << endl;
        exit(EXIT_FAILURE);
    }

    int n = arraySize / numChildren;                            // Number of strings the children will check.
    int r = arraySize % numChildren;                            // Remainder of strings the parent will check.

    cout << "Searching " << argv[1] << " for \'" << key << "\'..." << endl;

    if(r != 0)                                                  // Only the parent will run this code; she checks for
    {                                                           //    any strings that were not evenly distributed
                                                                //    amongst the children.
       for(int i = 0; i < r; i++)
       {
           if(compareString(myStrings[(n*numChildren)+i], key)) // Check if the parent has found the string. "(n*numChildren)" tells
           {                                                    //    the parent where the start index is of the remainder strings
                                                                //    (stored at the end of the array). "+i" just means check the
                                                                //    the string at the current iteration of the loop.

               cout << "The Parent found \'" << key << "\', no need to fork any children!\n" << endl;
               exit(EXIT_SUCCESS);

           }
       }
    }

    for(unsigned int i = 0; i < numChildren; i++)
    {
       pid = fork();                                            // Create a new child process while saving the PID.

       if (pid == -1)                                           // There was an error.
       {
           perror("\nFork");
           exit(EXIT_FAILURE);
       }

       if (pid == 0)                                            // Child Process.
       {
            for(int j = 0; j < n; j++)                          // Loop as many times as there are strings to be checked
            {                                                   //    per child.
                if(compareString(myStrings[(i*n)+j], key))      // If the current string is a match, then return "true" to
                    exit(0);                                    //    the parent. "(i*n)" is the index where the current child
                                                                //    needs to start searching in the myStrings array. "+j"
                                                                //    just means check the string at the current iteration of
                                                                //    the loop.
            }
            exit(1);                                            // Or else return "false" to the parent.
       }

       if (pid > 0)                                             // Parent process.
       {
           childPIDs[i] = pid;                                  // Save the current child PID.
       }
   }

   if(pid > 0)                                                  // Parent process.
   {
       for(unsigned int i = 0; i < numChildren; i++)
       {
           if (wait(&exit_status) == -1)                        // Issue a wait command for each child.  Then, when the child
           {                                                    //    terminates, save the exit code of the child in "exit_status."
               perror("\nChild");
               exit(EXIT_FAILURE);
           }
           if(WEXITSTATUS(exit_status) == 0)                    // If the child returns 0, then the string was found. If the child
           {                                                    //    returns 1, then the string was not found.
              cout << "\n\'" << key << "\' was found!\n" << endl;
              murderChildren(childPIDs, numChildren);           // Kill any remaining children that may still be running.
              exit(EXIT_SUCCESS);
           }
       }
       cout << "\nNo string found.\n" << endl;
   }
   exit(EXIT_SUCCESS);
}


