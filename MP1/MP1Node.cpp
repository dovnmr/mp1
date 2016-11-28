/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"
string strTypes[] = {"JOINREQ","JOINREP","COMEGROUP","LEAVGROUP","PINGREQ","PINGACK","UPDATEGRUP","RNDPING"};
/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
    initMemberListTable(this->memberNode);
}
/*
Message::Message(Address* adr){
    memcpy(&this->adr,adr,sizeof(Address));
    Converte();
}

Message::Message(char* data){
    msg = (MsgTypes)*data;
    memcpy(&adr,(data+1),sizeof(Address));
    heartbeat = *(data+1 +sizeof(Address));
    Converte();
}

void Message::Converte(){
    for(int index = 0; index < 4; index++)
        convAdd.addr[index] = adr.addr[3-index];
}
*/




void MP1Node::addEntryToList(Address &addr){
    int id = *(int*)addr.addr;
    short port = *(short*)(&memberNode->addr.addr[4]);
    if (id != *(int*)(&memberNode->addr.addr)) {
        if (find_if(memberNode->memberList.begin(), memberNode->memberList.end(), [&](MemberListEntry member)->bool{
            return member.getid() == id;
        }) == memberNode->memberList.end()) {
            MemberListEntry node = MemberListEntry(id,port);
            memberNode->memberList.push_back(node);
            memberNode->nnb++;
            log->logNodeAdd(&memberNode->addr,&addr);
        }
    }
}

void MP1Node::rmEntryFromList(int id){
    Address to;
    auto it = find_if(memberNode->memberList.begin(), memberNode->memberList.end(), [id](MemberListEntry member)->bool{
        return member.id == id;
    });

    
    if (memberNode->myPos >= memberNode->memberList.begin() && memberNode->myPos < memberNode->memberList.end()) {
        if(it < memberNode->memberList.end()){
            memberNode->nnb--;
            memberNode->timeOutCounter = 0;
            memberNode->pingCounter = TFAIL;
            to = getAddress(it->getid());
            memberNode->memberList.erase(it);
 //           to = getAddress(it->getid());
            log->logNodeRemove(&memberNode->addr, &to);
        }
    }

}

/*
void MP1Node::informGroupList(MsgTypes type, int id){
//    string str = to_string(id);
    static char buffer[256];
    static char ids[4];
    sprintf(ids,"%u",id);
    static Address to;
    for(auto node = memberNode->memberList.begin(); node< memberNode->memberList.end(); node++){
        sprintf(buffer, "%d:%d",node->getid(),node->getport());
        to = Address(buffer);
        sendMessage(type, &to, ids, 3);
      }
}
*/

void MP1Node::informGroupList(MsgTypes type,  int id){
    static char buffer[128];
    Address to;
    for(auto node = memberNode->memberList.begin(); node< memberNode->memberList.end(); node++){
        sprintf(buffer, "%d:%d",node->getid(),node->getport());
        to = Address(buffer);
//        sendMessage(type, &to, entry);
        send(type, to, (char*)&id, sizeof(id));
    }
}

void MP1Node::updateGroupList(){
    static char buffer[256];
    buffer[0] = 0;
    static char addBuf[128];
//    string str;
    Address to;
    for(auto node = memberNode->memberList.begin(); node< memberNode->memberList.end(); node++){
        sprintf(buffer+strlen(buffer),"%u:%u:%lu:%lu;",node->getid(),node->getport(),node->getheartbeat(),node->gettimestamp());
    }
    sprintf(buffer+strlen(buffer),"%c", '\0');
    for(auto node = memberNode->memberList.begin(); node< memberNode->memberList.end(); node++){
        sprintf(addBuf, "%d:%d",node->getid(),node->getport());
        to = Address(addBuf);
        send(UPDATEGRUP, to, buffer, (int)strlen(buffer));
    }
}

Address MP1Node::getAddress(int id){
    Address addr;
    auto it = find_if(memberNode->memberList.begin(), memberNode->memberList.end(),
                   [id, & addr](MemberListEntry node) -> bool{
                       return node.id == id;});

    memcpy(addr.addr,(char*)&id,sizeof(int));
//    memcpy(&addr.addr[4],(char*)&it->port, sizeof(short));
    addr.addr[4] = 0; addr.addr[5] = 0;
    return addr;
}
/*
void MP1Node::sendMessage(MsgTypes type, Address *to, MemberListEntry entry){
    MessageHdr *msg;
    static char s[128];
    size_t msgsize = sizeof(MessageHdr) + sizeof(Address) + sizeof(entry);
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType=type;
    memcpy((char *)(msg+1), &memberNode->addr, sizeof(Address)); //6 byte (4 ip & 2 port)
    memcpy((char *)(msg+1+sizeof(Address)), (char*)&entry, sizeof(entry));
#ifdef DEBUGLOG
    sprintf(s,"Message %s send to %s for entri %d",strTypes[type].c_str(), to->getAddress().c_str(), entry.getid());
    log->LOG(&memberNode->addr, s);
#endif
    emulNet->ENsend(&memberNode->addr, (Address*)to , (char *)msg, (int)msgsize);
    free(msg);
}
*/
void MP1Node::sendMessage(MsgTypes type, Address* to){
    MessageHdr *msg;
    static char s[1024];
    Address toNonde =  Address(*to);
    size_t msgsize = sizeof(MessageHdr) + sizeof(Address) + sizeof(long) + 1;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType=type;
    memcpy((char *)(msg+1), &memberNode->addr, sizeof(Address)); //6 byte (4 ip & 2 port)
    memcpy((char *)(msg+1) + 1 + sizeof(Address), &memberNode->heartbeat, sizeof(long)); //+ 2 long
#ifdef DEBUGLOG
    sprintf(s,"Message %s send to %s",strTypes[type].c_str(), toNonde.getAddress().c_str());
    log->LOG(&memberNode->addr, s);
#endif
    emulNet->ENsend(&memberNode->addr, (Address*)&toNonde , (char *)msg, (int)msgsize);
    free(msg);
}

void MP1Node::sendMessage(MsgTypes type, Address *to, char* str, int size){
    MessageHdr *msg;
    static char s[1024];

    static Address toNode = Address(*to);
    size_t msgsize = 1 + sizeof(toNode) + sizeof(size) + size;
    msg =  (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType=type;
    memcpy((char *)(msg + 1), &toNode.addr[0], sizeof(toNode));
    memcpy((char*)(msg + 1 + sizeof(toNode)), &size, sizeof(size));
    memcpy((char*)(msg + 1 + sizeof(toNode) + sizeof(size)), str, sizeof(size));
    
//    memcpy((char *)(msg + 7), str, size);
    sprintf((char*)(msg+7),"%s" , str);
#ifdef DEBUGLOG
    sprintf(s,"Message %s send to %s with %s",strTypes[type].c_str(), toNode.getAddress().c_str(), str);
    log->LOG(&memberNode->addr, s);
#endif
    emulNet->ENsend(&memberNode->addr, &toNode , (char*)msg, (int)msgsize);
//    printf("%s\n",&msg[7]);
//    for(int index = 0; index < msgsize; index ++)
//        printf("%d -> %x\n", index, msg[index]);
    free(msg);
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/*
* Add member to Group
*/
bool MP1Node::addMemberToGroup(Address *address){
    static MemberListEntry entry;
//    NodeAddr add;
//    memcpy(&add, address, 4);
//    entry.id = add.id;
    entry.id = *(int*)address;
    entry.port = *(short*)(&address->addr[4]);
    memcpy(&entry.port, &address->addr[4], 2);
    entry.heartbeat = 0;
    entry.timestamp = 0;
    memberNode->nnb++;
    memberNode->memberList.push_back(entry);
    log->logNodeAdd(&memberNode->addr,address);
    return true;
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1; //TBD
    initMemberListTable(memberNode);
    memberNode->myPos = memberNode->memberList.begin();
//TBD - start timer to ping 2 random neighbords
    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
//	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
        log->logNodeAdd(&memberNode->addr,joinaddr);
#endif
//        memberNode->inGroup = true;
//        memberNode->timeOutCounter = 0; //start timer
    }
    else {
/*        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ; // 1 byte
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr)); //6 byte (4 ip & 2 port)
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long)); //+ 2 long
*/
        send(JOINREQ,*joinaddr);
        addEntryToList(*joinaddr);

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif
        // send JOINREQ message to introducer member
//        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);
//        free(msg);
    }
    return 1;
}

void MP1Node::send(MsgTypes type, Address &to, Address &from){
    MessageHdr *msg;
    size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr) + sizeof(long) + 1;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType = type;
    memcpy((char *)(msg+1), &from, sizeof(from)); //6 byte (4 ip & 2 port)
    memcpy((char *)(msg+1) + 1 + sizeof(from), &memberNode->heartbeat, sizeof(long)); //+ 2 long
    emulNet->ENsend(&memberNode->addr, &to, (char *)msg, (int)msgsize);
    free(msg);
}

void MP1Node::send(MsgTypes type, Address &to){
    MessageHdr *msg;
    size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr) + sizeof(long) + 1;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType = type;
    memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr)); //6 byte (4 ip & 2 port)
    memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long)); //+ 2 long
    emulNet->ENsend(&memberNode->addr, &to, (char *)msg, (int)msgsize);
    free(msg);
}

void MP1Node::send(MsgTypes type, Address &to, char *Message, int size){
    MessageHdr *msg;
    size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr) + sizeof(long) + +sizeof(size) + size + 1;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType = type;
    memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr)); //6 byte (4 ip & 2 port)
    memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long)); //+ 2 long
    memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr) + sizeof(long), &size, sizeof(size));
    memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr) + sizeof(long) + sizeof(size), Message, size);
    emulNet->ENsend(&memberNode->addr, &to, (char *)msg, (int)msgsize);
    free(msg);
}


/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
    
//TBD - send leav message to member list
    informGroupList(LEAVGROUP,*(int*)memberNode->addr.addr);
    memberNode->inGroup = false;
    memberNode->nnb = 0;
    memberNode->memberList.clear();
#ifdef DEBUGLOG
    log->LOG(&memberNode->addr, "Finish this node.");
#endif
    return 1;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {

//    Member * MemberNode; //Note that resieve maeesage
//    MemberNode = new Member();
//    memcpy((char*)MemberNode,env,sizeof(Member));
    static char buffer[256];
    memcpy(buffer, data, 256);
    MessageHdr msgType;
    memcpy(&msgType,data,sizeof(MessageHdr)); //Get mesage Type
    char * ptrData = data + sizeof(MessageHdr);
    Address addr, to;
    memcpy((char*)&addr.addr[0],(char*)ptrData, sizeof(addr.addr)); //Get adress message from
    ptrData += sizeof(addr.addr);
//    this->convertAddr(&addr);
    long heartbeat;
    memcpy((char*)&heartbeat, ptrData, sizeof(long));
    ptrData +=sizeof(long);
    MemberListEntry entry;
    vector<MemberListEntry> list;
    int id; short port,listSize;
    static char s[256];
    vector<MemberListEntry>::iterator it;
    string str;
#ifdef DEBUGLOG
    sprintf(s,"Got `Message %s from %s",strTypes[msgType.msgType].c_str(),addr.getAddress().c_str());
//    log->LOG(&MemberNode->addr, s);
    log->LOG(&memberNode->addr, s);
#endif
    switch(msgType.msgType){
//Got Group Join request
        case JOINREQ:
            #ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Got JOUN Request.");
            #endif
 //           sendMessage(JOINREP, &addr);
            send(JOINREP, addr);
            
            //add node to GROUP
            addMemberToGroup(&addr);
            
            //Update group about new member
            updateGroupList();
            break;
//Got Join Group request replay
        case JOINREP:
            #ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Got JOUN Replay.");
            memberNode->inGroup = true;//TBD get list og group
            memberNode->timeOutCounter = 0;
            addEntryToList(addr);
//            log->logNodeAdd(&memberNode->addr,&addr);
            #endif
            break;
//
        case COMEGROUP:
            #ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Got new member to the group.");
            #endif
            memcpy(&id,&addr,sizeof(int));
            entry.setid(id);
            memcpy(&port, &addr.addr[4], sizeof(short));
            entry.setport(port);
            memberNode->memberList.push_back(entry);
            memberNode->nnb++;
            break;
        case LEAVGROUP:
//            str = (char*)&data[1+sizeof(addr)];
//            memcpy(&addr.addr[0], data+1, 4);
//            memcpy(&addr.addr[4], data+5, 2);
            listSize = *(int*)++ptrData;
            ptrData += sizeof(int);
            id = *(int*)ptrData;
            to = getAddress(id);

#ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Member %d leavs the group.", id);
//            log->logNodeRemove(&memberNode->addr, &to);
#endif
            //            memcpy((char*)&entry, &data[1 + sizeof(Address)], sizeof(entry));
//            if (find_if(memberNode->memberList.begin(), memberNode->memberList.end(), [id](MemberListEntry node)->bool{
//                return id == node.id;
//            }) != memberNode->memberList.end()){
//                informGroupList(LEAVGROUP,id);
//            }
            rmEntryFromList(id);
            break;
//Got PING Request
        case PINGREQ:
//#ifdef DEBUGLOG
//            log->LOG(&memberNode->addr, "Ping request comes.");
//#endif
            sendMessage(PINGACK,&addr);
            break;
// Got PING Acknowledge
       case PINGACK:
#ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Ping ACK comes.");
#endif
        //chaeck if this ACK for rnd ping
            id = *(int*)addr.addr;
            send(PINGACK, to, addr);
            if (pingList.count(id)) {
                for_each(pingList[id].begin(), pingList[id].end(),[&](int toId){
                    to = getAddress(toId); //
                    this->send(PINGACK, to, addr);// to | in place of
                });
                pingList.erase(id);
            }
            else if (memberNode->myPos < memberNode->memberList.begin() || memberNode->myPos > memberNode->memberList.end()) {
                memberNode->myPos = memberNode->memberList.begin();
                memberNode->timeOutCounter = 0; //start timer
                memberNode->pingCounter = TFAIL;
                ++memberNode->myPos;
            }
            else if(id == memberNode->myPos->getid()){
                memberNode->timeOutCounter = 0; //start timer
                memberNode->pingCounter = TFAIL;
                ++memberNode->myPos;
            }
            break;
        case UPDATEGRUP:
            listSize = *(int*)++ptrData;
            ptrData += sizeof(int);
            ptrData[listSize] = 0;
#ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Updeate Group List come. \"%s\"", ptrData);
#endif
            updateMyMemberList(ptrData);
            break;
        case RNDPING:
            listSize = *(int*)++ptrData;
            ptrData += sizeof(int);
            id = *(int*)ptrData;
            if (find(pingList[id].begin(), pingList[id].end(), *(int*)addr.addr) == pingList[id].end()) {
                // someName not in name, add it
                pingList[id].push_back(*(int*)addr.addr);
                memcpy(to.addr,ptrData,sizeof(int));
                to.addr[4] = 0;
                send(PINGREQ, to);
            }
//            pingList[id].push_back(*(int*)addr.addr);
//            pingList.insert(pair<int,vecor<int>>(id,*(int*)addr.addr));//pier to, by name of

#ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Member %d try ping.", id);
#endif
            ////////
            break;
        default:
            cout << "Incorrect message type " << msgType.msgType <<'\n';
            return false;
    }

//    free(MemberNode);
//    free(msgRes);
    return true;
}

void MP1Node::updateMyMemberList(char* list){
    string str = string(list);
    static vector<string> members;
    static char buf[80];
    sprintf(buf, "Will add list of members: %s",str.c_str());
    log->LOG(&memberNode->addr, buf);
/*
    for_each(str.begin(), str.end(), [&members](char ch){
        string member;
        if(ch != ';'){member.push_back(ch);}
        else{
            members.push_back(member);
            member.clear();
        }
    });
*/
    size_t pos;
    while ((pos = str.find(';')) != string::npos) {
        sprintf(buf, "member to add: %s", str.substr(0,pos).c_str());
        log->LOG(&memberNode->addr, buf);
        members.push_back(str.substr(0,pos));
        str.erase(0,pos +1);
    }
    sprintf(buf, "members to add %lu", members.size());
    log->LOG(&memberNode->addr, buf);
    for_each(members.begin(), members.end(), [=](string member){//time
        Address addr;
        if(member.size()) addr =  Address(member);
        char bufer[80];
        if (*(int*)memberNode->addr.addr != *(int*)(addr.addr))
            addEntryToList(addr);
        else {
            sprintf(bufer, "find self address - ignor to add %s", memberNode->addr.getAddress().c_str());
            log->LOG(&memberNode->addr, bufer);
        }
    });
    members.clear();
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {
    static MemberListEntry failedMember;
    Address addr;
    static char s[256];

    //first time send PING request
//    if(memberNode->inGroup == true && memberNode->nnb > 0 ){
        memberNode->timeOutCounter++;
//        if(memberNode->timeOutCounter == TIMEOUT) //10
            sendPingRequest();
//    }

//    if(memberNode->timeOutCounter < TREMOVE + TIMEOUT) //TREMOVE = 20
    if(memberNode->timeOutCounter <  TREMOVE) //TREMOVE = 20
        return;

    if (memberNode->myPos >= memberNode->memberList.end() || memberNode->myPos < memberNode->memberList.begin())
        memberNode->myPos = memberNode->memberList.begin();
    
    printf(" Got timeout from node %d\n",memberNode->myPos->getid());
    memberNode->timeOutCounter = 0;
    
    sendRandomPing(memberNode->myPos->getid());

    //delete member and send PING request to the next memeber
    if(memberNode->pingCounter-- <= 0){
        memberNode->pingCounter = TFAIL;
        failedMember = *memberNode->myPos;
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Node %d signed as failed", failedMember.getid());
#endif
        if(memberNode->nnb > 0 ){
//TBD            ++memberNode->myPos;
            sprintf(s,"%d:%d",failedMember.getid(), failedMember.getport());
            addr = Address(s);
            log->logNodeRemove(&memberNode->addr, &addr);
            memberNode->memberList.erase(memberNode->myPos);
            memberNode->nnb--;
            informGroupList(LEAVGROUP,failedMember.getid());
//            sendPingRequest();
        }
        return;
    }
    return;
}

void MP1Node::sendRandomPing(int failedId){
    
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1,memberNode->nnb);
    int nodeId = distribution(generator);
    sendRandom(nodeId, failedId);

    if(memberNode->nnb > 1){
        nodeId = distribution(generator);
        sendRandom(nodeId, failedId);
    }
}

void MP1Node::sendRandom(int nodeId, int failedId){
    static char addBuf[64];
    
    auto it = find_if(memberNode->memberList.begin(), memberNode->memberList.end(), [nodeId](MemberListEntry member) -> bool{
        return member.id == nodeId;
    });
    sprintf(addBuf, "%d:%d",it->getid(), it->getport());
    Address to = Address(addBuf);
    send(RNDPING, to, (char*)&failedId, sizeof(int));
}

void MP1Node::sendPingRequest(){
    char s[256];
    if (!memberNode->inGroup )
        return;
    
    if(memberNode->myPos > memberNode->memberList.end() || memberNode->myPos < memberNode->memberList.begin())
        memberNode->myPos = memberNode->memberList.begin();
    
    //SEND ping to the next member
    sprintf(s,"%d:%d",memberNode->myPos->getid(), memberNode->myPos->getport());
    Address to = Address(s);
#ifdef DEBUGLOG
    log->LOG(&memberNode->addr, "Send ping.");
#endif
    //sendMessage(PINGREQ,&to);
    send(PINGREQ, to);
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

void MP1Node::convertAddr(Address *dst, Address* src){
    static char addr[4];
    for (int index =0; index < 4; index++)
        addr[index] = *(src->addr + 3 -index);
    memcpy(dst->addr,addr,4);
}

void MP1Node::convertAddr(Address *adress){
    static char addr[4];
    for (int index =0; index < 4; index++)
        addr[index] = *(adress->addr + 3 - index);
    memcpy(adress->addr, addr, 4);
}
