/* test.cpp - mtlogger usage example
 * This file is a part of mtlogger library
 * Copyright (c) mtlogger authors (see file `COPYRIGHT` for the license)
 */

#include <iostream>
#include <boost/array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/bind.hpp>

#include "logger/log.hpp"

static const int  NUM_IT = 1024;
void LogThreadFunc(boost::barrier * barrier, int thread_id)
{
	barrier->wait();
	for(int i = 0; i < NUM_IT; ++i)
	{
		//std::cout<<"thread "<<thread_id<<" it "<<i<<"\n";
		LOG("thread %d it %d", thread_id, i);
	}
	LOGFLUSH();
}


int main(int argc, char** argv)
{
	LOGINIT(argc>=2?argv[1]:"", 0);

	static const int NUM_THREADS = 64;
	typedef boost::array<boost::thread, NUM_THREADS> Threads;
	int i = 0;
	boost::barrier barrier(NUM_THREADS);
	Threads threads;
	for(Threads::iterator it = threads.begin(); it != threads.end(); ++it)
	{
		*it = boost::thread(boost::bind(&LogThreadFunc, &barrier, ++i));
	}
	std::for_each(threads.begin(), threads.end(), boost::bind(&boost::thread::join, _1));
	LOGLEVEL(logger::SeverityVerbose);
	LOGREOPEN();
	LOGNOTICE("Exit...");
	LOGFLUSH();
	return 0;
}

