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
 * interceptor.h
 *
 * Interceptor API specifies the Interceptor side interface between
 * the Interceptor and Engine components. This API contains functions
 * for Interceptor initialization, packet allocation, packet data access,
 * routing and packet sending.
 *
 */

#ifndef INTERCEPTOR_H
#define INTERCEPTOR_H

#include "sshinet.h"

/** The amount of space to reserve in packet header for the IPsec engine. */
#define SSH_INTERCEPTOR_UPPER_DATA_SIZE         192

/** The number of available extension selectors for platform-specific
    extensions. */
#define SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS 1

/** Size of interface system name. */
#define SSH_INTERCEPTOR_IFNAME_SIZE             64

/** Data type for the interceptor context. */
typedef struct SshInterceptorRec *SshInterceptor;

/** This type defines the type of a network interface interface.  The
    type is supposed to be sufficient to determine how to process the
    packets.  It is not expected to provide maximal detail to the
    user.  Regardless of the interface type, the higher-level code
    should not assume that incoming packets have been defragmented,
    nor will it assume that the interceptor will perform fragmentation
    for outgoing packets.  Thus, it will perform these functions
    itself.  It will not assume the underlying interceptor will do it.
    Routing will also be done by the higher level to determine which
    interface a packet should go out from. */
typedef enum {
  SSH_INTERCEPTOR_MEDIA_NONEXISTENT, /** The interface is not available */
  SSH_INTERCEPTOR_MEDIA_PLAIN,       /** Any interceptor w/o media headers*/
  SSH_INTERCEPTOR_MEDIA_ETHERNET,    /** Ethernet (rfc894) framing */
  SSH_INTERCEPTOR_MEDIA_FDDI,        /** FDDI (rfc1042/rfc1103) framing */
  SSH_INTERCEPTOR_MEDIA_TOKENRING,   /** Tokenring (rfc1042/rfc1469) framing */
  /** New types may be added here.  Look for one of the existing types
     to find all places in code that should be updated. */
  SSH_INTERCEPTOR_NUM_MEDIAS   /** Must be the last entry! */
} SshInterceptorMedia;

/** Protocol identifiers.  These identify recognized protocols (packet
    formats) in a portable manner.  This enumeration includes media
    types, but also all recognized higher-level protocols. */
typedef enum
{
  SSH_PROTOCOL_IP4,             /** IPv4 frame */
  SSH_PROTOCOL_IP6,             /** IPv6 frame */
  SSH_PROTOCOL_IPX,             /** IPX frame */
  SSH_PROTOCOL_ETHERNET,        /** Ethernet frame */
  SSH_PROTOCOL_FDDI,            /** FDDI frame */
  SSH_PROTOCOL_TOKENRING,       /** Token Ring frame */
  SSH_PROTOCOL_ARP,             /** ARP frame */
  SSH_PROTOCOL_OTHER,           /** some other type frame */
  SSH_PROTOCOL_NUM_PROTOCOLS    /** must be the last entry! */
} SshInterceptorProtocol;

/** Data type for an interface number. This type must be atleast as big as
    the system interface index. */
typedef SshUInt32 SshInterceptorIfnum;

/** Maximum value of interface number.
    All valid interface numbers must be smaller than this value. */
#define SSH_INTERCEPTOR_MAX_IFNUM ((SshInterceptorIfnum) 0xffffffff)

/** Reserved value for invalid interface number. */
#define SSH_INTERCEPTOR_INVALID_IFNUM SSH_INTERCEPTOR_MAX_IFNUM

/** Data structure for representing an address for a network interface. */
typedef struct SshInterfaceAddressRec
{
  /** Protocol for which the address is. */
  SshInterceptorProtocol protocol;

  /** The address itself. */
  union
  {
    /** IPv4 and IPv6. */
    struct
    {
      SshIpAddrStruct ip;
      SshIpAddrStruct mask;
      SshIpAddrStruct broadcast;
    } ip;

    /** IPX */
    struct
    {
      SshUInt32 net;
      unsigned char host[6];
    } ns;
  } addr;
} *SshInterfaceAddress, SshInterfaceAddressStruct;

/** Flags for the media direction information. */

/** Do not fragment before sending from engine. */
#define SSH_INTERCEPTOR_MEDIA_INFO_NO_FRAGMENT                  0x0001

/** Accessor for mtu member in SshInterceptorMediaDirectionInfo. */
#ifdef WITH_IPV6
#define SSH_INTERCEPTOR_MEDIA_INFO_MTU(info, is_ipv6) \
  ((is_ipv6) ? (info)->mtu_ipv6 : (info)->mtu_ipv4)
#else /* WITH_IPV6 */
#define SSH_INTERCEPTOR_MEDIA_INFO_MTU(info, is_ipv6) \
  ((info)->mtu_ipv4)
#endif /* WITH_IPV6 */

/** Media direction information.  This information is used when engine
    is sending packets to the interceptor. */
typedef struct SshInterceptorMediaDirectionInfoRec
{
  SshInterceptorMedia media;    /* media type */
  SshUInt32 flags;              /* flags */
  size_t mtu_ipv4;              /* mtu for the direction (ipv4) */
#ifdef WITH_IPV6
  size_t mtu_ipv6;              /* mtu for the direction (ipv6) */
#endif /* WITH_IPV6 */
} *SshInterceptorMediaDirectionInfo, SshInterceptorMediaDirectionInfoStruct;

/** Flag values for flags in SshInterceptorInterface */
/* Interface type */
#define SSH_INTERFACE_FLAG_VIP         0x0001
#define SSH_INTERFACE_FLAG_POINTOPOINT 0x0002
#define SSH_INTERFACE_FLAG_BROADCAST   0x0004
/* Interface link status */
#define SSH_INTERFACE_FLAG_LINK_DOWN   0x0100

/** Data structure for providing information about a network
    interface. */
typedef struct
{
  SshInterceptorMediaDirectionInfoStruct to_protocol;
  SshInterceptorMediaDirectionInfoStruct to_adapter;
  char name[SSH_INTERCEPTOR_IFNAME_SIZE]; /** system name for the interface */
  SshInterceptorIfnum ifnum;    /** Interface number */
  SshUInt32 num_addrs;          /** Number of addresses for the interface. */
  SshInterfaceAddress addrs;    /** mallocated array of address structures. */
  unsigned char media_addr[16]; /** MAC address, medium size and format */
  size_t media_addr_len;        /** Length of the MAC address. */

  SshUInt32 flags;              /** Flags for the interface. */

  /** Context pointer for owner/user for this SshInterceptorInterface.
      Can be used for e.g. storing interface-specific
      NAT-configuration. */
  void *ctx_user;
} SshInterceptorInterface;

/*  Packet flag bits. */
#define SSH_PACKET_FROMPROTOCOL  0x00000001U /** Packet from the protocol. */
#define SSH_PACKET_FROMADAPTER   0x00000002U /** Packet from the adapter. */
#define SSH_PACKET_IP4HDRCKSUMOK 0x00000004U /** IPv4 header cksum checked. */
#define SSH_PACKET_FORWARDED     0x00000008U /** Packet was forwarded. */
#define SSH_PACKET_HWCKSUM       0x00000010U /** TCP/UDP cksum done by NIC. */
#define SSH_PACKET_MEDIABCAST    0x00000020U /** Packet was media broadcast. */
#define SSH_PACKET_UNMODIFIED    0x00000200U /** Unmodified packet. */

/*  This flag specifies that the engine is allowed to fragment the packet if
    the packet is too large to fit into interafce MTU. Some operating
    system handle the fragmentation after us and therefore this flag
    may be set for some outbound data packets. The packet is guaranteed
    to have been originated from local stack and stack has indicated
    that this packet can be fragmented. */
#define SSH_PACKET_FRAGMENTATION_ALLOWED  0x00000400U

/*  Flag bits with mask 0x00000fff are reserved for interceptor. */
/*  Flag bits with mask 0xfffff000 are reserved for IPSEC engine. */

/** Macro to access upper-level data in the packet header. */
#define SSH_INTERCEPTOR_PACKET_DATA(packet, type) \
  ((type)(&(packet)->upper_data))

/** Data structure for a packet.  These data structures can only be
    allocated by the interceptor; higher-level code must never
    directly allocate these (the interceptor implementation may
    actually use a larger structure containing this public
    structure). */
typedef struct SshInterceptorPacketRec
{
  /** Flags for the packet.  The SSH_PACKET_* bitmasks are used.  Code
      above the interceptor is not allowed to modify flags 0x001-0x800;
      they may be used internally by the interceptor to pass
      information from packet_cb/ssh_interceptor_packet_alloc to
      ssh_interceptor_send/ssh_interceptor_packet_free.  During
      certain times, such when applying asynchronous IPSEC
      transformations, this field may be changed concurrently by
      another thread (or interrupt) even when another thread is
      processing the packet. Care should be taken with locking in
      those situations. */
  SshUInt32 flags;

  /** Number of the interface that this packet arrived from */
  SshInterceptorIfnum ifnum_in;

  /** Number of the interface that this packet going out */
  SshInterceptorIfnum ifnum_out;

  /** Format of the packet (protocol identifier). */
  SshInterceptorProtocol protocol;

  /** Path MTU stored for this packet, which must be respected at
      media send (if interface MTU is smaller than this value, then
      the media send routine must send ICMP PMTU message and discard
      the packet). If 0, means use the interface MTU only. */
  SshUInt32 pmtu;

#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
  /** Platform-dependent extension selectors for things like user id,
      virtual network identifier, etc. */
  SshUInt32 extension[SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS];
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
  /** Route key selector that was used in the route lookup */
  SshUInt16 route_selector;
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  /** Buffer that can be used by higher level software to store its
      data (such as cached media addresses).  This field can only be
      accessed using the SSH_INTERCEPTOR_PACKET_DATA macro. */
  union {
    /** Contents of this union are private (they are there to
	guarantee proper alignment for any data structure stored
	here). */
    long force_alignment_l; int force_alignment_i; short force_alignment_s;
    void *force_alignment_v;
    SshUInt16 force_alignment_i16; SshUInt32 force_alignment_i32;
    SshUInt64 force_alignment_i64;
    char force_size_dont_use_this_directly[SSH_INTERCEPTOR_UPPER_DATA_SIZE];
    /* Should have double here if floating point was allowed. */
  } upper_data;

  /** Next pointer on freelist.  This can also be used by higher-level
      code to put the packet on a list. */
  struct SshInterceptorPacketRec *next;
} *SshInterceptorPacket, SshInterceptorPacketStruct;

/** Error codes for route add / remove functions. */
typedef enum {
  SSH_INTERCEPTOR_ROUTE_ERROR_OK = 0,
  SSH_INTERCEPTOR_ROUTE_ERROR_NONEXISTENT = 1,
  SSH_INTERCEPTOR_ROUTE_ERROR_OUT_OF_MEMORY = 2,
  SSH_INTERCEPTOR_ROUTE_ERROR_ALREADY_EXISTS = 3,
  SSH_INTERCEPTOR_ROUTE_ERROR_UNDEFINED = 255
} SshInterceptorRouteError;

/** Flag values for the route add / remove functions. */

/** Ignore non-existent routes when attempting to remove the route. */
#define SSH_INTERCEPTOR_ROUTE_FLAG_IGNORE_NONEXISTENT   0x0001

/** Data structure for routing key, used in route lookups and routing table
    manipulation.

    The route lookup is performed for the destination address, using other
    fields in the routing key to enforce routing policies. It is a fatal
    error to call ssh_interceptor_route using a SshInterceptorRouteKey
    which does not have the destination address set.

    Use the provided macros for setting fields in SshInterceptorRouteKey.

    Note that on platforms that do not support policy routing, the route lookup
    uses only the destination address. On other platforms other fields of the
    SshInterceptorRouteKey may be used in the route lookup. */
typedef struct SshInterceptorRouteKeyRec
{
  /** Destination address, mandatory */
  SshIpAddrStruct dst;
  /** Source address, optional */
  SshIpAddrStruct src;
  /** IP protocol identifier, optional */
  SshUInt32 ipproto;
  /** Interface number, optional.
      Note that this field specifies either the inbound interface number
      or the outbound interface number, depending on the value of the
      'selector' field. */
  SshUInt32 ifnum;
  /** Network layer fields */
  union
  {
    /** IPv4 TOS, optional */
    struct
    {
      SshUInt8 tos;
    } ip4;
    /** IPv6 priority and flow label, optional */
    struct
    {
      SshUInt8 priority;
      SshUInt32 flow;
    } ip6;
    /** For encoding / decoding */
    unsigned char raw[5];
  } nh;
  /** Transport layer fields */
  union
  {
    /** TCP / UDP ports, optional */
    struct
    {
      SshUInt16 dst_port;
      SshUInt16 src_port;
    } tcp;
    /** ICMP type and code, optional */
    struct
    {
      SshUInt8 type;
      SshUInt8 code;
    } icmp;
    /** ESP / AH spi, optional */
    struct
    {
      SshUInt32 spi;
    } ipsec;
    /** For encoding / decoding */
    unsigned char raw[4];
  } th;
#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
  /** Platform-dependent extension selectors, optional */
  SshUInt32 extension[SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS];
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */
  /** Bitmap of selectors that are to be used in the route lookup.
      Use the provided macros to add selectors to the routing key,
      do not access this field directly. The highest 3 bits of
      'selector' are reserved for flags defined below. */
  SshUInt16 selector;
} *SshInterceptorRouteKey, SshInterceptorRouteKeyStruct;

/*  Selector values for 'selector' bitmap */

/** Source address */
#define SSH_INTERCEPTOR_ROUTE_KEY_SRC                    0x0001
/** IP protocol identifier */
#define SSH_INTERCEPTOR_ROUTE_KEY_IPPROTO                0x0002
/** Inbound interface number */
#define SSH_INTERCEPTOR_ROUTE_KEY_IN_IFNUM               0x0004
/** Outbound interface number */
#define SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM              0x0008
/** IPv4 type of service */
#define SSH_INTERCEPTOR_ROUTE_KEY_IP4_TOS                0x0010
/** Platform-dependent Extension selectors */
#define SSH_INTERCEPTOR_ROUTE_KEY_EXTENSION              0x1000

/*  Flag values for 'selector' bitmap */

/** Packets going to this destination are transformed by the engine and the
    resulting packet may be larger than the path MTU reported to the IP stack
    by the engine. */
#define SSH_INTERCEPTOR_ROUTE_KEY_FLAG_TRANSFORM_APPLIED 0x2000
/** Source address belongs to one of the local interfaces. */
#define SSH_INTERCEPTOR_ROUTE_KEY_FLAG_LOCAL_SRC         0x4000

/** A callback function of this type will be called when the
    interceptor is first opened, and from then on whenever there is a
    change in the interface list (e.g., a new interface goes up or
    down or the address of an interface changes).  This function will
    be given a list of the interfaces.  The supplied array will only
    be valid for the duration of this call; the implementation of this
    function is supposed to copy the information if it is going to
    need it later.  Note that this function may be called
    asynchronously, concurrently with any other functions.  */
typedef void (*SshInterceptorInterfacesCB)(SshUInt32 num_interfaces,
                                           SshInterceptorInterface *ifs,
                                           void *context);

/** Callback functions of this type are called whenever a packet is
    received from the network or from a protocol.  This function must
    eventually free the packet, either by calling
    ssh_interceptor_packet_free on the packet or by passing it to the
    ssh_interceptor_send function.  Note that this function may be
    called asynchronously, concurrently with any other functions.

    When a packet is passed to this callback, the `pp->flags' field
    may contain arbitrary flags in the bits reserved for the
    interceptor (mask 0x00000fff).  This callback is not allowed to
    modify any of those bits; they must be passed intact to
    ssh_interceptor_send or ssh_interceptor_packet_free.  Any other
    bits (mask 0xfffff000) will be zero when the packet is sent to
    this callback; those bits may be used freely by this callback.
    They are not used by the interceptor. */
typedef void (*SshInterceptorPacketCB)(SshInterceptorPacket pp, void *context);

/** A callback function of this type is used to notify code above the
    interceptor that routing information has changed, and any cached
    routing data should be thrown away and refreshed by a new route
    lookup. */
typedef void (*SshInterceptorRouteChangeCB)(void *context);

/** Opens the packet interceptor.  This must be called before using
    any other interceptor functions.  This registers the callbacks
    that the interceptor will use to notify the higher levels of
    received packets or changes in the interface list.  The interface
    callback will be called once either during this call or soon after
    this has returned.

    The `machine_context' argument is intended to be meaningful only
    to machine-specific code.  It is passed through from the
    machine-specific main program.  One example of its possible uses
    is to identify a virtual router in systems that implement multiple
    virtual routers in a single software environment.  Most
    implementations will ignore this argument.

    The `packet_cb' callback will be called whenever a packet is
    received from either a network adapter or a protocol stack.  The
    first calls may arrive already before this function has returned.

    The `interfaces_cb' callback will be called once soon after
    opening the interceptor (possibly before this call returns).  From
    then on, it will be called whenever there is a change in the
    interface list (e.g., the IP address of an interface is changed,
    or a PPP interface goes up or down).

    The `route_change_cb' callback should be called whenever there is
    a change in routing information.  Implementing this callback is
    optional but beneficial in e.g. router environments (the
    information is not easily available on all systems).

    The `context' argument is passed to the callbacks.

    This function returns TRUE if opening the interceptor was
    successful.  The interceptor object is returned in the
    `interceptor_return' argument.  Most systems will only allow a
    single interceptor to be opened; however, some systems may support
    multiple interceptors identified by the `machine_context'
    argument.  This returns FALSE if an error occurs (e.g., no
    interceptor kernel module is loaded on this system, or the
    interceptor is already open).

    Care must be taken regarding concurrency control in systems that have
    multithreaded IP stacks.  In particular:
     - packet_cb and interfaces_cb may get called before this function
       returns.  It is, however, guaranteed that `*interceptor_return'
       has been set before the first call to either of them.
     - the interceptor cannot be closed while there are calls or packets
       out.  The ssh_interceptor_stop function must be used.
     In such systems, additional concurrency may be introduced by timeouts
     and actions from the policy manager connection. */
Boolean ssh_interceptor_open(void *machine_context,
                             SshInterceptorPacketCB packet_cb,
                             SshInterceptorInterfacesCB interfaces_cb,
                             SshInterceptorRouteChangeCB route_change_cb,
                             void *context,
                             SshInterceptor *interceptor_return);

/** Sends a packet to the network or to the protocol stacks.  This
    will eventually free the packet by calling
    ssh_interceptor_packet_free.  The `media_header_len' argument
    specifies how many bytes from the start of the packet are media
    (link-level) headers.  It will be 0 if the interceptor operates
    directly at protocol level. If the configure define
    INTERCEPTOR_IP_ALIGNS_PACKETS is set, this function must ensure
    that the IP header of the packet is aligned to a word boundary.

    This function relies on 'pp->ifnum_out' being an 'ifnum' which has
    previously been reported via a SshInterceptorInterfacesCB. It does
    not have to be valid at that precise point in time. If 'pp->ifnum_out'
    is an ifnum which may have been previously reported to the
    SshInterceptorInterfacesCB, then ssh_interceptor_send() MUST check
    that it is valid, or otherwise discard the packet.  Also,
    pp->protocol (and the corresponding encapsulation) may be
    incorrect for the interface denoted by 'pp->ifnum_out'. The packet
    should be dropped also in this case.

    This function can be called concurrently from multiple threads,
    but only for one packet at a time.  It is ok to call this even
    before ssh_interceptor_open has actually returned (from a
    `packet_cb' or `interface_cb' callback). */

void ssh_interceptor_send(SshInterceptor interceptor,
                          SshInterceptorPacket pp,
                          size_t media_header_len);

/** Enables or disables packet interception. */
void ssh_interceptor_enable_interception(SshInterceptor interceptor,
					 Boolean enable);

/** Stops the packet interceptor.  After this call has returned, no
    new calls to the packet and interfaces callbacks will be made.
    The interceptor keeps track of how many threads are processing
    packet, interface, or have pending route callbacks, and this
    function returns TRUE if there are no callbacks/pending calls to
    those functions.  This returns FALSE if threads are still
    executing in those callbacks or routing callbacks are pending.

    After calling this function, the higher-level code should wait for
    packet processing to continue, free all packet structures received
    from that interceptor, and then call ssh_interceptor_close.  It is
    not an error to call this multiple times (the latter calls are
    ignored).

    It is forbidden to hold ANY locks when calling
    ssh_interceptor_stop(). */

Boolean ssh_interceptor_stop(SshInterceptor interceptor);

/** Closes the packet interceptor.  This function can only be called when
     - ssh_interceptor_stop has been called
     - all packet and interface callbacks from this interceptor have
       returned.
     - all ssh_interceptor_route completions have been called
     - all packets received from the packet callbacks from this interceptor
       have been freed.

   It is illegal to call any packet interceptor functions (other than
   ssh_interceptor_open) after this call.  This function cannot be
   called from an interceptor callback.

   This function can be called from one thread only for any particular
   interceptor.  If multiple interceptors are supported, then this may be
   called for different interceptors asynchronously. */
void ssh_interceptor_close(SshInterceptor interceptor);

/** Completion function for route lookup.  This function is called when the
    route lookup is complete.
     reachable    FALSE if the destination cannot be reached, TRUE otherwise
     next_hop_gw  IP address of next hop gw or destination
     ifnum        network interface to which to send the packet
     mtu          path mtu, or 0 if not known (= should use link mtu)
     context      context argument supplied to route request.

    This function may get called concurrently from multiple threads. */
typedef void (*SshInterceptorRouteCompletion)(Boolean reachable,
                                              SshIpAddr next_hop_gw,
                                              SshInterceptorIfnum ifnum,
                                              size_t mtu,
                                              void *context);

/** Looks up routing information for the routing key specified
    by `key'.  Calls the callback function either during this
    call or some time later.  The purpose of the callback function is
    to allow this function to perform asynchronous operations, such as
    forwarding the routing request to a user-level process.  This
    function will not be very efficient on some systems, and calling
    this on a per-packet basis should be avoided if possible.
    This function expects that 'key' is valid only for the duration
    of the call, and will take a local copy of it, if necessary.

    Note that if this function is implemented by forwarding the
    request to a user-level process, care must be taken to never lose
    replies.  The completion function must *always* be called.  For
    example, if the policy manager interface is used to pass the
    requests to the policy manager process, and the interface is
    closed, the completion function must still be called for all
    requests.  Code may be needed to keep track of which requests are
    waiting for replies from the user-level process.

    This function can be called concurrently from multiple threads.
    While legal, new calls to this function after calling
    ssh_interceptor_stop should be avoided, because it is not possible
    to call ssh_interceptor_close until all route lookup completions
    have been called. */
void ssh_interceptor_route(SshInterceptor interceptor,
                           SshInterceptorRouteKey key,
                           SshInterceptorRouteCompletion completion,
                           void *context);

/** Allocates a packet of at least the given size.  Packets can only
    be allocated using this function (either internally by the
    interceptor or by other code by calling this function).
    Typically, this takes a packet header from a free list, stores a
    pointer to a platform-specific packet object, and returns the
    packet header.  This should be re-entrant and support concurrent
    operations if the IPSEC engine is re-entrant on the target
    platform.  Other functions in this interface should be re-entrant
    for different packet objects, but only one operation will be in
    progress at any given time for a single packet object.  This
    returns NULL if no more packets can be allocated.  On systems that
    support concurrency, this can be called from multiple threads
    concurrently.

    This sets initial values for the mandatory fields of the packet
    that always need to be initialized.  However, any of these fields
    can be modified later. */
SshInterceptorPacket ssh_interceptor_packet_alloc(SshInterceptor interceptor,
                                                 SshUInt32 flags,
                                                 SshInterceptorProtocol proto,
                                                 SshInterceptorIfnum ifnum_in,
                                                 SshInterceptorIfnum ifnum_out,
                                                 size_t total_len);


/** Frees the packet.

    All packets allocated by ssh_interceptor_packet_alloc must
    eventually be freed using this function by either calling this
    explicitly or by passing the packet to the ssh_interceptor_send
    function.  Typically, this calls a suitable function to
    free/release the platform-specific packet object, and puts the
    packet header on a free list.  This function should be re-entrant,
    so if a free list is used, it should be protected by a lock in
    systems that implement concurrency in the IPSEC Engine.  Multiple
    threads may call this function concurrently for different packets,
    but not for the same packet. */
void ssh_interceptor_packet_free(SshInterceptorPacket pp);

/** Returns the total length of the packet in bytes.  Multiple threads may
    call this function concurrently, but not for the same packet. */
size_t ssh_interceptor_packet_len(SshInterceptorPacket pp);


/** Copies data into the packet. Space for the new data must already
    have been allocated. It is a fatal error to attempt to copy beyond
    the allocated packet. Multiple threads may call this function
    concurrently, but not for the same packet. Returns TRUE if
    successfull and FALSE otherwise. If error occurs then the pp is
    already freed by this function, and the caller must not refer to
    it anymore.

    There is a generic version of this function inside the engine, in
    case interceptor does not want to implement this. If interceptor
    implements this function it must define the
    INTERCEPTOR_HAS_PACKET_COPYIN pre-processor symbol. */
Boolean ssh_interceptor_packet_copyin(SshInterceptorPacket pp, size_t offset,
                                      const unsigned char *buf, size_t len);

/** Copies data out from the packet.  Space for the new data must
    already have been allocated.  It is a fatal error to attempt to
    copy beyond the allocated packet. Multiple threads may call this
    function concurrently, but not for the same packet.

    There is a generic version of this function inside the engine, in
    case interceptor does not want to implement this. If interceptor
    implements this function it must define the
    INTERCEPTOR_HAS_PACKET_COPYOUT pre-processor symbol. */
void ssh_interceptor_packet_copyout(SshInterceptorPacket pp, size_t offset,
                                    unsigned char *buf, size_t len);


/** These two routines provide way to export and import
    interceptor-specific internal packet data as an opaque binary data
    block.

    If the export routine returns FALSE, the packet `pp' is
    invalidated. If it returnes TRUE, but *data_ret is NULL, then no
    internal data was exported. If *data_ret is non-NULL, then
    *len_return contains the length of *data_ret in bytes. The caller
    must free the *data_ret value using ssh_free.

    The import routine returns TRUE if the data was imported
    successfully, otherwise it returns FALSE and the packet `pp' is
    invalidated. It is a fatal error to call import routine on the
    same packet more than once.

    Notice: If the interceptor does not define these routines, then
    the engine provides dummy versions.

    Notice: This routine overlaps a bit with
    ssh_interceptor_packet_alloc_and_copy_ext_data, as it could be
    implemented as:

        new_pp = ssh_interceptor_packet_alloc(...);
        ssh_interceptor_packet_copy(pp, 0, ..., new_pp, 0);
        ssh_interceptor_packet_export_internal_data(pp, &data, &len);
        ssh_interceptor_packet_import_internal_data(new_pp, data, len);
        ssh_free(data);

    The main purpose of these routines is to allow some per-packet
    interceptor-specific data to be transported to the usermode
    engine. Under the kernel IPSec Engine, these routines are not
    actually used at all. */
Boolean ssh_interceptor_packet_export_internal_data(SshInterceptorPacket pp,
                                                    unsigned char **data_ret,
                                                    size_t *len_return);

Boolean ssh_interceptor_packet_import_internal_data(SshInterceptorPacket pp,
                                                    const unsigned char *data,
                                                    size_t len);

void ssh_interceptor_packet_discard_internal_data(unsigned char *data,
						  size_t data_len);


/** Copy data from one packet to another. Start from the
    `source_offset' and copy `bytes_to_copy' bytes to
    `destination_offset' in the destination packet. If the destination
    packet cannot be written then return FALSE, and the destination
    packet has been freed by this function. The source packet is not
    freed even in case of error. If data copying was successfull then
    return TRUE.

    This function can also be implemented so that it will simply
    increment the reference counts in the source packet and share the
    actual data without copying it at all. There is a generic version
    of this function inside the engine, in case interceptor does not
    want to implement this. If interceptor implements this function it
    must define INTERCEPTOR_HAS_PACKET_COPY pre-processor symbol. */
Boolean ssh_interceptor_packet_copy(SshInterceptorPacket source_pp,
                                    size_t source_offset,
                                    size_t bytes_to_copy,
                                    SshInterceptorPacket destination_pp,
                                    size_t destination_offset);

#ifdef DEBUG_LIGHT
#define KERNEL_INTERCEPTOR_USE_FUNCTIONS
#endif /* DEBUG_LIGHT */

#ifdef INTERCEPTOR_HAS_PLATFORM_INCLUDE
#include "platform_interceptor.h"
#endif /* INTERCEPTOR_HAS_PLATFORM_INCLUDE */

#endif /* INTERCEPTOR_H */
