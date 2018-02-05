//Jacob Vargo, Guarab KC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sched.h"

void timer_interrupt(SCHEDULER *s){
	//simulate a timer interrupt from hardware. This should initiate the context switch procedure
	// - Context switch must save active process' state into the PROCESS structure
	// - Context switch must load the next process' state into the scheduler
	unsigned int a;
	int i;
	PID pid;
	double b, min;
	PROCESS *proc;
	RETURN ret;

	/* save active registers to current PROCESS */
	s->process_list[s->current].saved_registers = s->active_registers;


	/* deschedule current process if needed */
	proc = &s->process_list[s->current];
	proc->total_cpu_time++;
	if (proc != &s->process_list[0] && (proc->state == PS_EXITED || proc->total_cpu_time >= (int) proc->job_time)) proc->state = PS_NONE;

	/* find new current process and switch to it */
	switch (s->scheduler_algorithm){
		case SA_ROUND_ROBIN:
			pid = s->current;
			for (i = s->current+1; i != pid; i = (i+1) % MAX_PROCESSES){
				if (s->process_list[i].state == PS_RUNNING){
					s->current = i;
					break;
				}
			}
			break;

		case SA_FAIR:
			// switched/total *100
			proc = &s->process_list[s->current];
			if (proc->switched_cpu_time == 0) //no division by 0
				min = (1.0*proc->total_cpu_time/(proc->switched_cpu_time + 1)) * 100;
			else
				min = (1.0*proc->total_cpu_time/proc->switched_cpu_time) * 100;
			for (i = 0; i < MAX_PROCESSES; i++){
				proc = &s->process_list[i];
				if (proc->state == PS_NONE) continue;
				else if (proc->switched_cpu_time == 0){
					b =(1.0*proc->total_cpu_time/proc->switched_cpu_time + 1) * 100;
				}else{
					b =(1.0*proc->total_cpu_time/proc->switched_cpu_time) * 100;
				}//give fairness to less cpu time use %
				if (b < min){
					s->current = i;
					min = b;
				}
			}
			break;

		case SA_FCFS:
			proc = &s->process_list[s->current];
			if (proc->state == PS_RUNNING && proc != &s->process_list[0]) break;
			pid = s->current;
			a = s->process_list[pid].switched_cpu_time;
			for (i = s->current+1; i != pid; i = (i+1) % MAX_PROCESSES){
				if (s->process_list[i].switched_cpu_time > a){
					s->current = i;
					a = s->process_list[i].switched_cpu_time;
				}
			}
			break;

		case SA_SJF:
			proc = &s->process_list[s->current];
			if (proc->state == PS_RUNNING && proc != &s->process_list[0]) break;
			pid = 0;
			for (i = 1; i < MAX_PROCESSES; i++){
				proc = &s->process_list[i];
				if (proc->state == PS_RUNNING && (s->process_list[pid].job_time == -1 || (s->process_list[pid].job_time > proc->job_time && proc->job_time != -1))){
					pid = i;
				}
			}
			if (pid == 0){
				for (i = 1; i < MAX_PROCESSES; i++){
					proc = &s->process_list[i];
					if (proc->state == PS_RUNNING){
						pid = i;
					}
				}
			}
			s->current = pid;
			break;
	}
	proc = &s->process_list[s->current];
	proc->switched++;
	proc->switched_cpu_time = 0;

	/* load PROCESS saved registers into active register */
	s->active_registers = proc->saved_registers;

	//run process
	if (proc->switched == 1) proc->init(&s->active_registers, &ret);
	else{
		proc->step(&s->active_registers, &ret);
	}
	proc->state = ret.state;
	proc->total_cpu_time += ret.cpu_time_taken;
	if (proc->state == PS_SLEEPING) proc->sleep_time_remaining = ret.sleep_time;

	/* decrement sleep timers, increment switched_cpu_time */
	for (i = 0; i < MAX_PROCESSES; i++){
		proc = &s->process_list[i];
		if (proc->state == PS_SLEEPING){
			proc->sleep_time_remaining -= ret.cpu_time_taken;
			if (proc->sleep_time_remaining == 0) proc->state = PS_RUNNING;
		}
		if (proc->state != PS_NONE)
			proc->switched_cpu_time += ret.cpu_time_taken;
	}
}

SCHEDULER *new_scheduler(PROCESS_CODE_PTR(code)){
	//Create a new scheduler
	//This funciton needs to:
	// - Create a new scheduler (on heap)
	// - Schedule process 1 (init)
	// - Set process 1 to current process
	// - Return the created scheduler

	SCHEDULER *sched;
	PROCESS *init;
	sched = (SCHEDULER *) malloc(sizeof(SCHEDULER));
	init = &sched->process_list[0];
	init->name = strdup("init");
	init->pid = 1;
	init->switched = 0;
	init->total_cpu_time = 0;
	init->switched_cpu_time = 0;
	init->sleep_time_remaining = 0;
	init->job_time = -1;
	init->state = PS_RUNNING;
	init->init = code;
	init->step = code;
	sched->current = init->pid-1;
	sched->scheduler_algorithm = SA_ROUND_ROBIN; //what is the initial algorithm supposed to be?
	return sched;
}

int fork(SCHEDULER *s, PID src_p){
	//Duplicate a process (copied, NOT SHARED)
	//The passed scheduler forks a process that is a duplicate of src_p
	src_p--;
	if (s == NULL || src_p < 0 || MAX_PROCESSES < src_p || s->process_list[src_p].state == PS_NONE){
		fprintf(stderr, "Cannot fork process: %d\n", src_p+1);
		return src_p+1;
	}
	PID i = 0;
	while((s->process_list[i].state != PS_NONE || i == src_p) && i < MAX_PROCESSES){
		i++;
	}
	//printf("fork pid: %d to: %d\n", src_p+1, i+1);
	s->process_list[i].name = strdup(s->process_list[src_p].name);
	s->process_list[i].pid = i+1;
	s->process_list[i].switched = 0;
	s->process_list[i].total_cpu_time = 0;
	s->process_list[i].switched_cpu_time = 0;
	s->process_list[i].sleep_time_remaining = 0;
	s->process_list[i].job_time = s->process_list[src_p].job_time;
	s->process_list[i].state = s->process_list[src_p].state;
	s->process_list[i].init = s->process_list[src_p].init;
	s->process_list[i].step = s->process_list[src_p].step;
	
	return i+1;
}

int exec(SCHEDULER *s, PID pid, const char *new_name, PROCESS_CODE_PTR(init), PROCESS_CODE_PTR(step), int job_time){
	//Overwrited the current process with the new information
	//This exec is called on any PID that IS NOT 1!
	//exec overwrites the given process with the new name, init, and step
	
	pid--;
	if (pid == 0 || s == NULL || pid < 0 || MAX_PROCESSES <= pid || s->process_list[pid].state == PS_NONE || new_name == NULL){
		fprintf(stderr, "Cannot exec on pid %d\n", pid+1);
		return pid+1;
	}
	//printf("exec pid: %d\n", pid+1);
	free(s->process_list[pid].name);
	s->process_list[pid].name = (char*) malloc(sizeof(char) * (strlen(new_name)+1));
	s->process_list[pid].name = strdup(new_name);
	s->process_list[pid].pid = pid+1;
	s->process_list[pid].switched = 0;
	s->process_list[pid].total_cpu_time = 0;
	s->process_list[pid].switched_cpu_time = 0;
	s->process_list[pid].sleep_time_remaining = 0;
	s->process_list[pid].job_time = job_time;
	s->process_list[pid].state = PS_RUNNING;
	s->process_list[pid].init = init;
	s->process_list[pid].step = step;
	return pid+1;
}
