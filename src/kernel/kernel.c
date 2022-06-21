#include <stdlib.h>
#include "kernel.h"
#include "list.h"
#include "oslib.h"

#ifndef NULL
#define NULL 0
#endif


/*****************************************************************************
 * Global variables
 *****************************************************************************/

static uint32_t id=1;
Task * tsk_running = NULL;	/* pointer to ready task list : the first
                                     node is the running task descriptor */
Task * tsk_prev = NULL;
Task * tsk_sleeping = NULL;	/* pointer to sleeping task list */

/*****************************************************************************
 * SVC dispatch
 *****************************************************************************/
/* sys_add
 *   test function
 */
int sys_add(int a, int b)
{
    return a+b;
}

/* syscall_dispatch
 *   dispatch syscalls
 *   n      : syscall number
 *   args[] : array of the parameters (4 max)
 */
int32_t svc_dispatch(uint32_t n, uint32_t args[])
{
	int32_t result=-1;

    switch(n) {
      case 0:
          result = sys_add( (int)args[0], (int)args[1] );
          break;
      /* A COMPLETER */
      case 1 :
			  result = (int32_t) os_alloc( (unsigned int)args[0] );
			  break;

      case 2 :
    	  os_free( (void*)args[0] );
			  break;

      case 3 :
    	  sys_os_start();
      		  break;

      case 4 :
    	  	  result = (int32_t) sys_task_new( (TaskCode)args[0], (uint32_t)args[1] );
      		  break;

      case 5 :
    	  	  result = (int32_t) sys_task_id( );
      		  break;

      case 6 :
    	  sys_task_kill();
      		  break;

      case 7 :
    	  sys_task_wait( (uint32_t)args[0] );
      		  break;

      case 8 :
    	  	  result = (int32_t) sys_sem_new( (uint32_t)(args[0]) );
      		  break;

      case 9 :
    	  sys_sem_p( (Semaphore*)(args[0]) );
      		  break;

      case 10 :
    	  sys_sem_v( (Semaphore*)(args[0]) );
      		  break;
    }
    return result;
}

void sys_switch_ctx()
{
	SCB->ICSR |= 1<<28; // set PendSV to pending
}
/*****************************************************************************
 * Round robin algorithm
 *****************************************************************************/
#define SYS_TICK  10	// system tick in ms

uint32_t sys_tick_cnt=0;

/* tick_cb
 *   system tick callback: task switching, ...
 */
void sys_tick_cb()
{

	// swich tasks
	tsk_prev = tsk_running;
	tsk_prev->status = TASK_READY;
	tsk_running = tsk_running->next;
	tsk_running->status = TASK_RUNNING;
	// handle sleeping task
	Task *var= tsk_sleeping;
	for (int i=0; i< list_size(tsk_sleeping); i++){
		var->delay-=SYS_TICK;
		if (var->delay<0){
			// remettre la task dans la boucle des runnings
			tsk_running->status = TASK_READY;
			var->status = TASK_RUNNING;
			tsk_sleeping = list_remove_head(tsk_sleeping, &var) ;
			tsk_running = list_insert_head(tsk_running, var);
		}
		var= tsk_sleeping->next;
	}
	sys_switch_ctx();


//	list_display(tsk_running);
}

void SysTick_Handler(void)
{
	sys_tick_cnt++;

	if (sys_tick_cnt == SYS_TICK) {
		sys_tick_cnt = 0;
		sys_tick_cb();
	}
}

/*****************************************************************************
 * General OS handling functions
 *****************************************************************************/

/* sys_os_start
 *   start the first created task
 */
int32_t sys_os_start()
{
	tsk_running->status = TASK_RUNNING;

	sys_switch_ctx();
    // Reset BASEPRI
    __set_BASEPRI(0);

	// Set systick reload value to generate 1ms interrupt
    SysTick_Config(SystemCoreClock / 1000U);
    return 0;
}

/*****************************************************************************
 * Task handling functions
 *****************************************************************************/
void task_kill();

/* sys_task_new
 *   create a new task :
 *   func      : task code to be run
 *   stacksize : task stack size
 *
 *   Stack frame:
 *      |    xPSR    |
 *      |     PC     |
 *      |     LR     |
 *      |     R12    |    ^
 *      |     R3     |    ^
 *      |     R2     |    | @++
 *      |     R1     |
 *      |     R0     |
 *      +------------+
 *      |     R11    |
 *      |     R10    |
 *      |     R9     |
 *      |     R8     |
 *      |     R7     |
 *      |     R6     |
 *      |     R5     |
 *      |     R4     |
 *      +------------+
 *      | EXC_RETURN |
 *      |   CONTROL  | <- sp
 *      +------------+
 */
int32_t sys_task_new(TaskCode func, uint32_t stacksize)
{
	// get a stack with size multiple of 8 bytes
	uint32_t size = stacksize>96 ? 8*(((stacksize-1)/8)+1) : 96;
	
	/* A COMPLETER */
	Task* Tache = (Task*)malloc(sizeof(Task)+size);

	if (!Tache){ // si pointe sur rien
		return -1;
	}

	Tache->id = id++;
	Tache->splim = (uint32_t *)Tache+1;
	Tache->status = TASK_READY;
	Tache->sp = Tache->splim + (size/4); //Size est en octet. On veut aller à une adresse en uint32 donc 32/4 = 8
	Tache->delay = 0;

	Tache->sp -= 18;//reserve space for context

	Tache->sp[0] = (/*tsk->sp[0]*/0x0)|(0x1<<0);//CONTROL = unprivileged
	Tache->sp[1] = 0xFFFFFFFD;//EXC_RETURN = thread, psp
	Tache->sp[15] = (uint32_t)task_kill;//LR (calling context)
	Tache->sp[16] = (uint32_t)func;//PC
	Tache->sp[17] = 1<<24;//xPSR = 1<<24

	tsk_running = list_insert_tail(tsk_running, Tache);


    return Tache->id;
}

/* sys_task_kill
 *   kill oneself
 */
int32_t sys_task_kill()
{
	Task* t = tsk_running;
	tsk_running = list_remove_head(tsk_running, &t);
	tsk_running->status = TASK_RUNNING;
	sys_switch_ctx();
	free(t);

	return -1;
}

/* sys_task_id
 *   returns id of task
 */
int32_t sys_task_id()
{

	if (!tsk_running){
		return -1;
	}
    return tsk_running->id;
}


/* sys_task_yield
 *   run scheduler to switch to another task
 */
int32_t sys_task_yield()
{

    return -1;
}

/* task_wait
 *   suspend the current task until timeout
 */
int32_t sys_task_wait(uint32_t ms)
{
	Task *var;
	tsk_running = list_remove_head(tsk_running, &var) ;
	tsk_sleeping = list_insert_tail(tsk_sleeping, var);
	var->delay=ms;
	tsk_prev = var;
	tsk_prev->status = TASK_WAITING;
	tsk_running->status = TASK_RUNNING;
	sys_switch_ctx();
    return -1;
}


/*****************************************************************************
 * Semaphore handling functions
 *****************************************************************************/

/* sys_sem_new
 *   create a semaphore
 *   init    : initial value
 */
Semaphore * sys_sem_new(int32_t init)
{
	Semaphore* sem = (Semaphore*)malloc(sizeof(Semaphore));
	sem->count = init;
	sem->waiting= NULL;
	return sem;
}

/* sys_sem_p
 *   take a token
 */
int32_t sys_sem_p(Semaphore * sem)
{
	sem->count--;
	if (sem->count<0) {
		Task *var;
		tsk_running = list_remove_head(tsk_running, &var) ;
		sem->waiting = list_insert_tail(sem->waiting, var);
		var->status = TASK_WAITING;
		tsk_running->status = TASK_RUNNING;
		tsk_prev = var;
		sys_switch_ctx();
	}



	return -1;
}

/* sys_sem_v
 *   release a token
 */
int32_t sys_sem_v(Semaphore * sem)
{
	sem->count++;
	if (list_size(sem->waiting)!=0) {
		Task *var;
		tsk_prev = tsk_running;
		sem->waiting = list_remove_head(sem->waiting, &var) ;
		tsk_running = list_insert_head(tsk_running, var);
		tsk_prev->status = TASK_WAITING;
		tsk_running->status = TASK_RUNNING;
		sys_switch_ctx();
	}

	return -1;
}
