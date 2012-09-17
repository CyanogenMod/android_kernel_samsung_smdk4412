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
 * engine.h
 *
 * Engine API specifies the Engine side interface between the Interceptor
 * and the Engine components.
 *
 */

#ifndef ENGINE_H
#define ENGINE_H

/******************************** Data types ********************************/

/* Definition of the type for the engine object. */
typedef struct SshEngineRec *SshEngine;

/* A function of this type is used to send messages from the engine to
   the policy manager.  The function should return TRUE if the message
   was actually sent, and FALSE otherwise.  This should always
   eventually free `data' with ssh_free.  The packet in the buffer
   starts with a 32-bit length MSB first.  If the connection to the
   policy manager is not open, this should return FALSE and free
   `data' using ssh_free.  Warning: this function is called from
   ssh_debug and ssh_warning; thus, this is not allowed to emit
   debugging or warning messages.  This function can be called
   concurrently, and must perform appropriate locking. */
typedef Boolean (*SshEngineSendProc)(unsigned char *data, size_t len,
                                     Boolean reliable,
                                     void *machine_context);

/***************************************************************************
 * Functions called by the machine-dependent main program
 ***************************************************************************/

/* Flags for the ssh_engine_start function. */
#define SSH_ENGINE_DROP_IF_NO_IPM       0x00000001
#define SSH_ENGINE_NO_FORWARDING        0x00000002

/* Creates the engine object.  Among other things, this opens the
   interceptor, initializes filters to default values, and arranges to send
   messages to the policy manager using the send procedure.  The send
   procedure will not be called until from the bottom of the event loop.
   The `machine_context' argument is passed to the interceptor and the
   `send' callback, but is not used otherwise.  This function can be
   called concurrently for different machine contexts, but not otherwise.
   The first packet and interface callbacks may arrive before this has
   returned. */
SshEngine ssh_engine_start(SshEngineSendProc send,
                           void *machine_context,
                           SshUInt32 flags);

/* Stops the engine, closes the interceptor, and destroys the
   engine object.  This does not notify IPM interface of the close;
   that must be done by the caller before calling this.  This returns
   TRUE if the engine was successfully stopped (and the object freed),
   and FALSE if the engine cannot yet be freed because there are
   threads inside the engine or uncancellable callbacks expected to
   arrive.  When this returns FALSE, the engine has started stopping,
   and this should be called again after a while.  This function can
   be called concurrently with packet/interface callbacks or timeouts
   for this engine, or any functions for other engines.*/
Boolean ssh_engine_stop(SshEngine engine);

/* The machine-specific main program should call this when the policy
   manager has opened the connection to the engine.  This also
   sends the version packet to the policy manager.  This function can
   be called concurrently with packet/interface callbacks or timeouts. */
void ssh_engine_notify_ipm_open(SshEngine engine);

/* This function is called whenever the policy manager closes the
   connection to the engine.  This is also called when the engine is
   stopped.  This function can be called concurrently with
   packet/interface callbacks or timeouts. */
void ssh_engine_notify_ipm_close(SshEngine engine);

/* This function should be called by the machine-dependent main
   program whenever a packet for this engine is received from
   the policy manager.  The data should not contain the 32-bit length
   or the type (they have already been processed at this stage, to
   check for possible machine-specific packets).  The `data' argument
   remains valid until this function returns; it should not be freed
   by this function.  This function can be called concurrently. */
void ssh_engine_packet_from_ipm(SshEngine engine,
                                SshUInt32 type,
                                const unsigned char *data, size_t len);

/******************************** Version global ****************************/

/* This is statically (compile-time) initialized to SSH_ENGINE_VERSION */
extern const char ssh_engine_version[];

/* This is statically (compile-time) initialized to a value containing
   information about the SSH_ENGINE_VERSION, compilation time,
   compiler etc. etc. etc. It can be used by interceptors, usermode
   engine etc. for startup output or somesuch. Debug information,
   basically, and can vary quite much depending on the compilation
   environment. */
extern const char ssh_engine_compile_version[];

/* Suffix to append to the device name.  This is defined by the
   engine. */
extern const char ssh_device_suffix[];

#endif /* ENGINE_H */
