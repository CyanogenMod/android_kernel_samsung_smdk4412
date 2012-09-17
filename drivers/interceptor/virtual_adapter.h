/* Netfilter Driver for IPSec VPN Client
 *
 * Copyright(c)   2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 *
 * Kernel-mode virtual adapter interface.
 *
 * File: virtual_adapter.h
 *
 */


#ifndef VIRTUAL_ADAPTER_H
#define VIRTUAL_ADAPTER_H

#include "interceptor.h"

/* ***************************** Data Types **********************************/

/** Error codes for virtual adapter operations. */
typedef enum {
  /** Success */
  SSH_VIRTUAL_ADAPTER_ERROR_OK              = 0,
  /** Success, status callback will be called again */
  SSH_VIRTUAL_ADAPTER_ERROR_OK_MORE         = 1,
  /** Nonexistent adapter */
  SSH_VIRTUAL_ADAPTER_ERROR_NONEXISTENT     = 2,
  /** Address configuration error */
  SSH_VIRTUAL_ADAPTER_ERROR_ADDRESS_FAILURE = 3,
  /** Route configuration error */
  SSH_VIRTUAL_ADAPTER_ERROR_ROUTE_FAILURE   = 4,
  /** Parameter configuration error */
  SSH_VIRTUAL_ADAPTER_ERROR_PARAM_FAILURE   = 5,
  /** Memory allocation error */
  SSH_VIRTUAL_ADAPTER_ERROR_OUT_OF_MEMORY   = 6,
  /** Undefined internal error */
  SSH_VIRTUAL_ADAPTER_ERROR_UNKNOWN_ERROR   = 255
} SshVirtualAdapterError;

/** Virtual adapter state. */
typedef enum {
  /** Invalid value */
  SSH_VIRTUAL_ADAPTER_STATE_UNDEFINED        = 0,
  /** Up */
  SSH_VIRTUAL_ADAPTER_STATE_UP               = 1,
  /** Down */
  SSH_VIRTUAL_ADAPTER_STATE_DOWN             = 2,
  /** Keep existing state */
  SSH_VIRTUAL_ADAPTER_STATE_KEEP_OLD         = 3,
} SshVirtualAdapterState;

/** Optional parameters for a virtual adapter. */
struct SshVirtualAdapterParamsRec
{
  /** Virtual adapter mtu. */
  SshUInt32 mtu;

  /** DNS server IP addresses. */
  SshUInt32 dns_ip_count;
  SshIpAddr dns_ip;

  /** WINS server IP addresses. */
  SshUInt32 wins_ip_count;
  SshIpAddr wins_ip;

  /** Windows domain name. */
  char *win_domain;

  /** Netbios node type. */
  SshUInt8 netbios_node_type;
};

typedef struct SshVirtualAdapterParamsRec SshVirtualAdapterParamsStruct;
typedef struct SshVirtualAdapterParamsRec *SshVirtualAdapterParams;

/* ******************* Sending and Receiving Packets *************************/

/** Send packet `pp' to the IP stack like it was received by the
    virtual adapter indexed by `pp->ifnum_in'. The packet `pp' must be a
    plain IP packet (IPv4 or IPv6) or it must be of the same media type
    as the virtual adapter (most probably ethernet). This will free the
    packet `pp'.

    NOTE: LOCKS MUST NOT BE HELD DURING CALLS TO ssh_virtual_adapter_receive()!
*/
void ssh_virtual_adapter_send(SshInterceptor interceptor,
			      SshInterceptorPacket pp);

/** Callback function of this type is called whenever a packet is
    sent out via a virtual adapter. This function must eventually free the
    packet by calling ssh_interceptor_packet_free.

    This function is used for handling only those ARP, neighbor discovery,
    and DHCP packets that are not handled by the generic engine packet
    callback. This function must not be used for passing normal
    IP packets.

    Note that this function may be called asynchronously. */
typedef void (*SshVirtualAdapterPacketCB)(SshInterceptor interceptor,
                                          SshInterceptorPacket pp,
                                          void *adapter_context);

/* **************************** Generic Callbacks ****************************/

/** A callback function of this type is called to notify about the
   success of a virtual adapter operation.

   - The argument `error' describes whether the operation was successful or
     not. If `error' indicates success, the arguments `adapter_ifnum',
     `adapter_name', and `state' contain information about the virtual adapter.

   - The argument `adapter_ifnum' is an unique interface identifier for the
     virtual adapter.  You can use it configuring additional routes for
     the adapter, and for configuring and destroying the adapter.

   - The argument `adapter_name' contains the name of the created adapter.
     The returned name is the same that identifies the interface in the
     interceptor's interface callback.

   - The argument `adapter_context' is the context attached to the virtual
     adapter in the call to ssh_virtual_adapter_attach.

   Arguments `adapter_ifnum', `adapter_name', `adapter_state', and
   `adapter_context' are valid only for the duration of the callback. */
typedef void (*SshVirtualAdapterStatusCB)(SshVirtualAdapterError error,
                                          SshInterceptorIfnum adapter_ifnum,
                                          const unsigned char *adapter_name,
					  SshVirtualAdapterState adapter_state,
					  void *adapter_context,
                                          void *context);

/* **************************** Attach and Detach ****************************/

/** A callback function of this type is called to notify about detachment
    of a virtual adapter from the engine. The callback is meant only as a
    destructor of the virtual adapter context, and thus should be called
    whenever there is no possibility of `packet_cb' being called again.
    The argument `adapter_context' is the context attached to the virtual
    adapter in the call to ssh_virtual_adapter_attach. */
typedef void (*SshVirtualAdapterDetachCB)(void *adapter_context);

/** Attaches the packet callback and context to a virtual adapter.

   - The argument `packet_cb' specifies a callback function that is
     called when a packet is received from the virtual adapter. The argument
     `adapter_context' is the context for this callback.

   - The optional argument `detach_cb' specifies an engine-level
     destructor for `adapter_context'. It must be called when the
     interceptor is done with virtual adapter, i.e. when the `packet_cb'
     will not be called for this adapter again. Even in case of instant
     failure, the `detach_cb' must be called.

   - The operation completion callback `callback' must be called to notify
     about success or failure of the attachment. In failure cases `callback'
     must be called after `detach_cb'. The argument `context' is the context
     data for this callback.

   NOTE: LOCKS MUST NOT BE HELD DURING CALLS TO ssh_virtual_adapter_attach()!
*/
void ssh_virtual_adapter_attach(SshInterceptor interceptor,
				SshInterceptorIfnum adapter_ifnum,
                                SshVirtualAdapterPacketCB packet_cb,
                                SshVirtualAdapterDetachCB detach_cb,
                                void *adapter_context,
                                SshVirtualAdapterStatusCB callback,
                                void *context);

/** Detach the virtual adapter `adapter_ifnum'. The success of the
    operation is notified by calling the callback function `callback'.

    The detach operation will fail with the error code
    SSH_VIRTUAL_ADAPTER_NONEXISTENT if the virtual adapter was never
    attached. Detaching a virtual adapter must generate an interface callback
    for the interceptor.

    If a detach callback was specified in ssh_virtual_adapter_attach, then
    this `detach_cb' must be called before calling the operation completion
    callback `callback'.

    NOTE: LOCKS MUST NOT BE HELD DURING CALLS TO ssh_virtual_adapter_detach()!
*/
void ssh_virtual_adapter_detach(SshInterceptor interceptor,
				SshInterceptorIfnum adapter_ifnum,
				SshVirtualAdapterStatusCB callback,
				void *context);

/** Detach all configured virtual adapters. This is called when the engine
    is stopped during interceptor is unloading. This function must call
    `detach_cb' for each detached virtual adapter that defines `detach_cb'.

    NOTE: LOCKS MUST NOT BE HELD DURING CALLS TO
    ssh_virtual_adapter_detach_all()! */
void ssh_virtual_adapter_detach_all(SshInterceptor interceptor);

/* *************************** Configuration *********************************/

#ifdef INTERCEPTOR_IMPLEMENTS_VIRTUAL_ADAPTER_CONFIGURE

/** Configures the virtual adapter `adapter_ifnum' with `state', `addresses',
    and `params'. The argument `adapter_ifnum' must be the valid interface
    index of a virtual adapter that has been attached to the engine during
    pm connect.

    The argument `adapter_state' specifies the state to configure for the
    virtual adapter.

    The arguments `num_addresses' and `addresses' specify the IP addresses
    for the virtual adapter. The addresses must specify the netmask. If
    `addresses' is NULL, the address configuration will not be changed.
    Otherwise the existing addresses will be removed from the virtual adapter
    and specified addresses will be added. To clear all addresses from the
    virtual adapter, specify `addresses' as non-NULL and `num_addresses' as 0.

    The argument `params' specifies optional parameters for the virtual
    adapter. If `params' is non-NULL, then the existing params will be cleared
    and the specified params will be set for the virtual adapter.

    Any configured interface addresses must survive interface state changes.

    If the define INTERCEPTOR_IMPLEMENTS_VIRTUAL_ADAPTER_CONFIGURE is not
    defined then the interceptor does not support kernel level virtual
    adapter configure.
*/
void ssh_virtual_adapter_configure(SshInterceptor interceptor,
				   SshInterceptorIfnum adapter_ifnum,
				   SshVirtualAdapterState adapter_state,
				   SshUInt32 num_addresses,
				   SshIpAddr addresses,
				   SshVirtualAdapterParams params,
				   SshVirtualAdapterStatusCB callback,
				   void *context);

#endif /* INTERCEPTOR_IMPLEMENTS_VIRTUAL_ADAPTER_CONFIGURE */

/* *************************** Virtual Adapter Status ************************/

/** This function will call `callback' for virtual adapter `adapter_ifnum'.
    If `adapter_ifnum' equals SSH_INTERCEPTOR_INVALID_IFNUM, then this
    function calls `callback' once for each existing virtual adapter with
    error code SSH_VIRTUAL_ADAPTER_ERROR_OK_MORE, and once with error code
    SSH_VIRTUAL_ADAPTER_ERROR_NONEXISTENT after enumerating all virtual
    adapters. */
void ssh_virtual_adapter_get_status(SshInterceptor interceptor,
				    SshInterceptorIfnum adapter_ifnum,
				    SshVirtualAdapterStatusCB callback,
				    void *context);

/* *************************** Utility functions *****************************/

/** These generic utility functions that are implemented in files
    virtual_adapter_misc.c and virtual_adapter_util.s, and they use the
    virtual adapter API described above. */

/** Initializes engine side virtual adapter contexts and attaches the existing
    virtual adapters to engine. */
SshVirtualAdapterError ssh_virtual_adapter_init(SshInterceptor interceptor);

/** Unitializes engine side virtual adapter contexts and detaches virtual
    adapters from the engine. */
SshVirtualAdapterError ssh_virtual_adapter_uninit(SshInterceptor interceptor);

/** Creates a pseudo Ethernet hardware address for the virtual adapter
    `adapter_ifnum'. The address is formatted in the buffer `buffer' which
    must have space for SSH_ETHERH_ADDRLEN bytes. */
void
ssh_virtual_adapter_interface_ether_address(SshInterceptorIfnum adapter_ifnum,
					    unsigned char *buffer);

/** Create a pseudo Ethernet hardware address for the IP address `ip'.
    The address `ip' can be an IPv4 or an IPv6 address.  The address if
    formatted in the buffer `buffer' which must have space for
    SSH_ETHERH_ADDRLEN bytes. */
void ssh_virtual_adapter_ip_ether_address(SshIpAddr ip, unsigned char *buffer);

/** Encode virtual adapter params `params' to buffer `buffer'. This
    function returns TRUE on success. The caller must ssh_free the allocated
    memory in `data'. */
Boolean
ssh_virtual_adapter_param_encode(SshVirtualAdapterParams params,
				 unsigned char **data, size_t *len);

/** Decode virtual adapter params from table `data' into `params'. This
    function returns TRUE on success. */
Boolean
ssh_virtual_adapter_param_decode(SshVirtualAdapterParams params,
				 const unsigned char *data, size_t len);

#endif /* not VIRTUAL_ADAPTER_H */
