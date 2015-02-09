 #Serial Downloader

 This program acts as a serial downloader; making use of the fork() and exec() commands.  It  
 reads in one URL at a time from an input file. Once a URL is read-in, a child is forked to 
 execute wget on that URL.  The parent waits until the current child is finished before forking 
 a new child with the next URL.  This continues until the end of the input file.

 #Parallel Downloader

 This program acts as a parallel downloader; making use of the fork() and exec() commands.  It reads 
 in n URLs from a file and then immediately forks off n children which execute wget.  All children 
 run simultaneously while the parent waits for them all to finish.

 #Multi-Process Linear Search Tool

 This program searches an input file for a string specified by the user.  It does this by branching
 off 'n' child processes (again, specified by the user).  These parameters are entered on the command
 line: "./multi-search <FILENAME> <KEY> <NUMBER OF PROCESSES>." For example, "./multi-search animals.txt
 penguin 5" will search an input file for 'penguin' using 5 child processes.  If the string is found,
 then the program outputs a success message.