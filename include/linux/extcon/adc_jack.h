/*
 * include/linux/extcon/adc_jack.h
 *
 * Analog Jack extcon driver with ADC-based detection capability.
 *
 * Copyright (C) 2012 Samsung Electronics
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _EXTCON_ADC_JACK_H_
#define _EXTCON_ADC_JACK_H_ __FILE__
#include <linux/extcon.h>

/**
 * struct adc_jack_data - internal data for adc_jack device driver
 * @edev	- extcon device.
 * @cable_names	- list of supported cables.
 * @num_cables	- size of cable_names.
 * @adc_condition	- list of adc value conditions.
 * @num_condition	- size of adc_condition.
 * @irq		- irq number of attach/detach event (0 if not exist).
 * @handling_delay	- interrupt handler will schedule extcon event
 *			handling at handling_delay jiffies.
 * @handler	- extcon event handler called by interrupt handler.
 * @get_adc	- a callback to get ADC value to identify state.
 * @ready	- true if it is safe to run handler.
 */
struct adc_jack_data {
	struct extcon_dev edev;

	const char **cable_names;
	int num_cables;
	struct adc_jack_cond *adc_condition;
	int num_conditions;

	int irq;
	unsigned long handling_delay; /* in jiffies */
	struct delayed_work handler;

	int (*get_adc)(u32 *value);

	bool ready;
};

/**
 * struct adc_jack_cond - condition to use an extcon state
 * @state	- the corresponding extcon state (if 0, this struct denotes
 *		the last adc_jack_cond element among the array)
 * @min_adc	- min adc value for this condition
 * @max_adc	- max adc value for this condition
 *
 * For example, if { .state = 0x3, .min_adc = 100, .max_adc = 200}, it means
 * that if ADC value is between (inclusive) 100 and 200, than the cable 0 and
 * 1 are attached (1<<0 | 1<<1 == 0x3)
 *
 * Note that you don't need to describe condition for "no cable attached"
 * because when no adc_jack_cond is met, state = 0 is automatically chosen.
 */
struct adc_jack_cond {
	u32 state; /* extcon state value. 0 if invalid */

	u32 min_adc;
	u32 max_adc;
};

/**
 * struct adc_jack_pdata - platform data for adc jack device.
 * @name	- name of the extcon device. If null, "adc-jack" is used.
 * @cable_names	- array of cable names ending with null. If the array itself
 *		if null, extcon standard cable names are chosen.
 * @adc_contition	- array of struct adc_jack_cond conditions ending
 *			with .state = 0 entry. This describes how to decode
 *			adc values into extcon state.
 * @irq		- IRQ number that is triggerred by cable attach/detach
 *		events. If irq = 0, use should manually update extcon state
 *		with extcon APIs.
 * @irq_flags	- irq flags used for the @irq
 * @handling_delay_ms	- in some devices, we need to read ADC value some
 *			milli-seconds after the interrupt occurs. You may
 *			describe such delays with @handling_delay_ms, which
 *			is rounded-off by jiffies.
 * @get_adc	- the callback to read ADC value to identify cable states.
 */
struct adc_jack_pdata {
	const char *name;
	/*
	 * NULL if standard extcon names are used.
	 * The last entry should be NULL
	 */
	const char **cable_names;
	/* The last entry's state should be 0 */
	struct adc_jack_cond *adc_condition;

	int irq; /* Jack insertion/removal interrupt */
	unsigned long irq_flags;
	unsigned long handling_delay_ms; /* in ms */

	/* When we have ADC subsystem, this can be generalized. */
	int (*get_adc)(u32 *value);
};

#endif /* _EXTCON_ADC_JACK_H */
