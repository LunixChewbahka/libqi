/*
 *  Author(s):
 *  - Chris  Kilner <ckilner@aldebaran-robotics.com>
 *  - Cedric Gestes <gestes@aldebaran-robotics.com>
 *
 *  Copyright (C) 2010, 2011 Aldebaran Robotics
 */

#include <qi/log.hpp>
#include <qi/os.hpp>
#include <list>
#include <map>
#include <cstring>

#include <qi/log/consoleloghandler.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/lockfree/fifo.hpp>
#include <boost/function.hpp>

#define RTLOG_BUFFERS (128)


#define CAT_SIZE 64
#define FILE_SIZE 128
#define FUNC_SIZE 64
#define LOG_SIZE 2048

namespace qi {
  namespace log {
    static LogLevel _glVerbosity = qi::log::info;
    static bool _glContext = false;
    static bool _glSyncLog = false;

    typedef struct sPrivateLog
    {
      LogLevel _logLevel;
      char     _category[CAT_SIZE];
      char     _file[FILE_SIZE];
      char     _function[FUNC_SIZE];
      int      _line;
      char     _log[LOG_SIZE];
    } privateLog;

    static privateLog             rtLogBuffer[RTLOG_BUFFERS];
    static volatile unsigned long rtLogPush = 0;

    static ConsoleLogHandler gConsoleLogHandler;

    static class rtLog
    {
    public:
      inline rtLog();
      inline ~rtLog();

      void run();
      void printLog();

    public:
      bool                       rtLogInit;
      boost::thread              rtLogThread;
      boost::mutex               rtLogWriteLock;
      boost::condition_variable  rtLogReadyCond;

      boost::lockfree::fifo<privateLog*>     logs;
      std::map<std::string, logFuncHandler > logHandlers;
    } rtLogInstance;


    void rtLog::printLog()
    {
      privateLog* pl;
      while (logs.dequeue(&pl))
      {
        boost::mutex::scoped_lock lock(rtLogWriteLock);
        if (!logHandlers.empty())
        {
          std::map<std::string, logFuncHandler >::iterator it;
          for (it = logHandlers.begin();
               it != logHandlers.end(); ++it)
          {
            (*it).second(pl->_logLevel,
                         pl->_category,
                         pl->_log,
                         pl->_file,
                         pl->_function,
                         pl->_line);
          }
        }
      }
    }

    void rtLog::run()
    {
      while (rtLogInit)
      {
        {
          boost::mutex::scoped_lock lock(rtLogWriteLock);
          rtLogReadyCond.wait(lock);
        }

        printLog();
      }
    };

    inline rtLog::rtLog()
    {
      rtLogInit = true;
      rtLogThread = boost::thread(&rtLog::run, &rtLogInstance);
    };

    inline rtLog::~rtLog()
    {
      if (!rtLogInit)
        return;

      rtLogInit = false;

      rtLogThread.interrupt();
      rtLogThread.join();

      printLog();
    }

    static void my_strcpy_log(char *dst, const char *src, int len) {
      if (!src)
        src = "(null)";

      int messSize = strlen(src);
      // check if the last char is a \n
      if (src[messSize - 1] == '\n')
      {
        // Get the size to memcpy (don't forget we need 1 space more for \0)
        int strSize = messSize < len  ? messSize : len - 1;
       #ifndef _WIN32
        memcpy(dst, src, strSize);
       #else
        memcpy_s(dst, len,  src, strSize);
       #endif
        dst[strSize] = 0;
        return;
      }

     #ifndef _WIN32
      // Get the size to memcpy (we need 2 spaces more for \n\0)
      int strSize = messSize < len - 1  ? messSize : len - 2;
      memcpy(dst, src, strSize);
      dst[strSize] = '\n';
      dst[strSize + 1] = '\0';
     #else
      // Get the size to memcpy (we need 3 spaces more for \r\n\0)
      int strSize = messSize < len - 2  ? messSize : len - 3;
      memcpy_s(dst, len,  src, strSize);
      dst[strSize] = '\r';
      dst[strSize + 1] = '\n';
      dst[strSize + 2] = '\0';
     #endif
    }

    static void my_strcpy(char *dst, const char *src, int len) {
      if (!src)
        src = "(null)";
     #ifdef _MSV_VER
      strncpy_s(dst, len, src, _TRUNCATE);
     #else
      strncpy(dst, src, len);
      dst[len - 1] = 0;
     #endif
    }

    void log(const LogLevel    verb,
             const char       *category,
             const char       *msg,
             const char       *file,
             const char       *fct,
             const int         line)

    {
      if (!rtLogInstance.rtLogInit)
        return;

      int tmpRtLogPush = ++rtLogPush % RTLOG_BUFFERS;
      privateLog* pl = &(rtLogBuffer[tmpRtLogPush]);

      pl->_logLevel = verb;
      pl->_line = line;

      my_strcpy(pl->_category, category, CAT_SIZE);
      my_strcpy(pl->_file, file, FILE_SIZE);
      my_strcpy(pl->_function, fct, FUNC_SIZE);
      my_strcpy_log(pl->_log, msg, LOG_SIZE);

      if (_glSyncLog)
      {
        if (!rtLogInstance.logHandlers.empty())
        {
          std::map<std::string, logFuncHandler >::iterator it;
          for (it = rtLogInstance.logHandlers.begin();
               it != rtLogInstance.logHandlers.end(); ++it)
          {
            (*it).second(pl->_logLevel,
                         pl->_category,
                         pl->_log,
                         pl->_file,
                         pl->_function,
                         pl->_line);
          }
        }
      }
      else
      {
        rtLogInstance.logs.enqueue(pl);
        rtLogInstance.rtLogReadyCond.notify_one();
      }
    }

    void consoleLogHandler(const LogLevel    verb,
                           const char       *category,
                           const char       *msg,
                           const char       *file,
                           const char       *fct,
                           const int         line)
    {
      gConsoleLogHandler.log(verb, category, msg, file, fct, line);
    }



    static class LogHandlerInit
    {
    public:
      LogHandlerInit()
      {
        addLogHandler(consoleLogHandler, "consoleloghandler");
      }
    } gLogHandlerInit;


    void addLogHandler(logFuncHandler fct, const std::string& name)
    {
      {
        boost::mutex::scoped_lock l(rtLogInstance.rtLogWriteLock);
        rtLogInstance.logHandlers[name] = fct;
      }
    }

    void removeLogHandler(const std::string& name)
    {
      {
        boost::mutex::scoped_lock l(rtLogInstance.rtLogWriteLock);
        rtLogInstance.logHandlers.erase(name);
      }
    }

    const LogLevel stringToLogLevel(const char* verb)
    {
      std::string v(verb);
      if (v == "silent")
        return qi::log::silent;
      if (v == "fatal")
        return qi::log::fatal;
      if (v == "error")
        return qi::log::error;
      if (v == "warning")
        return qi::log::warning;
      if (v == "info")
        return qi::log::info;
      if (v == "verbose")
        return qi::log::verbose;
      if (v == "debug")
        return qi::log::debug;
      return qi::log::info;
    }

    const char *logLevelToString(const LogLevel verb)
    {
      static const char *sverb[] = {
        "[SILENT]", // never shown
        "[FATAL]",
        "[ERROR]",
        "[WARN ]",
        "[INFO ]",
        "[VERB ]",
        "[DEBUG]"
      };
      return sverb[verb];
    }

    void setVerbosity(const LogLevel lv)
    {
      const char *verbose = std::getenv("VERBOSE");

      if (verbose)
        _glVerbosity = (LogLevel)atoi(verbose);
      else
        _glVerbosity = lv;
    };

    LogLevel getVerbosity()
    {
      return _glVerbosity;
    };

    void setContext(bool ctx)
    {
      const char *context = std::getenv("CONTEXT");

      if (context)
        _glContext = (atoi(context) > 0 ? true : false);
      else
        _glContext = ctx;
    };

    bool getContext()
    {
      return _glContext;
    };

    void setSyncLog(bool sync)
    {
      _glSyncLog = sync;
    };

  } // namespace log
} // namespace qi
