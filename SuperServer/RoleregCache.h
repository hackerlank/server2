#pragma once
#include "common.h"
#include <set>

enum NetType
{
  NetType_near = 0,//����·�ɣ������������ŷ���������ͨ������ͨ������
  NetType_far = 1,  //Զ��·�ɣ�����������ͨ����������ͨ�������ŷ�����
};


/**
 * \brief ���������֮���������ϵ
 *
 */
extern hash_map<int,std::vector<int> > serverSequence;

/**
 * \brief ��ʼ��������֮���������ϵ
  
   gcc 
 *
 */
void initServerSequence();




class RoleregCache
{

  public:

    struct Data
    {
      WORD wdServerID;      /**< ��������� */
      DWORD accid;        /**< �˺ű�� */
      char name[MAX_NAMESIZE];  /**< ��ɫ���� */
      WORD state;          /**< ����״̬��λ��� */

      Data(const Cmd::Super::t_Charname_Gateway &cmd)
      {
        wdServerID = cmd.wdServerID;
        accid = cmd.accid;
        strncpy(name,cmd.name,sizeof(name));
        state = cmd.state;
      }

      Data(const Data &data)
      {
        wdServerID = data.wdServerID;
        accid = data.accid;
        strncpy(name,data.name,sizeof(name));
        state = data.state;
      }

      Data &operator=(const Data &data)
      {
        wdServerID = data.wdServerID;
        accid = data.accid;
        strncpy(name,data.name,sizeof(name));
        state = data.state;
        return *this;
      }
    };

    ~RoleregCache() {};

    static RoleregCache &getInstance()
    {
      if (NULL == instance)
        instance = new RoleregCache();

      return *instance;
    }

    static void delInstance()
    {
      SAFE_DELETE(instance);
    }

    void add(const Cmd::Super::t_Charname_Gateway &cmd);
    void timeAction(const zTime &ct);

    bool msgParse_loginServer(WORD wdServerID,DWORD accid,char name[MAX_NAMESIZE],WORD state);

  private:

    RoleregCache() {};

    static RoleregCache *instance;

    zTime actionTimer;

    typedef std::list<Data> DataCache;
    zMutex mlock;
    DataCache datas;

};

