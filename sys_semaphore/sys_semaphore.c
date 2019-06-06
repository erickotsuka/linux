#include <asm/barrier.h>
#include <asm/current.h>
#include <linux/linkage.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

#define SEMAPHORE_MAX_INSTANCES (30)

typedef struct __semaphore
{
	int id;
	int counter;
	wait_queue_head_t queue;
} semaphore_t;

static int next_semaphore_id = 0;
static semaphore_t semaphore_list[SEMAPHORE_MAX_INSTANCES];

static int create_semaphore(int initial_counter)
{
	if (next_semaphore_id >= SEMAPHORE_MAX_INSTANCES)
	{
		return -1;
	}

	semaphore_list[next_semaphore_id].id = next_semaphore_id;
	semaphore_list[next_semaphore_id].counter = initial_counter;
	init_waitqueue_head(&(semaphore_list[next_semaphore_id].queue));
	return next_semaphore_id++;
}

static long block(int sem_id)
{
	wait_event(semaphore_list[sem_id].queue, (semaphore_list[sem_id].counter > 0));
	return 0;
}

static long unblock(int sem_id)
{
	wake_up(&(semaphore_list[sem_id].queue));
	return 0;
}

asmlinkage long sys_init_semaphore(int initial_counter)
{
	return create_semaphore(initial_counter);
}

asmlinkage long sys_up(int sem_id)
{
	local_irq_disable();
	semaphore_list[sem_id].counter++;
	if (semaphore_list[sem_id].counter == 1)
	{
		unblock(sem_id);
	}
	local_irq_enable();
	return 0;
}

asmlinkage long sys_down(int sem_id)
{
	local_irq_disable();
	if (semaphore_list[sem_id].counter == 0)
	{
		block(sem_id);
	}
	semaphore_list[sem_id].counter--;
	local_irq_enable();
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Erick Ribeiro Otsuka <erick.r.otsuka@gmail.com>");
MODULE_DESCRIPTION("Semaphore init/up/down module");
