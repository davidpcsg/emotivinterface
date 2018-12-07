/*
   WebServer.cpp

   Copyright (C) 2003-2004 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

   Thanks to Tom Lynn who pointed out an error in this source code.
*/

#include <ctime>
#include <cstdlib>
#ifdef _MSC_VER
#include <process.h>
#endif
#include <iostream>
#include <string>
#include <map>
#include <sstream>
//<process.h>
#include <cstring>
#include <memory>

#include "webserver.h"
#include "socket.h"
#include "UrlHelper.h"
#include "base64.h"

//#include <boost/thread.hpp>

webserver::request_func webserver::request_func_=0;

void webserver::Request(void* param) {
  std::auto_ptr<WeberverWrapper> pWrapper((WeberverWrapper*)param);
  void* pCaller = pWrapper->pCaller;
  std::auto_ptr<Socket> s(reinterpret_cast<Socket*>(pWrapper->ptr_s));

  std::string line = s->ReceiveLine();
  if (line.empty()) {
    return ;
  }

  http_request req;

  if (line.find("GET") == 0) {
    req.method_="GET";
  }
  else if (line.find("POST") == 0) {
    req.method_="POST";
  }
  else if (line.find("PUT") == 0) {
    req.method_="PUT";
  }
  else if (line.find("DELETE") == 0) {
	  req.method_="DELETE";
  }

  std::string path;
  std::map<std::string, std::string> params;

  size_t posStartPath = line.find(" ") + 1;

  SplitGetReq(line.substr(posStartPath), path, params);

  req.status_ = "202 OK";
  req.s_      = s.get();
  req.path_   = path;
  req.params_ = params;

  static const std::string authorization   = "Authorization: Basic ";
  static const std::string accept          = "Accept: "             ;
  static const std::string accept_language = "Accept-Language: "    ;
  static const std::string accept_encoding = "Accept-Encoding: "    ;
  static const std::string user_agent      = "User-Agent: "         ;
  static const std::string content_type    = "Content-Type: "       ;
  static const std::string content_length  = "Content-Length: "     ;

  while(1) {
    line=s->ReceiveLine();

    if (line.empty()) break;
    unsigned int pos_cr_lf = (unsigned int)line.find_first_of("\x0a\x0d");
    if (pos_cr_lf == 0) break; // Fim do Header

    line = line.substr(0,pos_cr_lf);

    if (line.substr(0, authorization.size()) == authorization) 
	{
      req.authentication_given_ = true;
      std::string encoded = line.substr(authorization.size());
      std::string decoded = base64_decode(encoded);

      unsigned int pos_colon = (unsigned int)decoded.find(":");

      req.username_ = decoded.substr(0, pos_colon);
      req.password_ = decoded.substr(pos_colon+1 );
    }
    else if (line.substr(0, accept.size()) == accept) 
	{
      req.accept_ = line.substr(accept.size());
    }
    else if (line.substr(0, accept_language.size()) == accept_language) 
	{
      req.accept_language_ = line.substr(accept_language.size());
    }
    else if (line.substr(0, accept_encoding.size()) == accept_encoding) 
	{
      req.accept_encoding_ = line.substr(accept_encoding.size());
    }
    else if (line.substr(0, user_agent.size()) == user_agent) 
	{
      req.user_agent_ = line.substr(user_agent.size());
    }
	else if (line.substr(0, content_type.size()) == content_type) 
	{
	  req.content_type_ = line.substr(content_type.size());
    }
	else if (line.substr(0, content_length.size()) == content_length) 
	{
	  req.content_length_= atoi(line.substr(content_length.size()).c_str());
    }
  }

  if(req.content_length_ > 0)
  {
	  req.body_ = s->ReceiveBytes(req.content_length_);
  }

  //if(req.content_type_ == "application/x-www-form-urlencoded")
  //{
	 // ParseParams(req.body_, params);
	 // req.params_ = params;
  //}


  request_func_(&req, pCaller);

  std::stringstream str_str;
  str_str << req.answer_.size();

  time_t ltime;
  time(&ltime);
  tm* gmt= gmtime(&ltime);

  static std::string const serverName = "EmotivServer";

  char* asctime_remove_nl = asctime(gmt);
  asctime_remove_nl[24] = 0;

  s->SendBytes("HTTP/1.1 ");

  if (! req.auth_realm_.empty() ) {
    s->SendLine("401 Unauthorized");
    s->SendBytes("WWW-Authenticate: Basic Realm=\"");
    s->SendBytes(req.auth_realm_);
    s->SendLine("\"");
  }
  else {
    s->SendLine(req.status_);
  }
  s->SendLine(std::string("Date: ") + asctime_remove_nl + " GMT");
  s->SendLine(std::string("Server: ") +serverName);
  s->SendLine("Connection: close");
  s->SendLine(std::string("Content-Type: ") + req.content_type_ + std::string("; charset=ISO-8859-1"));
  s->SendLine("Content-Length: " + str_str.str());
  s->SendLine("");
  s->SendLine(req.answer_, false);
  s->Close();
}

webserver::webserver()
{
	this->end_server = false;
}

webserver::~webserver()
{
}

void webserver::listen(unsigned int port_to_listen, request_func r, void* pCaller)
{
	SocketServer socket_server(port_to_listen, 5000, NonBlockingSocket);
	request_func_ = r;

	while (!this->end_server)
	{
		Socket* ptr_s = socket_server.Accept();
		if (!ptr_s)
		{
			//boost::this_thread::sleep(boost::posix_time::milliseconds(200));
#ifdef _MSC_VER
			Sleep(200);
#else
			usleep(200000);
#endif
			continue;
		}

		std::auto_ptr<WeberverWrapper> wrapper(new WeberverWrapper());
		wrapper->pCaller = pCaller;
		wrapper->ptr_s = ptr_s;

	#ifdef _MSC_VER
		HANDLE thread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Request, wrapper.get(), 0, 0);

		if (thread != 0) {
			wrapper.release();
			CloseHandle(thread);
		}
	#else
		pthread_t tid;
		pthread_attr_t attr = {0};
		int code = 0;

		if (code = pthread_attr_init(&attr))
			std::cout << "pthread_attr_init: " << strerror(code) << '\n';
		if (code = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
			std::cout << "pthread_attr_setdetachstate: " << strerror(code) << '\n';
		if (code = pthread_create(&tid, &attr, (void*(*)(void*))&Request, wrapper.get()))
			std::cout << "pthread_create: " << strerror(code) << '\n';
		else
			wrapper.release();
		if (code = pthread_attr_destroy(&attr))
			std::cout << "pthread_destroy: " << strerror(code) << '\n';
	#endif
	}
}

void webserver::end()
{
	this->end_server = true;
}