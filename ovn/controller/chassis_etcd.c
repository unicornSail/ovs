
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "cetcd.h"
#include "ofctrl.h"
#include "chassis_etcd.h"
#include "cetcd_array.h"
#include "ovn/lib/ovn-sb-idl.h"
#include "openvswitch/dynamic-string.h"
#include "openvswitch/json.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(chassis_etcd);

struct chassis_etcd {
    char *etcd_ip_address;
    char *etcd_path_data;
    cetcd_array address;
    cetcd_client etcd_client;
    pthread_t thread;
    char *etcd_node_key;  // chassis_name
};

static struct chassis_etcd *g_chassis_etcd;

static void
chassis_etcd_data_destroy(struct chassis_etcd_sync *chassis_rec) {
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

static struct chassis_etcd_sync *
chassis_etcd_data(struct json *json_data) {
    struct chassis_etcd_sync *chassis_new = calloc(1, sizeof(struct chassis_etcd_sync));
    smap_init(&chassis_new->external_ids);
    struct shash_node *chassis_shash_node;
    SHASH_FOR_EACH (chassis_shash_node, json_object(json_data)) {
        if (strcmp(chassis_shash_node->name, sbrec_chassis_columns[SBREC_CHASSIS_COL_ENCAPS].name) == 0) {
            struct json_array *json_encaps = json_array(chassis_shash_node->data);
            chassis_new->encaps = calloc(json_encaps->n, sizeof(struct encap_etcd_sync *));
            for (int i = 0; i < json_encaps->n; i++) {
                struct encap_etcd_sync *encap_new = calloc(1, sizeof(struct encap_etcd_sync));
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

static char *
chassis_etcd_json(struct chassis_etcd_sync *chassis_ovn) {
    struct json *chassis_json = json_object_create();
    struct json *encaps_json = json_array_create_empty();
    for (int i = 0; i < chassis_ovn->n_encaps; i++) {
        struct json *encap_json = json_object_create();
        json_object_put_string(encap_json, sbrec_encap_columns[SBREC_ENCAP_COL_IP].name, chassis_ovn->encaps[i]->ip);
        struct json *encap_option_json = json_object_create();
        struct smap_node *node;
        SMAP_FOR_EACH (node, &chassis_ovn->encaps[i]->options) {
            json_object_put_string(encap_option_json, node->key, node->value);
        }
        json_object_put(encap_json, sbrec_encap_columns[SBREC_ENCAP_COL_OPTIONS].name, encap_option_json);
        json_object_put_string(encap_json, sbrec_encap_columns[SBREC_ENCAP_COL_TYPE].name, chassis_ovn->encaps[i]->type);
        json_array_add(encaps_json, encap_json);
    }
    struct json *external_ids_json = json_object_create();
    struct smap_node *node;
    SMAP_FOR_EACH (node, &chassis_ovn->external_ids) {
        json_object_put_string(external_ids_json, node->key, node->value);
    }
    struct json *vtep_logical_switches_json = json_array_create_empty();
    for (int i = 0; i < chassis_ovn->n_vtep_logical_switches; i++) {
        json_array_add(vtep_logical_switches_json, json_string_create(chassis_ovn->vtep_logical_switches[i]));
    }
    json_object_put(chassis_json, sbrec_chassis_columns[SBREC_CHASSIS_COL_ENCAPS].name, encaps_json);
    json_object_put(chassis_json, sbrec_chassis_columns[SBREC_CHASSIS_COL_EXTERNAL_IDS].name, external_ids_json);
    json_object_put_string(chassis_json, sbrec_chassis_columns[SBREC_CHASSIS_COL_HOSTNAME].name, chassis_ovn->hostname);
    json_object_put_string(chassis_json, sbrec_chassis_columns[SBREC_CHASSIS_COL_NAME].name, chassis_ovn->name);
    json_object_put(chassis_json, sbrec_chassis_columns[SBREC_CHASSIS_COL_NB_CFG].name, json_integer_create((long long int)chassis_ovn->nb_cfg));
    json_object_put(chassis_json, sbrec_chassis_columns[SBREC_CHASSIS_COL_VTEP_LOGICAL_SWITCHES].name, vtep_logical_switches_json);
    char * chassis_string = json_to_string(chassis_json, JSSF_SORT);
    json_destroy(chassis_json);
    VLOG_INFO("chassis json data: %s. ", chassis_string);
    return chassis_string;
}

static const struct chassis_etcd_sync *
chassis_etcd_get_chassis(const char *chassis_id)
{
    char chassis_name[100];
    strcpy(chassis_name, g_chassis_etcd->etcd_path_data);
    strcat(chassis_name, chassis_id);
    cetcd_response *resp = cetcd_get(&g_chassis_etcd->etcd_client, chassis_name);
    if(resp->err) {
        VLOG_ERR("get %s key to etcd failed. error :%d, %s (%s)\n", 
            chassis_name, resp->err->ecode, resp->err->message, resp->err->cause);
        cetcd_response_release(resp);
        return NULL;
    }
    struct json * chassis_json = json_from_string(resp->node->value);
    struct chassis_etcd_sync * chassis_rec = chassis_etcd_data(chassis_json);
    json_destroy(chassis_json);
    cetcd_response_release(resp);
    return chassis_rec;
}

static void
chassis_etcd_pthread_loop(void)
{
    while(1){
        sleep(1);
        if (g_chassis_etcd->etcd_node_key == NULL) {
            continue;
        }
        cetcd_response *resp = cetcd_update(&g_chassis_etcd->etcd_client, g_chassis_etcd->etcd_node_key, NULL, 2, 1);
        if(resp->err) {
            VLOG_ERR("update %s ttl to etcd failed. error :%d, %s (%s)\n", 
                g_chassis_etcd->etcd_node_key, resp->err->ecode, resp->err->message, resp->err->cause);
        }
        cetcd_response_release(resp);
    }
}

void 
chassis_etcd_sync_data(struct chassis_etcd_sync *chassis_ovn){
    //write_etcd
    char *chassis_data = chassis_etcd_json(chassis_ovn);
    char chassis_id[100];
    strcpy(chassis_id, g_chassis_etcd->etcd_path_data);
    strcat(chassis_id, chassis_ovn->name);
    if (g_chassis_etcd->etcd_node_key != NULL) {
        free(g_chassis_etcd->etcd_node_key);
    }
    g_chassis_etcd->etcd_node_key = xstrdup(chassis_id);
    cetcd_response *resp = cetcd_set(&g_chassis_etcd->etcd_client, g_chassis_etcd->etcd_node_key, chassis_data, 2);
    if(resp->err) {
        VLOG_ERR("set %s ttl to etcd failed. error :%d, %s (%s)\n", 
            g_chassis_etcd->etcd_node_key, resp->err->ecode, resp->err->message, resp->err->cause);
    }
    cetcd_response_release(resp);
}

void
chassis_etcd_init(void)
{
    g_chassis_etcd = calloc(1, sizeof(struct chassis_etcd));
    if (g_chassis_etcd == NULL) {
        return;
    }

    g_chassis_etcd->etcd_ip_address = "127.0.0.1:2379";
    g_chassis_etcd->etcd_path_data = "/chassis/";

    /*initialize the address array*/
    (void)cetcd_array_init(&g_chassis_etcd->address, 3);

    /*append the address of etcd*/
    (void)cetcd_array_append(&g_chassis_etcd->address, g_chassis_etcd->etcd_ip_address);

    /*initialize the cetcd_client*/
    (void)cetcd_client_init(&g_chassis_etcd->etcd_client, &g_chassis_etcd->address);

    /*initialize the update ttl timer*/
    pthread_create(&g_chassis_etcd->thread, NULL, (void *(*)(void *))chassis_etcd_pthread_loop, NULL);
}

void
chassis_etcd_fini(void)
{
    cetcd_client_destroy(&g_chassis_etcd->etcd_client);
    cetcd_array_destroy(&g_chassis_etcd->address);
    if (g_chassis_etcd->etcd_node_key != NULL) {
        free(g_chassis_etcd->etcd_node_key);
    }
    pthread_cancel(g_chassis_etcd->thread);
    pthread_join(g_chassis_etcd->thread, 0);
    if (g_chassis_etcd != NULL) {
        free(g_chassis_etcd);
        g_chassis_etcd = NULL;
    }
}

static const char *
pop_tunnel_name(uint32_t *type)
{
    if (*type & GENEVE) {
        *type &= ~GENEVE;
        return "geneve";
    } else if (*type & STT) {
        *type &= ~STT;
        return "stt";
    } else if (*type & VXLAN) {
        *type &= ~VXLAN;
        return "vxlan";
    }

    OVS_NOT_REACHED();
}

/* Returns this chassis's Chassis record, if it is available and is currently
 * amenable to a transaction. */
bool
chassis_etcd_run(struct controller_ctx *ctx, const char *chassis_id,
            const struct ovsrec_bridge *br_int)
{
    if (!ctx->ovnsb_idl_txn) {
        return false;
    }

    const struct ovsrec_open_vswitch *cfg;
    const char *encap_type, *encap_ip;
    static bool inited = false;

    cfg = ovsrec_open_vswitch_first(ctx->ovs_idl);
    if (!cfg) {
        VLOG_INFO("No Open_vSwitch row defined.");
        return false;
    }

    encap_type = smap_get(&cfg->external_ids, "ovn-encap-type");
    encap_ip = smap_get(&cfg->external_ids, "ovn-encap-ip");
    if (!encap_type || !encap_ip) {
        VLOG_INFO("Need to specify an encap type and ip");
        return false;
    }

    char *tokstr = xstrdup(encap_type);
    char *save_ptr = NULL;
    char *token;
    uint32_t req_tunnels = 0;
    for (token = strtok_r(tokstr, ",", &save_ptr); token != NULL;
         token = strtok_r(NULL, ",", &save_ptr)) {
        uint32_t type = get_tunnel_type(token);
        if (!type) {
            VLOG_INFO("Unknown tunnel type: %s", token);
        }
        req_tunnels |= type;
    }
    free(tokstr);

    const char *hostname = smap_get_def(&cfg->external_ids, "hostname", "");
    char hostname_[HOST_NAME_MAX + 1];
    if (!hostname[0]) {
        if (gethostname(hostname_, sizeof hostname_)) {
            hostname_[0] = '\0';
        }
        hostname = hostname_;
    }

    const char *bridge_mappings = smap_get_def(&cfg->external_ids, "ovn-bridge-mappings", "");
    const char *datapath_type =
        br_int && br_int->datapath_type ? br_int->datapath_type : "";
    const char *node_type = smap_get_def(&cfg->external_ids, "ovn-chassis-types", "");

    struct ds iface_types = DS_EMPTY_INITIALIZER;
    ds_put_cstr(&iface_types, "");
    for (int j = 0; j < cfg->n_iface_types; j++) {
        ds_put_format(&iface_types, "%s,", cfg->iface_types[j]);
    }
    ds_chomp(&iface_types, ',');
    const char *iface_types_str = ds_cstr(&iface_types);

    const struct chassis_etcd_sync *chassis_rec
        = chassis_etcd_get_chassis(chassis_id);

    if (chassis_rec) {
        bool same = true;
        if (strcmp(hostname, chassis_rec->hostname)) {
            same = false;
        }

        /* Determine new values for Chassis external-ids. */
        const char *chassis_bridge_mappings
            = smap_get_def(&chassis_rec->external_ids, "ovn-bridge-mappings", "");
        const char *chassis_datapath_type
            = smap_get_def(&chassis_rec->external_ids, "datapath-type", "");
        const char *chassis_iface_types
            = smap_get_def(&chassis_rec->external_ids, "iface-types", "");
        const char *chassis_node_type
            = smap_get_def(&chassis_rec->external_ids, "ovn-chassis-types", "");

        /* If any of the external-ids should change, update them. */
        if (strcmp(bridge_mappings, chassis_bridge_mappings) ||
            strcmp(datapath_type, chassis_datapath_type) ||
            strcmp(iface_types_str, chassis_iface_types) ||
            strcmp(node_type, chassis_node_type)) {
            same = false;
        }
        
        if (chassis_rec->nb_cfg != ofctrl_get_cur_cfg()) {
            same = false;
        }

        /* Compare desired tunnels against those currently in the database. */
        uint32_t cur_tunnels = 0;
        for (int i = 0; i < chassis_rec->n_encaps; i++) {
            cur_tunnels |= get_tunnel_type(chassis_rec->encaps[i]->type);
            same = same && !strcmp(chassis_rec->encaps[i]->ip, encap_ip);

            same = same && smap_get_bool(&chassis_rec->encaps[i]->options,
                                         "csum", false);
        }
        same = same && req_tunnels == cur_tunnels;

        if (same) {
            /* Nothing changed. */
            inited = true;
            ds_destroy(&iface_types);
            return true;
        } else if (!inited) {
            struct ds cur_encaps = DS_EMPTY_INITIALIZER;
            for (int i = 0; i < chassis_rec->n_encaps; i++) {
                ds_put_format(&cur_encaps, "%s,",
                              chassis_rec->encaps[i]->type);
            }
            ds_chomp(&cur_encaps, ',');

            VLOG_WARN("Chassis config changing on startup, make sure "
                      "multiple chassis are not configured : %s/%s->%s/%s",
                      ds_cstr(&cur_encaps),
                      chassis_rec->encaps[0]->ip,
                      encap_type, encap_ip);
            ds_destroy(&cur_encaps);
        }
    }

    struct chassis_etcd_sync *chassis_ovn = calloc(1, sizeof(struct chassis_etcd_sync));
    chassis_ovn->name = xstrdup(chassis_id);
    chassis_ovn->hostname = xstrdup(hostname);
    smap_init(&chassis_ovn->external_ids);
    smap_add(&chassis_ovn->external_ids, "ovn-bridge-mappings", bridge_mappings);
    smap_add(&chassis_ovn->external_ids, "datapath-type", datapath_type);
    smap_add(&chassis_ovn->external_ids, "iface-types", iface_types_str);
    smap_add(&chassis_ovn->external_ids, "ovn-chassis-types", node_type);

    ds_destroy(&iface_types);
    int n_encaps = count_1bits(req_tunnels);
    const struct smap options = SMAP_CONST1(&options, "csum", "true");

    chassis_ovn->encaps = calloc(n_encaps, sizeof(struct encap_etcd_sync *));
    for (int i = 0; i < n_encaps; i++) {
        chassis_ovn->encaps[i] = calloc(1, sizeof(struct encap_etcd_sync));
        const char *type = pop_tunnel_name(&req_tunnels);
        chassis_ovn->encaps[i]->type = xstrdup(type);
        chassis_ovn->encaps[i]->ip = xstrdup(encap_ip);
        smap_clone(&chassis_ovn->encaps[i]->options, &options);
        chassis_ovn->n_encaps ++;
    }

    chassis_ovn->nb_cfg = ofctrl_get_cur_cfg();

    chassis_etcd_sync_data(chassis_ovn);

    chassis_etcd_data_destroy(chassis_ovn);

    inited = true;
    return false;
}


