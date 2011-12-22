/*
 *@author: Frederico Gonçalves
 *
 *This is the header file for the proc entry which registers and 
 *prints filters.
 *
 *Reading the file will print the registered filters.
 *Writting to the file will (un)register new filters. (See 
 *specification in .c file)
 */

#ifndef __PROC_FILTERS_H__
#define __PROC_FILTERS_H__

#ifdef CONFIG_SYNCH_ADHOC

extern void initialize_proc_filters(void);
extern void teardown_proc_filters(void);
extern void mess_proc_entry(void);

#endif

#endif
