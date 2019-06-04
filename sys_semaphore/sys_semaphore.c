#include <asm/barrier.h>
#include <asm/current.h>
#include <linux/linkage.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

DECLARE_WAIT_QUEUE_HEAD(sem_queue);

static int next_semaphore_id = 0;
static int semaphore_counter = 0;

static int create_semaphore(int initial_counter)
{
	semaphore_counter = initial_counter;
	return next_semaphore_id;
}

static long block(void)
{
	wait_event(sem_queue, (semaphore_counter > 0));
	return 0;
}

static long unblock(void)
{
	wake_up_all(&sem_queue);
	return 0;
}

asmlinkage long sys_init_semaphore(int initial_counter)
{
	return create_semaphore(initial_counter);
}

asmlinkage long sys_up(int sem_id)
{
	local_irq_disable();
	semaphore_counter++;
	if (semaphore_counter == 1)
	{
		unblock();
	}
	local_irq_enable();
	return 0;
}

asmlinkage long sys_down(int sem_id)
{
	local_irq_disable();
	if (semaphore_counter == 0)
	{
		block();
	}
	else
	{
		semaphore_counter--;
	}
	local_irq_enable();
	return 0;
}

/**
 * Remove this function when task is complete
 */
asmlinkage long sys_sem_debug(void)
{
	printk("semaphore_counter: %d\n", semaphore_counter);
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Erick Ribeiro Otsuka <erick.r.otsuka@gmail.com>");
MODULE_DESCRIPTION("Semaphore init/up/down module");
