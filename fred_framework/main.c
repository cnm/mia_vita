/*
 * This file contains the entry point and exit point to the interceptor framework kernel module.
 *
 * Initializes 2 proc entries. One which contains the registered rules for each interceptor
 * and another which is used to communicate from user land to kernel land and register/unregister
 * rules.
 *
 * Initializes the filter chains and hooks.
 *
 * Initializes the interceptor and rule manager.
 * */

#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef __MODULE__
#define __MODULE__
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/in.h>
#include <linux/slab.h>

#include "interceptor_manager.h"
#include "rule_manager.h"
#include "interceptor.h"
#include "chains.h"
#include "hooks.h"
#include "proc_entry.h"
#include "proc_registry.h"

/*
 * Function which registers the interceptor framework kernel module
 *
 * It:
 *   1.     Creates a proc entry for listing the installed interceptors and their rules
 *      1.1    Creates the buffer to be used in proc
 *      1.2    Creates /proc/interceptor_list
 *   2.     Creates a proc to allow for rules to be inserted and removed
 *      2.1    Created the /proc/interceptor_registry
 *   3.     Starts the chains
 *      3.1    Initializes 5 changes (Pre, LocalIn, Fwd, LocalOut, Post
 *      3.2    Each chain has a klist which are the filters
 *   4.     Starts hooks
 *   5.     Starts interceptor manager
 *      5.1    Start a klist for receiving the interceptors (like aggregation or desaggregation)
 *   6.     Starts rule manager
 *      6.1    Start a klist for receiving the rules (many rules can be for one interceptor)
 * */
int init_module() {
	//Register interceptors and rules
	create_interceptor_proc_entry();

	//Register/unregister rules by writing to this file
	create_proc_registry();

	init_chains();

	init_hooks();

	if (!start_interceptor_manager())
		return -ENOMEM;
	if (!start_rule_manager())
		return -ENOMEM;

	printk(KERN_INFO " Interceptor framework module loaded.\n");

	return 0;
}

/*
 * Function which unloads the module
 * */
void cleanup_module() {
	stop_interceptor_manager();
	stop_rule_manager();
	free_hooks();
	clear_interceptor_proc_entry();
	remove_proc_registry();
printk(KERN_INFO " Interceptor framework module unloaded.\n");
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Frederico Gonçalves, [frederico.lopes.goncalves@gmail.com]");
MODULE_DESCRIPTION("Interceptor framework. This is the base framework which exports symbols so other modules can register interceptors.");
