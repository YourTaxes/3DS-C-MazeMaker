# 3DS-C-pong-remake
This is a remake of the mazemaker game engine I made for my high school programming class, 
which I revisited recently and found was completely broken (Update: I fixed it), so I am remaking it for the 3ds.
because why not.

Cool Features:
* Fully featured state swapping system. Modeled after the Unity Scene system, all states defined with 4 funcitons for each step of the game loop can be seemlessly swapped between just by returning an enum to the stat to swap to (or NONE to not swap). All of the states use a framework where they use a dynamically allocated struct to hold global variables specific to the current state, which are allocated and freed automatically on the state being loaded in, with me only needing to define what the values are.