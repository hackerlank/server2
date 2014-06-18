/**
 * \brief zebra��ĿGateway������,�����û�ָ����ת�������ܽ��ܵ�
 */

#include "GatewayServer.h"

GatewayService *GatewayService::instance = NULL;
zTCPTaskPool * GatewayService::taskPool = NULL;
bool GatewayService::service_gold=true;
bool GatewayService::service_stock=true;
DWORD merge_version = 0;

/**
 * \brief ��ʼ���������������
 *
 * ʵ�����麯��<code>x_service::init</code>
 *
 * \return �Ƿ�ɹ�
 */
bool GatewayService::init()
{
  Xlogger->debug("GatewayService::init");
  verify_client_version = ZEBRA_CLIENT_VERSION;
  Xlogger->info("�������汾��:%d",verify_client_version);
  
  //���ع�������(��ͼ)��Ϣ
  if (!country_info.init())
  {
    Xlogger->error("���ص�ͼ����ʧ��!");
  }

  //��ʼ�������̳߳�
  int state = state_none;
  to_lower(Seal::global["threadPoolState"]);
  if ("repair" == Seal::global["threadPoolState"]
      || "maintain" == Seal::global["threadPoolState"])
    state = state_maintain;

  taskPool = new zTCPTaskPool(atoi(Seal::global["threadPoolServer"].c_str()),state,65000);
  if (NULL == taskPool
      || !taskPool->init())
    return false;

  strncpy(pstrIP,x_socket::getIPByIfName(Seal::global["ifname"].c_str()),MAX_IP_LENGTH - 1);
  Xlogger->info("GatewayService::init(%s)",pstrIP);

  if (!x_subnetservice::init())
  {
    return false;
  }

  const Cmd::Super::ServerEntry *serverEntry = NULL;
  
  //���ӻỰ������
  serverEntry = getServerEntryByType(SESSIONSERVER);
  if (NULL == serverEntry)
  {
    Xlogger->error("�����ҵ��Ự�����������Ϣ,�������ӻỰ������");
    return false;
  }
  sessionClient = new SessionClient("�Ự�������ͻ���",serverEntry->pstrIP,serverEntry->wdPort);
  if (NULL == sessionClient)
  {
    Xlogger->error("û���㹻�ڴ�,���ܽ����Ự�������ͻ���ʵ��");
    return false;
  }
  if (!sessionClient->connectToSessionServer())
  {
    Xlogger->error("GatewayService::init ���ӻỰ������ʧ��");
    //return false;
  }
  sessionClient->start();

  //���ӼƷѷ�����
  serverEntry = getServerEntryByType(BILLSERVER);
  if (NULL == serverEntry)
  {
    Xlogger->error("�����ҵ��Ʒѷ����������Ϣ,�������ӼƷѷ�����");
    return false;
  }
  accountClient = new BillClient("�Ʒѷ������ͻ���",serverEntry->pstrIP,serverEntry->wdPort,serverEntry->wdServerID);
  if (NULL == accountClient)
  {
    Xlogger->error("û���㹻�ڴ�,���ܽ����Ʒѷ������ͻ���ʵ��");
    return false;
  }
  if (!accountClient->connectToBillServer())
  {
    Xlogger->error("GatewayService::init ���ӼƷѷ�����ʧ��");
    return false;
  }
  accountClient->start();

  //�������еĳ���������
  serverEntry = getServerEntryByType(SCENESSERVER);
  if (serverEntry)
  {
    if (!SceneClientManager::getInstance().init())
      return false;
  }  

  //�������еĵ���������
  serverEntry = getServerEntryByType(RECORDSERVER);
  if (NULL == serverEntry)
  {
    Xlogger->error("�����ҵ����������������Ϣ,�������ӵ���������");
    return false;
  }
  recordClient = new RecordClient("�����������ͻ���",serverEntry->pstrIP,serverEntry->wdPort);
  if (NULL == recordClient)
  {
    Xlogger->error("û���㹻�ڴ�,���ܽ��������������ͻ���ʵ��");
    return false;
  }
  if (!recordClient->connectToRecordServer())
  {
    Xlogger->error("GatewayService::init ���ӵ���������ʧ��");
    return false;
  }
  recordClient->start();

  //����С��Ϸ������
  serverEntry = getServerEntryByType(MINISERVER);
  if (NULL == serverEntry)
  {
    Xlogger->error("�����ҵ�С��Ϸ�����������Ϣ,��������С��Ϸ������");
    return false;
  }
  miniClient = new MiniClient("С��Ϸ�������ͻ���",serverEntry->pstrIP,serverEntry->wdPort,serverEntry->wdServerID);
  if (NULL == miniClient)
  {
    Xlogger->error("û���㹻�ڴ�,���ܽ���С��Ϸ�������ͻ���ʵ��");
    return false;
  }
  if (!miniClient->connectToMiniServer())
  {
    Xlogger->error("GatewayService::init ����С��Ϸ������ʧ��");
    return false;
  }
  miniClient->start();

  if (!GateUserManager::getInstance()->init())
    return false;

  GatewayTimeTick::getInstance().start();

  Xlogger->debug("��ʼ���ɹ���ɣ�");
  return true;
}

/**
 * \brief �½���һ����������
 *
 * ʵ�ִ��麯��<code>x_netservice::newTCPTask</code>
 *
 * \param sock TCP/IP����
 * \param addr ��ַ
 */
void GatewayService::newTCPTask(const SOCKET sock,const struct sockaddr_in *addr)
{
  Xlogger->debug("GatewayService::newTCPTask");
  GatewayTask *tcpTask = new GatewayTask(taskPool,sock,addr);
  if (NULL == tcpTask)
    //�ڴ治��,ֱ�ӹر�����
    ::close(sock);
  else if (!taskPool->addVerify(tcpTask))
  {
    //�õ���һ����ȷ����,��ӵ���֤������
    SAFE_DELETE(tcpTask);
  }
}

bool GatewayService::notifyLoginServer()
{
  Xlogger->debug("GatewayService::notifyLoginServer");
  using namespace Cmd::Super;
  t_GYList_Gateway tCmd;

  tCmd.wdServerID = wdServerID;
  tCmd.wdPort     = wdPort;
  strncpy(tCmd.pstrIP,pstrIP,sizeof(tCmd.pstrIP));
  if (!GatewayService::getInstance().isTerminate())
  {
    tCmd.wdNumOnline = getPoolSize();
    printf("����Ŀǰ��������:%d\n",tCmd.wdNumOnline);
  }
  else
  {
    tCmd.wdNumOnline = 0;
  }
  tCmd.state = getPoolState();
  tCmd.zoneGameVersion = verify_client_version;

  return sendCmdToSuperServer(&tCmd,sizeof(tCmd));
}

/**
 * \brief �������Թ����������ָ��
 *
 * ��Щָ�������غ͹��������������ָ��<br>
 * ʵ�����麯��<code>x_subnetservice::msgParse_SuperService</code>
 *
 * \param pNullCmd ��������ָ��
 * \param nCmdLen ��������ָ���
 * \return �����Ƿ�ɹ�
 */
bool GatewayService::msgParse_SuperService(const Cmd::t_NullCmd *pNullCmd,const DWORD nCmdLen)
{
  using namespace Cmd::Super;

  if (CMD_GATEWAY == pNullCmd->cmd)
  {
    switch(pNullCmd->para)
    {
      case PARA_GATEWAY_RQGYLIST:
         Xlogger->info("PARA_GATEWAY_RQGYLIST");
                 return notifyLoginServer();
	  case PARA_NOTIFYGATE_FINISH:
		  {
				if(!startUpFinish)
					startUpFinish = true;
		  }
		  break;
      case PARA_CHARNAME_GATEWAY:
        {
          t_Charname_Gateway *rev = (t_Charname_Gateway *)pNullCmd;

                    Xlogger->info("PARA_CHARNAME_GATEWAY");
          GateUser *pUser=GateUserManager::getInstance()->getUserByAccID(rev->accid);
          if (pUser
              && pUser->isCreateState()
              && rev->state & ROLEREG_STATE_TEST)
          {
            if (rev->state & ROLEREG_STATE_HAS)
            {
              //������ɫʧ��,��ɫ�����ظ�
              pUser->nameRepeat();
              Xlogger->warn("��ɫ���ظ� GatewayService::msgParse_SuperService");
            }
            else
            {
              if (!recordClient->sendCmd(&pUser->createCharCmd,sizeof(pUser->createCharCmd)))
                return false;
            }
          }

          return true;
        }
        break;
    }
  }

  Xlogger->error("GatewayService::msgParse_SuperService(%d,%d,%d)",pNullCmd->cmd,pNullCmd->para,nCmdLen);
  return false;
}

/**
 * \brief �������������
 *
 * ʵ���˴��麯��<code>x_service::final</code>
 *
 */
void GatewayService::final()
{
  Xlogger->debug("GatewayService::final");
  GatewayTimeTick::getInstance().final();
  GatewayTimeTick::getInstance().join();
  GatewayTimeTick::delInstance();
  GateUserManager::getInstance()->removeAllUser(); 

  if (taskPool)
  {
    taskPool->final();
    SAFE_DELETE(taskPool);
  }
  //�������taskPool֮����,�����down��
  //SceneClientManager::getInstance().final();
  SceneClientManager::delInstance();
  // */
  if (sessionClient)
  {
    sessionClient->final();
    sessionClient->join();
    SAFE_DELETE(sessionClient);
  }
  if (recordClient)
  {
    recordClient->final();
    recordClient->join();
    SAFE_DELETE(recordClient);
  }
  x_subnetservice::final();

  GatewayTaskManager::delInstance();

  LoginSessionManager::delInstance();

  GateUserManager::delInstance();

}

/**
 * \brief ��ȡ�����ļ�
 *
 */
class GatewayConfile:public zConfile
{
  bool parseYour(const xmlNodePtr node)
  {
    if (node)
    {
      xmlNodePtr child=parser.getChildNode(node,NULL);
      while(child)
      {
        parseNormal(child);
        child=parser.getNextNode(child,NULL);
      }
      return true;
    }
    else
      return false;
  }
};

/**
 * \brief ���¶�ȡ�����ļ�,ΪHUP�źŵĴ�����
 *
 */
void GatewayService::reloadConfig()
{
  Xlogger->debug("GatewayService::reloadConfig");
  GatewayConfile gc;
  gc.parse("GatewayServer");
  if ("true" == Seal::global["rolereg_verify"])
    GatewayService::getInstance().rolereg_verify = true;
  else
    GatewayService::getInstance().rolereg_verify = false;
  
  //ָ���⿪��
  if (Seal::global["cmdswitch"] == "true")
  {
    zTCPTask::analysis._switch = true;
    x_tcp_client::analysis._switch=true;
  }
  else
  {
    zTCPTask::analysis._switch = false;
    x_tcp_client::analysis._switch=false;
  }
  
  if (!country_info.reload())
  {
    Xlogger->error("���¼��ع�������!");
  }

  merge_version = atoi(Seal::global["merge_version"].c_str());
#ifdef _DEBUG
  Xlogger->debug("[����]: ���¼��غ����汾��",merge_version);
#endif  
}

/**
 * \brief ���������
 *
 * \param argc ��������
 * \param argv �����б�
 * \return ���н��
 */
int main(int argc,char **argv)
{
  Xlogger=new zLogger("GatewayServer");

  //����ȱʡ����
  Seal::global["countryorder"] = "0";

  //���������ļ�����
  GatewayConfile gc;
  if (!gc.parse("GatewayServer"))
    return EXIT_FAILURE;

  //������־����
  Xlogger->setLevel(Seal::global["log"]);
  //����д������־�ļ�
  if ("" != Seal::global["logfilename"]){
    Xlogger->addLocalFileLog(Seal::global["logfilename"]);
    Xlogger->removeConsoleLog();
    }

  //Xlogger->debug("%s",Seal::global["rolereg_verify"].c_str());
  if ("true" == Seal::global["rolereg_verify"])
    GatewayService::getInstance().rolereg_verify = true;
  else
    GatewayService::getInstance().rolereg_verify = false;
  //ָ���⿪��
  if (Seal::global["cmdswitch"] == "true")
  {
    zTCPTask::analysis._switch = true;
    x_tcp_client::analysis._switch=true;
  }
  else
  {
    zTCPTask::analysis._switch = false;
    x_tcp_client::analysis._switch=false;
  }

  merge_version = atoi(Seal::global["merge_version"].c_str());

  Seal_Startup();
  
  GatewayService::getInstance().main();
  //GatewayService::delInstance();

  return EXIT_SUCCESS;
}

