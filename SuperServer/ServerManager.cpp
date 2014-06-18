#include "ServerManager.h"

#include "ServerTask.h"
//#include "SuperServer.h"

ServerManager *ServerManager::instance = NULL;

void ServerManager::addServer(shared_ptr<ServerTask> task) {
	if (task) {
		boost::mutex::scoped_lock lock(mutex_container);
		container.push_front(task);
	}
}

void ServerManager::removeServer(shared_ptr<ServerTask> task) {
	if (task) {
		boost::mutex::scoped_lock lock(mutex_container);
		container.remove(task);
	}
}

shared_ptr<ServerTask> ServerManager::getServer(WORD wdServerID) {
	Containter_const_iterator it;
	shared_ptr<ServerTask> retval;

	boost::mutex::scoped_lock lock(mutex_container);
	for(it = container.begin(); it != container.end(); it++) {
		if ((*it)->getID() == wdServerID) {
			retval = *it;
			break;
		}
	}

	return retval;
}

bool ServerManager::uniqueAdd(shared_ptr<ServerTask> task) {
	boost::mutex::scoped_lock lock(mutex_hashmap);

	ServerTaskHashmap_const_iterator it = taskUniqueContainer.find(task->getID());
	if (it != taskUniqueContainer.end()) {
		Xlogger->error("ServerManager::uniqueAdd");
		return false;
	}
	taskUniqueContainer.insert(ServerTaskHashmap_pair(task->getID(),task));
	return true;
}

/**
 * \brief ��֤����������Ƿ��Ѿ�����
 *
 * \param wdServerID ���������
 * \return ��֤�Ƿ�ɹ�
 */
bool ServerManager::uniqueVerify(const WORD wdServerID)
{
	//less error probe
	boost::mutex::scoped_lock lock(mutex_hashmap);

	ServerTaskHashmap_const_iterator it = taskUniqueContainer.find(wdServerID);
	if (it != taskUniqueContainer.end())
	{
		return false;
	}
	return true;
}

/**
 * \brief ��Ψһ��������ɾ��һ����������
 *
 * \param task ��������������
 * \return ɾ���Ƿ�ɹ�
 */
bool ServerManager::uniqueRemove(shared_ptr<ServerTask> task)
{
	boost::mutex::scoped_lock lock(mutex_hashmap);

	ServerTaskHashmap_iterator it = taskUniqueContainer.find(task->getID());
	if (it != taskUniqueContainer.end())
	{
		taskUniqueContainer.erase(it);
		return true;
	}
	else
	{
		Xlogger->error("ServerManager::uniqueRemove");
		return false;
	}
}

/**
 * \brief �����������еķ������㲥ָ��
 *
 * \param pstrCmd ���㲥��ָ��
 * \param nCmdLen ָ���
 * \return �㲥�Ƿ�ɹ�
 */
bool ServerManager::broadcast(const void *pstrCmd,int nCmdLen)
{
	bool retval = true;

	boost::mutex::scoped_lock lock(mutex_container);
	for(Containter_const_iterator it = container.begin(); it != container.end(); it++)
	{
		if (!(*it)->sendCmd(pstrCmd,nCmdLen))
			retval = false;
	}

	return retval;
}

/**
 * \brief ���ݷ�������Ź㲥ָ��
 *
 * \param wdServerID ���㲥ָ��ķ��������
 * \param pstrCmd ���㲥��ָ��
 * \param nCmdLen ָ���
 * \return �㲥�Ƿ�ɹ�
 */
bool ServerManager::broadcastByID(const WORD wdServerID,const void *pstrCmd,int nCmdLen)
{
	bool retval = false;

	boost::mutex::scoped_lock lock(mutex_container);
	for(Containter_const_iterator it = container.begin(); it != container.end(); it++)
	{
		if ((*it)->getID() == wdServerID)
		{
			retval = (*it)->sendCmd(pstrCmd,nCmdLen);
			break;
		}
	}

	return retval;
}

/**
 * \brief ���ݷ��������͹㲥ָ��
 *
 * \param wdType ���㲥ָ��ķ���������
 * \param pstrCmd ���㲥��ָ��
 * \param nCmdLen ָ���
 * \return �㲥�Ƿ�ɹ�
 */
bool ServerManager::broadcastByType(const WORD wdType,const void *pstrCmd,int nCmdLen)
{
	bool retval = true;

	boost::mutex::scoped_lock lock(mutex_container);
	for(Containter_const_iterator it = container.begin(); it != container.end(); it++)
	{    
		if ((*it)->getType() == wdType && !(*it)->sendCmd(pstrCmd,nCmdLen))
			retval = false;
	}

	return retval;
}

/**
 * \brief ͳ��һ��������������
 * \return �õ�һ�����ĵ�ǰ����������
 */
const DWORD ServerManager::caculateOnlineNum()
{
	boost::mutex::scoped_lock lock(mutex_container);

	DWORD retval = 0;
	for(Containter_const_iterator it = container.begin(); it != container.end(); it++)
	{
		if ((*it)->getType() == GATEWAYSERVER)
			retval += (*it)->getOnlineNum();
	}

	return retval;
}

/**
 * \brief �յ�notifyOther�ظ�
 * \param srcID Դ���������
 * \param wdServerID Ŀ�ķ��������
 */
void ServerManager::responseOther(const WORD srcID,const WORD wdServerID) {
	boost::mutex::scoped_lock lock(mutex_hashmap);

	ServerTaskHashmap_const_iterator it = taskUniqueContainer.find(srcID);
	if (it != taskUniqueContainer.end()) {
		if (it->second)
			it->second->responseOther(wdServerID);
	}
	else {
		Xlogger->error("ServerManager::responseOther find srcid=%u",srcID);
	}
}

/*
void ServerManager::execEvery() {
	boost::mutex::scoped_lock lock(mutex);
	for(Containter_const_iterator it = container.begin(); it != container.end(); it++) {
		(*it)->doCmd();
	}
}
*/

void ServerManager::checkSequence(){
	//add lock will cause dead lock, later deal it
	boost::mutex::scoped_lock lock(mutex_hashmap);

	ServerTaskHashmap_iterator it = taskUniqueContainer.begin();
	while (it != taskUniqueContainer.end()) {
		shared_ptr<ServerTask> ptr = it->second;
		if (ptr->get_state() == SYNC_WAITING) {
			Xlogger->debug("%s",__PRETTY_FUNCTION__);
			ptr->checkDependency();
		}
		++it;
	}
}
