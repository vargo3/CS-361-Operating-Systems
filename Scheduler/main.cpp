// test driver for sched

#include <stdio.h>
#include <stdlib.h>
#include "sched.h"
#define REG_SIZE 2

PROCESS_CODE(start){
	r->state = PS_RUNNING;
	r->cpu_time_taken = 1;
	r->sleep_time = 0;
	return;
}

PROCESS_CODE(step1){
	r->state = PS_SLEEPING;
	r->cpu_time_taken = 3;
	r->sleep_time = 5;
	return;
}

PROCESS_CODE(step2){
	r->state = PS_RUNNING;
	r->cpu_time_taken = 7;
	r->sleep_time = 0;
	return;
}

PROCESS_CODE(step3){
	r->state = PS_EXITED;
	r->cpu_time_taken = 15;
	r->sleep_time = 0;
	return;
}

int main(int argc, char ** argv){
	SCHEDULER *sched;
	PROCESS * cur;
	PID pid;
	int i;
	PROCESS_CODE_PTR(init_ptr);

	init_ptr = &start;
	sched = new_scheduler(init_ptr);
	sched->scheduler_algorithm = SA_FCFS;
	pid = 1;
	pid = exec(sched, pid, "new_process", init_ptr, &step1, 1);// this should fail. cannot overwrite pid 1.
	pid = fork(sched, pid);
	pid = exec(sched, pid, "Alpha", init_ptr, &step1, 20);
	pid = fork(sched, pid);
	pid = exec(sched, pid, "Bravo", init_ptr, &step2, 50);
	pid = fork(sched, pid);
	pid = exec(sched, pid, "Charlie", init_ptr, &step3, 90);
	pid = fork(sched, pid);
	pid = exec(sched, pid, "Delta", init_ptr, &step2, 150);

	cur = &sched->process_list[sched->current];
	printf("%20.20s pid: %d state: %d total_cpu_time: %d job_time: %d\n", cur->name, cur->pid, cur->state, cur->total_cpu_time, cur->job_time);
	for (i = 0; i < 40; i++){
		if (i % 10 == 0){
			pid = fork(sched, 1);
			pid = exec(sched, pid, "Charlie's Ghost", init_ptr, &step3, 90);
		}
		timer_interrupt(sched);
		cur = &sched->process_list[sched->current];
		printf("%20.20s pid: %d state: %d total_cpu_time: %d job_time: %d\n", cur->name, cur->pid, cur->state, cur->total_cpu_time, cur->job_time);
	}
}
