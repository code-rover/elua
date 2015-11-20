#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "ae.h"
#include "anet.h"
#include <assert.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


#define PORT 4444
#define MAX_LEN 10 //1024

struct Client {
	int fd;
	lua_State *L;
	
};


//��Ŵ�����Ϣ���ַ���
char g_err_string[1024];

//�¼�ѭ������
aeEventLoop *g_event_loop = NULL;

//��ʱ������ڣ����һ�仰
int PrintTimer(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
	static int i = 0;
	printf("Test Output: %d\n", i++);
	//10����ٴ�ִ�иú���
	return 10000;
}

//ֹͣ�¼�ѭ��
void StopServer()
{
	aeStop(g_event_loop);
}


int lwrite(lua_State *L) {
	int fd = lua_tointeger(L, -3);
	const char* msg = lua_tostring(L, -2);	
	int sz = lua_tointeger(L, -1);

	printf("fd: %d, msg: %s, sz: %d", fd, msg, sz);

	int r = write(fd, msg, sz);
	lua_pushinteger(L, r);
	return 1;
};

void ClientClose(aeEventLoop *el, int fd, int err, void* privdata)
{
	//���errΪ0����˵���������˳�����������쳣�˳�
	if( 0 == err )
		printf("Client quit: %d\n", fd);
	else if( -1 == err )
		//fprintf(stderr, "Client Error: %s\n", strerror(errno));
		fprintf(stderr, "Client Error: %d\n", errno);

	//ɾ����㣬�ر��ļ�
	aeDeleteFileEvent(el, fd, AE_READABLE);

	
	struct Client *p = privdata;
	assert(p);		

	lua_getglobal(p->L, "OnClose");

	lua_pushinteger(p->L, fd);

	lua_pcall(p->L, 1, 0, 0);

	close(fd);
}

//�����ݴ������ˣ���ȡ����
void ReadFromClient(aeEventLoop *el, int fd, void *privdata, int mask)
{
	char buffer[MAX_LEN] = { 0 };
	
	int res = read(fd, buffer, 2);
	assert(res == 2);

	res = read(fd, buffer, res);

	int res = read(fd, buffer, MAX_LEN);
	printf("read: %d\n", res);
	if( res <= 0 )
	{
		ClientClose(el, fd, res, privdata);
	}
	else
	{
		struct Client *p = privdata;
		assert(p);		

		lua_getglobal(p->L, "OnData");

		lua_pushinteger(p->L, fd);
		lua_pushlstring(p->L, buffer, res);

		lua_pcall(p->L, 2, 0, 0);

		//res = write(fd, buffer, MAX_LEN);
		//if( -1 == res )
		//	ClientClose(el, fd, res, privdata);
	}
}

//����������
void AcceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
	int cfd, cport;
	char ip_addr[128] = { 0 };
	cfd = anetTcpAccept(g_err_string, fd, ip_addr, &cport);
	printf("Connected from %s:%d\n", ip_addr, cport);


	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

        lua_register(L, "lwrite", lwrite);

        int r = luaL_loadfile(L, "script.lua");
        if(r != 0) {
                fprintf(stderr, "loadfile error\n");
		close(fd);
                return;
        };      
        lua_pcall(L, 0, 0, 0);

        lua_getglobal(L, "OnConnect");

        lua_pushinteger(L, cfd);

        lua_pcall(L, 1, 0, 0);	

	struct Client *p = (struct Client*)malloc(sizeof(*p));
	p->fd = cfd;
	p->L = L;

	if( aeCreateFileEvent(el, cfd, AE_READABLE,
		ReadFromClient, p) == AE_ERR )
	{
		fprintf(stderr, "client connect fail: %d\n", fd);
		close(fd);
	}
}

int main()
{

	printf("Start\n");

	signal(SIGINT, StopServer);

	//��ʼ�������¼�ѭ��
	g_event_loop = aeCreateEventLoop(1024*10);

	//���ü����¼�
	int fd = anetTcpServer(g_err_string, PORT, NULL);
	if( ANET_ERR == fd )
		fprintf(stderr, "Open port %d error: %s\n", PORT, g_err_string);

	if( aeCreateFileEvent(g_event_loop, fd, AE_READABLE, 
		AcceptTcpHandler, NULL) == AE_ERR )
		fprintf(stderr, "Unrecoverable error creating server.ipfd file event.");

	//���ö�ʱ�¼�
	aeCreateTimeEvent(g_event_loop, 1, PrintTimer, NULL, NULL);

	//�����¼�ѭ��
	aeMain(g_event_loop);

	//ɾ���¼�ѭ��
	aeDeleteEventLoop(g_event_loop);

	printf("End\n");
	return 0;
}

