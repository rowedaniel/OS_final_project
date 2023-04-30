/**
 * @author Jeff Butler 
 * @author Daniel Neshyba-Rowe
 * 
*/

#include <sel4cp.h>

/**
 * Types for timer driver
*/
typedef uint64_t timestamp_t;
/* typedef unsigned long long uint64_t; */
typedef unsigned int uint32_t;
typedef void (*timer_callback_t)(int);


/**
 * Initialises the driver. Initialises the timer with the registers 
 * mapped at the provided virtual address. This should start the 
 * internal counter and allow timers to be registered.
*/

// NOTE: changed from:
/* int start_timer(unsigned char *timer_vaddr); */
// to:
int start_timer(uintptr_t timer_vaddr);

/**
 * Read the current time in microseconds from the internal counter timer (timer E).
*/
timestamp_t get_time(void);

/**
 * Registers a callback function be called after the specified 
 * interval (in microseconds, though actual wakeup resolution 
 * will depend on the timer resolution). Several registrations 
 * may be pending at any time. The return value is zero on failure, 
 * otherwise a unique identifier for this timeout. This identifier 
 * can be used to remove a timeout. After a timeout has occurred, 
 * or the timeout has been removed, the identifier may be reused.
*/
uint32_t register_timer(uint64_t delay, timer_callback_t callback, void *data);

/**
 * Remove a previously registered timer callback, using the unique 
 * identifier returned by register_timer.
*/
int remove_timer(uint32_t id);
/**
 * Stops operation of the driver. This will remove any outstanding 
 * time requests.
*/
int stop_timer(void);

// Currently thinking of replacing this with the notified() method below.
// This is the way which the Australians do it, but the notified (taken
// from example timer for odroid) definitely works
/**
 * Function to be called by the IRQ dispatch whenever an IRQ is 
 * triggered by the timer hardware. The IRQ dispatch will pass 
 * in the data and IRQ number that the callback was registered 
 * with and an IRQ handler capability that must be used to 
 * acknowledge the IRQ.
*/
/* int timer_irq(void *data, seL4_Word irq, seL4_IRQHandler irq_handler); */


/**
 * seL4 method of initializing the driver
 */
void init(void);

/**
 * seL4 method of recieving interrupts
 */
void notified(sel4cp_channel ch);
