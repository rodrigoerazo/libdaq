/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
** Copyright (C) 2010-2013 Sourcefire, Inc.
** Author: Michael R. Altizer <mialtize@cisco.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _DAQ_API_H
#define _DAQ_API_H

#include <daq_common.h>

#define DAQ_BASE_API_VERSION    0x00010001

typedef struct _daq_base_api
{
    /* Sanity/Version checking */
    uint32_t api_version;
    uint32_t api_size;
    /* Instance configuration accessors */
    DAQ_Module_h (*module_config_get_module) (DAQ_ModuleConfig_h modcfg);
    const char *(*module_config_get_input) (DAQ_ModuleConfig_h modcfg);
    int (*module_config_get_snaplen) (DAQ_ModuleConfig_h modcfg);
    unsigned (*module_config_get_timeout) (DAQ_ModuleConfig_h modcfg);
    unsigned (*module_config_get_msg_pool_size) (DAQ_ModuleConfig_h modcfg);
    DAQ_Mode (*module_config_get_mode) (DAQ_ModuleConfig_h modcfg);
    const char *(*module_config_get_variable) (DAQ_ModuleConfig_h modcfg, const char *key);
    int (*module_config_first_variable) (DAQ_ModuleConfig_h modcfg, const char **key, const char **value);
    int (*module_config_next_variable) (DAQ_ModuleConfig_h modcfg, const char **key, const char **value);
    DAQ_ModuleConfig_h (*module_config_get_next) (DAQ_ModuleConfig_h modcfg);
    /* Instance operations */
    void (*instance_set_context) (DAQ_Instance_h instance, void *context);
    void *(*instance_get_context) (DAQ_Instance_h instance);
    void (*instance_set_errbuf) (DAQ_Instance_h instance, const char *format, ...);
} DAQ_BaseAPI_t;

#define DAQ_MODULE_API_VERSION    0x00010004

typedef struct _daq_module_api
{
    /* The version of the API this module implements. */
    const uint32_t api_version;
    /* The size of this structure (for sanity checking). */
    const uint32_t api_size;
    /* The version of the DAQ module itself - can be completely arbitrary. */
    const uint32_t module_version;
    /* The name of the module (sfpacket, xvnim, pcap, etc.) */
    const char *name;
    /* Various flags describing the module and its capabilities (Inline-capabale, etc.) */
    const uint32_t type;
    /* The function the module loader *must* call first to prepare the module for any other function calls. */
    int (*prepare) (const DAQ_BaseAPI_t *base_api);
    /* Get a pointer to an array describing the DAQ variables accepted by this module.
        Returns the size of the retrieved array. */
    int (*get_variable_descs) (const DAQ_VariableDesc_t **var_desc_table);
    /* Initialize the device for packet acquisition with the supplied configuration.
       This should not start queuing packets for the application. */
    int (*initialize) (const DAQ_ModuleConfig_h config, DAQ_Instance_h instance);
    /* Set the module's BPF based on the given string */
    int (*set_filter) (void *handle, const char *filter);
    /* Complete device opening and begin queuing packets if they have not been already. */
    int (*start) (void *handle);
    /* Inject a new packet going either the same or opposite direction as the specified message. */
    int (*inject) (void *handle, DAQ_Msg_h msg, const uint8_t *packet_data, uint32_t len, int reverse);
    /* Force breaking out of the acquisition loop after the current iteration. */
    int (*breakloop) (void *handle);
    /* Stop queuing packets, if possible */
    int (*stop) (void *handle);
    /* Close the device and clean up */
    void (*shutdown) (void *handle);
    /* Get the status of the module (one of DAQ_STATE_*). */
    DAQ_State (*check_status) (void *handle);
    /* Populates the <stats> structure with the current DAQ stats.  These stats are cumulative. */
    int (*get_stats) (void *handle, DAQ_Stats_t *stats);
    /* Resets the DAQ module's internal stats. */
    void (*reset_stats) (void *handle);
    /* Return the configured snaplen */
    int (*get_snaplen) (void *handle);
    /* Return a bitfield of the device's capabilities */
    uint32_t (*get_capabilities) (void *handle);
    /* Return the instance's Data Link Type */
    int (*get_datalink_type) (void *handle);
    /* Return the index of the given named device if possible. */
    int (*get_device_index) (void *handle, const char *device);
    /* Modify a flow */
    int (*modify_flow) (void *handle, DAQ_Msg_h msg, const DAQ_ModFlow_t *modify);
    /* Read new configuration */
    int (*hup_prep) (void *handle, void **new_config);
    /* Swap new and old configuration */
    int (*hup_apply) (void *handle, void *new_config, void **old_config);
    /* Destroy old configuration */
    int (*hup_post) (void *handle, void *old_config);
    /** DAQ API to program a FST/EFT entry for dynamic protocol data channel
     *
     * @param [in] handle      DAQ module handle
     * @param [in] hdr         DAQ packet header of the control channel packet.
     * @param [in] dp_key      Key structure of the data channel flow
     * @param [in] packet_data Packet of the companion control channel packet.
     * @param [in] params      Parameters to control the PST/EFT entry.
     * @return                 Error code of the API. 0 - success.
     */
    int (*dp_add_dc) (void *handle, DAQ_Msg_h msg, DAQ_DP_key_t *dp_key,
                      const uint8_t *packet_data, DAQ_Data_Channel_Params_t *params);
    /* Query a flow */
    int (*query_flow) (void *handle, DAQ_Msg_h msg, DAQ_QueryFlow_t *query);

    unsigned (*msg_receive) (void *handle, const unsigned max_recv, const DAQ_Msg_t *msgs[], DAQ_RecvStatus *rstat);
    int (*msg_finalize) (void *handle, const DAQ_Msg_t *msg, DAQ_Verdict verdict);

    /* Query message pool info */
    int (*get_msg_pool_info) (void *handle, DAQ_MsgPoolInfo_t *info);
} DAQ_ModuleAPI_t;

#define DAQ_ERRBUF_SIZE 256

/* This is a convenience macro for safely printing to DAQ error buffers.  It must be called on a known-size character array. */
#ifdef WIN32
inline void DPE(char *var, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    snprintf(var, sizeof(var), ap);

    va_end(ap);
}
#else
#define DPE(var, ...) snprintf(var, sizeof(var), __VA_ARGS__)
#endif

#endif /* _DAQ_API_H */
