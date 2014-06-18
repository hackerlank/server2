/**

 * \brief ʵ��ͳһ�û�ƽ̨�ͻ������ӵĹ�������
 */
#include "BillServer.h"

zLogger *BillClientManager::tlogger = NULL;
int    BillClientManager::source = -1;

/**
 * \brief ���캯��
 */
BillClientManager::BillClientManager() : billClientPool(NULL),actionTimer(),maxID(0)
{
}

/**
 * \brief ��������
 */
BillClientManager::~BillClientManager()
{
  SAFE_DELETE(billClientPool);
}

/**
 * \brief ��ʼ��������
 * \param bc �Ʒѻص�����
 * \return ��ʼ���Ƿ�ɹ�
 */
bool BillClientManager::init(const std::string &confile,const std::string &tradelog,BillCallback &bc)
{
  Xlogger->debug("BillClientManager::init");
  this->bc.action = bc.action;

  billClientPool = new x_tcp_clientTaskPool(atoi(Seal::global["threadPoolClient"].c_str()));
  if (NULL == billClientPool
      || !billClientPool->init())
    return false;

  zXMLParser xml;
  if (!xml.initFile(confile))
  {
    Xlogger->error("����ͳһ�û�ƽ̨�Ʒѷ������б��ļ� %s ʧ��",confile.c_str());
    return false;
  }
  xmlNodePtr root = xml.getRootNode("Seal");
  if (root)
  {
    xmlNodePtr source_node = xml.getChildNode(root,"sourceID");
    xml.getNodeContentNum(source_node,&source,sizeof(source));
    xmlNodePtr zebra_node = xml.getChildNode(root,"BillServerList");
    while(zebra_node)
    {
      if (strcmp((char *)zebra_node->name,"BillServerList") == 0)
      {
        xmlNodePtr server_node = xml.getChildNode(zebra_node,"server");
        while(server_node)
        {
          if (strcmp((char *)server_node->name,"server") == 0)
          {
            xmlNodePtr node = xml.getChildNode(server_node,"entry");
            while(node)
            {
              if (strcmp((char *)node->name,"entry") == 0)
              {
                Seal::global["unifyBillServer"] = "";
                Seal::global["unifyBillPort"] = ""; 
                if (xml.getNodePropStr(node,"ip",Seal::global["unifyBillServer"])
                    && xml.getNodePropStr(node,"port",Seal::global["unifyBillPort"]))
                {
                  Xlogger->debug("unifyBillServer: %s,%s",
                      Seal::global["unifyBillServer"].c_str(),
                      Seal::global["unifyBillPort"].c_str());
                  billClientPool->put(new BillClient(Seal::global["unifyBillServer"],
                        atoi(Seal::global["unifyBillPort"].c_str()),this->bc,maxID));
                }
              }

              node = xml.getNextNode(node,NULL);
            }

            maxID++;
          }

          server_node = xml.getNextNode(server_node,NULL);
        }
      }

      zebra_node = xml.getNextNode(zebra_node,NULL);
    }
  }

  //��ʼ�����׼�¼��log
  BillClientManager::tlogger = new zLogger("ClientTrade");
  //���ý�����־����debug
  BillClientManager::tlogger->setLevel("debug");
  //����д������־�ļ�
  if ("" != tradelog)
    BillClientManager::tlogger->addLocalFileLog(tradelog);

  Xlogger->debug("tradelogfilename = %s,source = %d",tradelog.c_str(),source);

  return true;
}

/**
 * \brief ���ڼ���������ӵĶ�����������
 * \param ct ��ǰʱ��
 */
void BillClientManager::timeAction(const zTime &ct)
{
  if (actionTimer.elapse(ct) >= 4)
  {
    if (billClientPool)
      billClientPool->timeAction(ct);
    actionTimer = ct;
  }
}

/**
 * \brief ������������Ѿ��ɹ�������
 * \param billClient ����ӵ�����
 */
void BillClientManager::add(BillClient *billClient)
{
  if (billClient)
  {
    rwlock.wrlock();
    allClients.insert(billClient);
    rwlock.unlock();
  }
}

/**
 * \brief ���������Ƴ��Ͽ�������
 * \param billClient ���Ƴ�������
 */
void BillClientManager::remove(BillClient *billClient)
{
  if (billClient)
  {
    rwlock.wrlock();
    std::pair<iter,iter> sp = allClients.equal_range(billClient);
    for(iter it = sp.first; it != sp.second; ++it)
    {
      if (billClient == (*it))
      {
        allClients.erase(it);
        rwlock.unlock();
        return;
      }
    }
    rwlock.unlock();
  }
}

bool BillClientManager::action(BillData *bd)
{
  if (maxID)
  {
    rwlock.rdlock();
    for(iter it = allClients.begin(); it != allClients.end(); ++it)
    {
      if (bd->uid % maxID == (*it)->getID()
          && (*it)->action(bd))
      {
        rwlock.unlock();
        return true;
      }
    }
    rwlock.unlock();
  }
  return false;
}
void BillClientManager::execEvery()
{
  iter it;
  BillClient *task=NULL;
  rwlock.rdlock();
  it = allClients.begin();
  for (; it != allClients.end() ; it ++)
  {
    task=*it;
    task->doCmd();
  }
  rwlock.unlock();

}
