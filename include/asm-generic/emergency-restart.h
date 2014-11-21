#ifndef _ASM_GENERIC_EMERGENCY_RESTART_H
#define _ASM_GENERIC_EMERGENCY_RESTART_H

static inline void machine_emergency_restart(void)
{
#ifdef CONFIG_ANDROID_WIP
	machine_restart("emergency");
#else
	machine_restart(NULL);
#endif
}

#endif /* _ASM_GENERIC_EMERGENCY_RESTART_H */
