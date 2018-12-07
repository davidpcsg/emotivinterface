/* 
   Socket.cpp

   Copyright (C) 2002-2004 René Nyffenegger

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
*/
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <iostream>
#include <cerrno>
#include <cstring>
#ifndef _MSC_VER
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <Winsock2.h>
#include <Ws2tcpip.h>
#endif

#include "socket.h"

using namespace std;

#ifndef _MSC_VER
enum { SOCKET_ERROR = -1 };
typedef struct timeval TIMEVAL;
#endif


void Socket::Start() {
#ifdef _MSC_VER
  if (!nofSockets_) {
    WSADATA info;
    if (WSAStartup(MAKEWORD(2,0), &info)) {
      throw "Could not start WSA";
    }
  }
#endif
  ++nofSockets_;
}

void Socket::End() {
#ifdef _MSC_VER
  WSACleanup();
#endif
}

Socket::Socket() : s_(0) {
	nofSockets_= 0;
  Start();
  // UDP: use SOCK_DGRAM instead of SOCK_STREAM
  s_ = socket(AF_INET,SOCK_STREAM,0);

  if (s_ == INVALID_SOCKET) {
    throw "INVALID_SOCKET";
  }

  //refCounter_ = new int(1);
}

Socket::Socket(SOCKET_TYPE s) : s_(s) {
	nofSockets_= 0;
  Start();
  //refCounter_ = new int(1);
};

Socket::~Socket() {
  //if (! --(*refCounter_)) {
    Close();
    //delete refCounter_;
  //}

  --nofSockets_;
  if (!nofSockets_) End();
  
}

Socket::Socket(const Socket& o) {
  //refCounter_=o.refCounter_;
  //(*refCounter_)++;
  s_         =o.s_;


  nofSockets_++;
}

Socket& Socket::operator =(Socket& o) {
//  (*o.refCounter_)++;


//  refCounter_=o.refCounter_;
  s_         =o.s_;


  nofSockets_++;


  return *this;
}

void Socket::Close() {
#ifdef _MSC_VER
  closesocket(s_);
#else
  close(s_);
#endif
}

std::string Socket::ReceiveBytes() {
  std::string ret;
  char buf[1024];
 
 for ( ; ; ) {
  u_long arg = 0;
#ifdef _MSC_VER
  if (ioctlsocket(s_, FIONREAD, &arg) != 0)
#else
  if (ioctl(s_, FIONREAD, &arg) != 0)
#endif

   break;
  if (arg == 0)
   break;
  if (arg > 1024)
   arg = 1024;
  int rv = recv (s_, buf, arg, 0);
  if (rv <= 0)
   break;
  std::string t;
  t.assign (buf, rv);
  ret += t;
 }
 
  return ret;
}


std::string Socket::ReceiveBytes(unsigned int n)
{
std::string ret;

  while (n) {
    char buffer[1024] = {0};
    int count = recv(s_, buffer, n < 1023 ? n : 1023, 0);

    if (count == -1)
      break;
    ret += buffer;
    n -= count;
  }

  return ret;
#if 0

	ret.resize(n+1);

	u_long arg = 0;
#ifdef _MSC_VER
	if (ioctlsocket(s_, FIONREAD, &arg) != 0)
#else
	if (ioctl(s_, FIONREAD, &arg) != 0)
#endif
		return std::string("");
	int rv = recv (s_, &ret[0], n, 0);
	if (rv <= 0)
		return std::string("");
	ret[n] = '\0';
	return ret;
#endif
}


#define EAGAIN 11 /* Try again */
std::string Socket::ReceiveLine() {
  //char teste[10240];
  //recv(s_, teste, 10240, 0);
  std::string ret;
    while (1)
	{
     char r;
     switch(recv(s_, &r, 1, 0)) 
	 {
       case 0: // not connected anymore;
         return "";
       case -1:
          if (errno == EAGAIN)
		  {
             return ret;
          }
		  else
		  {
            // not connected anymore
           return "";
         }
     }
     ret += r;
     if (r == '\n')  return ret;
    }
}

void Socket::SendLine(std::string s, bool bret /*= true*/) {
  if (bret)
	s += '\n';
  send(s_,s.c_str(),(int)s.length(),0);
}

void Socket::SendBytes(const std::string& s) {
  send(s_,s.c_str(),(int)s.length(),0);
}

SocketServer::SocketServer(int port, int connections, TypeSocket type) {
  sockaddr_in sa;

  memset(&sa, 0, sizeof(sa));

  sa.sin_family = PF_INET;             
  sa.sin_port = htons(port);          
  s_ = socket(AF_INET, SOCK_STREAM, 0);
  if (s_ == INVALID_SOCKET) {
    throw "INVALID_SOCKET";
  }

  if(type==NonBlockingSocket) {
    u_long arg = 1;
#ifdef _MSC_VER
    ioctlsocket(s_, FIONBIO, &arg); //setando o nonblocking socket
#else
    ioctl(s_, FIONBIO, &arg);
#endif
  }

  int on = 1;
  setsockopt(s_, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

  /* bind the socket to the internet address */
  if (bind(s_, (sockaddr *)&sa, sizeof(sockaddr_in)) == SOCKET_ERROR) {
#ifdef _MSC_VER
    closesocket(s_);
#else
    close(s_);
#endif
    throw "INVALID_SOCKET";
  }
  
  listen(s_, connections);                               
}

Socket* SocketServer::Accept() {
  SOCKET_TYPE new_sock = accept(s_, 0, 0);
  if (new_sock == INVALID_SOCKET) {
#ifdef _MSC_VER
          int rc = WSAGetLastError();
          if(rc==WSAEWOULDBLOCK) {
#else
          if (errno == EWOULDBLOCK) {
#endif
                  return 0; // non-blocking call, no request pending
          }
          else {
            throw "Invalid Socket";
      }
  }



  Socket* r = new Socket(new_sock);
  return r;
}

SocketClient::SocketClient(const std::string& host, int port) : Socket() {
  

	std::string error;
	addrinfo* ai = 0;
	addrinfo hints =  {0};
	int code = 0;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	code = getaddrinfo(host.c_str(),0,&hints,&ai);
	if (code)
	{
	#ifdef _MSC_VER
		error = gai_strerrorA(code);
	#else
		error = gai_strerror(code);
	#endif
		throw error;
	}

	sockaddr_in addr;
	memcpy(&addr,ai->ai_addr,ai->ai_addrlen);
	addr.sin_port = htons(port);
	freeaddrinfo(ai);
	if (connect(s_,(sockaddr*)&addr,sizeof(sockaddr)))
	{
	#ifdef _MSC_VER
		error = strerror(WSAGetLastError());
	#else
		error = strerror(errno);
	#endif
		throw(error);
	}

}

SocketSelect::SocketSelect(Socket const * const s1, Socket const * const s2, TypeSocket type) {
  FD_ZERO(&fds_);
  FD_SET(const_cast<Socket*>(s1)->s_,&fds_);
  if(s2) {
    FD_SET(const_cast<Socket*>(s2)->s_,&fds_);
  }     


  TIMEVAL tval;
  tval.tv_sec  = 0;
  tval.tv_usec = 1;


  TIMEVAL *ptval;
  if(type==NonBlockingSocket) {
    ptval = &tval;
  }
  else { 
    ptval = 0;
  }


  if (select (0, &fds_, (fd_set*) 0, (fd_set*) 0, ptval) 
      == SOCKET_ERROR) throw "Error in select";
}

bool SocketSelect::Readable(Socket const * const s) {
  if (FD_ISSET(s->s_,&fds_)) return true;
  return false;
}
