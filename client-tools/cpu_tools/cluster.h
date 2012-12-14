/**
 * This is CPU performance bottleneck detection source code.
 */

#ifndef __CLUSTER_H__
#define __CLUSTER_H__

typedef enum
{
    INIT,
    DENSE,
    SPARSE,
    TRANSIT,
    SPORADIC,
} status;

typedef struct record
{
    unsigned long cr3;
    unsigned long rsp;
    unsigned long timestamp;
    struct record *next;
} *pRecord, record_t;

typedef struct rsp_list_entry
{
    unsigned int clusterID;
    double density;
    unsigned long rsp;
    unsigned long tg;
    status st;
    struct rsp_list_entry *prev, *next;
} rsp_list_entry_t;

typedef struct cr3_list_entry
{
    unsigned long cr3;
    double density;
    unsigned long tg;
    status st;
    unsigned long counter;
    struct rsp_list_entry *rsp_table;
    struct cr3_list_entry *prev, *next;
} cr3_list_entry_t;

typedef struct cr3_table_entry
{
    struct cr3_list_entry *ptr;
} cr3_table_entry_t;


/**
 * Initialize the CR3 table to NULL
 */
void init();

/**
 * Density Threshold Function
 */
inline double density_threshold(unsigned long tg, unsigned long nowtime);

/**
 * Release memory of rsp_table when a CR3 is delete
 */
void free_rsp_table(struct rsp_list_entry *rsp_table);

/**
 * Cluster rsp
 */
void gap_cluster_rsp(struct rsp_list_entry **rsp_table);

/**
 * Update the rsp_table of a CR3
 */
void gap_update_rsp_table(struct rsp_list_entry **rsp_table, unsigned long timestamp);

/**
 * Update all CR3 entry's density at every gap
 */
void gap_update_cr3_density(unsigned long timestamp);


/**
 * Update the corresponding RSP entry for a specific CR3
 */
void update_rsp(cr3_list_entry_t *ptr, unsigned long rsp, unsigned long time);


/**
 * Update CR3 entry at a data record come in
 */
void update_cr3_list(cr3_list_entry_t *ptr, cr3_list_entry_t **head, cr3_list_entry_t **tail);


/**
 * Entry point for every data record
 */
void updateData(unsigned long cr3, unsigned long rsp, unsigned long timestamp);

#endif
