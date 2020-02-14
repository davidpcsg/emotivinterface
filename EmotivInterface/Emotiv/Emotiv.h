#include "MentalCommandDetection.h"
#include "IedkErrorCode.h"
#include "IEmoStateDLL.h"
//#include "Message.h"
#include <vector>
#include <list>
#include "Iedk.h"
#include <Windows.h>
#include "value.h"
#include "IEmoStatePerformanceMetric.h"


#pragma once

using namespace std;

class DataEmotiv
{
	public:
		unsigned int				id;
		int							noise;
		float						power;
		float						skill;
		int							current_action;
		int							current_training_action;
		int							neutral_status;
		int							running_status;
		double						stress_score;
		double						engagement_boredom_score;
		double						excitement_score;
		double						interest_score;
		double						relaxation_score;
		vector<int>					anodes;
};

class Emotiv
{
	private:
		CRITICAL_SECTION			m_critical;
		EmoEngineEventHandle		eEvent_;
		std::string*				pMessage_;
		EmoStateHandle				eState_;
		bool						bConnect_;
		int							userCloudID_;
		int							userID_;
		vector<DataEmotiv>			players_;
		HANDLE						pThread_;

		Json::Value					json_anode_status_, json_status_;

		static void handleMentalCommandEvent(LPVOID pParam);
		vector<DataEmotiv>::iterator FindPlayer(int userId);
		void RemovePlayer(int userId);
		void WriteJsonAnodesStatus(LPVOID pParam);
		void WriteJsonStatus(LPVOID pParam);
		void showTrainedActions();

	public:
		Emotiv();
		~Emotiv();

		static		DWORD WINAPI DoPower(LPVOID pParam);		
		int			StartTraining(int action);
		void		EraseTraining(unsigned int userid);
		void		AbortTraining(unsigned int userid);
		void		SetUser(int userID);
		bool		DisconnectCloud();
		bool		ConnectCloud();
		bool		Disconnect();
		bool		Connect();
		float		GetOveralSkill();

		Json::Value GetJsonAnodeStatus();
		Json::Value GetJsonStatus();
		int			GetCloudID();
};

bool const EMOTIV_SMART = false;
int const EMOTIV_THRESHOLD = 77;


