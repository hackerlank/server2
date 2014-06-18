/**
 * \brief ʵ��ͳһ�û�ƽ̨�ͻ������ӵĹ�������
 */


#include "SuperServer.h"

/**
 * \brief ���Ψһʵ��ָ��
 */
FLClientManager *FLClientManager::instance = NULL;

/**
 * \brief ���캯��
 */
FLClientManager::FLClientManager()
{
  flClientPool = NULL;
}

/**
 * \brief ��������
 */
FLClientManager::~FLClientManager()
{
  SAFE_DELETE(flClientPool);
}

/**
 * \brief ��ʼ��������
 * \return ��ʼ���Ƿ�ɹ�
 */
bool FLClientManager::init()
{
  Xlogger->debug("FLClientManager::init");
  flClientPool = new x_tcp_clientTaskPool(atoi(Seal::global["threadPoolClient"].c_str()));
  if (NULL == flClientPool
      || !flClientPool->init())
    return false;

  zXMLParser xml;
  if (!xml.initFile(Seal::global["confdir"] + "loginServerList.xml"))
  {
    Xlogger->error("����ͳһ�û�ƽ̨��½�������б��ļ�ʧ��");
    return false;
  }
  xmlNodePtr root = xml.getRootNode("Seal");
  if (root)
  {
    xmlNodePtr zebra_node = xml.getChildNode(root,"LoginServerList");
    while(zebra_node)
    {
      if (strcmp((char *)zebra_node->name,"LoginServerList") == 0)
      {
        xmlNodePtr node = xml.getChildNode(zebra_node,"server");
        while(node)
        {
          if (strcmp((char *)node->name,"server") == 0)
          {
              std::string FLServer,FLPort;

            FLServer = "";
            FLPort   = "";
            if (xml.getNodePropStr(node,"ip",FLServer)
                && xml.getNodePropStr(node,"port",FLPort))
            {
              Xlogger->debug("LoginServer: %s,%s",FLServer.c_str(),FLPort.c_str());
              flClientPool->put(new FLClient(FLServer,atoi(FLPort.c_str())));
            }
          }

          node = xml.getNextNode(node,NULL);
        }
      }

      zebra_node = xml.getNextNode(zebra_node,NULL);
    }
  }

  Xlogger->info("����ͳһ�û�ƽ̨��½�������б��ļ��ɹ�");
  return true;
}

/**
 * \brief ���ڼ���������ӵĶ�����������
 * \param ct ��ǰʱ��
 */
void FLClientManager::timeAction(const zTime &ct)
{
  Xlogger->debug("FLClientManager::timeAction");
  if (actionTimer.elapse(ct) > 4)
  {
    if (flClientPool)
      flClientPool->timeAction(ct);
    actionTimer = ct;
  }
}

/**
 * \brief ������������Ѿ��ɹ�������
 * \param flClient ����ӵ�����
 */
void FLClientManager::add(FLClient *flClient)
{
  Xlogger->debug("FLClientManager::add");
  if (flClient)
  {
    zRWLock_scope_wrlock scope_wrlock(rwlock);
    const_iter it = allClients.find(flClient->getTempID());
    if (it == allClients.end())
    {
      allClients.insert(value_type(flClient->getTempID(),flClient));
    }
  }
}

/**
 * \brief ���������Ƴ��Ͽ�������
 * \param flClient ���Ƴ�������
 */
void FLClientManager::remove(FLClient *flClient)
{
  Xlogger->debug("FLClientManager::remove");
  if (flClient)
  {
    zRWLock_scope_wrlock scope_wrlock(rwlock);
    iter it = allClients.find(flClient->getTempID());
    if (it != allClients.end())
    {
      allClients.erase(it);
    }
  }
}

/**
 * \brief ��ɹ����������ӹ㲥ָ��
 * \param pstrCmd ���㲥��ָ��
 * \param nCmdLen ���㲥ָ��ĳ���
 */
void FLClientManager::broadcast(const void *pstrCmd,int nCmdLen)
{
  Xlogger->debug("FLClientManager::broadcast");
  zRWLock_scope_rdlock scope_rdlock(rwlock);
  for(iter it = allClients.begin(); it != allClients.end(); ++it)
  {
    it->second->sendCmd(pstrCmd,nCmdLen);
  }
}

/**
 * \brief ��ָ���ĳɹ����ӹ㲥ָ��
 * \param tempid ���㲥ָ���������ʱ���
 * \param pstrCmd ���㲥��ָ��
 * \param nCmdLen ���㲥ָ��ĳ���
 */
void FLClientManager::sendTo(const WORD tempid,const void *pstrCmd,int nCmdLen)
{
  Xlogger->debug("FLClientManager::sendTo");
  zRWLock_scope_rdlock scope_rdlock(rwlock);
  iter it = allClients.find(tempid);
  if (it != allClients.end())
  {
    it->second->sendCmd(pstrCmd,nCmdLen);
  }
}

