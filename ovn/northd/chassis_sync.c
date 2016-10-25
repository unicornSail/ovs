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

#include <string.h>
#include <unistd.h>
#include "cetcd.h"
#include "chassis_sync.h"
#include "ovn/lib/ovn-sb-idl.h"
#include "openvswitch/json.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(chassis_sync);

struct chassis_sync {
    char *etcd_ip_address;
    char *etcd_path_data;
    cetcd_array address;
    cetcd_array watchers;
    cetcd_client etcd_client;
    cetcd_watch_id watcher_id;
    struct ovsdb_idl *ovnsb_idl;
    struct ovsdb_idl_txn *ovnsb_idl_txn;
};

static struct chassis_sync *g_chassis_sync;

/* Chassis table. */
struct ovn_chassis_sync {
    /* encaps column. */
    struct ovn_encap_sync **encaps;
    size_t n_encaps;

    /* external_ids column. */
    struct smap external_ids;

    /* hostname column. */
    char *hostname;    /* Always nonnull. */

    /* name column. */
    char *name;    /* Always nonnull. */

    /* nb_cfg column. */
    int64_t nb_cfg;

    /* vtep_logical_switches column. */
    char **vtep_logical_switches;
    size_t n_vtep_logical_switches;
};

/* Encap table. */
struct ovn_encap_sync {
    /* ip column. */
    char *ip; /* Always nonnull. */

    /* options column. */
    struct smap options;

    /* type column. */
    char *type; /* Always nonnull. */
};

static bool
ovn_chassis_sync_encap_same(const struct sbrec_chassis *chassis_rec, 
                                            const struct ovn_chassis_sync *chassis_new){
    if (chassis_rec->n_encaps!= chassis_new->n_encaps) {
        return false;
    }
    for (int i =0; i < chassis_new->n_encaps; i++) {
        int encap_find = 0;
        for (int j =0; j < chassis_rec->n_encaps; j++) {
            if (strcmp(chassis_rec->encaps[i]->ip, chassis_new->encaps[j]->ip) && 
                strcmp(chassis_rec->encaps[i]->type, chassis_new->encaps[j]->type) &&
                smap_equal(&chassis_new->external_ids, &chassis_rec->external_ids) == true){
                encap_find = 1;
            }
        }
        if (encap_find == 0) {
            return false;
        }
    }
    return true;
}

static void
ovn_chassis_sync_txn_commit(void) {
    enum ovsdb_idl_txn_status status = ovsdb_idl_txn_commit_block(g_chassis_sync->ovnsb_idl_txn);
    VLOG_INFO("ovsdb_idl commit status %d.", status);
    if (status == TXN_ERROR) {
        VLOG_INFO("ovsdb_idl commit error: %s.", ovsdb_idl_txn_get_error(g_chassis_sync->ovnsb_idl_txn));
    }
    ovsdb_idl_txn_destroy(g_chassis_sync->ovnsb_idl_txn);
    // ovsdb_idl_run(g_chassis_sync->ovnsb_idl);
}

static void
ovn_chassis_sync_delete(const char *chassis_name)
{
    const struct sbrec_chassis *chassis_rec;
    SBREC_CHASSIS_FOR_EACH (chassis_rec, g_chassis_sync->ovnsb_idl) {
        if (strcmp(chassis_name, chassis_rec->name) == 0) {
            for (int i = 0; i < chassis_rec->n_encaps; i++) {
                sbrec_encap_delete((const struct sbrec_encap *)chassis_rec->encaps[i]);
            }
            sbrec_chassis_delete(chassis_rec);
            break;
        }
    }
}

static void
ovn_chassis_sync_create(const struct ovn_chassis_sync *chassis_new)
{
    const struct sbrec_chassis *chassis_rec;
    SBREC_CHASSIS_FOR_EACH (chassis_rec, g_chassis_sync->ovnsb_idl) {
        if (strcmp(chassis_new->name, chassis_rec->name) == 0) {
            if (ovn_chassis_sync_encap_same(chassis_rec, chassis_new) == false) {
                struct sbrec_encap **encaps = calloc(chassis_new->n_encaps, sizeof(struct sbrec_encap *));
                for (int i =0; i < chassis_new->n_encaps; i++) {
                    encaps[i] = sbrec_encap_insert(g_chassis_sync->ovnsb_idl_txn);
                    sbrec_encap_set_ip(encaps[i], chassis_new->encaps[i]->ip);
                    sbrec_encap_set_options(encaps[i], &chassis_new->encaps[i]->options);
                    sbrec_encap_set_type(encaps[i], chassis_new->encaps[i]->type);              
                }
                sbrec_chassis_set_encaps(chassis_rec, encaps, chassis_new->n_encaps);
                free(encaps);
            }
            if (smap_equal(&chassis_new->external_ids, &chassis_rec->external_ids) == false) {
                sbrec_chassis_set_external_ids(chassis_rec, &chassis_new->external_ids);
            }            
            if (strcmp(chassis_new->hostname, chassis_rec->hostname) != 0) {
                sbrec_chassis_set_hostname(chassis_rec, chassis_new->hostname);
            }
            if (chassis_new->nb_cfg != chassis_rec->nb_cfg) {
                sbrec_chassis_set_nb_cfg(chassis_rec, chassis_new->nb_cfg);
            }
            // sbrec_chassis_set_vtep_logical_switches(chassis_rec, (const char **)chassis_new->vtep_logical_switches, chassis_new->n_vtep_logical_switches);
            return;
        }
    }
    struct sbrec_chassis *chassis_rec_new = sbrec_chassis_insert(g_chassis_sync->ovnsb_idl_txn);
    struct sbrec_encap **encaps_data = calloc(chassis_new->n_encaps, sizeof(struct sbrec_encap *));
    for (int i =0; i < chassis_new->n_encaps; i++) {
        encaps_data[i] = sbrec_encap_insert(g_chassis_sync->ovnsb_idl_txn);
        sbrec_encap_set_ip(encaps_data[i], chassis_new->encaps[i]->ip);
        sbrec_encap_set_options(encaps_data[i], &chassis_new->encaps[i]->options);
        sbrec_encap_set_type(encaps_data[i], chassis_new->encaps[i]->type);              
    }
    sbrec_chassis_set_encaps(chassis_rec_new, encaps_data, chassis_new->n_encaps);
    sbrec_chassis_set_external_ids(chassis_rec_new, &chassis_new->external_ids);
    sbrec_chassis_set_hostname(chassis_rec_new, chassis_new->hostname);
    sbrec_chassis_set_name(chassis_rec_new, chassis_new->name);
    sbrec_chassis_set_nb_cfg(chassis_rec_new, chassis_new->nb_cfg);
    sbrec_chassis_set_vtep_logical_switches(chassis_rec_new, (const char **)chassis_new->vtep_logical_switches, chassis_new->n_vtep_logical_switches);
    free(encaps_data);
}

static void
ovn_chassis_sync_data_destroy(struct ovn_chassis_sync *chassis_rec) {
    for (int i = 0; i < chassis_rec->n_encaps; i++) {
        if (chassis_rec->encaps[i] != NULL) {
            if (chassis_rec->encaps[i]->ip != NULL) {
                free(chassis_rec->encaps[i]->ip);
            }
            if (chassis_rec->encaps[i]->type != NULL) {
                free(chassis_rec->encaps[i]->type);
            }
            smap_destroy(&chassis_rec->encaps[i]->options);
        }  
    }
    if (chassis_rec->encaps != NULL) {
        free(chassis_rec->encaps);
    }
    smap_destroy(&chassis_rec->external_ids);
    if (chassis_rec->hostname != NULL) {
        free(chassis_rec->hostname);
    }
    if (chassis_rec->name != NULL) {
        free(chassis_rec->name);
    }
    for (int i = 0; i < chassis_rec->n_vtep_logical_switches; i++) {
        if (chassis_rec->vtep_logical_switches[i] != NULL) {
            free(chassis_rec->vtep_logical_switches[i]);
        }        
    }
    if (chassis_rec->vtep_logical_switches != NULL) {
        free(chassis_rec->vtep_logical_switches);
    }
}

static struct ovn_chassis_sync *
ovn_chassis_sync_data_create(struct json *json_data) {
    struct ovn_chassis_sync *chassis_new = calloc(1, sizeof(struct ovn_chassis_sync));
    smap_init(&chassis_new->external_ids);
    struct shash_node *chassis_shash_node;
    SHASH_FOR_EACH (chassis_shash_node, json_object(json_data)) {
        if (strcmp(chassis_shash_node->name, sbrec_chassis_columns[SBREC_CHASSIS_COL_ENCAPS].name) == 0) {
            struct json_array *json_encaps = json_array(chassis_shash_node->data);
            chassis_new->encaps = calloc(json_encaps->n, sizeof(struct ovn_encap_sync *));
            for (int i = 0; i < json_encaps->n; i++) {
                struct ovn_encap_sync *encap_new = calloc(1, sizeof(struct ovn_encap_sync));
                smap_init(&encap_new->options);
                struct shash_node *encap_shash_node = NULL;
                SHASH_FOR_EACH (encap_shash_node, json_object(json_encaps->elems[i])) {
                    if (strcmp(encap_shash_node->name, sbrec_encap_columns[SBREC_ENCAP_COL_IP].name) == 0) {
                        encap_new->ip = xstrdup(json_string(encap_shash_node->data));
                    }
                    if (strcmp(encap_shash_node->name, sbrec_encap_columns[SBREC_ENCAP_COL_OPTIONS].name) == 0) {
                        struct shash_node *options_shash_node = NULL;
                        SHASH_FOR_EACH (options_shash_node, json_object(encap_shash_node->data)) {
                            smap_add_once(&encap_new->options, options_shash_node->name, xstrdup(json_string(options_shash_node->data)));
                        }
                    }
                    if (strcmp(encap_shash_node->name, sbrec_encap_columns[SBREC_ENCAP_COL_TYPE].name) == 0) {
                        encap_new->type = xstrdup(json_string(encap_shash_node->data));
                    }
                }
                chassis_new->n_encaps++;
                chassis_new->encaps[i] = encap_new;
            }
        }
        if (strcmp(chassis_shash_node->name, sbrec_chassis_columns[SBREC_CHASSIS_COL_EXTERNAL_IDS].name) == 0) {
            struct shash_node *external_ids_node;
            SHASH_FOR_EACH (external_ids_node, json_object(chassis_shash_node->data)) {
                smap_add_once(&chassis_new->external_ids, external_ids_node->name, xstrdup(json_string(external_ids_node->data)));
            }
        }
        if (strcmp(chassis_shash_node->name, sbrec_chassis_columns[SBREC_CHASSIS_COL_HOSTNAME].name) == 0) {
            chassis_new->hostname = xstrdup(json_string(chassis_shash_node->data));
        }
        if (strcmp(chassis_shash_node->name, sbrec_chassis_columns[SBREC_CHASSIS_COL_NAME].name) == 0) {
            chassis_new->name = xstrdup(json_string(chassis_shash_node->data));
        } 
        if (strcmp(chassis_shash_node->name, sbrec_chassis_columns[SBREC_CHASSIS_COL_NB_CFG].name) == 0) {
            chassis_new->nb_cfg = json_integer(chassis_shash_node->data);
        }
        if (strcmp(chassis_shash_node->name, sbrec_chassis_columns[SBREC_CHASSIS_COL_VTEP_LOGICAL_SWITCHES].name) == 0) {
            struct json_array *json_vtep_logical_switches = json_array(chassis_shash_node->data);
            chassis_new->vtep_logical_switches = calloc(json_vtep_logical_switches->n, sizeof(char *));
            for (int i = 0; i < json_vtep_logical_switches->n; i++) {
                chassis_new->vtep_logical_switches[i] = xstrdup(json_string(json_vtep_logical_switches->elems[i]));
                chassis_new->n_vtep_logical_switches++;
            }
        }
    }
    return chassis_new;
}

/*
 * 收到etcd变化
 */
static int
ovn_chassis_sync_change(void *user_data, cetcd_response *resp) {
    user_data = user_data;

    g_chassis_sync->ovnsb_idl_txn = ovsdb_idl_txn_create(g_chassis_sync->ovnsb_idl);
    if (resp->action == ETCD_SET || 
        resp->action == ETCD_UPDATE || 
        resp->action == ETCD_CREATE ||
        resp->action == ETCD_CAS) {
        VLOG_INFO("add or update %s. ", resp->node->key);
        struct json *json_data = json_from_string(resp->node->value);
        struct ovn_chassis_sync * chassis_data = ovn_chassis_sync_data_create(json_data);
        ovn_chassis_sync_create(chassis_data);
        ovn_chassis_sync_data_destroy(chassis_data);
        json_destroy(json_data);
    } else if (resp->action == ETCD_DELETE || 
               resp->action == ETCD_EXPIRE || 
               resp->action == ETCD_CAD) {
        VLOG_INFO("delete chassis %s.", resp->node->key);
        ovn_chassis_sync_delete(resp->node->key + strlen(g_chassis_sync->etcd_path_data) + 1);
    }
    ovn_chassis_sync_txn_commit();

    return 0;
}

/*
 * 收到etcd变化
 */
static int
ovn_chassis_sync(void) {
    cetcd_response *resp = cetcd_lsdir(&g_chassis_sync->etcd_client, g_chassis_sync->etcd_path_data, 1, 1);
    if(resp->err) {
        VLOG_ERR("lsdir %s from etcd failed. error :%d, %s (%s)\n", 
            g_chassis_sync->etcd_path_data, resp->err->ecode, resp->err->message, resp->err->cause);
        cetcd_response_release(resp);
        return 1;
    }
    cetcd_array *nodes = resp->node->nodes;
    g_chassis_sync->ovnsb_idl_txn = ovsdb_idl_txn_create(g_chassis_sync->ovnsb_idl);

    if (nodes == NULL) {
        VLOG_INFO("lsdir %s from etcd no nodes.", g_chassis_sync->etcd_path_data);
        const struct sbrec_chassis *chassis_rec_del, *chassis_next_del;
        SBREC_CHASSIS_FOR_EACH_SAFE (chassis_rec_del, chassis_next_del, g_chassis_sync->ovnsb_idl) {
            for (int i = 0; i < chassis_rec_del->n_encaps; i++) {
                sbrec_encap_delete((const struct sbrec_encap *)chassis_rec_del->encaps[i]);
            }
            sbrec_chassis_delete(chassis_rec_del);
        }
        ovn_chassis_sync_txn_commit();
        cetcd_response_release(resp);
        return 0;
    }
    const struct sbrec_chassis *chassis_rec, *chassis_next;
    SBREC_CHASSIS_FOR_EACH_SAFE (chassis_rec, chassis_next, g_chassis_sync->ovnsb_idl) {
        int find_chassis = 0;
        for (int i = 0; i < cetcd_array_size(nodes); ++i) {
            cetcd_response_node *node = cetcd_array_get(nodes, i);
            if (strcmp(node->key, chassis_rec->name) == 0) {
                find_chassis = 1;
            }
        }
        if (find_chassis == 0) {
            for (int i = 0; i < chassis_rec->n_encaps; i++) {
                sbrec_encap_delete((const struct sbrec_encap *)chassis_rec->encaps[i]);
            }
            sbrec_chassis_delete(chassis_rec);
        }
    }

    for (size_t i = 0; i < cetcd_array_size(nodes); ++i) {
        cetcd_response_node *node = cetcd_array_get(nodes, i);
        int find_chassis = 0;
        const struct sbrec_chassis *chassis_rec_node;
        SBREC_CHASSIS_FOR_EACH (chassis_rec_node, g_chassis_sync->ovnsb_idl) {
            if (strcmp(node->key, chassis_rec_node->name) == 0) {
                find_chassis = 1;
            }
        }
        if (find_chassis == 0) {
            struct json *json_data = json_from_string(node->value);
            struct ovn_chassis_sync *chassis_new = ovn_chassis_sync_data_create(json_data);
            struct sbrec_chassis *chassis_rec_new = sbrec_chassis_insert(g_chassis_sync->ovnsb_idl_txn);
            struct sbrec_encap **encaps_data = calloc(chassis_new->n_encaps, sizeof(struct sbrec_encap *));
            for (int i =0; i < chassis_new->n_encaps; i++) {
                encaps_data[i] = sbrec_encap_insert(g_chassis_sync->ovnsb_idl_txn);
                sbrec_encap_set_ip(encaps_data[i], chassis_new->encaps[i]->ip);
                sbrec_encap_set_options(encaps_data[i], &chassis_new->encaps[i]->options);
                sbrec_encap_set_type(encaps_data[i], chassis_new->encaps[i]->type);              
            }
            sbrec_chassis_set_encaps(chassis_rec_new, encaps_data, chassis_new->n_encaps);
            sbrec_chassis_set_external_ids(chassis_rec_new, &chassis_new->external_ids);
            sbrec_chassis_set_hostname(chassis_rec_new, chassis_new->hostname);
            sbrec_chassis_set_name(chassis_rec_new, chassis_new->name);
            sbrec_chassis_set_nb_cfg(chassis_rec_new, chassis_new->nb_cfg);
            sbrec_chassis_set_vtep_logical_switches(chassis_rec_new, (const char **)chassis_new->vtep_logical_switches, chassis_new->n_vtep_logical_switches);
            free(encaps_data);
            ovn_chassis_sync_data_destroy(chassis_new);
            json_destroy(json_data);
        }
    }

    ovn_chassis_sync_txn_commit();

    cetcd_response_release(resp);

    return 0;
}

/*
 * 侦听etcd变化事件
 */
static void
ovn_chassis_sync_add_watcher(void)
{
    (void)cetcd_array_init(&g_chassis_sync->watchers, 3);
    cetcd_add_watcher(&g_chassis_sync->watchers, cetcd_watcher_create(&g_chassis_sync->etcd_client, g_chassis_sync->etcd_path_data, 0, 1, 0, ovn_chassis_sync_change, NULL));
    g_chassis_sync->watcher_id = cetcd_multi_watch_async(&g_chassis_sync->etcd_client, &g_chassis_sync->watchers);
}

/* 
 * 侦听etcd变化事件
 */
void
ovn_chassis_sync_init(const char *ovnsb_db)
{
    g_chassis_sync = calloc(1, sizeof(struct chassis_sync));
    if (g_chassis_sync == NULL) {
        return;
    }

    g_chassis_sync->etcd_ip_address = "127.0.0.1:2379";
    g_chassis_sync->etcd_path_data = "/chassis/";
    g_chassis_sync->ovnsb_idl = ovsdb_idl_create(ovnsb_db, &sbrec_idl_class, false, true);
    ovsdb_idl_add_table(g_chassis_sync->ovnsb_idl, &sbrec_table_chassis);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_chassis_col_name);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_chassis_col_hostname);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_chassis_col_encaps);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_chassis_col_nb_cfg);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_chassis_col_vtep_logical_switches);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_chassis_col_external_ids);
    ovsdb_idl_add_table(g_chassis_sync->ovnsb_idl, &sbrec_table_encap);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_encap_col_ip);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_encap_col_type);
    ovsdb_idl_add_column(g_chassis_sync->ovnsb_idl, &sbrec_encap_col_options);
    ovsdb_idl_get_initial_snapshot(g_chassis_sync->ovnsb_idl);
    ovsdb_idl_run(g_chassis_sync->ovnsb_idl);

    /*initialize the address array*/
    (void)cetcd_array_init(&g_chassis_sync->address, 3);

    /*append the address of etcd*/
    (void)cetcd_array_append(&g_chassis_sync->address, g_chassis_sync->etcd_ip_address);

    /*initialize the cetcd_client*/
    (void)cetcd_client_init(&g_chassis_sync->etcd_client, &g_chassis_sync->address);

    (void)ovn_chassis_sync();

    /*initialize the chassis change watcher*/
    ovn_chassis_sync_add_watcher();
}

void
ovn_chassis_sync_fini(void)
{
    cetcd_multi_watch_async_stop(&g_chassis_sync->etcd_client, g_chassis_sync->watcher_id);
    cetcd_client_destroy(&g_chassis_sync->etcd_client);
    cetcd_array_destroy(&g_chassis_sync->address);
    cetcd_array_destroy(&g_chassis_sync->watchers);
    ovsdb_idl_destroy(g_chassis_sync->ovnsb_idl);

    if (g_chassis_sync != NULL) {
        free(g_chassis_sync);
        g_chassis_sync = NULL;
    }
}

