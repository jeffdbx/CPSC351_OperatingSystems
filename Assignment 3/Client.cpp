//=========================================================================================================
//
//  Jeff Bohlin
//  December 9th, 2012
//  CS351 - TTR 5:30
//  Professor Gofman
//  Assignment 3 Part I Client
//
//  Implementation of server and client programs.  The client requests records from the server by
//      sending record ids over the message queue.
//
//  The client program has the following structure and function (per out Assignment 3 hand-out):
//
//  1. The client program connects to the message queue previously established by the server.
//  2. If the connection is successful, then the client goes into an infinite loop where it:
//          -Generates a random id between 0 and 10000.
//          -Sends the id to the server via the message queue.
//          -Waits for the server to reply with the requested record.
//          -Displays the record received.
//          -Repeats the process.
//
//  The client shall be invoked as ./client. NOTE: The server and client programs need to be running at
//      the same time.
//
//  Sources used to help complete this assignment:
//
//  1. Professor Gofman's sample .cpp files.
//
//=========================================================================================================


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <iomanip>
#include <iostream>

using namespace std;

// The structure that holds each person's record 
struct record
{
    int id;
    string fname;
    string lname;
};

// The structure representing the message 
struct msgBuff
{
    // IMPORTANT: every message structure must start with this 
    long messageType;

    // The requested record id 
    int requestID;

    // The ID of the returned record 
    int id;

    // Some char buffer 
    char buff[100];
};

int main()
{
    cout << "Wait patiently and let me run for a minute or two and I'll start printing records.";
    // Use a random file and a random character to generate a unique key. Same parameters to 
    // this function will always generate the same value. This is how multiple processes can 
    // connect to the same queue.
    key_t key = ftok("/bin/ls", 'z');

    // Was the key allocation successful ? 
    if (key < 0)
    {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Connect to the message queue; fail if there is no message queue associated with
    // this key. This function returns the id of the queue.
    int msqid = msgget(key, 0666);

    // Was the allocation a success? 
    if (msqid < 0)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    // Instantiate a message buffer 
    msgBuff msg;

    // Seed the random number generator 
    srand(time(NULL));

    // Loop forever 
    for (;;)
    {
        // Set the message type to 2 - this must match the 4th parameter of msgrcv() in the server
        msg.messageType = 2;

        // Set the data fields of the message 
        msg.requestID = rand() % 10000;
        msg.id = -1;

        // Send the message:
        // @param msqid: the id of the message queue
        // @param msg: the message structure where to store the received message
        // @param sizeof(msg) - sizeof(long): size of the message excluding 
        // the required first member (messageType) which is required.
        // 0 - flag values (not useful for this assignment).
        if (msgsnd(msqid, &msg, sizeof(msg) - sizeof(long), 0) < 0)
        {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }

        // Receive the reply from the server 
        if (msgrcv(msqid, &msg, sizeof(msg) - sizeof(long), 2, 0) < 0)
        {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }

        // If the message ID is not -1, then we have a valid record 
        if (msg.id > -1)
        {
            cout << "Match found!" << endl;
            cout << left << setw(6) << msg.id << setw(20) << msg.buff << endl << endl;
        }

        // Take a short nap before sending another request 
        //usleep(5000);
    }

    exit(EXIT_SUCCESS);
}