// your PA3 BoundedBuffer.h code here
#ifndef _BOUNDEDBUFFER_H_
#define _BOUNDEDBUFFER_H_

#include <queue>
#include <vector>


#include <stdio.h>
#include <string>
#include <thread>
#include <mutex>
#include <assert.h>
#include <condition_variable>
#include <string.h>


class BoundedBuffer {
private:
    // max number of items in the buffer
	int cap;

    /* The queue of items in the buffer
     * Note that each item a sequence of characters that is best represented by a vector<char> for 2 reasons:
	 *  1. An STL std::string cannot keep binary/non-printables
	 *  2. The other alternative is keeping a char* for the sequence and an integer length (b/c the items can be of variable length)
	 * While the second would work, it is clearly more tedious
     */
	std::queue<std::vector<char>> q;

	// add necessary synchronization variables and data structures 
	std::mutex m;
	std::condition_variable data_push;  
	std::condition_variable slot_pop;

public:
	BoundedBuffer (int _cap);
	~BoundedBuffer ();

	void push (char* msg, int size);
	int pop (char* msg, int size);

	size_t size ();
};

#endif

