
#include "lib/smap.h"
#include "ovn-controller.h"
#include "vswitch-idl.h"


#ifndef OVN_CONTROLLER_CHASSIS_ETCD_H
#define OVN_CONTROLLER_CHASSIS_ETCD_H 1

/* Chassis table. */
struct chassis_etcd_sync {
    /* encaps column. */
    struct encap_etcd_sync **encaps;
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
struct encap_etcd_sync {
    /* ip column. */
    char *ip; /* Always nonnull. */

    /* options column. */
    struct smap options;

    /* type column. */
    char *type; /* Always nonnull. */
};

void
chassis_etcd_init(void);
void
chassis_etcd_fini(void);

void 
chassis_etcd_sync_data(struct chassis_etcd_sync *chassis_ovn);

bool
chassis_etcd_run(struct controller_ctx *ctx, const char *chassis_id,
            const struct ovsrec_bridge *br_int);


#endif /* ovn/chassis_etcd.h */

