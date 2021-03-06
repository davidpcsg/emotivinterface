 // EmotivInterface.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include "Emotiv\Emotiv.h"
#include "webserver/webserver.h"

#include "value.h"
#include "writer.h"

DWORD WINAPI RosPublisher(LPVOID pParam)
{
	Emotiv* pEmotiv = (Emotiv*)pParam;
	return 0;
}

void listen_func(webserver::http_request* req, void* pCaller) {
	Emotiv* pEmotiv = (Emotiv*)pCaller;

	if (req->method_ == "GET" && req->path_ == "/emotiv") {
		std::cout << req->method_ << std::endl;
		std::cout << "Foi chamada a funcao. " << pEmotiv->GetCloudID() << std::endl;

		Json::Value root, status, contact;
		root["action_status"] = pEmotiv->GetJsonStatus();
		root["contact_status"] = pEmotiv->GetJsonAnodeStatus();
		req->answer_ = root.toStyledString();
	}
}

int main()
{
	Emotiv emotiv;
	int iterator = 0;
	webserver server;
	server.listen(1234, listen_func, &emotiv);
    return 0;
}

