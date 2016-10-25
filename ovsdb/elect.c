/* Copyright (c) 2009, 2010, 2011, 2016 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "cetcd.h"
#include "elect.h"
#include "jsonrpc.h"
#include "unixctl.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(elect);

struct elect {
    char *etcd_ip_address;
    char *etcd_path_data;
    cetcd_array address;
    cetcd_array watchers;
    cetcd_client etcd_client;
    cetcd_watch_id watcher_id;
    char *local_domain;
    char *etcd_node_key;
    pthread_t thread;
    struct jsonrpc *rpc_client;
    int sync_flag;
};

static struct elect *g_elect;

static char *
my_xstrdup(const char *s)
{
    unsigned int length = strlen(s);
    char *d = malloc(length + 1);
    memcpy(d, s, length);
    d[length] = '\0';
    return d;
}

/*
 * 获取主节点Ip, key值最小的就是leader
 * 成功返回0，失败返回错误码
 */
static int
ovsdb_elect_get_leader(char **sync_from, int *is_backup)
{
    cetcd_response *resp = cetcd_lsdir(&g_elect->etcd_client, g_elect->etcd_path_data, 1, 1);
    if(resp->err) {
        VLOG_ERR("lsdir %s from etcd failed. error :%d, %s (%s)\n", 
            g_elect->etcd_path_data, resp->err->ecode, resp->err->message, resp->err->cause);
        cetcd_response_release(resp);
        return 1;
    }

    cetcd_response_node *small_node = NULL;
    cetcd_array *nodes = resp->node->nodes;
    if (nodes == NULL) {
        VLOG_ERR("lsdir %s from etcd no nodes.", g_elect->etcd_path_data);
        cetcd_response_release(resp);
        return 1;
    }
    for (int i = 0; i < cetcd_array_size(nodes); ++i) {
        cetcd_response_node *node = cetcd_array_get(nodes, i);
        if (small_node == NULL) {
            small_node = node;
        } else {
            if (strcmp(small_node->key, node->key) > 0) {
                small_node = node;
            }
        }
    }
    if (small_node == NULL) {
        VLOG_ERR("faild get small key node from etcd by %s.", g_elect->etcd_path_data);
        cetcd_response_release(resp);
        return 1;
    }    
    if (strcmp(small_node->key, g_elect->etcd_node_key) == 0) {
        *is_backup = 0;
    } else {
        *is_backup = 1;
    }
    *sync_from = my_xstrdup(small_node->value);
    cetcd_response_release(resp);

    return 0;
}

/*
 * 通过命令行同步主进程主备状态
 * 成功返回0，失败返回错误码
 */
static int
ovsdb_elect_sync_to_ovsdb(void *sync_from_config_, void *sync_from_get_, int _is_backup) {
    int return_data = 0;
    char *result;
    char *err;
    char *argv[1] = {sync_from_get_};
    if (strcmp(sync_from_get_, *((char **)sync_from_config_)) != 0) {
        return_data = unixctl_client_transact(g_elect->rpc_client, "ovsdb-server/set-active-ovsdb-server", 1, argv, &result, &err);
        if (return_data != 0) {
            VLOG_ERR("transact ovsdb-server/set-active-ovsdb-server failed.");
            return return_data;
        }
        if (_is_backup) {
            return_data = unixctl_client_transact(g_elect->rpc_client, "ovsdb-server/connect-active-ovsdb-server", 0, NULL, &result, &err);
            if (return_data != 0) {
                VLOG_ERR("transact ovsdb-server/connect-active-ovsdb-server failed.");
            }
        } else {
            return_data = unixctl_client_transact(g_elect->rpc_client, "ovsdb-server/disconnect-active-ovsdb-server", 0, NULL, &result, &err);
            if (return_data != 0) {
                VLOG_ERR("transact ovsdb-server/disconnect-active-ovsdb-server failed.");
            }
        }
    }
    return return_data;
}

/*
 * 收到etcd变化事件后同步主进程主备状态
 */
static int
ovsdb_elect_sync(void *sync_from_config, cetcd_response *resp) {
    resp = resp;

    char *_sync_from = NULL;
    int _is_backup = 0;

    int err = ovsdb_elect_get_leader(&_sync_from, &_is_backup);
    if (err != 0) {
        return err;
    }
    if (ovsdb_elect_sync_to_ovsdb(sync_from_config, _sync_from, _is_backup) == 0) {
        g_elect->sync_flag = 1;
    }
    return 0;
}

/*
 * 侦听etcd变化事件
 */
static void
ovsdb_elect_add_watcher(void *sync_from)
{
    (void)cetcd_array_init(&g_elect->watchers, 3);
    cetcd_add_watcher(&g_elect->watchers, cetcd_watcher_create(&g_elect->etcd_client, g_elect->etcd_path_data, 0, 1, 0, ovsdb_elect_sync, sync_from));
    g_elect->watcher_id = cetcd_multi_watch_async(&g_elect->etcd_client, &g_elect->watchers);
}

/*
 * 向etcd更新本节点标记的TTL
 */
static void
ovsdb_elect_update_node(void)
{
    cetcd_response *resp = cetcd_update(&g_elect->etcd_client, g_elect->etcd_node_key, NULL, 2, 1);
    if(resp->err) {
        VLOG_ERR("update %s ttl to etcd failed. error :%d, %s (%s)\n", 
            g_elect->etcd_node_key, resp->err->ecode, resp->err->message, resp->err->cause);
    }
    cetcd_response_release(resp);
}

/*
 * 写入etcd本节点标记
 */
static void
ovsdb_elect_set_node(void)
{
    cetcd_response *resp = cetcd_create_in_order(&g_elect->etcd_client, g_elect->etcd_path_data, g_elect->local_domain, 2);
    if(resp->err) {
        VLOG_ERR("create %s:%s key to etcd failed. error :%d, %s (%s)\n", 
            g_elect->etcd_path_data, g_elect->local_domain, resp->err->ecode, resp->err->message, resp->err->cause);
        cetcd_response_release(resp);
        return;
    }
    g_elect->etcd_node_key = my_xstrdup(resp->node->key);
    cetcd_response_release(resp);
}

/*
 * 获取etcd本节点标记
 */
static int
ovsdb_elect_get_node(void)
{
    cetcd_response *resp = cetcd_get(&g_elect->etcd_client, g_elect->etcd_node_key);
    if(resp->err) {
        VLOG_ERR("get %s:%s key to etcd failed. error :%d, %s (%s)\n", 
            g_elect->etcd_node_key, g_elect->local_domain, resp->err->ecode, resp->err->message, resp->err->cause);
        cetcd_response_release(resp);
        return 1;
    }
    cetcd_response_release(resp);
    return 0;
}

/*
 * elect主线程loop
 */
static void
ovsdb_elect_pthread_loop(void *sync_from)
{
    while(1){
        sleep(1);
        if (ovsdb_elect_get_node() != 0) {
            ovsdb_elect_set_node();
        }
        ovsdb_elect_update_node();
        if (g_elect->sync_flag == 0) {
            ovsdb_elect_sync(sync_from, NULL);
        }
    }
}

/*
 * 初始化同步server和主备配置
 */
static void
ovsdb_elect_init_config(void *sync_from, void *is_backup)
{
    char *_sync_from = NULL;
    int _is_backup = 0;
    if (ovsdb_elect_get_leader(&_sync_from, &_is_backup) == 0) {
        *(char **)sync_from = _sync_from;
        *(char *)is_backup = _is_backup;
        g_elect->sync_flag = 1;
    }
}

/* 
 * 创建定时器，定时向etcd更新自己TTL
 * 侦听etcd变化事件
 */
void
ovsdb_elect_init(char *local_domain_, void *sync_from, void *is_backup)
{
    g_elect = calloc(1, sizeof(struct elect));
    if (g_elect == NULL) {
        return;
    }

    g_elect->etcd_ip_address = "127.0.0.1:2379";
    g_elect->etcd_path_data = "/northdb/";
    g_elect->local_domain = my_xstrdup(local_domain_);

    /*initialize the address array*/
    (void)cetcd_array_init(&g_elect->address, 3);

    /*append the address of etcd*/
    (void)cetcd_array_append(&g_elect->address, g_elect->etcd_ip_address);

    /*initialize the cetcd_client*/
    (void)cetcd_client_init(&g_elect->etcd_client, &g_elect->address);

    /*initialize the cetcd active node*/
    ovsdb_elect_set_node();

    /*initialize the update ttl timer*/
    pthread_create(&g_elect->thread, NULL, (void *(*)(void *))ovsdb_elect_pthread_loop, sync_from);

    /*initialize the etcd change watcher*/
    ovsdb_elect_add_watcher(sync_from);

    /*initialize the leader and sync-server config*/
    ovsdb_elect_init_config(sync_from, is_backup);
}

void
ovsdb_elect_init_unixctl_client(const char *path)
{
    /*initialize the unixctl client*/
    int err = unixctl_client_create(path, &g_elect->rpc_client);
    if (err != 0) {
        VLOG_FATAL("unixctl client create failed. path: %s", path);
    }
}

void
ovsdb_elect_fini(void)
{
    cetcd_multi_watch_async_stop(&g_elect->etcd_client, g_elect->watcher_id);
    cetcd_client_destroy(&g_elect->etcd_client);
    cetcd_array_destroy(&g_elect->address);
    cetcd_array_destroy(&g_elect->watchers);
    if (g_elect->etcd_node_key != NULL) {
        free(g_elect->etcd_node_key);
    }
    if (g_elect->local_domain != NULL) {
        free(g_elect->local_domain);
    }
    pthread_cancel(g_elect->thread);
    pthread_join(g_elect->thread, 0);
    jsonrpc_close(g_elect->rpc_client);
    if (g_elect != NULL) {
        free(g_elect);
        g_elect = NULL;
    }
}

