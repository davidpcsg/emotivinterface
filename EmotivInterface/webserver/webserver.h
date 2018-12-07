/*
   WebServer.h

   Copyright (C) 2003-2004 Ren� Nyffenegger

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

   Ren� Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#include <string>
#include <map>

class Socket;

class WeberverWrapper
{
public:
	void* pCaller;
	void* ptr_s;
};

#ifdef _MSC_VER
#define THREAD_CALLTYPE __cdecl
#else
#define THREAD_CALLTYPE
#endif

class webserver
{
  public:
    struct http_request {
    
      http_request() : 
		content_length_(0),
		authentication_given_(false) {}
    
      Socket*                            s_;
      std::string                        method_;
      std::string                        path_;
      std::map<std::string, std::string> params_;

      std::string                        accept_;
      std::string                        accept_language_;
      std::string                        accept_encoding_;
      std::string                        user_agent_;
	  std::string						 content_type_;
	  unsigned int						 content_length_;
    
      /* status_: used to transmit server's error status, such as
         o  202 OK
         o  404 Not Found 
         and so on */
      std::string                        status_;
    
      /* auth_realm_: allows to set the basic realm for an authentication,
         no need to additionally set status_ if set */
      std::string                        auth_realm_;
    
      std::string                        answer_;

	  std::string						 body_;
    
      /*   authentication_given_ is true when the user has entered a username and password.
           These can then be read from username_ and password_ */
      bool authentication_given_;
      std::string username_;
      std::string password_;
    };

    typedef   void (*request_func) (http_request*, void*);
    webserver();
	~webserver();

	void listen(unsigned int port_to_listen, request_func r, void* pCaller);
	void end();

  private:
    static void THREAD_CALLTYPE Request(void*);
    static request_func request_func_;
	bool end_server;
};