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

#ifndef _REDIS_H_
#define _REDIS_H_
#include <valuearray.h>
#include <value.h>
#include <rts2db/constraints.h>
#include <rts2db/devicedb.h>
#include <rts2db/plan.h>
#include <rts2db/target.h>
#include <devclient.h>
#include <hiredis.h>

class RedisProxy : public rts2db::DeviceDb
{

public:

    RedisProxy (int argc, char **argv);

    virtual ~RedisProxy (void);

    virtual void postEvent (rts2core::Event * event);

    /* virtual void deviceReady (rts2core::Connection * conn); */

    virtual int info ();

    virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

    virtual int commandAuthorized (rts2core::Connection * conn);

    void stateChangedEvent(rts2core::Connection *conn, rts2core::ServerState *new_state);
    
    void valueChangedEvent (rts2core::Connection *conn, rts2core::Value *new_value);

    rts2core::ConnNotify *getNotifyConnection () { return notifyConn; }

    virtual rts2core::DevClient *createOtherType (rts2core::Connection *conn, int other_device_type);

    virtual void message (rts2core::Message & msg);

protected:
    virtual int processOption (int in_opt);

    virtual int init ();

    virtual int reloadConfig ();

    virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

    virtual int deleteConnection (rts2core::Connection * in_conn);

private:
    rts2core::ConnNotify *notifyConn;
    redisContext *redisConn;
};

class RedisProxyClient : public rts2core::DevClient
{

public:
    RedisProxyClient(rts2core::Connection *conn):rts2core::DevClient(conn){}
    
    virtual void stateChanged (rts2core::ServerState *state)
    {
        (getMaster())->stateChangedEvent(getConnection(), state);
        rts2core::DevClient::stateChanged(state);
    }

    virtual void valueChanged (rts2core::Value *value)
    {
        (getMaster())->valueChangedEvent(getConnection(), value);
        rts2core::DevClient::valueChanged(value);
    }

protected:
    virtual RedisProxy *getMaster()
    {
        return (RedisProxy*) rts2core::DevClient::getMaster();
    }
};

#endif // _REDIS_H_
