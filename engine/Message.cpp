/**
 * Message.cpp
 * This file is part of the YATE Project http://YATE.null.ro
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2004-2023 Null Team
 *
 * This software is distributed under multiple licenses;
 * see the COPYING file in the main directory for licensing
 * information for this specific distribution.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "yatengine.h"
#include <string.h>

using namespace TelEngine;

class QueueWorker : public GenObject, public Thread
{
public:
    inline QueueWorker(MessageQueue* queue)
	: Thread("MessageQueueWorker"),m_queue(queue)
	{}
    virtual ~QueueWorker();
protected:
    virtual void run();
private:
    RefPointer<MessageQueue> m_queue;
};

Message::Message(const char* name, const char* retval, bool broadcast)
    : NamedList(name),
      m_return(retval), m_timeEnqueue((uint64_t)0), m_timeDispatch((uint64_t)0),
      m_data(0), m_notify(false), m_broadcast(broadcast)
{
    XDebug(DebugAll,"Message::Message(\"%s\",\"%s\",%s) [%p]",
	name,retval,String::boolText(broadcast),this);
}

Message::Message(const Message& original)
    : NamedList(original),
      m_return(original.retValue()), m_time(original.msgTime()),
      m_timeEnqueue(original.m_timeEnqueue), m_timeDispatch(original.m_timeDispatch),
      m_data(0),
      m_notify(false), m_broadcast(original.broadcast())
{
    XDebug(DebugAll,"Message::Message(&%p) [%p]",&original,this);
}

Message::Message(const Message& original, bool broadcast)
    : NamedList(original),
      m_return(original.retValue()), m_time(original.msgTime()),
      m_timeEnqueue(original.m_timeEnqueue), m_timeDispatch(original.m_timeDispatch),
      m_data(0),
      m_notify(false), m_broadcast(broadcast)
{
    XDebug(DebugAll,"Message::Message(&%p,%s) [%p]",
	&original,String::boolText(broadcast),this);
}

Message::~Message()
{
    XDebug(DebugAll,"Message::~Message() '%s' [%p]",c_str(),this);
    userData(0);
}

void* Message::getObject(const String& name) const
{
    if (name == YATOM("Message"))
	return const_cast<Message*>(this);
    return NamedList::getObject(name);
}

void Message::userData(RefObject* data)
{
    if (data == m_data)
	return;
    m_notify = false;
    RefObject* tmp = m_data;
    if (data && !data->ref())
	data = 0;
    m_data = data;
    if (tmp)
	tmp->deref();
}

void Message::dispatched(bool accepted)
{
    if (!m_notify)
	return;
    MessageNotifier* hook = YOBJECT(MessageNotifier,m_data);
    if (hook)
	hook->dispatched(*this,accepted);
}

void Message::resetMsg(Time tm)
{
    m_return.clear();
    m_time = m_timeEnqueue = m_timeDispatch = tm;
    if (Engine::trackParam())
	clearParam(Engine::trackParam());
}

String Message::encode(const char* id) const
{
    String s("%%>message:");
    s << String::msgEscape(id) << ":" << (unsigned int)m_time.sec() << ":";
    commonEncode(s);
    return s;
}

String Message::encode(bool received, const char* id) const
{
    String s("%%<message:");
    s << String::msgEscape(id) << ":" << received << ":";
    commonEncode(s);
    return s;
}

int Message::decode(const char* str, String& id)
{
    String s("%%>message:");
    if (!str || ::strncmp(str,s.c_str(),s.length()))
	return -1;
    // locate the SEP after id
    const char *sep = ::strchr(str+s.length(),':');
    if (!sep)
	return s.length();
    // locate the SEP after time
    const char *sep2 = ::strchr(sep+1,':');
    if (!sep2)
	return sep-str;
    id.assign(str+s.length(),(sep-str)-s.length());
    int err = -1;
    id = id.msgUnescape(&err);
    if (err >= 0)
	return err+s.length();
    String t(sep+1,sep2-sep-1);
    unsigned int tm = 0;
    t >> tm;
    if (!t.null())
	return sep-str;
    m_time=tm ? ((u_int64_t)1000000)*tm : Time::now();
    return commonDecode(str,sep2-str+1);
}

int Message::decode(const char* str, bool& received, const char* id)
{
    String s("%%<message:");
    s << id << ":";
    if (!str || ::strncmp(str,s.c_str(),s.length()))
	return -1;
    // locate the SEP after received
    const char *sep = ::strchr(str+s.length(),':');
    if (!sep)
	return s.length();
    String rcvd(str+s.length(),(sep-str)-s.length());
    rcvd >> received;
    if (!rcvd.null())
	return s.length();
    return sep[1] ? commonDecode(str,sep-str+1) : -2;
}

void Message::commonEncode(String& str) const
{
    str << msgEscape() << ":" << m_return.msgEscape();
    unsigned n = length();
    for (unsigned i = 0; i < n; i++) {
	NamedString *s = getParam(i);
	if (s)
	    str << ":" << s->name().msgEscape('=') << "=" << s->msgEscape();
    }
}

int Message::commonDecode(const char* str, int offs)
{
    str += offs;
    // locate SEP after name
    const char *sep = ::strchr(str,':');
    if (!sep)
	return offs;
    String chunk(str,sep-str);
    int err = -1;
    chunk = chunk.msgUnescape(&err);
    if (err >= 0)
	return offs+err;
    if (!chunk.null())
	*this = chunk;
    offs += (sep-str+1);
    str = sep+1;
    // locate SEP or EOL after retval
    sep = ::strchr(str,':');
    if (sep)
	chunk.assign(str,sep-str);
    else
	chunk.assign(str);
    chunk = chunk.msgUnescape(&err);
    if (err >= 0)
	return offs+err;
    m_return = chunk;
    // find and assign name=value pairs
    while (sep) {
	offs += (sep-str+1);
	str = sep+1;
	sep = ::strchr(str,':');
	if (sep)
	    chunk.assign(str,sep-str);
	else
	    chunk.assign(str);
	if (chunk.null())
	    continue;
	chunk = chunk.msgUnescape(&err);
	if (err >= 0)
	    return offs+err;
	int pos = chunk.find('=');
	switch (pos) {
	    case -1:
		clearParam(chunk);
		break;
	    case 0:
		return offs+err;
	    default:
		setParam(chunk.substr(0,pos),chunk.substr(pos+1));
	}
    }
    return -2;
}


void MessageFilter::setFilter(NamedString* filter)
{
    Regexp* r = YOBJECT(Regexp,filter);
    if (r)
	setFilter(new MatchingItemRegexp(filter->name(),*r));
    else if (filter)
	setFilter(new MatchingItemString(filter->name(),*filter));
    else
	clearFilter();
    TelEngine::destruct(filter);
}

void MessageFilter::set(MatchingItemBase*& dest, MatchingItemBase* src)
{
    if (dest == src)
	return;
    MatchingItemBase* tmp = dest;
    dest = src;
    TelEngine::destruct(tmp);
}


MessageHandler::MessageHandler(const char* name, unsigned priority,
	const char* trackName, bool addPriority)
    : String(name),
      m_trackName(trackName), m_trackNameOnly(trackName), m_priority(priority),
      m_dispatcher(0), m_counter(0)
{
    DDebug(DebugAll,"MessageHandler::MessageHandler('%s',%u,'%s',%s) [%p]",
	name,priority,trackName,String::boolText(addPriority),this);
    if (addPriority && m_trackName)
	m_trackName << ":" << priority;
    m_counter = Thread::getCurrentObjCounter(true);
}

MessageHandler::~MessageHandler()
{
    DDebug(DebugAll,"MessageHandler::~MessageHandler() '%s', %u [%p]",
	safe(),m_priority,this);
    cleanup();
}

void MessageHandler::cleanup()
{
    if (m_dispatcher) {
	m_dispatcher->uninstall(this);
	m_dispatcher = 0;
    }
    clearFilter();
}

void MessageHandler::destruct()
{
    cleanup();
    String::destruct();
}

void MessageHandler::safeNowInternal()
{
    WLock lck(m_dispatcher ? &m_dispatcher->handlersLock() : 0);
    // when the unsafe counter reaches zero we're again safe to destroy
    int v = --m_unsafe;
    if (v < 0)
	Debug(DebugFail,"MessageHandler(%s) unsafe=%d dispatcher=(%p) [%p]",
	    safe(),v,m_dispatcher,this);
}

bool MessageHandler::receivedInternal(Message& msg)
{
    bool ok = received(msg);
    safeNowInternal();
    return ok;
}


bool MessageRelay::receivedInternal(Message& msg)
{
    MessageReceiver* receiver = m_receiver;
    int id = m_id;
    safeNowInternal(); // At this point the relay itself may be uninstalled
    return receiver && receiver->received(msg,id);
}


MessageDispatcher::MessageDispatcher(const char* trackParam)
    : m_handlersLock("DispatcherHandlers"), m_messagesLock("DispatcherMsgs"), 
      m_hooksLock("DispatcherHooks"),
      m_msgAppend(&m_messages), m_hookAppend(&m_hooks),
      m_trackParam(trackParam), m_changes(0), m_warnTime(0),
      m_enqueueCount(0), m_dequeueCount(0), m_dispatchCount(0),
      m_queuedMax(0), m_msgAvgAge(0),
      m_traceTime(false), m_traceHandlerTime(false),
      m_hookCount(0), m_hookHole(false)
{
    XDebug(DebugInfo,"MessageDispatcher::MessageDispatcher('%s') [%p]",trackParam,this);
}

MessageDispatcher::~MessageDispatcher()
{
    XDebug(DebugInfo,"MessageDispatcher::~MessageDispatcher() [%p]",this);
    clear();
}

void MessageDispatcher::clear()
{
    WLock lck(m_handlersLock);
    m_handlers.clear();
    lck.acquire(m_hooksLock);
    m_hookAppend = &m_hooks;
    m_hooks.clear();
}

bool MessageDispatcher::install(MessageHandler* handler)
{
    DDebug(DebugAll,"MessageDispatcher::install(%p)",handler);
    if (!handler)
	return false;
    WLock lck(m_handlersLock);
    ObjList *l = m_handlers.find(handler);
    if (l)
	return false;
    unsigned p = handler->priority();
    int pos = 0;
    for (l=&m_handlers; l; l=l->next(),pos++) {
	MessageHandler *h = static_cast<MessageHandler *>(l->get());
	if (!h)
	    continue;
	if (h->priority() < p)
	    continue;
	if (h->priority() > p)
	    break;
	// at the same priority we sort them in pointer address order
	if (h > handler)
	    break;
    }
    m_changes++;
    if (l) {
	XDebug(DebugAll,"Inserting handler [%p] on place #%d",handler,pos);
	l->insert(handler);
    }
    else {
	XDebug(DebugAll,"Appending handler [%p] on place #%d",handler,pos);
	m_handlers.append(handler);
    }
    handler->m_dispatcher = this;
    if (handler->null())
	Debug(DebugInfo,"Registered broadcast message handler %p",handler);
    return true;
}

bool MessageDispatcher::uninstall(MessageHandler* handler)
{
    DDebug(DebugAll,"MessageDispatcher::uninstall(%p)",handler);
    WLock lck(m_handlersLock);
    handler = static_cast<MessageHandler *>(m_handlers.remove(handler,false));
    if (handler) {
	m_changes++;
	if (handler->m_unsafe > 0) {
	    DDebug(DebugNote,"Waiting for unsafe MessageHandler %p '%s'",
		handler,handler->c_str());
	    // wait until handler is again safe to destroy
	    do {
		lck.drop();
		Thread::yield();
		lck.acquire(m_handlersLock);
	    } while (handler->m_unsafe > 0);
	}
	if (handler->m_unsafe != 0)
	    Debug(DebugFail,"MessageHandler %p has unsafe=%d",handler,(int)handler->m_unsafe);
	handler->m_dispatcher = 0;
    }
    return (handler != 0);
}

bool MessageDispatcher::dispatch(Message& msg)
{
#ifdef XDEBUG
    Debugger debug("MessageDispatcher::dispatch","(%p) (\"%s\")",&msg,msg.c_str());
#endif

    u_int64_t t = 0;
    if (m_warnTime || m_traceTime) {
	Time now;
	if (m_warnTime)
	    t = now;
	if (m_traceTime)
	    msg.m_timeDispatch = now;
    }

    bool retv = false;
    bool counting = getObjCounting();
    NamedCounter* saved = Thread::getCurrentObjCounter(counting);
    String hTrackName;
    unsigned int hTrackPos = 0;
    bool hTrackTime = m_traceHandlerTime;
    ObjList *l = &m_handlers;
    RLock lck(m_handlersLock);
    m_dispatchCount++;
    for (; l; l=l->next()) {
	MessageHandler *h = static_cast<MessageHandler*>(l->get());
	if (h && (h->null() || *h == msg)) {
	    if (h->filter() && !h->filter()->matchListParam(msg))
		continue;
	    if (counting)
		Thread::setCurrentObjCounter(h->objectsCounter());

	    unsigned int c = m_changes;
	    unsigned int p = h->priority();
	    if (trackParam() && h->trackName()) {
		NamedString* tracked = msg.getParam(trackParam());
		if (tracked)
		    tracked->append(h->trackName(),",");
		else
		    msg.addParam(trackParam(),h->trackName());
		if (hTrackTime) {
		    hTrackName = h->trackName();
		    hTrackPos = tracked ? tracked->length() : hTrackName.length();
		}
	    }
	    // mark handler as unsafe to destroy / uninstall
	    h->m_unsafe++;
	    lck.drop();

	    u_int64_t tm = (m_warnTime || hTrackTime) ? Time::now() : 0;

	    retv = h->receivedInternal(msg) || retv;

	    if (tm) {
		tm = Time::now() - tm;
		if (m_warnTime && tm > m_warnTime) {
		    lck.acquire(m_handlersLock);
		    const char* name = (c == m_changes) ? h->trackName().c_str() : 0;
		    Debug(DebugInfo,"Message '%s' [%p] passed through %p%s%s%s in " FMT64U " usec",
			msg.c_str(),&msg,h,
			(name ? " '" : ""),(name ? name : ""),(name ? "'" : ""),tm);
		}
		if (hTrackTime && hTrackName) {
		    NamedString* tracked = msg.getParam(trackParam());
		    unsigned int start = hTrackPos - hTrackName.length();
		    if (tracked && start < tracked->length()) {
			if (0 == ::strncmp(tracked->c_str() + start,hTrackName.c_str(),hTrackName.length())) {
			    String buf;
			    buf.printf("#%u.%03u",(unsigned int)(tm / 1000),
				(unsigned int)(tm % 1000));
			    char c = (*tracked)[hTrackPos];
			    if (!c)
				*tracked << buf;
			    else if (',' == c) // Message re-dispatched. New handler name added
				tracked->insert(hTrackPos,buf,buf.length());
			}
		    }
		}
	    }

	    if (retv && !msg.broadcast())
		break;
	    lck.acquire(m_handlersLock);
	    if (c == m_changes)
		continue;
	    // the handler list has changed - find again
	    NDebug(DebugAll,"Rescanning handler list for '%s' [%p] at priority %u",
		msg.c_str(),&msg,p);
	    ObjList* l2 = &m_handlers;
	    for (l = l2; l; l=l->next()) {
		MessageHandler *mh = static_cast<MessageHandler*>(l->get());
		if (!mh)
		    continue;
		if (mh == h)
		    // exact match - silently continue where we left
		    break;

		// gone past last handler priority - exit with last handler
		if ((mh->priority() > p) || ((mh->priority() == p) && (mh > h))) {
		    Debug(DebugAll,"Handler list for '%s' [%p] changed, skipping from %p (%u) to %p (%u)",
			msg.c_str(),&msg,h,p,mh,mh->priority());
		    // l will advance in the outer for loop so use previous
		    l = l2;
		    break;
		}
		l2 = l;
	    }
	    if (!l)
		break;
	}
    }
    lck.drop();
    if (counting)
	Thread::setCurrentObjCounter(msg.getObjCounter());
    msg.dispatched(retv);
    if (counting)
	Thread::setCurrentObjCounter(saved);

    if (t) {
	t = Time::now() - t;
	if (t > m_warnTime) {
	    unsigned n = msg.length();
	    String p;
	    p << "\r\n  retval='" << msg.retValue().safe("(null)") << "'";
	    for (unsigned i = 0; i < n; i++) {
		NamedString *s = msg.getParam(i);
		if (s)
		    p << "\r\n  param['" << s->name() << "'] = '" << *s << "'";
	    }
	    Debug("Performance",DebugMild,"Message %p '%s' returned %s in " FMT64U " usec%s",
		&msg,msg.c_str(),retv ? "true" : "false",t,p.safe());
	}
    }

    lck.acquire(m_hooksLock);
    if (m_hookHole && !m_hookCount) {
	// compact the list, remove the holes
	for (l = &m_hooks; l; l = l->next()) {
	    while (!l->get()) {
		if (!l->next())
		    break;
		if (l->next() == m_hookAppend)
		    m_hookAppend = &m_hooks;
		l->remove();
	    }
	}
	m_hookHole = false;
    }
    m_hookCount++;
    for (l = m_hooks.skipNull(); l; l = l->skipNext()) {
	RefPointer<MessagePostHook> ph = static_cast<MessagePostHook*>(l->get());
	if (ph) {
	    lck.drop();
	    if (ph->matchesMsg(msg)) {
		if (counting)
		    Thread::setCurrentObjCounter(ph->getObjCounter());
		ph->dispatched(msg,retv);
	    }
	    ph = 0;
	    lck.acquire(m_hooksLock);
	}
    }
    m_hookCount--;
    lck.drop();
    if (counting)
	Thread::setCurrentObjCounter(saved);

    return retv;
}

bool MessageDispatcher::enqueue(Message* msg)
{
    WLock lck(m_messagesLock);
    if (!msg || m_messages.find(msg))
	return false;
    if (m_traceTime)
	msg->m_timeEnqueue = Time::now();
    m_msgAppend = m_msgAppend->append(msg);
    u_int64_t count = (++m_enqueueCount) - m_dequeueCount;
    if (m_queuedMax < count)
	m_queuedMax = count;
    return true;
}

bool MessageDispatcher::dequeueOne()
{
    WLock lck(m_messagesLock);
    if (m_messages.next() == m_msgAppend)
	m_msgAppend = &m_messages;
    Message* msg = static_cast<Message *>(m_messages.remove(false));
    if (!msg)
	return false;
    m_dequeueCount++;
    uint64_t age = Time::now() - msg->msgTime();
    if (age < 60000000)
	m_msgAvgAge = (3 * m_msgAvgAge + age) >> 2;
    lck.drop();
    dispatch(*msg);
    msg->destruct();
    return true;
}

void MessageDispatcher::dequeue()
{
    while (dequeueOne())
	;
}

unsigned int MessageDispatcher::messageCount()
{
    RLock lck(m_messagesLock);
    return (unsigned int)(m_enqueueCount - m_dequeueCount);
}

unsigned int MessageDispatcher::handlerCount()
{
    RLock lck(m_handlersLock);
    return m_handlers.count();
}

unsigned int MessageDispatcher::postHookCount()
{
    RLock lck(m_hooksLock);
    return m_hooks.count();
}

void MessageDispatcher::getStats(u_int64_t& enqueued, u_int64_t& dequeued, u_int64_t& dispatched, u_int64_t& queueMax)
{
    RLock lck(m_messagesLock);
    enqueued = m_enqueueCount;
    dequeued = m_dequeueCount;
    queueMax = m_queuedMax;
    lck.acquire(m_handlersLock);
    dispatched = m_dispatchCount;
}

bool MessageDispatcher::setHook(MessagePostHook* hook, bool remove)
{
    if (!hook)
	return false;
    WLock lck(m_hooksLock);
    ObjList* l = m_hooks.find(hook);
    if (remove) {
	// zero the hook, we'll compact it later when safe
	if (!l)
	    return false;
	l->set(0,false);
	m_hookHole = true;
    }
    else if (l)
	return false;
    else
	m_hookAppend = m_hookAppend->append(hook);
    return true;
}

static inline bool matchHandler(const MessageHandler& h, const String* nameMatch,
    const String* trackNameMatch)
{
    return (!nameMatch || nameMatch->matches((const String&)(h)))
	&& (!trackNameMatch || trackNameMatch->matches(h.trackNameOnly()));
}

unsigned int MessageDispatcher::fillHandlersInfo(const String* nameMatch,
    const String* trackNameMatch, ObjList* details, unsigned int* total)
{
    unsigned int n = 0;
    unsigned int matched = 0;
    RLock lck(m_handlersLock);
    for (ObjList* o = m_handlers.skipNull(); o; o = o->skipNext()) {
	n++;
	MessageHandler* h = static_cast<MessageHandler*>(o->get());
	if (!matchHandler(*h,nameMatch,trackNameMatch))
	    continue;
	matched++;
	if (!details)
	    continue;
	String* tmp = new String;
	tmp->printf("%s=%u|%s|%s",h->safe(),h->priority(),h->trackNameOnly().safe(),
	    h->filter() ? "yes" : "no");
	details = details->append(tmp);
    }
    if (total)
	*total = n;
    return matched;
}

unsigned int MessageDispatcher::dumpHandlersInfo(const String* nameMatch,
    const String* trackNameMatch, ObjList& buf, unsigned int* total)
{
    ObjList* add = &buf;
    unsigned int n = 0;
    unsigned int matched = 0;
    RLock lck(m_handlersLock);
    for (ObjList* o = m_handlers.skipNull(); o; o = o->skipNext()) {
	n++;
	MessageHandler* h = static_cast<MessageHandler*>(o->get());
	if (!matchHandler(*h,nameMatch,trackNameMatch))
	    continue;
	matched++;
	String* tmp = new String;
	tmp->printf("%s priority=%u trackname='%s'",h->safe(),h->priority(),
	    h->trackNameOnly().safe());
	if (h->filter()) {
	    String tmp2;
	    *tmp << "\r\n  Filter:"
		<< MatchingItemDump::dumpItem(h->filter(),tmp2,"\r\n  ","  ");
	}
	add = add->append(tmp);
    }
    if (total)
	*total = n;
    return matched;
}


MessageNotifier::~MessageNotifier()
{
}

/**
 * class MessageQueue
 */

static const char* s_queueMutexName = "MessageQueue";

MessageQueue::MessageQueue(const char* queueName, int numWorkers)
    : Mutex(true,s_queueMutexName), m_filters(queueName), m_count(0)
{
    XDebug(DebugAll,"Creating MessageQueue for %s",queueName);
    for (int i = 0;i < numWorkers;i ++) {
	QueueWorker* worker = new QueueWorker(this);
	worker->startup();
	m_workers.append(worker);
    }
    m_append = &m_messages;
}

void MessageQueue::received(Message& msg)
{
    Engine::dispatch(msg);
}

MessageQueue::~MessageQueue()
{
    XDebug(DebugAll,"Destroying MessageQueue for %s",m_filters.c_str());
}

void MessageQueue::clear()
{
    Lock myLock(this);
    for (ObjList* o = m_workers.skipNull();o;o = o->skipNext()) {
	QueueWorker* worker = static_cast<QueueWorker*>(o->get());
	worker->cancel();
	o->setDelete(false);
    }
    m_workers.clear();
    m_messages.clear();
}

bool MessageQueue::enqueue(Message* msg)
{
    if (!msg)
	return false;
    Lock myLock(this);
    m_append = m_append->append(msg);
    m_count++;
    return true;
}

bool MessageQueue::dequeue()
{
    Lock myLock(this);
    ObjList* o = m_messages.skipNull();
    if (!o)
	return false;
    if (m_messages.next() == m_append)
	m_append = &m_messages;
    Message* msg = static_cast<Message*>(m_messages.remove(false));
    if (!msg)
	return false;
    m_count--;
    myLock.drop();
    received(*msg);
    TelEngine::destruct(msg);
    return true;
}

void MessageQueue::addFilter(const char* name, const char* value)
{
    Lock myLock(this);
    m_filters.setParam(name,value);
}

void MessageQueue::removeFilter(const String& name)
{
    Lock myLock(this);
    m_filters.clearParam(name);
}

bool MessageQueue::matchesFilter(const Message& msg)
{
    Lock myLock(this);
    if (msg != m_filters)
	return false;
    for (unsigned int i = 0;i < m_filters.length();i++) {
	NamedString* param = m_filters.getParam(i);
	if (!param)
	    continue;
	NamedString* match = msg.getParam(param->name());
	if (!match || *match != *param)
	    return false;
    }
    return true;
}

void MessageQueue::removeThread(Thread* thread)
{
    if (!thread)
	return;
    Lock myLock(this);
    m_workers.remove((GenObject*)thread,false);
}

/**
 * class QueueWorker
 */

QueueWorker::~QueueWorker()
{
    if (m_queue)
	m_queue->removeThread(this);
    m_queue = 0;
}

void QueueWorker::run()
{
    if (!m_queue)
	return;
    while (true) {
	if (!m_queue->count()) {
	    Thread::idle(true);
	    continue;
	}
	m_queue->dequeue();
	Thread::check(true);
    }
}


/* vi: set ts=8 sw=4 sts=4 noet: */
