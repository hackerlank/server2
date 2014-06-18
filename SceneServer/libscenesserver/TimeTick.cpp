/**
 * \brief ʱ��ص�����
 */
//#include <zebra/ScenesServer.h>
#include "scriptTickTask.h"
#include "duplicateManager.h"

#define MAX_CMD_GROUP 10
zRTime SceneTimeTick::currentTime;
SceneTimeTick *SceneTimeTick::instance = NULL;

/**
 * \brief ��ͼ�ص�����
 */
struct EverySceneEntryAction : public SceneCallBack
{
  const DWORD group;
  EverySceneEntryAction(const DWORD group) : group(group) {}
  bool exec(Scene *scene)
  {
    if (scene)
    {
      if (scene->SceneEntryAction(SceneTimeTick::currentTime,group))
      {
        /**
         * ���������Ҫ��̬����ж�ص�ͼ������Ŷ
         * ^-^ ��������,���ڵ��߳�,����������û����,ֻ��Ҫע������������ĵ�����
         */
        SceneTaskManager::getInstance().execEvery();
      }
    }
    return true;
  }
};

/**
 * \brief ������npc�ӵ�ai�����б���
 */
/*class AddSpecialNpcCallBack : public specialNpcCallBack
{
  private:
    MonkeyNpcs &affectNpc;
    const DWORD group;
    const bool _every;
  public:
    AddSpecialNpcCallBack(MonkeyNpcs &affectNpc,const DWORD g,const bool every):affectNpc(affectNpc),group(g),_every(every){}
    bool exec(SceneNpc * npc)
    {
      if (_every || npc->tempid%MAX_NPC_GROUP==group)
      {
        affectNpc.insert(npc);
      }
      return true;
    }
};*/

/**
 * \brief ʱ��ѭ��,���Ͷ�ʱ�¼�,������Ӱ��npc ai��
 */
void SceneTimeTick::run()
{
  const int timeout_value = 500;
  const int timeout_value2 = 300;
  DWORD step = 0;
  int t = 0;
  while(!isFinal())
  {
    zThread::msleep((10-t)>0?(10-t):1);
    //��ȡ��ǰʱ��
    currentTime.now();

	//sky һ���ʱ��ѭ���������鴦��Roll����
	if(_one_sec(currentTime)) {
		SceneManager::getInstance().TeamRollItme();
	}

    if (_five_sec(currentTime)) {
      OnTimer event(1);
      EventTable::instance().execute(event);
      ScenesService::getInstance().checkAndReloadConfig();
    }
    sessionClient->doCmd();
    recordClient->doCmd();
    SceneTaskManager::getInstance().execEvery();

    //specialNpc
    //MonkeyNpcs affectNpc;
    //AddSpecialNpcCallBack asncb(affectNpc,step,t > timeout_value2);
    //SceneNpcManager::getMe().execAllSpecialNpc(asncb);
    SceneNpc::AI(currentTime,SceneNpcManager::getMe().getSepcialNpc(),step,t > timeout_value2);

    //250 usec
    EverySceneEntryAction esea(step);
    //�����е�ͼ���ûص�����
    SceneManager::getInstance().execEveryScene(esea);

#if 0
    if (0==step)
    {
      //20-25 usec
      sessionClient->doCmd();
      recordClient->doCmd();
      SceneTaskManager::getInstance().execEvery();
    }
#endif

    if (_one_min(currentTime))
    {//��������,һ�����ж�һ��ȫ������
      CountryDareM::getMe().timer();

      //ˢ������ȫ�ֱ���
      if (GlobalVar::server_id()) { //ugly,TO BE FIXED

        ALLVARS(update);
        ALLVARS(save);
      }

      SceneManager::getInstance().checkUnloadOneScene();
    }

    step = (++step) % MAX_NPC_GROUP;

    zRTime e;
    t = currentTime.elapse(e);
    if (t > timeout_value)
    {
      Xlogger->debug("---------- 1��ѭ����ʱ %u ����----------",t);
    }

	scriptTaskManagement::getInstance().execAll();

	duplicateManager::getInstance().doClear();
	/*scriptTaskManagement::iterator it = _tasklist->begin();
	scriptTaskManagement::iterator end = _tasklist->end();
	for( ; it != end; ++it)
	{
		time_t t = (time(NULL) - it->second->lastTime);
		if(it->second->doTask(t))
		{
			it->second->lastTime = time(NULL);
		}
	}*/

  }
}
