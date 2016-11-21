/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"
#include <random>

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5
#define TIMEOUT 10

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,
    JOINREP,
    COMEGROUP,
    LEAVGROUP,
    PINGREQ,
    PINGACK,
    UPDATEGRUP,
    RNDPING,
    DUMMYLASTMSGTYPE
};


/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
}MessageHdr;
/*
typedef union NodeAddr{
    int id;
    char addr[4];
}NodeAddr;

class Message{
public:
    MsgTypes msg;
    NodeAddr adr;
    NodeAddr convAdd;
    long heartbeat;
    char end;
    Message(Address* adr);
    Message(char* data);
private:
    void Converte(void);
};*/

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];
    map<int, vector<int>> pingList;
    void convertAddr(Address* dst, Address *src);
    void convertAddr(Address* addr);
//    void sendMessage(MsgTypes type, Address* to, MemberListEntry about);
    void sendMessage(MsgTypes type, Address* to);
    void sendMessage(MsgTypes type, Address* to, char* str, int size);
    //void sendMessage(MsgTypes type, Address* to, MemberListEntry entry);
    void send(MsgTypes type, Address & to, char* msg, int size);
    void send(MsgTypes type, Address & to);
    void send(MsgTypes type, Address & to, Address & from);
    void sendPingRequest();
    void informGroupList(MsgTypes type, int id);
    void updateGroupList();
    void updateMyMemberList(char* list);
    void addEntryToList(Address &adr);
    void rmEntryFromList(int id);
    void sendRandomPing(int faileId);
    void sendRandom(int nodeId, int faileId);
    Address getAddress(int id);

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
    bool addMemberToGroup( Address* addr );
	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
