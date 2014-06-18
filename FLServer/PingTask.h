#pragma once

#include "tcp_task.h"
#include <string>
#include <boost/timer.hpp>
#include <boost/unordered_map.hpp>

class PingTask : public tcp_task
{
public:
	/**
	* \brief ���캯��
	* \param pool ���������ӳ�
	* \param sock TCP/IP�׽ӿ�
	*/
	PingTask(io_service& ios):tcp_task(ios) { }
	~PingTask() {};

	bool verify_msg(const Cmd::t_NullCmd* cmd, const uint32_t len);
	int recycleConn();
	//  void addToContainer();
	//  bool uniqueAdd();
	//  bool uniqueRemove();
	virtual void msg_parse(const void* ptr, const uint32_t len);
	bool msgParse(const Cmd::t_NullCmd* ptNull,const DWORD);
private:
};
