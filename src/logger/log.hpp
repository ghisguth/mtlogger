/* log.hpp - main logger interface
 * This file is a part of mtlogger library
 * Copyright (c) mtlogger authors (see file `COPYRIGHT` for the license)
 */

#ifndef LOGGER_LOG_H_
#define LOGGER_LOG_H_

#include <stdarg.h>

#include <string>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

#include "logger/logger.hpp"

namespace logger
{
enum Severity
{
	SeverityDebug = 0,
	SeverityVerbose,
	SeverityInfo,
	SeverityNotice,
	SeverityWarning,
	SeverityError,
	SeverityFatalError,
};

template<Severity level, char symbol>
struct MessageLogger
{
	static void Log(const char * message, ...)
#if defined(__GNUC__)
		__attribute__ ((__format__ (__printf__, 1, 2)))
#endif
	{
		if((int)level>=Logger::GetInstance()->GetLogLevel())
		{
			if(Logger::GetInstance()->IsInitialized())
			{
				Logger::LogBufferTSPtr & log_buffer = Logger::GetInstance()->GetBuffer();
				if(!log_buffer.get())
				{
					log_buffer.reset(new Logger::LogBuffer);
				}
				Logger::LogThreadIDTSPtr & thread_id = Logger::GetInstance()->GetThreadID();
				if(!thread_id.get())
				{
					thread_id.reset(new std::string(
							boost::lexical_cast<std::string>(boost::this_thread::get_id()))
						);
				}

				char * buffer = &(*log_buffer)[0];
				time_t timestamp;
				struct tm lts;
				time(&timestamp);
				localtime_r(&timestamp, &lts);

				int offset = snprintf(buffer, (*log_buffer).size(), "%02d.%02d.%04d %02d:%02d:%02d [%14s] - <%c> ",
									lts.tm_mday, lts.tm_mon + 1, 1900+lts.tm_year, lts.tm_hour, lts.tm_min, lts.tm_sec,
									(*thread_id).c_str(), symbol);
				if((size_t)offset >= (*log_buffer).size())
					return;

				va_list args;
				va_start(args, message);
				offset += vsnprintf(buffer + offset, (*log_buffer).size() - offset, message, args);
				va_end(args);


				if((size_t)offset + 2 >= (*log_buffer).size())
				{
					offset = (*log_buffer).size() - 2 - 3;
					buffer[offset++] = '.';
					buffer[offset++] = '.';
					buffer[offset++] = '.';
				}
				buffer[offset++] = '\n';

				if(level == SeverityFatalError){
					std::cerr << std::string(buffer, buffer + offset);
				}
				Logger::GetInstance()->AddMessage(*log_buffer, (size_t)offset);
				if(level == SeverityFatalError){
					Logger::GetInstance()->Flush();
				}
			}
		}
	}
};

template<Severity level, char symbol>
struct MessageSteamLogger
{
	static std::stringstream * GetStream()
	{
		if((int)level>=Logger::GetInstance()->GetLogLevel())
		{
			if(Logger::GetInstance()->IsInitialized())
			{
				Logger::LogStreamTSPtr & log_stream = Logger::GetInstance()->GetStream();
				if(!log_stream.get())
				{
					log_stream.reset(new std::stringstream);
				}
				Logger::LogThreadIDTSPtr & thread_id = Logger::GetInstance()->GetThreadID();
				if(!thread_id.get())
				{
					thread_id.reset(new std::string(
							boost::lexical_cast<std::string>(boost::this_thread::get_id()))
						);
				}

				time_t timestamp;
				struct tm lts;
				time(&timestamp);
				localtime_r(&timestamp, &lts);

				*log_stream<<std::setfill('0')<<std::setw(2)<<lts.tm_mday
					<<"."<<std::setfill('0')<<std::setw(2)<<lts.tm_mon+1
					<<"."<<std::setfill('0')<<std::setw(4)<<1900+lts.tm_year
					<<" "<<std::setfill('0')<<std::setw(2)<<lts.tm_hour
					<<":"<<std::setfill('0')<<std::setw(2)<<lts.tm_min
					<<":"<<std::setfill('0')<<std::setw(2)<<lts.tm_sec
					<<" ["<<std::setfill(' ')<<std::setw(14)<<*thread_id
					<<"] - <"<<symbol<<"> ";

				log_stream->clear();
				return log_stream.get();
			}
		}
		return 0;
	}

	static void Send(std::stringstream * stream)
	{
		if(stream && (int)level>=Logger::GetInstance()->GetLogLevel() && Logger::GetInstance()->IsInitialized())
		{
			*stream << "\n";

			if(level == SeverityFatalError){
				std::cerr<<(*stream).str();
			}
			Logger::GetInstance()->AddMessage(*stream);
			if(level == SeverityFatalError){
				Logger::GetInstance()->Flush();
			}
			stream->str("");
			stream->clear();
		}
	}
};
} // namespace logger

#define LOGINIT logger::Logger::GetInstance()->Initialize
#define LOGFLUSH logger::Logger::GetInstance()->Flush
#define LOGREOPEN logger::Logger::GetInstance()->Reopen
#define LOGLEVEL logger::Logger::GetInstance()->SetLogLevel

#define LOGDBG(msg...) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityDebug) \
	{ \
		logger::MessageLogger<logger::SeverityDebug, '*'>::Log(msg); \
	}
#define LOGVERB(msg...) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityVerbose) \
	{ \
		logger::MessageLogger<logger::SeverityVerbose, '.'>::Log(msg); \
	}
#define LOG(msg...) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityInfo) \
	{ \
		logger::MessageLogger<logger::SeverityInfo, ' '>::Log(msg); \
	}
#define LOGNOTICE(msg...) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityNotice) \
	{ \
		logger::MessageLogger<logger::SeverityNotice, '?'>::Log(msg); \
	}
#define LOGWARN(msg...) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityWarning) \
	{ \
		logger::MessageLogger<logger::SeverityWarning, '$'>::Log(msg); \
	}
#define LOGERR(msg...) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityError) \
	{ \
		logger::MessageLogger<logger::SeverityError, '!'>::Log(msg); \
	}
#define LOGFATAL(msg...) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityFatalError) \
	{ \
		logger::MessageLogger<logger::SeverityFatalError, '#'>::Log(msg); \
	}

#define LOGSDBG(msg) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityDebug) \
	{ \
		typedef logger::MessageSteamLogger<logger::SeverityDebug, '*'> L; \
		std::stringstream * stream(L::GetStream()); \
		if(stream) \
		{ \
			*stream << msg; \
			L::Send(stream); \
		} \
	}
#define LOGSVERB(msg) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityVerbose) \
	{ \
		typedef logger::MessageSteamLogger<logger::SeverityVerbose, '.'> L; \
		std::stringstream * stream(L::GetStream()); \
		if(stream) \
		{ \
			*stream << msg; \
			L::Send(stream); \
		} \
	}
#define LOGS(msg) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityInfo) \
	{ \
		typedef logger::MessageSteamLogger<logger::SeverityInfo, ' '> L; \
		std::stringstream * stream(L::GetStream()); \
		if(stream) \
		{ \
			*stream << msg; \
			L::Send(stream); \
		} \
	}
#define LOGSNOTICE(msg) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityNotice) \
	{ \
		typedef logger::MessageSteamLogger<logger::SeverityNotice, '?'> L; \
		std::stringstream * stream(L::GetStream()); \
		if(stream) \
		{ \
			*stream << msg; \
			L::Send(stream); \
		} \
	}
#define LOGSWARN(msg) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityWarning) \
	{ \
		typedef logger::MessageSteamLogger<logger::SeverityWarning, '$'> L; \
		std::stringstream * stream(L::GetStream()); \
		if(stream) \
		{ \
			*stream << msg; \
			L::Send(stream); \
		} \
	}
#define LOGSERROR(msg) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityError) \
	{ \
		typedef logger::MessageSteamLogger<logger::SeverityError, '!'> L; \
		std::stringstream * stream(L::GetStream()); \
		if(stream) \
		{ \
			*stream << msg; \
			L::Send(stream); \
		} \
	}
#define LOGSFATAL(msg) \
	if(logger::Logger::GetInstance()->GetLogLevel() <= logger::SeverityFatalError) \
	{ \
		typedef logger::MessageSteamLogger<logger::SeverityFatalError, '#'> L; \
		std::stringstream * stream(L::GetStream()); \
		if(stream) \
		{ \
			*stream << msg; \
			L::Send(stream); \
		} \
	}

#endif /*LOGGER_LOG_H_*/
