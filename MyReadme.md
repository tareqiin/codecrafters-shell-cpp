using name space std cost me alot so I got rid of it.

the tokenize function breaks an input command string into separate tokens while handling single-qouted strings
tokenize("ls  -l   '/home/user dir'");
        -> ["ls", "-l", "/home/user dir"]

handle commands function handles commands as its name suggests 

I used stringstream for parsing, {it's Heavier, slower for concatenation} 


in the external command
execvp() requires arguments as an array of C-style strings (char*[]), not C++ std::string

fork() duplicates the current process: 
                the parent process continues running 
                the child process is a clone that can be used to run the command
if pid == 0 then we r in the child process
if pid > 0 then we r in the parent process
otherwise fork failed

parent shell waits for the child process to finsih using waitpid
perror -> error handling 

if execvp succeeds, the current child process becomes the command other wise it prints not found message
-> the parent shell waits for the child process to finish using waitpid
-> this prevents creating ZOMBIE processes AND ensures commands finish before showing the nest shell prompt
