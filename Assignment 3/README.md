Client

 Implementation of server and client programs. The client requests records from the server by
 sending record ids over the message queue.

 The client program has the following structure and function (per out Assignment 3 hand-out):

 1. The client program connects to the message queue previously established by the server.
 2. If the connection is successful, then the client goes into an infinite loop where it:
  1. Generates a random id between 0 and 10000.
  2. Sends the id to the server via the message queue.
  3. Waits for the server to reply with the requested record.
  4. Displays the record received.
  5. Repeats the process.

Server

 Implementation of server and client programs. The server program maintains
 a hash table of records and a pool of threads. The server retrieves a request from the message
 queue and wakes up a thread in the thread pool. The awakened thread then uses the id to
 retrieve the corresponding record from the hash table and sends the record to the client over
 the message queue.

 The server has the following structure and function (as described in our Assignment 3 hand-out):

 1. The server is invoked with two command line arguments specifying:
 The name of the file storing a list of records.
 The number of threads.
 2. The server reads the specified file the stores the records in the hash table.
 3. Creates a message queue using msgget() system call.
 4. Creates the specified number of threads.
 5. Each thread joins a thread pool.
 6. The parent thread then checks the message queue for new messages using msgrcv() system call.
 7. When a new message arrives the parent thread retrieves the message, adds it to a list of
 received messages, and wakes up a thread from the thread pool.
 8. The awakened thread removes a message from the list and checks the id contained in the
 message. The id indicates that the client who sent the message wants to know the record
 associated with the respective id.
 9. The thread then checks if the record with a given id exists in the table and if so retrieves
 it. The retrieved record is then sent to the client over the message queue. If the record
 does not exist, then the server sends back a record with id field set to -1.
 10. The thread then removes the next message from the list and repeats the same process. If
 the list is empty, then the thread goes back to the thread pool.
 11. The server also simulates a scenario where the hash table is constantly being updated with
 new records. You will simulate this by using 5 separate threads that periodically (e.g.
 every 1 second) randomly generate records and insert them into the hash table.
 12. When the user presses Ctrl-c, the server catches the SIGINT signal, removes the message
 queue using msgctl(), deallocates any other resources (e.g. mutexes, condition variables,
 etc) and exits. To intercept the signal you will need to define a custom signal handler.

Mmap

 This program completes the following tasks:

 1. The program is invoked with the source file name and the destination file name as parameters.
 2. The program uses the mmap() system call to map both files into memory.
 3. The program uses the memory-mapped file memory to to copy the source file to the destination file.
 
Bonus

 This program completes the following tasks:

 1. The program is invoked with the input file name and number of threads to be created as parameters.
 2. The program then reads in integers from a file and stores them in an array.
 3. The program then splits up this array, evenly, according to the number of threads created.
 4. Then each thread performs a quick sort on his partition in parallel.
 5. Finally, after all threads are done, a merge sort is performed to put all of the partitions
 in the correct order.
