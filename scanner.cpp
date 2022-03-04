//TCP port scanner
//by Wojciech Chojnowski 26/04/2021
//this project make use of SFML library https://www.sfml-dev.org/
//for cmp202 project

#include <SFML/Network.hpp>
#include "Semaphore.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <windows.h>
#include <stdio.h>

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::thread;
using std::mutex;

int currentport; //starting value and a counter for current value
int lastport; //when cin add +1 eg when scanning for 1 - 443 this should be 444
bool done = false;//used to signal maxthreads function when to stop

mutex currentport_mutex;
mutex result_mutex;
Semaphore semaphore(500); //max concurrent threads (max 1000 for me) (this will be increased to 900 if your cpu will allow it look controllmaxthreads();)
thread myThread[65536];// huh according to profiler this does not affect memory used, either 1 or 65535 it takes same amound of space.....


void portopen(int portrange, string ip)//attempt to connect to TCP socket, if server responds then its open, otherwise its closed
{
	//create a TCP socket https://www.sfml-dev.org/tutorials/2.5/network-socket.php

	currentport_mutex.lock();
	int localcurrentport = currentport;
	currentport += portrange;
	currentport_mutex.unlock();

	for (int i = 1; ((i < (portrange + 1)) and (localcurrentport < lastport)); i++) { // change from thread generator to a port scanner :)
		localcurrentport++;
		sf::TcpSocket socket;
		bool open = (socket.connect(sf::IpAddress(ip), localcurrentport, sf::seconds(10)) == sf::Socket::Done);// if port does not respond within a timeout limit then its closed (possible 3rd arguemnt to define timeout)

		socket.disconnect();//close socket, sfml tutorial does not show to do that but ill do it for good practice.
		if (open == 1) {//if port is open
			result_mutex.lock();
			cout << "Port " << localcurrentport << " Is " << "open" << endl;
			result_mutex.unlock();
		}
	}
		semaphore.signal(); // free semaphore
}

//this part was made by sayyed mohsen zahraee
//https://stackoverflow.com/a/13947253
//this 2 functions are used to return % of processor used
//----------------------------------------------------------------------------------------------------------------
CHAR cpuusage(void);
//-----------------------------------------------------
typedef BOOL(__stdcall* pfnGetSystemTimes)(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);
static pfnGetSystemTimes s_pfnGetSystemTimes = NULL;

static HMODULE s_hKernel = NULL;
//-----------------------------------------------------
void GetSystemTimesAddress()
{
	if (s_hKernel == NULL)
	{
		s_hKernel = LoadLibrary("Kernel32.dll");
		if (s_hKernel != NULL)
		{
			s_pfnGetSystemTimes = (pfnGetSystemTimes)GetProcAddress(s_hKernel, "GetSystemTimes");
			if (s_pfnGetSystemTimes == NULL)
			{
				FreeLibrary(s_hKernel); s_hKernel = NULL;
			}
		}
	}
}
//----------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------
// cpuusage(void)
// ==============
// Return a CHAR value in the range 0 - 100 representing actual CPU usage in percent.
//----------------------------------------------------------------------------------------------------------------
CHAR cpuusage()
{
	FILETIME               ft_sys_idle;
	FILETIME               ft_sys_kernel;
	FILETIME               ft_sys_user;

	ULARGE_INTEGER         ul_sys_idle;
	ULARGE_INTEGER         ul_sys_kernel;
	ULARGE_INTEGER         ul_sys_user;

	static ULARGE_INTEGER    ul_sys_idle_old;
	static ULARGE_INTEGER  ul_sys_kernel_old;
	static ULARGE_INTEGER  ul_sys_user_old;

	CHAR  usage = 0;

	// we cannot directly use GetSystemTimes on C language
	/* add this line :: pfnGetSystemTimes */
	s_pfnGetSystemTimes(&ft_sys_idle,    /* System idle time */
		&ft_sys_kernel,  /* system kernel time */
		&ft_sys_user);   /* System user time */

	CopyMemory(&ul_sys_idle, &ft_sys_idle, sizeof(FILETIME)); // Could been optimized away...
	CopyMemory(&ul_sys_kernel, &ft_sys_kernel, sizeof(FILETIME)); // Could been optimized away...
	CopyMemory(&ul_sys_user, &ft_sys_user, sizeof(FILETIME)); // Could been optimized away...

	usage =
		(
		(
			(
			(
				(ul_sys_kernel.QuadPart - ul_sys_kernel_old.QuadPart) +
				(ul_sys_user.QuadPart - ul_sys_user_old.QuadPart)
				)
				-
				(ul_sys_idle.QuadPart - ul_sys_idle_old.QuadPart)
				)
			*
			(100)
			)
			/
			(
			(ul_sys_kernel.QuadPart - ul_sys_kernel_old.QuadPart) +
				(ul_sys_user.QuadPart - ul_sys_user_old.QuadPart)
				)
			);

	ul_sys_idle_old.QuadPart = ul_sys_idle.QuadPart;
	ul_sys_user_old.QuadPart = ul_sys_user.QuadPart;
	ul_sys_kernel_old.QuadPart = ul_sys_kernel.QuadPart;

	return usage;
}
//----------------------------------------------------------------------------------------------------------------
//thanks to user sayyed mohsen zahraee for that code
//https://stackoverflow.com/a/13947253

void controllmaxthreads()//this will adjust number of max threads (semaphore(x)) based on cpu usage
{
	GetSystemTimesAddress();
	int cpuu;
	while (done == false)
	{
		cpuu = cpuusage();

		if (cpuu < 85) { //vlow cpu usage, create more threads
			semaphore.increasesemaphore();

		}
		else if (cpuu > 90) {//too much threads
			semaphore.lowersemaphore();
		}
		else {//wait, but how ?
			cout << "error at cpu controll function";
		}
		Sleep(500);
	}
}

int main()
{
	string ip; //"172.18.8.14"
	int portrange;//number of ports to scan per thread, use 8 for tests

	cout << "TCP port scanner" << endl;
	cout << "enter targets ip adress" << endl;
	cin >> ip;//we are all adults, please dont enter wacky things like sqr(-5.344).....
	cout << "enter starting port to scan" << endl;
	cin >> currentport;
	cout << "enter last port to scan" << endl;
	cin >> lastport;
	cout << "enter ports per thread (pick a number between 3 and 8)" << endl;
	cin >> portrange;
	lastport++;
	currentport--;

		 myThread[0] = thread(controllmaxthreads);
		 for (int i = currentport; i < lastport; i++) {
			 myThread[i] = thread(portopen, portrange, ip);
			 semaphore.wait(); //semaphore keeps track of max concurrent connections
			 myThread[i].detach(); //huh, this detaches thread making it independent, key is that this terminates thread after its finished (cannot be joined)).
		 }
		 done = true; //signal maxthreads function to stop
		 myThread[0].join();

		 Sleep((portrange * 2000) + 1500); //allow detached ports to finish, 2 seconds per port plus 1.5 seconds
		 cout << "scan finished" << endl;
		 cin >> ip;//just halt main form terminating
	return 0;
}