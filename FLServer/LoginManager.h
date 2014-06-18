#pragma once
#include "common.h"
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/smart_ptr.hpp>
#include "flcmd.h"

using namespace boost;

class LoginTask;

template <typename T,typename RTValue = bool>
struct zEntryCallback
{
	virtual RTValue exec(T *e)=0;
	virtual ~zEntryCallback(){};
};

class LoginManager {
public:

	/**
	** \brief ������������û���Ŀ
	**
	**/
	static DWORD maxGatewayUser;

	/**
	* \brief ����ص�������
	*
	*/
	typedef zEntryCallback<LoginTask,void> LoginTaskCallback;

	~LoginManager() {}

	static LoginManager &getInstance() {
		if (NULL == instance)
			instance = new LoginManager();

		return *instance;
	}

	static void delInstance() { SAFE_DELETE(instance); }

	bool add(shared_ptr<LoginTask> task);
	bool remove(shared_ptr<LoginTask> task);
	bool broadcast(const DWORD loginTempID,const void *pstrCmd,int nCmdLen);
	void verifyReturn(const Cmd::t_NewLoginSession &session);
	void loginReturn(const DWORD loginTempID,const BYTE retcode,const bool tm = true);
	void execAll(LoginTaskCallback &cb);

private:
	LoginManager(){}

	static LoginManager *instance;
	/**
	* \brief ������������
	*
	*/
	typedef boost::unordered_map<DWORD,shared_ptr<LoginTask> > LoginTaskHashmap;
	/**
	* \brief ������������������
	*
	*/
	typedef LoginTaskHashmap::iterator LoginTaskHashmap_iterator;
	/**
	* \brief ����������������������
	*
	*/
	typedef LoginTaskHashmap::const_iterator LoginTaskHashmap_const_iterator;
	/**
	* \brief ����������ֵ������
	*
	*/
	typedef LoginTaskHashmap::value_type LoginTaskHashmap_pair;
	boost::mutex mlock;
	/**
	* \brief �����ӹ�����������
	*
	*/
	LoginTaskHashmap loginTaskSet;
};
