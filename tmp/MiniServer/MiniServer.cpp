/**
 * \brief zebra��Ŀ�Ʒѷ�����
 *
 */

#include "MiniServer.h"

zDBConnPool *MiniService::dbConnPool = NULL;
MiniService *MiniService::instance = NULL;
DBMetaData* MiniService::metaData = NULL;

/**
 * \brief ��ʼ���������������
 *
 * ʵ�����麯��<code>x_service::init</code>
 *
 * \return �Ƿ�ɹ�
 */
bool MiniService::init()
{
  dbConnPool = zDBConnPool::newInstance(NULL);
  if (NULL == dbConnPool
      || !dbConnPool->putURL(0,Seal::global["mysql"].c_str(),false))
  {
    MessageBox(NULL,"�������ݿ�ʧ��","MiniServer",MB_ICONERROR);
    return false;
  }

  metaData = DBMetaData::newInstance("");

  if (NULL == metaData
      || !metaData->init(Seal::global["mysql"]))
  {
    MessageBox(NULL,"�������ݿ�ʧ��","MiniServer",MB_ICONERROR);
    return false;
  }

  //��ʼ�������̳߳�
  int state = state_none;
  to_lower(Seal::global["threadPoolState"]);
  if ("repair" == Seal::global["threadPoolState"]
      || "maintain" == Seal::global["threadPoolState"])
    state = state_maintain;
  taskPool = new zTCPTaskPool(atoi(Seal::global["threadPoolServer"].c_str()),state);
  if (NULL == taskPool
      || !taskPool->init())
    return false;

  strncpy(pstrIP,x_socket::getIPByIfName(Seal::global["ifname"].c_str()),MAX_IP_LENGTH - 1);

  MiniTimeTick::getInstance().start();
  if (!MiniHall::getMe().init()) return false;

  if (!x_subnetservice::init())
    return false;

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
void MiniService::newTCPTask(const SOCKET sock,const struct sockaddr_in *addr)
{
  MiniTask *tcpTask = new MiniTask(taskPool,sock,addr);
  if (NULL == tcpTask)
    //�ڴ治�㣬ֱ�ӹر�����
    ::close(sock);
  else if (!taskPool->addVerify(tcpTask))
  {
    //�õ���һ����ȷ���ӣ���ӵ���֤������
    SAFE_DELETE(tcpTask);
  }
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
bool MiniService::msgParse_SuperService(const Cmd::t_NullCmd *pNullCmd,const DWORD nCmdLen)
{
  using namespace Cmd::Super;

  return true;
}

/**
 * \brief �������������
 *
 * ʵ���˴��麯��<code>x_service::final</code>
 *
 */
void MiniService::final()
{
  MiniTimeTick::getInstance().final();
  MiniTimeTick::getInstance().join();
  MiniTimeTick::delInstance();

  if (taskPool)
  {
    SAFE_DELETE(taskPool);
  }

  MiniTaskManager::delInstance();
  MiniUserManager::delInstance();

  x_subnetservice::final();

  zDBConnPool::delInstance(&dbConnPool);
  Xlogger->debug("MiniService::final");
}

/**
 * \brief ��ȡ�����ļ�
 *
 */
class MiniConfile:public zConfile
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
 * \brief ���¶�ȡ�����ļ���ΪHUP�źŵĴ�����
 *
 */
void MiniService::reloadConfig()
{
  Xlogger->debug("MiniService::reloadConfig");
  MiniConfile rc;
  rc.parse("MiniServer");
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
  Xlogger=new zLogger("MiniServer");

  //����ȱʡ����

  //���������ļ�����
  MiniConfile rc;
  if (!rc.parse("MiniServer"))
    return EXIT_FAILURE;

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

  //������־����
  Xlogger->setLevel(Seal::global["log"]);
  //����д������־�ļ�
  if ("" != Seal::global["logfilename"]){
    Xlogger->addLocalFileLog(Seal::global["logfilename"]);
        Xlogger->removeConsoleLog();
  }

  Seal_Startup();
  
  MiniService::getInstance().main();
  MiniService::delInstance();

  return EXIT_SUCCESS;
}
