BASYNC (Bad ASYNC) is a single header C++ async/await system based on
the ideas of Simon Tathams coroutine hack for Putty. (same "minor" weakness of not allowing BAWAIT inside a switch)

Thanks to C++11 and lambdas this system is not tied to any static variables but can be freely instantiated.
(although it does produce some memory allocations if that is an issue for users).

Usage-wise it's patterned after the EcmaScript async/await based on promises.

Licenced under a Zlib licence so acknowledgement is nice but not requierd.

See test.cpp for a comperhensive example (compiling the code and running will help visualize that is happening)

