/*
 * Redis daemon.
 * Copyright (C) 2016 Guangyu Zhang
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <string>
#include "redis.h"

using namespace std;

RedisProxy::RedisProxy (int in_argc, char **in_argv):rts2db::DeviceDb (in_argc, in_argv, DEVICE_TYPE_REDIS, "REDIS")
{
    redisConn = redisConnect("127.0.0.1", 6379);
    if (redisConn != NULL && redisConn->err) {
        logStream (MESSAGE_ERROR) << "Redis connection error: " << redisConn->errstr << sendLog;
    }
}

RedisProxy::~RedisProxy (void)
{

}

int RedisProxy::processOption (int in_opt)
{
    return rts2db::DeviceDb::processOption (in_opt);
}

int RedisProxy::init ()
{
	int ret = rts2db::DeviceDb::init ();
	if (ret)
		return ret;

	notifyConn = new rts2core::ConnNotify (this);
	rts2db::MasterConstraints::setNotifyConnection (notifyConn);

	ret = notifyConn->init ();
	if (ret)
		return ret;

	addConnection (notifyConn);

	return ret;
}

int RedisProxy::reloadConfig ()
{
	int ret;
	// Configuration *_config;
	ret = rts2db::DeviceDb::reloadConfig ();
	if (ret)
		return ret;
	// _config = Configuration::instance ();

	return 0;
}

int RedisProxy::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	return rts2db::DeviceDb::setValue (oldValue, newValue);
}

int RedisProxy::deleteConnection (rts2core::Connection * in_conn)
{
    redisReply *r, *r2;
    string connName;
    if (in_conn == getSingleCentralConn())
        connName = "centrald";
    else
        connName = string(in_conn->getName());
    r = (redisReply*)redisCommand(redisConn, "SREM rts2:devices %s", connName.c_str());
    freeReplyObject(r);
    r = (redisReply*)redisCommand(redisConn, "KEYS rts2:%s*", connName.c_str());
    for (size_t i=0; i < r->elements; i++) {
        r2 = (redisReply*)redisCommand(redisConn, "DEL %s", r->element[i]->str);
        freeReplyObject(r2);
    }
    freeReplyObject(r);
    r = (redisReply*)redisCommand(redisConn, "PUBLISH %s disconnect", connName.c_str());
    freeReplyObject(r);
	return 0;
}

void RedisProxy::postEvent (rts2core::Event * event)
{
	rts2core::Device::postEvent (event);
}

int RedisProxy::info ()
{
	return rts2db::DeviceDb::info ();
}

void RedisProxy::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	return rts2db::DeviceDb::changeMasterState (old_state, new_state);
}

int RedisProxy::commandAuthorized (rts2core::Connection * conn)
{
    if (conn->isCommand ("stop"))
    {
        return 0;
    }
	return rts2db::DeviceDb::commandAuthorized (conn);
}

rts2core::DevClient *RedisProxy::createOtherType (rts2core::Connection *conn, int other_device_type)
{
    redisReply *r;
    string connName(conn->getName());
    if (connName == "") connName = "centrald";
    r = (redisReply*)redisCommand(redisConn, "SADD rts2:devices %s", connName.c_str());
    freeReplyObject(r);
    r = (redisReply*)redisCommand(redisConn, "PUBLISH %s connect", connName.c_str());
    freeReplyObject(r);
    return new RedisProxyClient (conn);
}

void RedisProxy::stateChangedEvent(rts2core::Connection *conn, rts2core::ServerState *new_state)
{
    redisReply *r;
    string connName(conn->getName());
    if (conn == getSingleCentralConn())
        connName = "centrald";
    r = (redisReply*)redisCommand(redisConn, "SET rts2:%s:State %d", connName.c_str(), new_state->getValue());
    freeReplyObject(r);
    r = (redisReply*)redisCommand(redisConn, "PUBLISH %s state", connName.c_str());
    freeReplyObject(r);
}

void RedisProxy::valueChangedEvent(rts2core::Connection *conn, rts2core::Value *new_value)
{
    redisReply *r;
    string connName(conn->getName());
    if (conn == getSingleCentralConn())
        connName = "centrald";
    r = (redisReply*)redisCommand(redisConn, "SADD rts2:%s:values %s", connName.c_str(), string(new_value->getName()).c_str());
    freeReplyObject(r);
    string key = "rts2:" + connName + ":" + new_value->getName();
    r = (redisReply*)redisCommand(redisConn, "SET %s %s", key.c_str(), new_value->getValue());
    freeReplyObject(r);
    r = (redisReply*)redisCommand(redisConn, "PUBLISH %s value %s", connName.c_str(), new_value->getName().c_str());
    freeReplyObject(r);
}

void RedisProxy::message(rts2core::Message &msg)
{
    struct tm tmesg;
    time_t t = msg.getMessageTimeSec();
    localtime_r (&t, &tmesg);
    char buf[1000];
    snprintf(buf, 1000, "%02i:%02i:%02i.%03i %s %s %s", tmesg.tm_hour, tmesg.tm_min, tmesg.tm_sec,
             (int)(msg.getMessageTimeUSec() / 1000), msg.getMessageOName(), msg.getTypeString(), msg.getMessageString().c_str());

    redisReply *r;
    r = (redisReply*)redisCommand(redisConn, "PUBLISH message %s", buf);
    freeReplyObject(r);
}

int main (int argc, char **argv)
{
    RedisProxy proxy (argc, argv);
    return proxy.run();
}
