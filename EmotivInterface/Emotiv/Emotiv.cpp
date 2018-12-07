#include "Emotiv.h"
#include "EmotivCloudClient.h"
#include <algorithm>
#include <sstream>
#include "reader.h"
#include "value.h"
#include "json.h"
#include <iostream>

#pragma warning (disable : 4482)
#define			USERID		0
#define			USERNAME		"davidpcsg"
#define			PASSWORD		"1q2w3e4rQ"
#define			PROFILENAME		"davidpcsg"
#define			PROFILEPATH		"C:/ProgramData/EmotivControlPanel/davidpcsg.emu"

using namespace std;

const char *byte_to_binary(long x)
{
	static char b[9];
	b[0] = '\0';

	int z;
	for (z = 8192; z > 0; z >>= 1)
	{
		strcat(b, ((x & z) == z) ? "1" : "0");
	}

	return b;
}

void CalculateScale(double& rawScore, double& maxScale,
	double& minScale, double& scaledScore) {

	if (rawScore<minScale)
	{
		scaledScore = 0;
	}
	else if (rawScore>maxScale)
	{
		scaledScore = 1;
	}
	else {
		scaledScore = ((rawScore - minScale) / (maxScale - minScale));
	}
}

void Emotiv::showTrainedActions()
{
	unsigned long pTrainedActionsOut = 0;
	IEE_MentalCommandGetTrainedSignatureActions(userID_, &pTrainedActionsOut);

	std::cout << "Trained Actions" << " : " << byte_to_binary(pTrainedActionsOut) << "\n";
}


DWORD WINAPI Emotiv::DoPower(LPVOID pParam)
{
	unsigned int				userID = -1;
	int							userCloudID		= -1;
	float						skill, power = 0.0;
	bool						onStateChanged = false;
	IEE_EEG_ContactQuality_t	contactQuality[18];

	Emotiv *pEmotiv = (Emotiv*)(pParam);

	if(!pEmotiv->Connect())
		return -1;

	if (!pEmotiv->ConnectCloud())
		return -1;

	while(pEmotiv->bConnect_)
	{
		int state = IEE_EngineGetNextEvent(pEmotiv->eEvent_);
		if (state == EDK_OK)
		{
			IEE_Event_t eventType = IEE_EmoEngineEventGetType(pEmotiv->eEvent_);
			
			int retCode = IEE_EmoEngineEventGetUserId(pEmotiv->eEvent_, &userID);
			pEmotiv->userID_ = userID;

			if(userID == -1)
				continue;

			DataEmotiv	data;

			switch (eventType)
			{
				case IEE_UserAdded:
					data.id	= userID;

					retCode = IEE_LoadUserProfile(pEmotiv->userID_, PROFILEPATH);
					if (retCode == EDK_OK) {
						pEmotiv->showTrainedActions();
					}

					data.noise = 0;
					EnterCriticalSection(&pEmotiv->m_critical);
					pEmotiv->players_.push_back(data);
					onStateChanged = true;
					LeaveCriticalSection(&pEmotiv->m_critical);
				break;

				case IEE_UserRemoved:
					EnterCriticalSection(&pEmotiv->m_critical);
					pEmotiv->RemovePlayer(userID);	
					onStateChanged = true;
					LeaveCriticalSection(&pEmotiv->m_critical);
				break;

				case IEE_EmoStateUpdated:
				{
					onStateChanged = true;
					if(IEE_EmoEngineEventGetEmoState(pEmotiv->eEvent_, pEmotiv->eState_) == EDK_OK)
					{
						vector<DataEmotiv>::iterator it = pEmotiv->players_.begin();
						it = pEmotiv->FindPlayer(userID);
						if(it  != pEmotiv->players_.end())
						{
							int status = IS_MentalCommandIsActive(pEmotiv->eState_);
							it->noise = status;

							IEE_MentalCommandAction_t actionType = IS_MentalCommandGetCurrentAction(pEmotiv->eState_);
							IEE_EEG_ContactQuality_t	contactQuality[18];
							IS_GetContactQualityFromAllChannels(pEmotiv->eState_, &contactQuality[0], 18);
							if (actionType == IEE_MentalCommandAction_t::MC_PUSH )
							{
								power = IS_MentalCommandGetCurrentActionPower(pEmotiv->eState_);
								IEE_MentalCommandGetActionSkillRating(userID, IEE_MentalCommandAction_t::MC_PUSH, &skill);
								it->power = int(power*100);
								it->current_action = 1;
								it->skill = (skill*100);
							}
							else if (actionType == IEE_MentalCommandAction_t::MC_PULL)
							{
								power = IS_MentalCommandGetCurrentActionPower(pEmotiv->eState_);
								IEE_MentalCommandGetActionSkillRating(userID, IEE_MentalCommandAction_t::MC_PULL, &skill);
								it->power = int(power*100);
								it->current_action = 2;
								it->skill = (skill * 100);
							}
							else if(actionType == IEE_MentalCommandAction_t::MC_NEUTRAL)
							{
								it->current_action = 0;
								it->power = 0;
								it->skill = 0;
							}

							double rawScore = 0;
							double minScale = 0;
							double maxScale = 0;
							IS_PerformanceMetricGetStressModelParams(pEmotiv->eState_, &rawScore, &minScale, &maxScale);
							CalculateScale(rawScore, maxScale, minScale, it->stress_score);
							IS_PerformanceMetricGetEngagementBoredomModelParams(pEmotiv->eState_, &rawScore, &minScale, &maxScale);
							CalculateScale(rawScore, maxScale, minScale, it->engagement_boredom_score);
							IS_PerformanceMetricGetRelaxationModelParams(pEmotiv->eState_, &rawScore, &minScale, &maxScale);
							CalculateScale(rawScore, maxScale, minScale, it->relaxation_score);
							IS_PerformanceMetricGetInstantaneousExcitementModelParams(pEmotiv->eState_, &rawScore, &minScale, &maxScale);
							CalculateScale(rawScore, maxScale, minScale, it->excitement_score);
							IS_PerformanceMetricGetInterestModelParams(pEmotiv->eState_, &rawScore, &minScale, &maxScale);
							CalculateScale(rawScore, maxScale, minScale, it->interest_score);


							EnterCriticalSection(&pEmotiv->m_critical);
							pEmotiv->WriteJsonStatus(pEmotiv);
							LeaveCriticalSection(&pEmotiv->m_critical);
						}
					}
					break;
				}

				case IEE_MentalCommandEvent:
				{
					handleMentalCommandEvent(pEmotiv);
					break;
				}
			}

			if(onStateChanged)
			{
				IS_GetContactQualityFromAllChannels(pEmotiv->eState_, &contactQuality[0], 18);
				vector<DataEmotiv>::iterator it = pEmotiv->players_.begin();
				it = pEmotiv->FindPlayer(userID);
				if(it != pEmotiv->players_.end())
				{
					it->anodes.clear();
					if(IS_GetWirelessSignalStatus(pEmotiv->eState_) != NO_SIG)
					{
						it->anodes.push_back(contactQuality[IEE_CHAN_AF3]); //AF3
						it->anodes.push_back(contactQuality[IEE_CHAN_AF4]); //AF4
						it->anodes.push_back(contactQuality[IEE_CHAN_T7]); //T7
						it->anodes.push_back(contactQuality[IEE_CHAN_T8]); //T8
						it->anodes.push_back(contactQuality[IEE_CHAN_Pz]); //PZ
					}
					else
					{
						it->anodes.push_back(0);
						it->anodes.push_back(0);
						it->anodes.push_back(0);
						it->anodes.push_back(0);
						it->anodes.push_back(0);
					}
				}

				EnterCriticalSection(&pEmotiv->m_critical);
				pEmotiv->WriteJsonStatus(pEmotiv);
				pEmotiv->WriteJsonAnodesStatus(pEmotiv);
				LeaveCriticalSection(&pEmotiv->m_critical);
			}

			userID = -1;
		} else {
			Sleep(1);
		}
	}
	
	return 0;
}



void Emotiv::WriteJsonStatus(LPVOID pParam)
{
	Json::Value root, player;
	Emotiv *pEmotiv = (Emotiv*)(pParam);
	vector<DataEmotiv>::iterator it = pEmotiv->players_.begin(), end = pEmotiv->players_.end();
	for(; it < end; it++)
	{
		Json::Value performanceMetrics;
		player["ID"] = it->id;
		player["current_action"] = it->current_action;
		//player["neutral_status"] = it->neutral_status;
		//player["running_status"] = it->running_status;
		player["power"] = it->power;
		player["skill"] = it->skill;
		performanceMetrics["stress"] = it->stress_score;
		performanceMetrics["engagement-boredom"] = it->engagement_boredom_score;
		performanceMetrics["excitement"] = it->excitement_score;
		performanceMetrics["relaxation"] = it->relaxation_score;
		performanceMetrics["interest"] = it->interest_score;
		player["metrics"] = performanceMetrics;
		root["players"].append(player);
	}
	json_status_ = root;
}

void Emotiv::WriteJsonAnodesStatus(LPVOID pParam)
{
	Json::Value root, player, anodes;

	Emotiv *pEmotiv = (Emotiv*)(pParam);
	vector<DataEmotiv>::iterator it = pEmotiv->players_.begin(), end = pEmotiv->players_.end();
	for(; it < end; it++)
	{
		player["ID"] = it->id;
		for(unsigned int i = 0; i < it->anodes.size(); i++)
			anodes.append(it->anodes[i]);

		player["anodes"]["AF3"] = anodes[0];
		player["anodes"]["AF4"] = anodes[1];
		player["anodes"]["T7"] = anodes[2];
		player["anodes"]["T8"] = anodes[3];
		player["anodes"]["PZ"] = anodes[4];
		player["active"] = it->noise;

		root["players"].append(player);
		anodes.clear();
	}

	json_anode_status_ = root;
}

Json::Value Emotiv::GetJsonAnodeStatus()
{
	return json_anode_status_;
}

Json::Value Emotiv::GetJsonStatus()
{
	return json_status_;
}

int Emotiv::GetCloudID()
{
	return userCloudID_;
}


//void Emotiv::SetMessage(Message* msg)
//{
//	pMessage_ = msg;
//}

vector<DataEmotiv>::iterator Emotiv::FindPlayer(int userId)
{
	return find_if(players_.begin(), players_.end(), [&](DataEmotiv const & d) {return d.id == userId; });
}

void Emotiv::RemovePlayer(int id)
{
	players_.erase(
		remove_if(players_.begin(), players_.end(), [&](DataEmotiv const & d)
		{
			return d.id == id;
		}),
		players_.end());
}

void Emotiv::SetUser(int userID)
{
	userID_ = userID;
}

Emotiv::Emotiv()
{
	userCloudID_		= -1;
	eEvent_				= IEE_EmoEngineEventCreate();
	eState_				= IEE_EmoStateCreate();
	bConnect_			= true;
	json_status_		= "{}";
	json_anode_status_  = "{}";
	InitializeCriticalSection(&m_critical);

	pThread_ = CreateThread(NULL, 0, DoPower, this, 0, 0);
}

Emotiv::~Emotiv()
{
	bConnect_ = false;
	WaitForSingleObject(pThread_, INFINITE);
	DeleteCriticalSection(&this->m_critical);
	Disconnect();
}

bool Emotiv::Disconnect()
{
	int ret = IEE_EngineDisconnect();
	IEE_EmoStateFree(eState_);
	IEE_EmoEngineEventFree(eEvent_);

	if (ret != EDK_OK)
		return false;

	return true;
}

bool Emotiv::DisconnectCloud()
{
	bool ret = EC_Logout(userCloudID_);
	EC_Disconnect();

	return ret;
}

bool Emotiv::ConnectCloud()
{
	int retCode = -1;
	retCode = EC_Connect();
	if( retCode != EDK_OK)
	{
		//TRACE("Cannot connect to Emotiv Cloud\n");
        return false;
	}

	retCode = EC_Login(USERNAME, PASSWORD);
	if( retCode != EDK_OK )
	{			
		//TRACE("Your login attempt has failed. The username or password may be incorrect\n");
        return false;
	}

	retCode = EC_GetUserDetail(&userCloudID_);
	if ( retCode != EDK_OK )
        return false;


	return true;
}

bool Emotiv::Connect()
{
	if (IEE_EngineConnect() != EDK_OK)
		return false;

	return true;
}

void Emotiv::AbortTraining(unsigned int userid)
{
	IEE_MentalCommandSetTrainingAction(userid, IEE_MentalCommandAction_t::MC_PUSH);
	IEE_MentalCommandSetTrainingControl(userid, IEE_MentalCommandTrainingControl_t::MC_REJECT);
}

void Emotiv::EraseTraining(unsigned int userid)
{
	IEE_MentalCommandSetTrainingAction(userid, IEE_MentalCommandAction_t::MC_NEUTRAL);
	IEE_MentalCommandSetTrainingControl(userid, IEE_MentalCommandTrainingControl_t::MC_ERASE);

	IEE_MentalCommandSetTrainingAction(userid, IEE_MentalCommandAction_t::MC_PUSH);
	IEE_MentalCommandSetTrainingControl(userid, IEE_MentalCommandTrainingControl_t::MC_ERASE);
}

int Emotiv::StartTraining(int action)
{
	int ret = -1;
	vector<DataEmotiv>::iterator it = FindPlayer(userID_);
	if(action == 0)
	{
		action = 1;
		it->current_training_action = 0;
	}
	else
	{
		action = 2;
		it->current_training_action = 1;
	}

	if (action == IEE_MentalCommandAction_t::MC_PUSH)
	{
		ret = IEE_MentalCommandSetActiveActions(userID_, IEE_MentalCommandAction_t::MC_PUSH);
		if(ret != EDK_OK)
		{
			return ret;
		}

		ret = IEE_MentalCommandSetTrainingAction(userID_, IEE_MentalCommandAction_t::MC_PUSH);
		if(ret != EDK_OK)
		{
			return ret;
		}
	}
	else if (action == IEE_MentalCommandAction_t::MC_NEUTRAL)
	{
		ret = IEE_MentalCommandSetTrainingAction(userID_, IEE_MentalCommandAction_t::MC_NEUTRAL);
		if(ret != EDK_OK)
		{
			return ret;
		}
	}

	ret = IEE_MentalCommandSetTrainingControl(userID_, IEE_MentalCommandTrainingControl_t::MC_START);
	if(ret != EDK_OK)
	{
		return -1;
	}

	return EDK_OK;
}

float Emotiv::GetOveralSkill()
{
	float skill = -1;
	int retCode = IEE_MentalCommandGetOverallSkillRating(userID_, &skill);
	return skill;
}

void Emotiv::handleMentalCommandEvent(LPVOID pParam)
{
	Emotiv *pEmotiv = (Emotiv*)(pParam);
	unsigned int userID = -1;
	std::ostringstream message;
	message.str("");

	IEE_EmoEngineEventGetUserId(pEmotiv->eEvent_, &userID);
    IEE_MentalCommandEvent_t eventType = IEE_MentalCommandEventGetType(pEmotiv->eEvent_);
	vector<DataEmotiv>::iterator it = pEmotiv->FindPlayer(userID);

	switch (eventType)
	{
		case IEE_MentalCommandTrainingStarted:
		{
			if(it->current_training_action == 0)
			{
				message << "Iniciando treinamento neutro para o usuario: " << userID;
				it->neutral_status = IEE_MentalCommandTrainingStarted;
			}
			else
			{
				message << "Iniciando treinamento corrida para o usuario: " << userID;
				it->running_status = IEE_MentalCommandTrainingStarted;
			}

			//pEmotiv->pMessage_->EmotivStatus(message);
			break;
		}

		case IEE_MentalCommandTrainingSucceeded:
		{
			if(it->current_training_action == 0)
			{
				message << "Treinamento neutro para o usuario: " << userID << " realizado com sucesso";
				it->neutral_status = IEE_MentalCommandTrainingSucceeded;
			}
			else
			{
				message << "Treinamento corrida para o usuario: " << userID << " realizado com sucesso";
				it->running_status = IEE_MentalCommandTrainingSucceeded;
			}
			
			//pEmotiv->pMessage_->EmotivStatus(message);
			IEE_MentalCommandSetTrainingControl(pEmotiv->userID_, MC_ACCEPT);
			break;
		}

		case IEE_MentalCommandTrainingFailed:
		{
			if(it->current_training_action == 0)
			{
				message << "Treinamento neutro para o usuario: " << userID << " falhou";
				it->neutral_status = IEE_MentalCommandTrainingFailed;
			}
			else
			{
				message << "Treinamento corrida para o usuario: " << userID << " falhou";
				it->running_status = IEE_MentalCommandTrainingFailed;
			}

			//pEmotiv->pMessage_->EmotivStatus(message);
			break;
		}

		case IEE_MentalCommandTrainingCompleted:
		{
			if(it->current_training_action == 0)
			{
				message << "Treinamento neutro para o usuario: " << userID << " completado com sucesso";
				it->neutral_status = IEE_MentalCommandTrainingCompleted;
			}
			else
			{
				message << "Treinamento corrida para o usuario: " << userID << " completado com sucesso";
				it->running_status = IEE_MentalCommandTrainingCompleted;
			}

			//pEmotiv->pMessage_->EmotivStatus(message);
			break;
		}

		case IEE_MentalCommandTrainingDataErased:
		{
			message << "Treinamento neutro e corrida para o usuario: " << userID << " apagado";
			it->neutral_status = IEE_MentalCommandTrainingDataErased;
			it->running_status = IEE_MentalCommandTrainingDataErased;
			it->power = 0.0;
			it->skill = 0.0;
			//pEmotiv->pMessage_->EmotivStatus(message);
			break;
		}

		case IEE_MentalCommandTrainingRejected:
		{
			if(it->current_training_action == 0)
			{
				message << "Treinamento neutro para o usuario: " << userID << " rejeitado";
				it->neutral_status = IEE_MentalCommandTrainingRejected;
			}
			else
			{
				message << "Treinamento corrida para o usuario: " << userID << " rejeitado";
				it->running_status = IEE_MentalCommandTrainingRejected;
			}

			//pEmotiv->pMessage_->EmotivStatus(message);
			break;
		}

		case IEE_MentalCommandTrainingReset:
		{
			if(it->current_training_action == 0)
			{
				message << "Treinamento neutro para o usuario: " << userID << " resetado";
				it->neutral_status = IEE_MentalCommandTrainingReset;
			}
			else
			{
				message << "Treinamento corrida para o usuario: " << userID << " resetado";
				it->running_status = IEE_MentalCommandTrainingReset;
			}
			
			//pEmotiv->pMessage_->EmotivStatus(message);
			break;
		}

		case IEE_MentalCommandAutoSamplingNeutralCompleted:
		{
			if(it->current_training_action == 0)
			{
				message << "Amostra automatica neutro para o usuario: " << userID;
				it->neutral_status = IEE_MentalCommandAutoSamplingNeutralCompleted;
			}

			break;
		}

		case IEE_MentalCommandSignatureUpdated:
		{
			if(it->current_training_action == 0)
			{
				message << "MentalCommand signature for user: " << userID;
				it->neutral_status = IEE_MentalCommandSignatureUpdated;
			}
			else
			{
				message << "MentalCommand signature for user: " << userID;
				it->running_status = IEE_MentalCommandSignatureUpdated;
			}
			break;
		}

		case IEE_MentalCommandNoEvent:
		{
			if(it->current_training_action == 0)
			{
				//pEmotiv->pMessage_->EmotivStatus("No event command");
				it->running_status = IEE_MentalCommandNoEvent;
			}
			else
			{
				//pEmotiv->pMessage_->EmotivStatus("No event command");
				it->neutral_status = IEE_MentalCommandNoEvent;
			}
			break;
		}

		default:
			break;
	}
}

