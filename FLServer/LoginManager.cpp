#include "LoginManager.h"
#include "FLServer.h"
#include "LoginTask.h"
#include "flcmd.h"
#include "GYListManager.h"

#define MAX_GATEWAYUSER 4000

LoginManager *LoginManager::instance = NULL;
DWORD LoginManager::maxGatewayUser=MAX_GATEWAYUSER;

bool LoginManager::add(shared_ptr<LoginTask> task) {
	Xlogger->debug("LoginManager::add");
	if (task) {
		task->genTempID();

		boost::mutex::scoped_lock scope_lock(mlock);

		LoginTaskHashmap_const_iterator it = loginTaskSet.find(task->getTempID());
		if (it != loginTaskSet.end()) {
			return false;
		}
		loginTaskSet.insert(LoginTaskHashmap_pair(task->getTempID(),task));
		return true;
	}
	else
		return false;
}

bool LoginManager::remove(shared_ptr<LoginTask> task) {
	Xlogger->debug("LoginManager::remove");
	if (task) {
		boost::mutex::scoped_lock scope_lock(mlock);
		loginTaskSet.erase(task->getTempID());
		return true;
	}
	return false;
}

bool LoginManager::broadcast(const DWORD loginTempID,const void *pstrCmd,int nCmdLen) {
	Xlogger->debug("LoginManager::broadcast");
	boost::mutex::scoped_lock scope_lock(mlock);
	LoginTaskHashmap_iterator it = loginTaskSet.find(loginTempID);
	if (it != loginTaskSet.end())
		return it->second->sendCmd(pstrCmd,nCmdLen);
	else
		return false;
}

void LoginManager::verifyReturn(const Cmd::t_NewLoginSession &session)
{
	boost::mutex::scoped_lock scope_lock(mlock);

	LoginTaskHashmap_iterator it = loginTaskSet.find(session.loginTempID);
	if (it != loginTaskSet.end())
	{
		shared_ptr<LoginTask> task = it->second;

		switch(session.state)
		{
			case 0:
				{
					Xlogger->debug("login success ,allocator gateway :gameZone = %u",session.gameZone);
					GYList * gy = GYListManager::getInstance().getAvl(session.gameZone);
					if (NULL == gy) {
						task->LoginReturn(Cmd::LOGIN_RETURN_GATEWAYNOTAVAILABLE);
						Xlogger->debug("gateway is not open");
					}
					else if (gy->wdNumOnline >= (maxGatewayUser - 10)) {
						task->LoginReturn(Cmd::LOGIN_RETURN_USERMAX);
						Xlogger->error("user num is full, current num = %d",gy->wdNumOnline);
					}
					else
					{
						//CEncrypt                      e;
						Cmd::FL::t_NewSession_Session tCmd;

						// [ranqd delete] δ�ɹ���½���û����������ȴ����ط���
						//	gy->wdNumOnline++;

						tCmd.session             = session;
						tCmd.session.wdGatewayID = gy->wdServerID;

						//����des������Կ                
						//e.random_key_des(&tCmd.session.des_key);
						/*
						   Xlogger->info("������Կ:%02x %02x %02x %02x %02x %02x %02x %02x",tCmd.session.des_key[0],tCmd.session.des_key[1],tCmd.session.des_key[2],tCmd.session.des_key[3],tCmd.session.des_key[4],tCmd.session.des_key[5],tCmd.session.des_key[6],tCmd.session.des_key[7]);
						 */
						/*
						   for (int i=0; i<sizeof(tCmd.session.des_key); i++)
						   tCmd.session.des_key[i] = (DWORD)randBetween(0,255);
						 */
						//bcopy(gy->pstrIP,tCmd.session.pstrIP,sizeof(tCmd.session.pstrIP),sizeof(tCmd.session.pstrIP));
						tCmd.session.dwpstrIP = gy->dwpstrIP;
						tCmd.session.wdPort = gy->wdPort;
						//ServerManager::getInstance().broadcast(session.gameZone,&tCmd,sizeof(tCmd));
					}
				}
				break;
			case 1:
				//�ʺŴ�������״̬
				task->LoginReturn(Cmd::LOGIN_RETURN_LOCK);
				Xlogger->error("�ʺ�������");
				break;
			case 4:
				//�ʺŴ��ڴ�����״̬
				task->LoginReturn(Cmd::LOGIN_RETURN_WAITACTIVE);
				Xlogger->error("�ʺŴ�����");
				break;
			default:
				break;
		}
	}
}

/**
* \brief ���ش�����뵽ָ���Ŀͻ���
* \param loginTempID ָ���Ŀͻ���������ʱ���
* \param retcode �����صĴ���
* \param tm ������Ϣ�Ժ��Ƿ�Ͽ�����,ȱʡ�ǶϿ�����
*/
void LoginManager::loginReturn(const DWORD loginTempID,const BYTE retcode,const bool tm)
{
	Xlogger->debug("LoginManager::loginReturn");
	boost::mutex::scoped_lock scope_lock(mlock);
	LoginTaskHashmap_iterator it = loginTaskSet.find(loginTempID);
	if (it != loginTaskSet.end())
		it->second->LoginReturn(retcode,tm);
}

/**
* \brief �������е�����Ԫ�ص��ûص�����
* \param cb �ص�����ʵ��
*/
void LoginManager::execAll(LoginTaskCallback &cb)
{
	Xlogger->debug("LoginManager::execAll");
	boost::mutex::scoped_lock scope_lock(mlock);
	for(LoginTaskHashmap_iterator it = loginTaskSet.begin(); it != loginTaskSet.end(); ++it)
	{
		cb.exec(it->second.get());
	}
}

