
#include "oslib.h"
//#include "include/board.h"
//#include "kernel.h"
//#include "list.h"

// pour tester
//
int test_add(int a, int b)
{
    int val;
    __ASM volatile ("svc 0\n\tmov %0, r0" : "=r" (val));
    return val;
}

/*****************************************************************************
 * Memory allocation functions
 *****************************************************************************/
void* os_alloc(unsigned int req)
{
	/* A COMPLETER */
	return malloc( (size_t) req);
	
	//return NULL;
}

void os_free(void *ptr)
{
	/* A COMPLETER */
	free(ptr);
}

/*****************************************************************************
 * General OS handling functions
 *****************************************************************************/

/* os_start
 *   start the first created task
 */
void os_start()
{
		 __ASM volatile ("svc 3");
}


/*****************************************************************************
 * Task handling functions
 *****************************************************************************/

/* task_new
 *   create a new task :
 *   func      : task code to be run
 *   stacksize : task stack size
 *   returns the task id
 */
int32_t task_new(TaskCode func, uint32_t stacksize)
{
	/* A COMPLETER */
	int32_t val;
	 __ASM volatile ("svc 4\n\tmov %0, r0" : "=r" (val));

    return val;
}

/* task_id
 *   returns id of task
 */
uint32_t task_id()
{
	int32_t val;
	 __ASM volatile ("svc 5\n\tmov %0, r0" : "=r" (val));

	return val;
	
	return 0;
}

/* task_kill
 *   kill oneself
 */
void task_kill()
{
	__ASM volatile ("svc 6");
}

/* task_yield
 *   run scheduler to switch to another task
 */
void task_yield()
{
	/* A COMPLETER */
}

/* task_wait
 *   suspend the current task until timeout
 */
void  task_wait(uint32_t ms)
{
	__ASM volatile ("svc 7");
}

/*****************************************************************************
 * Semaphore handling functions
 *****************************************************************************/

/* sem_new
 *   create a semaphore
 *   init    : initial value
 */
Semaphore * sem_new(int32_t init)
{
	Semaphore* val;
		 __ASM volatile ("svc 8\n\tmov %0, r0" : "=r" (val));

		return val;
}

/* sem_p
 *   take a semaphore
 */
void sem_p(Semaphore * sem)
{
	__ASM volatile ("svc 9");
}

/* sem_v
 *   release a semaphore
 */
void sem_v(Semaphore * sem)
{
	__ASM volatile ("svc 10");
}
