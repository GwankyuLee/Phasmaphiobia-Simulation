## This is a simulation inspired from Phasmaphobia video game, developed in C language.
This simulation aims to simulate a situation where :
there is a house with set rooms, 
randomly generated ghost in any of the rooms of the simulation,
inputted hunters,
and determine the outcome of whether the hunters are going to win or ghosts are going to win.

User can iunteract with it through a terminal and use the provided makefile to make the compiling easy for users.

Also, the rooms in the house is adjustable by the user but not through terminal.


## game algorithm:

user enters :
- hunter id 
- name

this creates multiple processes containing the exact same hunters entered by user.

each process runs a simulation consisting of hunter threads and ghost threads,
hunters move onto random adjacent rooms.
if they find evidence, they will return to the starting room using Breadth-First-Search algorithm to ensure fastest travel.
when hunters find three evidence matching a specific ghost.
they will return to Van and exit the simulation.

ghost will spawn randomly in the house, and also travel adjacent rooms. it performs 3 random actions and somtimes drops evidence into the room they are currently in.
ghost exits the house when they are bored.
the game ends when all hunters have found the evidence of the ghost. or if they all leave the house.

used pipes to communicate betwen parent-child processes for unidirectional communication

users can adjust the number of processes/simulations running concurrently by heading to `main.c` and changing `NUM_SIMULATIONS` values.


### main concepts used in this project:
- ipc(pipes)
- multi-threading
- multi-processing
- BFS

## instructions:

clone this repository or download the folder containing all the files.
in the terminal, in the folder path that contains all the files, type `make`
This will produce all the compiled files and linked object file(final executable) named simulation.
run the simulation by doing `./simulation` in current directory.

This program is also runnable through valgrind or helgrind to check for memory leaks and threading data races.
run `make valgrind` or `make helgrind` to do so.

and finally clean the produced files by `make clean`.


Disclaimer : 
average time value Might not work properly in windows. Designed for Linux/MacOS