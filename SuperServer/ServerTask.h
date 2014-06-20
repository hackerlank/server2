#pragma once

#include "common.h"
#include "tcp_task.h"
#include <string>
#include <boost/timer.hpp>
#include <boost/unordered_map.hpp>
#include "supercmd.h"

//class Cmd::t_NullCmd;

class ServerTask : public tcp_task
{
	friend class ServerManager;
	public:
		ServerTask(io_service& ios):tcp_task(ios) {
			wdPort = 0;

			OnlineNum = 0;

			sequenceOK = false;
			state = state_none;
		}

		virtual ~ServerTask();

		bool handle_verify(const void* ptr, const uint32_t len);
		bool handle_wait_sync(const void* ptr, const uint32_t len);
		void handle_msg(const void* ptr, const uint32_t len);
		void handle_timeout(const boost::system::error_code & code);
		int recycleConn();
		void addToContainer();
		void removeFromContainer();
		bool uniqueAdd();
		bool uniqueRemove();
		bool msgParse(const Cmd::t_NullCmd *,const DWORD);
		//virtual void msg_parse(const void* ptr, const uint32_t len);
		void responseOther(const WORD wdServerID);

		const WORD getID() const {
			return se_.wdServerID;
		}

		const WORD getType() const {
			return se_.wdServerType;
		}

		const DWORD getOnlineNum() const {
			return OnlineNum;
		}

		bool checkSequenceTime()
		{
			if (sequenceOK)
				return false;

			if (lastSequenceTime.elapsed() > 2.0)
			{
				lastSequenceTime.restart();
				return true;
			}

			return false;
		}

	private:

		Cmd::Super::ServerEntry se_;

		///! should remove, later
		std::string pstrName;	//server name
		std::string pstrIP;      /**< ������������ַ */
		WORD wdPort;            /**< �����������˿� */

		WORD wdExtPort;
		std::string pstrExtIP;
		
		WORD wdNetType;
		///!

		DWORD      OnlineNum;      /**< ��������ͳ�� */
		int state;

		DWORD m_tickTime;
		volatile bool tickFlag;

		boost::timer lastSequenceTime;        /**< ���һ�δ�������˳���ʱ�� */
		bool sequenceOK;          /**< �Ƿ��Ѿ��������������˳�� */

		bool verify(WORD wdType,const char *pstrIP);
		bool verifyTypeOK(const WORD wdType,std::vector<boost::shared_ptr<ServerTask> > &sv);
		bool checkDependency();
		bool notifyOther();
		bool notifyOther(WORD dstID);

		bool msgParse_Startup(const Cmd::t_NullCmd *pNullCmd,const DWORD nCmdLen);
		bool msgParse_Bill(const Cmd::t_NullCmd *pNullCmd,const DWORD nCmdLen);
		bool msgParse_Gateway(const Cmd::t_NullCmd *pNullCmd,const DWORD nCmdLen);
		bool msgParse_GmTool(const Cmd::t_NullCmd *pNullCmd,const DWORD nCmdLen);
		bool msgParse_CountryOnline(const Cmd::t_NullCmd *pNullCmd,const DWORD nCmdLen);

		/*
		//generate hash code
		struct key_hash
		{
			size_t operator()(const Cmd::Super::ServerEntry &x) const
			{
				std::size_t seed = 0;
				boost::hash_combine(seed, x.wdServerID);
				return seed;
			}
		};

		//equal or not
		struct key_equal : public std::binary_function<Cmd::Super::ServerEntry,Cmd::Super::ServerEntry,bool>
		{
			bool operator()(const Cmd::Super::ServerEntry &s1,const Cmd::Super::ServerEntry &s2) const
			{
				return s1.wdServerID == s2.wdServerID;
			}
		};
		*/

		typedef boost::unordered_map<Cmd::Super::ServerEntry,bool,Cmd::Super::key_hash,Cmd::Super::key_equal> Container;
		Container ses;

		//const char* GetServerTypeName(const WORD wdServerType);
	protected:
		void handle_error(const boost::system::error_code& error);
};

extern void initServerSequence();
