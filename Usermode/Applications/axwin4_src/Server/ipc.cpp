/*
 */
#include <ipc.hpp>
#include <list>
#include <IIPCChannel.hpp>
#include <algorithm>
#include <CClient.hpp>
#include <serialisation.hpp>
#include <ipc_proto.hpp>
#include <CCompositor.hpp>
extern "C" {
#include <assert.h>
};
#include <CIPCChannel_AcessIPCPipe.hpp>

namespace AxWin {
namespace IPC {

CCompositor*	gpCompositor;
::std::list<IIPCChannel*>	glChannels;
//::std::map<uint16_t,CClient*>	glClients;

void Initialise(const CConfigIPC& config, CCompositor& compositor)
{
	gpCompositor = &compositor;
	
	::std::string pipe_basepath = "axwin4";
	glChannels.push_back( new CIPCChannel_AcessIPCPipe( pipe_basepath ) );

	//glChannels.push_back( new CIPCChannel_TCP("0.0.0.0:2100") );
	
	//for( auto channel : config.m_channels )
	//{
	//	channels.push_back(  );
	//}
}

int FillSelect(fd_set& rfds)
{
	int ret = 0;
	for( auto channel : glChannels )
	{
		ret = ::std::max(ret, channel->FillSelect(rfds));
	}
	return ret;
}

void HandleSelect(fd_set& rfds)
{
	for( auto channel : glChannels )
	{
		channel->HandleSelect(rfds);
	}
}

void RegisterClient(CClient& client)
{
	// allocate a client ID, and save
	//client.m_id = 123;
	//glClients[client.m_id] = &client;
}

void DeregisterClient(CClient& client)
{
	//glClients.erase( client.m_id );
}



typedef void	MessageHandler_op_t(CClient& client, CDeserialiser& message);
MessageHandler_op_t	HandleMessage_Ping;
MessageHandler_op_t	HandleMessage_GetWindowAttr;
MessageHandler_op_t	HandleMessage_Reply;
MessageHandler_op_t	HandleMessage_CreateWindow;
MessageHandler_op_t	HandleMessage_CloseWindow;
MessageHandler_op_t	HandleMessage_SetWindowAttr;
MessageHandler_op_t	HandleMessage_AddRegion;
MessageHandler_op_t	HandleMessage_DelRegion;
MessageHandler_op_t	HandleMessage_SetRegionAttr;
MessageHandler_op_t	HandleMessage_PushData;
MessageHandler_op_t	HandleMessage_SendIPC;

MessageHandler_op_t	*message_handlers[] = {
	[IPCMSG_PING]       = &HandleMessage_Ping,
	[IPCMSG_REPLY]      = &HandleMessage_Reply,
	
	[IPCMSG_CREATEWIN]  = &HandleMessage_CreateWindow,
	[IPCMSG_CLOSEWIN]   = &HandleMessage_CloseWindow,
	[IPCMSG_SETWINATTR] = &HandleMessage_SetWindowAttr,
	[IPCMSG_GETWINATTR] = &HandleMessage_GetWindowAttr,
	
	[IPCMSG_RGNADD]     = &HandleMessage_AddRegion,
	[IPCMSG_RGNDEL]     = &HandleMessage_DelRegion,
	[IPCMSG_RGNSETATTR] = &HandleMessage_SetRegionAttr,
	[IPCMSG_RGNPUSHDATA]= &HandleMessage_PushData,	// to a region
	[IPCMSG_SENDIPC]    = &HandleMessage_SendIPC,	// Use the GUI server for low-bandwith inter-process messaging
};

void HandleMessage(CClient& client, CDeserialiser& message)
{
	unsigned int command = message.ReadU8();
	if( command >= sizeof(message_handlers)/sizeof(IPC::MessageHandler_op_t*) ) {
		// Drop, invalid command
		return ;
	}
	
	(message_handlers[command])(client, message);
}

void HandleMessage_Reply(CClient& client, CDeserialiser& message)
{
	// Reply to a sent message
	// - Not many messages need server-bound replies
	int orig_command = message.ReadU8();
	switch(orig_command)
	{
	case IPCMSG_PING:
		// Ping reply, mark client as still responding
		break;
	default:
		// Unexpected reply
		break;
	}
}

void HandleMessage_Ping(CClient& client, CDeserialiser& message)
{
	// A client has asked for a ping, we pong them back
	CSerialiser	reply;
	reply.WriteU8(IPCMSG_REPLY);
	reply.WriteU8(IPCMSG_PING);
	client.SendMessage(reply);
}

void HandleMessage_CreateWindow(CClient& client, CDeserialiser& message)
{
	uint16_t	parent_id = message.ReadU16();
	uint16_t	new_id = message.ReadU16();
	CWindow* parent = client.GetWindow( parent_id );
	
	client.SetWindow( new_id, gpCompositor->CreateWindow(client) );
}

void HandleMessage_CloseWindow(CClient& client, CDeserialiser& message)
{
	assert(!"TODO");
}

void HandleMessage_SetWindowAttr(CClient& client, CDeserialiser& message)
{
	assert(!"TODO");
}

void HandleMessage_GetWindowAttr(CClient& client, CDeserialiser& message)
{
	assert(!"TODO");
}

void HandleMessage_AddRegion(CClient& client, CDeserialiser& message)
{
	assert(!"TODO");
}

void HandleMessage_DelRegion(CClient& client, CDeserialiser& message)
{
	assert(!"TODO");
}

void HandleMessage_SetRegionAttr(CClient& client, CDeserialiser& message)
{
	assert(!"TODO");
}

void HandleMessage_PushData(CClient& client, CDeserialiser& message)
{
	assert(!"TODO");
}

void HandleMessage_SendIPC(CClient& client, CDeserialiser& message)
{
	assert(!"TODO");
}

};	// namespace IPC

IIPCChannel::~IIPCChannel()
{
}

};	// namespace AxWin

