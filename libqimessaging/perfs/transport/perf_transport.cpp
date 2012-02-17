/*
** Author(s):
**  - Cedric GESTES <gestes@aldebaran-robotics.com>
**
** Copyright (C) 2010, 2012 Aldebaran Robotics
*/


#include <vector>
#include <iostream>
#include <sstream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <qimessaging/transport.hpp>
#include <qimessaging/session.hpp>
#include <qimessaging/perf/dataperftimer.hpp>

#include <qimessaging/service_directory.hpp>
#include <qimessaging/gateway.hpp>

static int gLoopCount = 10000;
static const int gThreadCount = 1;

int main_client(std::string src)
{
  qi::perf::DataPerfTimer dp ("Transport synchronous call");
  qi::Session session;
  session.connect("tcp://127.0.0.1:12345");
  session.waitForConnected();

  qi::Object *obj = session.service("serviceTest");

  for (int i = 0; i < 12; ++i)
  {
    char character = 'c';
    unsigned int numBytes = (unsigned int)pow(2.0f, (int)i);
    std::string requeststr = std::string(numBytes, character);

    dp.start(gLoopCount, numBytes);
    for (int j = 0; j < gLoopCount; ++j)
    {
      static int id = 1;
      id++;
      char c = 1;
      c++;
      if (c == 0)
        c++;
      requeststr[2] = c;

      std::string result = obj->call<std::string>("reply", requeststr);
      if (result != requeststr)
        std::cout << "error content" << std::endl;
    }
    dp.stop(1);
  }
  return 0;
}

void start_client(int count)
{
  boost::thread thd[100];

  assert(count < 100);

  for (int i = 0; i < count; ++i)
  {
    std::stringstream ss;
    ss << "remote" << i;
    std::cout << "starting thread: " << ss.str() << std::endl;
    thd[i] = boost::thread(boost::bind(&main_client, ss.str()));
  }

  for (int i = 0; i < count; ++i)
    thd[i].join();
}


#include <qi/os.hpp>


int main_gateway()
{
  qi::Session       session;
  qi::Object        obj;
  qi::Gateway       gate;

  session.connect("tcp://127.0.0.1:5555");
  session.waitForConnected();

  gate.listen(&session, "tcp://127.0.0.1:12345");
  std::cout << "ready." << std::endl;

  while (1)
    qi::os::sleep(1);
}

#include <qimessaging/server.hpp>

std::string reply(const std::string &msg)
{
  return msg;
}

int main_server()
{
  qi::ServiceDirectory sd;
  sd.listen("tcp://127.0.0.1:5555");
  std::cout << "Service Directory ready." << std::endl;

  qi::Session       session;
  qi::Object        obj;
  qi::Server        srv;
  obj.advertiseMethod("reply", &reply);

  session.connect("tcp://127.0.0.1:5555");
  session.waitForConnected();

  srv.listen(&session, "tcp://127.0.0.1:9559");
  srv.registerService("serviceTest", &obj);
  std::cout << "serviceTest ready." << std::endl;

  while (1)
    qi::os::sleep(1);

  srv.stop();
  session.disconnect();
}

int main(int argc, char **argv)
{
  if (argc > 1 && !strcmp(argv[1], "--client"))
  {
    int threadc = 1;
    if (argc > 2)
      threadc = atoi(argv[2]);
    start_client(threadc);
  }
  else if (argc > 1 && !strcmp(argv[1], "--server"))
  {
    return main_server();
  }
  else if (argc > 1 && !strcmp(argv[1], "--gateway"))
  {
    return main_gateway();
  }
  else
  {
    //start the server
    boost::thread threadServer1(boost::bind(&main_server));
    boost::thread threadServer2(boost::bind(&main_gateway));
    sleep(1);
    start_client(gThreadCount);
  }
  return 0;
}
