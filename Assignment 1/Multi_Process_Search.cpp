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

#include <sys/wait.h>   // Needed for wait().
#include <stdlib.h>     // Needed for general utilities (atoi, etc.)
#include <signal.h>     // Needed for process events.
#include <unistd.h>     // Needed for fork().
#include <iostream>
#include <vector>
#include <fstream>
#include <string>

// This function opens a file and populates a vector with strings. It takes the input file
// variable, myStrings vector, file name, and string to search for, as parameters.
void getData(std::ifstream &myFile, std::vector<std::string> &myStrings, char *filename, std::string key)
{																
    std::string temp;
    myFile.open(filename);

    if (myFile.is_open())
    {
        while (myFile >> temp)
        {
            myStrings.push_back(temp);
        }
        myFile.close();
    }
    else
    {
        perror("\nFile");
        exit(EXIT_FAILURE);
    }
}

bool compareString(std::string x, std::string y)
{																
    return (x == y) ? true : false;
}

// This function kills all of the remaining children, according to their PID number.
void murderChildren(int *childPIDs, int numChildren)			
{  
    for (int i = 0; i < numChildren; i++)
    {
        kill(childPIDs[i], SIGKILL);
    }
}

int main(int argc, char* argv[])
{
    // There should be 4 parameters for correct execution. (3 entered by the user, and argv[0] which is 
    // simply the path where this program is stored).
    if (argc != 4) 	
    {
        std::cout <<    "\nRun the program with the following command: ./multi-search <FILENAME> <KEY> <NUMBER OF PROCESSES>.\n"
                        "   <KEY> being the string to search for.\n"
                        "   For example: \'./multi-search urls.txt abcdef 10\' will search the file for \'abcdef\' using 10 child\n"
                        "   processes.\n\n";

        exit(EXIT_FAILURE);
    }

    unsigned int numChildren = atoi(argv[3]);   // The number of child processes to be created.
    int exit_status;                            // The exit_status returned by each child.
    int *childPIDs;                             // An array to hold each child' PID.
    std::string key(argv[2]);                   // The string to search for (convert from a char*).
    pid_t pid;                                  // The current PID.
    std::vector<std::string> myStrings;         // A dynamic vector to hold all of the input strings.
    std::ifstream myFile;                       // File variable.

    // Call function to retrieve input data.
    getData(myFile, myStrings, argv[1], key);					

    // Store the size of the vector in a variable to reduce overhead.
    unsigned int const arraySize = myStrings.size();	

    // Create a new array to hold the PID of each child.
    childPIDs = new int[arraySize];     						

    std::cout << std::endl;

    if (numChildren < 1 || numChildren > arraySize)
    {
        std::cout << arraySize << " strings were read-in from the input file." << std::endl;
        std::cout << "The number of child processes must be between 1 and " << arraySize << ".\n" << std::endl;
        exit(EXIT_FAILURE);
    }

    int n = arraySize / numChildren;    // Number of strings the children will check.
    int r = arraySize % numChildren;    // Remainder of strings the parent will check.

    std::cout << "Searching " << argv[1] << " for \'" << key << "\'..." << std::endl;

    // Only the parent will run this code; she checks for any strings that were not evenly distributed
    // amongst the children.
    if (r != 0)													
    {															
        for (int i = 0; i < r; i++)
        {
            // Check if the parent has found the string. "(n*numChildren)" tells the parent where the
            // start index is of the remainder strings (stored at the end of the array). "+i" just means 
            // check the string at the current iteration of the loop.
            if (compareString(myStrings[(n*numChildren) + i], key))	
            {
                std::cout << "The Parent found \'" << key << "\', no need to fork any children!\n" << std::endl;
                exit(EXIT_SUCCESS);
            }
        }
    }

    for (unsigned int i = 0; i < numChildren; i++)
    {
        // Create a new child process while saving the PID.
        pid = fork();											

        // There was an error.
        if (pid == -1)											
        {
            perror("\nFork");
            exit(EXIT_FAILURE);
        }
        // Child Process.
        if (pid == 0)											
        {
            // Loop as many times as there are strings to be checked per child. If the current string is a match,
            // then return "true" to the parent. "(i*n)" is the index where the current child needs to start
            // searching in the myStrings array. "+j" just means check the string at the current iteration of
            // the loop.
            for (int j = 0; j < n; j++)							
            {													
                if (compareString(myStrings[(i*n) + j], key)) 
                    exit(0);  
            }
            // Or else return "false" to the parent.
            exit(1);											
        }
        // Parent process.
        if (pid > 0)												
        {
            // Save the current child PID.
            childPIDs[i] = pid;									
        }
    }
    // Parent process.
    if (pid > 0)													
    {
        for (unsigned int i = 0; i < numChildren; i++)
        {
            // Issue a wait command for each child.  Then, when the child terminates, save the exit
            // code of the child in "exit_status."
            if (wait(&exit_status) == -1)						
            {													
                perror("\nChild");
                exit(EXIT_FAILURE);
            }
            // If the child returns 0, then the string was found. If the child returns 1, then
            // the string was not found.
            if (WEXITSTATUS(exit_status) == 0)					
            {													
                std::cout << "\n\'" << key << "\' was found!\n" << std::endl;
                // Kill any remaining children that may still be running.
                murderChildren(childPIDs, numChildren);			
                exit(EXIT_SUCCESS);
            }
        }

        std::cout << "\nNo string found.\n" << std::endl;
    }
    exit(EXIT_SUCCESS);
}


