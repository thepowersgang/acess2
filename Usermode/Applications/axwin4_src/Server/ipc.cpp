/*
 */
#define __STDC_LIMIT_MACROS
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
	for( const auto channel : glChannels )
	{
		assert(channel);
		ret = ::std::max(ret, channel->FillSelect(rfds));
	}
	return ret;
}

void HandleSelect(const fd_set& rfds)
{
	for( const auto channel : glChannels )
	{
		assert(channel);
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


void SendNotify_Dims(CClient& client, unsigned int NewW, unsigned int NewH)
{
	_SysDebug("TODO: CClient::SendNotify_Dims");
}


void HandleMessage_Nop(CClient& client, CDeserialiser& message)
{
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

void HandleMessage_GetGlobalAttr(CClient& client, CDeserialiser& message)
{
	uint16_t	attr_id = message.ReadU16();
	
	CSerialiser	reply;
	reply.WriteU8(IPCMSG_REPLY);
	reply.WriteU8(IPCMSG_GETGLOBAL);
	reply.WriteU16(attr_id);
	
	switch(attr_id)
	{
	case IPC_GLOBATTR_SCREENDIMS: {
		uint8_t	screen_id = message.ReadU8();
		unsigned int w, h;
		gpCompositor->GetScreenDims(screen_id, &w, &h);
		reply.WriteU16( (w <= UINT16_MAX ? w : UINT16_MAX) );
		reply.WriteU16( (h <= UINT16_MAX ? h : UINT16_MAX) );
		break; }
	case IPC_GLOBATTR_MAXAREA:
		assert(!"TODO: IPC_GLOBATTR_MAXAREA");
		break;
	default:
		throw IPC::CClientFailure("Bad global attribute ID");
	}
	
	client.SendMessage(reply);
}

void HandleMessage_SetGlobalAttr(CClient& client, CDeserialiser& message)
{
	uint16_t	attr_id = message.ReadU16();
	
	switch(attr_id)
	{
	case IPC_GLOBATTR_SCREENDIMS:
		// Setting readonly
		break;
	case IPC_GLOBATTR_MAXAREA:
		assert(!"TODO: IPC_GLOBATTR_MAXAREA");
		break;
	default:
		throw IPC::CClientFailure("Bad global attribute ID");
	}
}

void HandleMessage_CreateWindow(CClient& client, CDeserialiser& message)
{
	uint16_t	new_id = message.ReadU16();
	//uint16_t	parent_id = message.ReadU16();
	//CWindow* parent = client.GetWindow( parent_id );
	::std::string	name = message.ReadString();
	
	::_SysDebug("_CreateWindow: (%i, '%s')", new_id, name.c_str());
	client.SetWindow( new_id, gpCompositor->CreateWindow(client, name) );
}

void HandleMessage_DestroyWindow(CClient& client, CDeserialiser& message)
{
	uint16_t	win_id = message.ReadU16();
	_SysDebug("_DestroyWindow: (%i)", win_id);
	
	CWindow*	win = client.GetWindow(win_id);
	if(!win) {
		throw IPC::CClientFailure("_DestroyWindow: Bad window");
	}
	client.SetWindow(win_id, 0);	
	
	// TODO: Directly inform compositor?
	delete win;
}

void HandleMessage_SetWindowAttr(CClient& client, CDeserialiser& message)
{
	uint16_t	win_id = message.ReadU16();
	uint16_t	attr_id = message.ReadU16();
	_SysDebug("_SetWindowAttr: (%i, %i)", win_id, attr_id);
	
	CWindow*	win = client.GetWindow(win_id);
	if(!win) {
		throw IPC::CClientFailure("_SetWindowAttr - Bad window");
	}
	
	switch(attr_id)
	{
	case IPC_WINATTR_DIMENSIONS: {
		uint16_t new_w = message.ReadU16();
		uint16_t new_h = message.ReadU16();
		win->Resize(new_w, new_h);
		break; }
	case IPC_WINATTR_POSITION: {
		int16_t new_x = message.ReadS16();
		int16_t new_y = message.ReadS16();
		win->Move(new_x, new_y);
		break; }
	case IPC_WINATTR_SHOW:
		win->Show( message.ReadU8() != 0 );
		break;
	case IPC_WINATTR_FLAGS:
		_SysDebug("TODO: IPC_WINATTR_FLAGS");
		break;
	case IPC_WINATTR_TITLE:
		assert(!"TODO: IPC_WINATTR_TITLE");
		break;
	default:
		_SysDebug("HandleMessage_SetWindowAttr - Bad attr %u", attr_id);
		throw IPC::CClientFailure("Bad window attr");
	}
}

void HandleMessage_GetWindowAttr(CClient& client, CDeserialiser& message)
{
	assert(!"TODO HandleMessage_GetWindowAttr");
}

void HandleMessage_SendIPC(CClient& client, CDeserialiser& message)
{
	assert(!"TODO HandleMessage_SendIPC");
}

void HandleMessage_GetWindowBuffer(CClient& client, CDeserialiser& message)
{
	uint16_t	win_id = message.ReadU16();
	_SysDebug("_SetWindowAttr: (%i)", win_id);
	
	CWindow*	win = client.GetWindow(win_id);
	if(!win) {
		throw IPC::CClientFailure("_PushData: Bad window");
	}
	
	uint64_t handle = win->m_surface.GetSHMHandle();
	
	CSerialiser	reply;
	reply.WriteU8(IPCMSG_REPLY);
	reply.WriteU16(win_id);
	reply.WriteU64(handle);
	client.SendMessage(reply);
}

void HandleMessage_PushData(CClient& client, CDeserialiser& message)
{
	uint16_t	win_id = message.ReadU16();
	uint16_t	x = message.ReadU16();
	uint16_t	y = message.ReadU16();
	uint16_t	w = message.ReadU16();
	uint16_t	h = message.ReadU16();
	//_SysDebug("_PushData: (%i, (%i,%i) %ix%i)", win_id, x, y, w, h);
	
	CWindow*	win = client.GetWindow(win_id);
	if(!win) {
		throw IPC::CClientFailure("_PushData: Bad window");
	}
	
	for( unsigned int row = 0; row < h; row ++ )
	{
		const ::std::vector<uint8_t> scanline_data = message.ReadBuffer();
		if( scanline_data.size() != w * 4 ) {
			_SysDebug("ERROR _PushData: Scanline buffer size mismatch (%i,%i)",
				scanline_data.size(), w*4);
			continue ;
		}
		win->DrawScanline(y+row, x, w, scanline_data.data());
	}
}
void HandleMessage_Blit(CClient& client, CDeserialiser& message)
{
	assert(!"TODO HandleMessage_Blit");
}
void HandleMessage_DrawCtl(CClient& client, CDeserialiser& message)
{
	assert(!"TODO HandleMessage_DrawCtl");
}
void HandleMessage_DrawText(CClient& client, CDeserialiser& message)
{
	assert(!"TODO HandleMessage_DrawText");
}

typedef void	MessageHandler_op_t(CClient& client, CDeserialiser& message);

MessageHandler_op_t	*message_handlers[] = {
	[IPCMSG_NULL]       = &HandleMessage_Nop,
	[IPCMSG_REPLY]      = &HandleMessage_Reply,
	[IPCMSG_PING]       = &HandleMessage_Ping,
	[IPCMSG_GETGLOBAL]  = &HandleMessage_GetGlobalAttr,
	[IPCMSG_SETGLOBAL]  = &HandleMessage_SetGlobalAttr,
	
	[IPCMSG_CREATEWIN]  = &HandleMessage_CreateWindow,
	[IPCMSG_CLOSEWIN]   = &HandleMessage_DestroyWindow,
	[IPCMSG_SETWINATTR] = &HandleMessage_SetWindowAttr,
	[IPCMSG_GETWINATTR] = &HandleMessage_GetWindowAttr,
	[IPCMSG_SENDIPC]    = &HandleMessage_SendIPC,	// Use the GUI server for low-bandwith IPC
	[IPCMSG_GETWINBUF]  = &HandleMessage_GetWindowBuffer,
	[IPCMSG_DAMAGERECT] = nullptr,
	[IPCMSG_PUSHDATA]   = &HandleMessage_PushData,	// to a window's buffer
	[IPCMSG_BLIT]       = &HandleMessage_Blit,	// Copy data from one part of the window to another
	[IPCMSG_DRAWCTL]    = &HandleMessage_DrawCtl,	// Draw a control
	[IPCMSG_DRAWTEXT]   = &HandleMessage_DrawText,	// Draw text
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

CClientFailure::CClientFailure(std::string&& what):
	m_what(what)
{
}
const char *CClientFailure::what() const throw()
{
	return m_what.c_str();
}
CClientFailure::~CClientFailure() throw()
{
}

};	// namespace IPC

IIPCChannel::~IIPCChannel()
{
}

};	// namespace AxWin

