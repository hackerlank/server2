/**
 * \brief 定义服务器信息采集连接的客户端管理容器
 */


#include "SuperServer.h"

/**
 * \brief 类的唯一实例指针
 */
InfoClientManager *InfoClientManager::instance = NULL;

/**
 * \brief 构造函数
 */
InfoClientManager::InfoClientManager()
{
  infoClientPool = NULL;
}

/**
 * \brief 析构函数
 */
InfoClientManager::~InfoClientManager()
{
  SAFE_DELETE(infoClientPool);
}

/**
 * \brief 初始化管理器
 * \return 初始化是否成功
 */
bool InfoClientManager::init()
{
  infoClientPool = new x_tcp_clientTaskPool(atoi(Seal::global["threadPoolClient"].c_str()));
  if (NULL == infoClientPool
      || !infoClientPool->init())
    return false;

  zXMLParser xml;
  if (!xml.initFile(Seal::global["confdir"] + "loginServerList.xml"))
  {
    Xlogger->error("加载统一用户平台InfoServer列表文件失败");
    return false;
  }
  xmlNodePtr root = xml.getRootNode("Seal");
  if (root)
  {
    xmlNodePtr zebra_node = xml.getChildNode(root,"InfoServerList");
    while(zebra_node)
    {
      if (strcmp((char *)zebra_node->name,"InfoServerList") == 0)
      {
        xmlNodePtr node = xml.getChildNode(zebra_node,"server");
        while(node)
        {
          if (strcmp((char *)node->name,"server") == 0)
          {
            std::string InfoServer,InfoPort;
            
            InfoServer = "";
            InfoPort   = "";
            if (xml.getNodePropStr(node,"ip",InfoServer)
                && xml.getNodePropStr(node,"port",InfoPort))
            {
              Xlogger->debug("InfoServer: %s,%s",InfoServer.c_str(),InfoPort.c_str());
              infoClientPool->put(new InfoClient(InfoServer,atoi(InfoPort.c_str())));
            }
          }

          node = xml.getNextNode(node,NULL);
        }
      }

      zebra_node = xml.getNextNode(zebra_node,NULL);
    }
  }

  Xlogger->info("加载统一用户平台InfoServer列表文件成功");
  return true;
}

/**
 * \brief 周期间隔进行连接的断线重连工作
 * \param ct 当前时间
 */
void InfoClientManager::timeAction(const zTime &ct)
{
  if (actionTimer.elapse(ct) > 4)
  {
    if (infoClientPool)
      infoClientPool->timeAction(ct);
    actionTimer = ct;
  }
}

/**
 * \brief 向容器中添加已经成功的连接
 * \param infoClient 待添加的连接
 */
void InfoClientManager::add(InfoClient *infoClient)
{
  if (infoClient)
  {
    zRWLock_scope_wrlock scope_wrlock(rwlock);
    const_iter it = allClients.find(infoClient->getTempID());
    if (it == allClients.end())
    {
      allClients.insert(value_type(infoClient->getTempID(),infoClient));
      setter.insert(infoClient);
    }
  }
}

/**
 * \brief 从容器中移除断开的连接
 * \param infoClient 待移除的连接
 */
void InfoClientManager::remove(InfoClient *infoClient)
{
  if (infoClient)
  {
    zRWLock_scope_wrlock scope_wrlock(rwlock);
    iter it = allClients.find(infoClient->getTempID());
    if (it != allClients.end())
    {
      allClients.erase(it);
      setter.erase(infoClient);
    }
  }
}

/**
 * \brief 向成功的所有连接广播指令
 * \param pstrCmd 待广播的指令
 * \param nCmdLen 待广播指令的长度
 */
bool InfoClientManager::broadcastOne(const void *pstrCmd,int nCmdLen)
{
  zRWLock_scope_rdlock scope_rdlock(rwlock);
  for(InfoClient_set::iterator it = setter.begin(); it != setter.end(); ++it)
  {
    if ((*it)->sendCmd(pstrCmd,nCmdLen))
      return true;
  }
  return false;
}

bool InfoClientManager::sendTo(const DWORD tempid,const void *pstrCmd,int nCmdLen)
{
  zRWLock_scope_rdlock scope_rdlock(rwlock);
  iter it = allClients.find(tempid);
  if (it == allClients.end())
    return false;
  else
    return it->second->sendCmd(pstrCmd,nCmdLen);
}

