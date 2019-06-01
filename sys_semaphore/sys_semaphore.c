#include <asm/barrier.h>
#include <asm/current.h>
#include <linux/linkage.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

#define set_task_state(tsk, state_value)		\
	do { (tsk)->state = (state_value); smp_mb(); } while (0)

#define SEMAPHORE_MAX_INSTANCES (30)

typedef struct __semaphore
{
	int id;
	int counter;
	wait_queue_head_t *queue;
} semaphore_t;

static int next_semaphore_id = 0;

static semaphore_t semaphore_list[SEMAPHORE_MAX_INSTANCES];

static int create_semaphore(int initial_counter)
{
	DECLARE_WAIT_QUEUE_HEAD(queue);
	semaphore_t new_semaphore;
	new_semaphore.id = next_semaphore_id;
	new_semaphore.counter = initial_counter;
	new_semaphore.queue = &queue;
	semaphore_list[next_semaphore_id] = new_semaphore;
	return next_semaphore_id++;
}

static int autoremove_autofree_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
	int ret = default_wake_function(wait, mode, sync, key);

	if (ret) {
		list_del_init(&wait->task_list);
		kfree(wait);
	}
	return ret;
}

static long stop_process(struct task_struct *p, wait_queue_head_t *queue)
{
	unsigned long flags;
	wait_queue_t *wait = kmalloc(sizeof(*wait), GFP_KERNEL);
	if (!wait)
		return -1;
	init_wait(wait);
	wait->private = p;
	wait->func = autoremove_autofree_wake_function;
	wait->flags |= WQ_FLAG_EXCLUSIVE;

	spin_lock_irqsave(&(queue->lock), flags);
	__add_wait_queue_tail(queue, wait);
	set_task_state(p, TASK_STOPPED);
	spin_unlock_irqrestore(&(queue->lock), flags);

	return 0;
}

static long block(int pid, wait_queue_head_t *queue)
{
	struct task_struct *p = find_task_by_vpid(pid);
	return p ? stop_process(p, queue) : -1;
}

static long unblock(wait_queue_head_t *queue)
{
	__wake_up(queue, TASK_STOPPED, 1, NULL);
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
		unblock(semaphore_list[sem_id].queue);
	}
	local_irq_enable();
	return 0;
}

asmlinkage long sys_down(int sem_id)
{
	local_irq_disable();
	if (semaphore_list[sem_id].counter == 0)
	{
		block(current->pid, semaphore_list[sem_id].queue);
	}
	semaphore_list[sem_id].counter--;
	local_irq_enable();
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Erick Ribeiro Otsuka <erick.r.otsuka@gmail.com>");
MODULE_DESCRIPTION("Semaphore init/up/down module");
