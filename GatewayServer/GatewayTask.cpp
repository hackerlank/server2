#include "GatewayTask.h"
#include "GatewayServer.h"

void GatewayTask::handle_msg(const void* ptr, const uint32_t len){
	Xlogger->debug("%s",__PRETTY_FUNCTION__);
	switch (m_state)
	{
		case VERIFY:
			{
				if (handle_verify(ptr, len))
				{
					m_state = OKAY;
					async_read();
				}
			}
			break;
		case OKAY:
			{
				//juse discard msg
				async_read();
			}
			break;
	}
}

bool GatewayTask::verifyLogin(const Cmd::stNullUserCmd *ptCmd)
{
	//Xlogger->info("GatewayTask::verifyLogin(id=%d type=%u)",ptCmd->wdServerID,ptCmd->wdServerType);
	using namespace Cmd;

	if (LOGON_USERCMD == ptCmd->byCmd  && USER_VERIFY_VER_PARA == ptCmd->byParam) {
		return true;
	}

	return false;
}

bool GatewayTask::handle_verify(const void* pstrCmd, const uint32_t len) {
	using namespace Cmd;
	if (verifyLogin((const Cmd::stNullUserCmd *)pstrCmd)) {
		Xlogger->debug("%s ok", __PRETTY_FUNCTION__);
		if (uniqueAdd())
		{
			return true;
		}
	}
	Xlogger->debug("%s failed", __PRETTY_FUNCTION__);
	handle_error(boost::system::error_code());
	return false;
}

