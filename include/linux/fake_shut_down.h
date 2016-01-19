#ifndef _FAKE_SHUT_DOWN_H
#define _FAKE_SHUT_DOWN_H

#include <linux/notifier.h>

/*
 * Commands for fake shut down.
 *
 * ON     Start fake shut down
 * OFF    Exit fake shut down
 */
#define	FAKE_SHUT_DOWN_CMD_ON		1
#define	FAKE_SHUT_DOWN_CMD_OFF		0

extern bool fake_shut_down;
extern struct raw_notifier_head fsd_notifier_list;

extern int register_fake_shut_down_notifier(struct notifier_block *);
extern int unregister_fake_shut_down_notifier(struct notifier_block *);

#endif /* _FAKE_SHUT_DOWN_H */
