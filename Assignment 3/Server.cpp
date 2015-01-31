//========================================================================================================================
//
// 	Jeff Bohlin
//	December 9th, 2012
// 	CS351 - TTR 5:30
// 	Professor Gofman
// 	Assignment 3 Part I Server
//
// 	Implementation of server and client programs. The server program maintains
//		a hash table of records and a pool of threads. The server retrieves a request from the message
//		queue and wakes up a thread in the thread pool. The awakened thread then uses the id to
//		retrieve the corresponding record from the hash table and sends the record to the client over
//		the message queue.
//
//	The server has the following structure and function (as described in our Assignment 3 hand-out):
//
//	1. The server is invoked with two command line arguments specifying:
//		The name of the file storing a list of records.
//		The number of threads.
//	2. The server reads the specified file the stores the records in the hash table.
//	3. Creates a message queue using msgget() system call.
//	4. Creates the specified number of threads.
//	5. Each thread joins a thread pool.
//	6. The parent thread then checks the message queue for new messages using msgrcv() system call.
//	7. When a new message arrives the parent thread retrieves the message, adds it to a list of
//		received messages, and wakes up a thread from the thread pool.
//	8. The awakened thread removes a message from the list and checks the id contained in the
//		message. The id indicates that the client who sent the message wants to know the record
//		associated with the respective id.
//	9. The thread then checks if the record with a given id exists in the table and if so retrieves
//		it. The retrieved record is then sent to the client over the message queue. If the record
//		does not exist, then the server sends back a record with id field set to -1.
//	10. The thread then removes the next message from the list and repeats the same process. If
//		the list is empty, then the thread goes back to the thread pool.
//	11. The server also simulates a scenario where the hash table is constantly being updated with
//		new records. You will simulate this by using 5 separate threads that periodically (e.g.
//		every 1 second) randomly generate records and insert them into the hash table.
//	12. When the user presses Ctrl-c, the server catches the SIGINT signal, removes the message
//		queue using msgctl(), deallocates any other resources (e.g. mutexes, condition variables,
//		etc) and exits. To intercept the signal you will need to define a custom signal handler.
//
//	Sources used to help complete this assignment:
//
//  1. Professor Gofman's sample .cpp files.
//  2. Cprogramming.com - Hash Table tutorial:
//       http://www.cprogramming.com/tutorial/computersciencetheory/hash-table.html
//  3. Stackoverflow.com forums - Mutex locking on single array cells:
//       http://stackoverflow.com/questions/11017851/openmp-locking-access-to-single-array-elements
//
//========================================================================================================================


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <signal.h>

using namespace std;

/* The structure representing each person's record. */
struct record
{
	int id;
	string fname;
	string lname;
};

/* The structure representing the message. */
struct msgBuff
{
	long messageType;		/* IMPORTANT: every message structure must start with this! */
	int requestID;			/* The requested record id. */
	int id;					/* The returned ID from the hash table. */
	char buff[100];			/* Buffer to store first and last name. */
};

/* The size of the hash table. */
const int TABLE_SIZE = 100;

/* The class to create a hash table. */
class HashMap {
private:

	/* Variable to hold the total number of records currently in the hash table. */
	int totalRecords;

	/* Each cell in the hash table will have a mutex and a linked list. */
	struct cell
	{
		pthread_mutex_t myMutex;
		list<record> recordList;
	} temp;

	/* The vector to hold each cell. Essentially, this is the actual hash table. */
	vector<cell> hashVector;

public:

	/* Constructor */
	HashMap()
	{
		totalRecords = 0;

		/* Populate each cell in the hash table. */
		for (int i = 0; i < TABLE_SIZE; ++i)
		{
			pthread_mutex_init(&temp.myMutex, NULL);
			hashVector.push_back(temp);
		}
	}

	/* Protect the current hash cell, then insert a record. */
	void insert(record temp, int key)
	{
		pthread_mutex_lock(&hashVector[key].myMutex);
		totalRecords++;
		hashVector[key].recordList.push_back(temp);
		pthread_mutex_unlock(&hashVector[key].myMutex);
	}

	 /* Do not actually delete the record from the table, just get a copy of it. */
	record retrieve(int id)
	{
		int key = id % TABLE_SIZE;

		/* Create an "empty" record, and default the id to -1. */
		record temp;
		temp.id = -1;
		temp.fname = "";
		temp.lname = "";

		pthread_mutex_lock(&hashVector[key].myMutex);
		if(!hashVector[key].recordList.empty())
		{
			list<record>::iterator it;
			for (it = hashVector[key].recordList.begin(); it != hashVector[key].recordList.end(); it++)
			{
				if(it->id == id)
				{
					temp.id = it->id;
					temp.fname = it->fname;
					temp.lname = it->lname;
				}
			}
		}
		/* Unlock before the return, or else the mutex will never get unlocked :) */
		pthread_mutex_unlock(&hashVector[key].myMutex);

		/* If there is a match within the list, it will be returned, if not then this will just
		 * return an empty record. */
		return temp;
	}

	/* This function wasn't required, but I added it for debugging purposes. */
	void printRecords()
	{
		list<record>::iterator it;
		cout << "\n******* All Records *******" << endl;
		for (int i = 0; i < TABLE_SIZE; i++)
		{
			pthread_mutex_lock(&hashVector[i].myMutex);
			if(!hashVector[i].recordList.empty())
			{
				cout << "Key [" << i << "]:" << endl;

				/* Side note: (*it).id is the same as it->id */
				for (it = hashVector[i].recordList.begin(); it != hashVector[i].recordList.end(); it++)
				{
					cout << left << setw(6) << (*it).id << setw(10)
						 << it->fname << setw(10) << it->lname << endl;
				}
				cout << endl;
			}
			pthread_mutex_unlock(&hashVector[i].myMutex);
		}
		cout << "Total records: " << totalRecords << endl << endl;
	}

	int getTotalRecords()
	{
		return totalRecords;
	}

	/* Default destructor. */
	~HashMap()
	{
		/* The hashVector and recordList should both be deallocated automatically. */
	}
};

/* Arrays of 20 possible first and last names to be chosen for a new record. */
const string fnames[20] = {"Vincent", 	"Sue", 		"Frank", 	"Carol", 	"Jeffrey",
		             	   "Christen", 	"Jerry", 	"Tiffany", 	"Daniel", 	"Maren",
		             	   "Eric", 		"Anne", 	"Johnny", 	"Carolyn", 	"Jack",
		             	   "Tristen", 	"Jason", 	"Tatiyana", "David", 	"Marilyn"};

const string lnames[20] = {"Johnson", 	"Soares", 	"Fitch", 	"Collins", 	"Jones",
		             	   "Rodriguez", "Vega", 	"Anderson", "Crenshaw", "Williams",
		             	   "Jacobs", 	"Sanders", 	"Palmer",	"Costanza", "Jeffries",
		             	   "Bohlin", 	"Kochhar", 	"Laguna", 	"Nguyen", 	"Fletcher"};



/* The over-ridden signal handler function */
void signalHandlerFunc( int );

/* "recordThreads" begin control here. */
void *createRecord( void * );

/* "consumerThreads" begin control here. */
void *consume( void * );

/* "producerThreads" begin control here. */
void *produce( void * );

/* The message queue's ID. This is global so that I can deallocate the queue within the signal
 * handler function. */
int msqid;

/* The number of consumer threads to be created (determined by the user). */
int numThreads;

/* The signal handler flag.  Used to tell the threads to stop computing and exit. */
bool sigflag = false;

/* The list to hold all of the pending messages. */
list<int> msgList;

/* The hash table, made global for ease of use. */
HashMap hashTable;

/* The threads to be used. "recordThreads' each generate a random record and insert it into
 * the hash table. 'producerThread' receives requests from the client and alerts the consumer.
 * '*consumerThread is a pointer to allow for the creation of consumer threads later on, which
 * service the client's request. */
pthread_t recordThreads[5], producerThread, *consumerThread;

/* The producer/consumer mutex. */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* The producer/consumer condtion variable. */
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* The condition flag. */
int condition = 0;

int main( int argc, char** argv )
{

	/* There should be 3 parameters for correct execution. (2 entered by the user,
	 * and argv[0] which is simply the path where this program is stored). */
	if (argc != 3)
	{
		cout << "\nRun the program with the following command: "
				"./server <FILENAME> <NUMBER OF THREADS>"
				"\nFor example: \'./server database.dat 10.' will "
				"load the file and create a thread pool of\n"
				"10 threads.\n\n" << endl;

		exit(EXIT_FAILURE);
	}

	/* Notify "signal" that the signal handler function has been over-ridden. */
	signal(SIGINT, signalHandlerFunc);

	/* Seed the random number generator (used for generation of new records). */
	srand( time( NULL ) );

	ifstream infile;
	infile.open(argv[1]);

	/* Make sure the file exists. */
	if(!infile)
	{
		cout << "File not found!" << endl;
		exit(EXIT_FAILURE);
	}

	record temp;
	int hashKey;

	/* Populate the hash table with records that have already been created (from a file). */
	while(!infile.eof())
	{
		infile >> temp.id >> temp.fname >> temp.lname;
		hashKey = temp.id % TABLE_SIZE;
		hashTable.insert(temp, hashKey);
	}

	infile.close();


	/* Use a random file and a random character to generate a unique key. Same parameters
	 * to this function will always generate the same value. This is how multiple processes
	 * can connect to the same queue. */
	key_t key = ftok("/bin/ls", 'z');

	/* 	Was the key allocation successful? */
	if(key < 0)
	{
		perror("ftok");
		exit(EXIT_FAILURE);
	}

	/* Allocate the message queue if it does not already exist. This function returns the
	 * id of the queue. */
	msqid = msgget(key, 0666 | IPC_CREAT);

	/* Was the allocation a success? */
	if(msqid < 0)
	{
		perror("msgget");
		exit(EXIT_FAILURE);
	}

	/*  The number of consumer threads to be created (designated by the user). */
	numThreads = atoi(argv[2]);

	/* Set of thread attributes. */
	pthread_attr_t attr;

	/* Get the default attributes. */
	pthread_attr_init(&attr);

	/* Create the producer thread. */
	if(pthread_create( &producerThread, &attr, &produce, NULL ) < 0)
	{
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	/* Create the consumer threads. */
	consumerThread = new pthread_t[numThreads];
	for(long i = 0; i < numThreads; i++)
	{
		if(pthread_create(&consumerThread[i], &attr, consume, (void *)i ) < 0)
		{
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	/* Create the threads that will insert new random records. */
	for(long i = 0; i < 5; i++)
	{
		if(pthread_create(&recordThreads[i], &attr, createRecord, NULL) < 0)
		{
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	/* IMPORANT! Leave these here because if CTRL-C is pressed, then the threads will come
	 * here to exit! */
	if(pthread_join(producerThread, NULL) < 0)
	{
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}
	for(long i = 0; i < numThreads; i++)
	{
		if(pthread_join(consumerThread[i], NULL) < 0)
		{
			perror("pthread_join");
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);
}

/**
 * The over-ridden signal handler function.
 * @param signum - The signal number generated when CTRL-C is pressed.
 */
void signalHandlerFunc( int signum )
{
	/* The signal was detected, so deallocate the message queue, tell the threads to stop
	 * computation, then exit. */
	sigflag = true;

	/* I have to deallocate the message queue inside this function. For some reason it doesn't
	 * work inside main() even after setting the flag to true. */
	cout << "\nDeallocating message queue. Exiting..." << endl;
	if(msgctl(msqid, IPC_RMID, NULL) < 0)
	{
		perror("msgctl");
	}

	cout << "Total records in the hash table: " << hashTable.getTotalRecords() << endl;

	exit(signum);
}

/**
 * The createRecords thread function.
 * @param arg - pointer to the thread local data - unused.
 */
void *createRecord( void *arg )
{
		record temp;
		int key;

		/* Loop forever and insert a new record every second. */
		for(;;)
		{
			/* Create a random ID number, and generate a random combination of first name
			 * and last name from the arrays declared above. */
			temp.id = rand() % 10000;
			temp.fname = fnames[(rand() % 20)];
			temp.lname = lnames[(rand() % 20)];
			key = temp.id % TABLE_SIZE;

			hashTable.insert(temp, key);

			sleep(1);
		}
}

/**
 * The consumer thread function.
 * @param i - pointer to the thread local data. When a new consumer thread is created inside of
 * main(), the ID number (really it's just the index number from the "consumerThreads" array) is
 * passed to this function.  It helps keep track of which thread is currently working.
 */
void *consume( void *i )
{
	/* The ID of the current thread. */
	long my_id = (long)i;

	/* Consume things forever. */
	for(;;)
	{
		/* If the signal handler was triggered by CTRL-C, then break out of the loop. */
		if(sigflag == true)
			break;

		/* Lock the mutex. */
		if(pthread_mutex_lock( &mutex ) < 0)
		{
			perror("pthread_mutex_lock");
			exit(EXIT_FAILURE);
		}

		/* If there is nothing to consume, then sleep */
		while( condition == 0 )

			/* Sleep on a condition variable until the
			 * producer wakes us up.
			 */
			if(pthread_cond_wait( &cond, &mutex ) < 0)
			{
				perror("pthread_cond_wait");
				exit(EXIT_FAILURE);
			}

		/* The variable to hold our message. */
		msgBuff msg;

		/* VERY IMPORTANT! This number MUST match the 4th parameter in msgrcv. */
		msg.messageType = 2;

		record temp;
		int requestedID;

		while(!msgList.empty())
		{
			/* Grab a requested ID that is currently in the msgList. */
			requestedID = msgList.front();
			msgList.pop_front();

			/* Check if the record exists. If it does, "temp" will become a valid record. If not,
			 * then "temp"'s ID field will be set to -1. */
			temp = hashTable.retrieve(requestedID);

			msg.id = temp.id;
			strcpy(msg.buff, temp.fname.c_str());
			strcat(msg.buff, " ");
			strcat(msg.buff, temp.lname.c_str());

			/* If the message ID returned is not -1, then we have a match. */
			if(msg.id > -1)
				printf("Matched record sent by thread %ld.\n",  my_id );

			/* Reply to the client with the record. */
			if(msgsnd(msqid, &msg, sizeof(msg) - sizeof(long), 0) < 0)
			{
				perror("msgsnd 'consume()'");
				exit(EXIT_FAILURE);
			}
		}

		/* Consume the item */
		condition = 0;

		/* Wake up the sleeping producer, you MUST use broadcast here to wake up all threads!
		 * This is because: what happens if the producer and another consumer are both asleep?
		 * Then there is a chance than when a consumer calls signal() it will wake up the other
		 * consumer and NOT the producer, which will result in a deadlock. */
		if(pthread_cond_broadcast( &cond ) < 0)
		{
			perror("pthread_cond_signal");
			exit(EXIT_FAILURE);
		}

		/* Unlock the mutex. */
		if(pthread_mutex_unlock( &mutex ) < 0)
		{
			perror("pthread_mutex_unlock");
			exit(EXIT_FAILURE);
		}
	}

}

/**
 * The producer thread function.
 * @param arg - pointer to the thread local data - unused.
 */
void *produce( void *arg )
{
	/* Produce things forever */
	for(;;)
	{
		/* If the signal handler was triggered by CTRL-C, then break out of the loop. */
		if(sigflag == true)
				break;

		/* Lock the mutex to protect the condition variable. */
		if(pthread_mutex_lock( &mutex ) < 0)
		{
			perror("pthread_mutex_lock");
			exit(EXIT_FAILURE);
		}

		/* I have produced something, that has not been
		 * yet consumed - sleep until the consumer
		 * wakes us up.
		 */
		while( condition == 1 )
		{
			/* Sleep on a condition variable until the
			 * the producer wakes us up.
			 */
			if(pthread_cond_wait( &cond, &mutex ) < 0)
			{
				perror("pthread_cond_wait");
				exit(EXIT_FAILURE);
			}
		}

		/* Instantiate a message buffer. */
		msgBuff msg;

		/* Receive the message from the client (record request). */
		if(msgrcv(msqid, &msg, sizeof(msg) - sizeof(long), 2, 0) < 0)
		{
			perror("msgrcv 'produce()'");
			exit(EXIT_FAILURE);
		}

		/* Put this message(the requested ID) into a list. */
		msgList.push_back(msg.requestID);

		/* I have produced something. */
		condition = 1;

		/* Wake up the sleeping consumer. */
		if(pthread_cond_signal( &cond ) < 0)
		{
			perror("pthread_cond_signal");
			exit(EXIT_FAILURE);
		}

		/* Release the lock. */
		if(pthread_mutex_unlock( &mutex ) < 0)
		{
			perror("pthread_mutex_unlock");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}
