#include "SharedMemory.hpp"
#include "SharedVideoFrame.hpp"

#include <assert.h>
#include <iostream>

int main(int argc, const char* argv[]) {
	sjq::SharedMemory shm1;
	assert(shm1.Create("hello", 128));
	memcpy(shm1.GetPtr(), "AAAAA", 5);

	sjq::SharedMemory shm2;
	assert(shm2.Open("hello"));

	std::cout << (const char*)(shm2.GetPtr()) << std::endl;

	return 0;
}