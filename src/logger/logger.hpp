/* logger.hpp - multithreaded logger implementation
 * This file is a part of mtlogger library
 * Copyright (c) mtlogger authors (see file `COPYRIGHT` for the license)
 */

#ifndef LOGGER_LOGGER_H_
#define LOGGER_LOGGER_H_

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/tss.hpp>
#include <boost/bind.hpp>

namespace logger
{
namespace
{
class OStreamEmptyDeleter
{
public:

		void operator()(std::ostream * p) { }
};
}
typedef boost::scoped_ptr<class Logger> LoggerSPtr;
class Logger
	: boost::noncopyable
{
public:

	typedef boost::array<char, 4096> LogBuffer;
	typedef boost::thread_specific_ptr<LogBuffer> LogBufferTSPtr;
	typedef boost::thread_specific_ptr<std::string> LogThreadIDTSPtr;
	typedef boost::thread_specific_ptr<std::stringstream> LogStreamTSPtr;
	typedef std::list<std::string> StringList;

	static Logger* GetInstance()
	{
		static LoggerSPtr the_instance(new Logger());
		return the_instance.get();
	}

	LogBufferTSPtr & GetBuffer() { return log_buffer_; }
	LogStreamTSPtr & GetStream() { return log_stream_; }
	LogThreadIDTSPtr & GetThreadID() { return log_thread_id_; }

	bool IsInitialized() const { return initialized_; }
	int GetLogLevel() const { return log_level_; }
	void SetLogLevel(int value) { log_level_ = value; }

	// must be called before any thread created
	void Initialize(const std::string & filename, int log_level)
	{
		if(!initialized_)
		{
			log_level_ = log_level_;
			filename_ = filename;
			initialized_ = true;
			OpenFile(filename_);
			logger_thread_ = boost::thread(boost::bind(&Logger::Run, this));
		}
	}

	void AddMessage(const LogBuffer & buffer, size_t size)
	{
		if(initialized_ && (size <= buffer.size()) && (size > 0))
		{
			{
				boost::unique_lock<boost::mutex> lock(sync_);
				string_list_.push_back(std::string(&buffer[0], &buffer[0]+size));
				++messages_added_;
			}
			WakeUp();
		}
	}

	void AddMessage(const std::stringstream & buffer)
	{
		if(initialized_)
		{
			{
				boost::unique_lock<boost::mutex> lock(sync_);
				string_list_.push_back(buffer.str());
				++messages_added_;
			}
			WakeUp();
		}
	}

	void Flush()
	{
		size_t currently_added = 0;
		bool need_continue = true;
		{
			boost::unique_lock<boost::mutex> lock(sync_);
			currently_added = messages_added_;
			need_continue = messages_added_ > messages_processed_;
			if(need_continue)
				++pending_flushes_;
		}
		while(need_continue)
		{
			boost::unique_lock<boost::mutex> lock(sync_);
			flush_condition_.timed_wait(lock, boost::get_system_time() + boost::posix_time::milliseconds(10));
			// stop on counter overlap
			need_continue = currently_added > messages_processed_ &&
				currently_added <= messages_processed_;
			if(!need_continue)
				--pending_flushes_;
		}
	}

	void Reopen()
	{
		need_reopen_ = true;
	}

	~Logger()
	{
		initialized_ = false;
		bool need_continue = true;
		while(need_continue)
		{
			{
				boost::unique_lock<boost::mutex> lock(sync_);
				need_continue = !string_list_.empty();
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(10));
		}
		shutdown_ = true;
		WakeUp();
		logger_thread_.join();
	}

private:

	Logger()
		: initialized_(false)
		, shutdown_(false)
		, log_level_(0)
		, need_wait_(false)
		, need_reopen_(false)
		, messages_added_(0)
		, messages_processed_(0)
		, pending_flushes_(0)
	{
	}

	void WakeUp()
	{
		need_wait_ = false;
		condition_.notify_one();
	}

	void Run()
	{
		while(!shutdown_)
		{
			StringList::iterator begin;
			StringList::iterator end;
			bool have_data = false;
			{
				boost::unique_lock<boost::mutex> lock(sync_);
				if(need_wait_ && string_list_.empty())
				{
					condition_.wait(lock);
				}
				need_wait_ = true;
				if(!string_list_.empty())
				{
					begin = string_list_.begin();
					end = --(string_list_.end());
					have_data = true;
				}
			}
			if(have_data)
			{
				size_t messages_written = LogRange(begin, end);
				{
					boost::unique_lock<boost::mutex> lock(sync_);
					string_list_.erase(begin, ++end);
					messages_processed_ += messages_written;
					if(pending_flushes_)
					{
						flush_condition_.notify_all();
					}
				}
			}

			if(need_reopen_)
			{
				CloseFile();
				boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
				OpenFile(filename_);
			}
		}
	}

	size_t LogRange(StringList::iterator begin, StringList::iterator end)
	{
		size_t messages_written = 0;
		if(!output_)
			return messages_written;
		for(StringList::iterator it = begin; ; ++it)
		{
			(*output_)<<*it;
			++messages_written;
			if(it == end)
				break;
		}
		return messages_written;
	}

	void OpenFile(const std::string & filename)
	{
		if(filename.empty())
		{
			output_.reset(&std::cout, OStreamEmptyDeleter());
		}
		else
		{
			output_.reset(new std::ofstream(filename.c_str(), std::ios::app | std::ios::binary));
			if(!*output_)
			{
				std::cerr<<"failed to open log file "<<filename<<"\n";
				OpenFile("");
			}
		}
	}

	void CloseFile()
	{
		output_.reset();
	}

private:

	bool initialized_;
	bool shutdown_;
	int log_level_;
	std::string filename_;
	LogBufferTSPtr log_buffer_;
	LogStreamTSPtr log_stream_;
	LogThreadIDTSPtr log_thread_id_;
	boost::thread logger_thread_;
	boost::shared_ptr<std::ostream> output_;
	boost::mutex sync_;
	boost::condition_variable condition_;
	boost::condition_variable flush_condition_;
	bool need_wait_;
	StringList string_list_;
	volatile bool need_reopen_;
	size_t messages_added_;
	size_t messages_processed_;
	size_t pending_flushes_;
};
} // namespace logger
#endif /*LOGGER_LOGGER_H_*/

