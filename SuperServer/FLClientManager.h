/**
 * \brief ͳһ�û�ƽ̨��½�������Ŀͻ��������������
 */
class FLClientManager
{

  public:

    ~FLClientManager();

    /**
     * \brief ��ȡ���Ψһʵ��
     * \return ���Ψһʵ������
     */
    static FLClientManager &getInstance()
    {
      if (NULL == instance)
        instance = new FLClientManager();

      return *instance;
    }

    /**
     * \brief �������Ψһʵ��
     */
    static void delInstance()
    {
      SAFE_DELETE(instance);
    }

    bool init();
    void timeAction(const zTime &ct);
    void add(FLClient *flClient);
    void remove(FLClient *flClient);
    void broadcast(const void *pstrCmd,int nCmdLen);
    void sendTo(const WORD tempid,const void *pstrCmd,int nCmdLen);

  private:

    FLClientManager();
    static FLClientManager *instance;

    /**
     * \brief �ͻ������ӹ����
     */
    x_tcp_clientTaskPool *flClientPool;
    /**
     * \brief ���ж�����������ʱ���¼
     */
    zTime actionTimer;

    /**
     * \brief ��������Ѿ��ɹ���������������
     */
    typedef hash_map<WORD,FLClient *> FLClientContainer;
    typedef FLClientContainer::iterator iter;
    typedef FLClientContainer::const_iterator const_iter;
    typedef FLClientContainer::value_type value_type;
    /**
     * \brief ��������Ѿ��ɹ�����������
     */
    FLClientContainer allClients;
    /**
     * \brief �������ʶ�д��
     */
    zRWLock rwlock;

};
