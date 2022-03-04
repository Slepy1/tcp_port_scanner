#pragma once

#include <mutex>
#include <condition_variable>

using std::mutex;
using std::unique_lock;
using std::condition_variable;

class Semaphore {//used to controll max number of connections

private:
	int startcounter;
	int counter; //keeps track how many more connections can be made
	mutex semaphore_mutex;//locks counter
	condition_variable at_max; //when at max connections (counter = 0) wait until a connection finishes

public:
	Semaphore(int maxconnections = 0)//constructor
	{
		counter = maxconnections;
		startcounter = maxconnections;
	}

	void lowersemaphore()//no real used in my case, but if someone decide to run this on a potato .......
	{
		if ((counter - 10) < (startcounter - 90)) {//limit min semaphore at 10

		}
		else {
			counter -= 10;
		}
	}

	void increasesemaphore()
	{
		if ((counter + 10) > (startcounter + 400)) { //limit max semaphore at 900

		}
		else {
			counter += 10;
		}
	}

	void resetsemaphore()//only for testing, delete me
	{
		counter = 500;
	}

	void signal() { //increase counter eg, connection is freed
		unique_lock<mutex> lock(semaphore_mutex);
		counter++; //critical section, need to be locked
		at_max.notify_one();//connection freed
	}
	void wait() { //decrease counter eg, connection is created
		unique_lock<mutex> lock(semaphore_mutex);
		while (counter == 0)//at connections limit, wait until 1 is freed
		{
			at_max.wait(lock);
		}
		counter--; //critical section, need to be locked
	}
};