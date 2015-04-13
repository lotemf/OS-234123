#include <iostream>
#include <string>
#include <map>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "syscall_maxproc.h"

using namespace std;

enum Command {FORK,KILL,COUNT,SET,GET,WAIT};

struct glob {
	struct {
		Command command;
		pid_t pid, child;
		bool ready;
		int limit;
	} in; //input
	struct {
		int result;
		bool ready; //means 'done' here
		int code;
	} out; //output
	bool exit; // test has completed, we should exit.
} *shared;

const struct timespec req={0,5000000}; //5ms

void do_commands() {
	pid_t pid=getpid();
	pid_t child;
	while(true) {
		while (!shared->in.ready) nanosleep(&req, NULL); //so this function won't take up all the CPU
		if (shared->exit) exit(0); //when tests have completed
		if (shared->in.pid!=pid) 
			continue; 
		shared->in.ready=false;
		switch (shared->in.command) {
		case KILL:
			shared->out.ready=true;
			exit(0);
			break;
		case WAIT:
			waitpid(shared->in.child,NULL,0);
			shared->out.result=0;
			break;
		case FORK:
			child=fork();
			if (child==0) { //child process
				//cout << "*** " << pid;
				pid=getpid(); //update pid
				//cout << "->" << pid << " ***" << endl;
				continue;
			}
			shared->out.result=child; //pid of child
			break;
		case COUNT:
			shared->out.result=get_subproc_count();
			break;
		case SET:
			shared->out.result=set_child_max_proc(shared->in.limit);
			break;
		case GET:
			shared->out.result=get_max_proc();
			break;
		}
		if (shared->out.result<0) shared->out.code=errno;
		shared->out.ready=true;
	}
}

void handle_input(pid_t first_child) {
	/* Possible commands:
	   kill <process>
	   fork <father> <son>
	   count <process>
	   set <process> <lim>
	   get <process>
	   zombie <process>
	   waitz <process>
	*/
	map<string,pid_t> m;
	map<string,string> parent;
	map<string,pid_t> zombies;
	m["main"]=first_child;
	string command,arg1,arg2;
	while (cin>>command) {
		shared->out.code=0;
		shared->out.ready=false;
		if (command=="kill") {
			cin>>arg1;
			if (m.find(arg1)==m.end()) {
				cout<<"unknown process: "<<arg1<<endl;
				continue;
			}
			shared->in.command=KILL;
			shared->in.pid=m[arg1];
			shared->in.ready=true;
			while (!shared->out.ready) 
				nanosleep(&req, NULL);
			if (arg1=="main") {
				waitpid(m[arg1],NULL,0);
			} else if (m.find(parent[arg1])!=m.end()) {
				shared->out.ready=false;
				shared->in.pid=m[parent[arg1]];
				shared->in.child=m[arg1];
				shared->in.command=WAIT;
				shared->in.ready=true;
				while (!shared->out.ready) 
					nanosleep(&req, NULL);
			}
			m.erase(arg1);
			if (arg1!="main") 
				parent.erase(arg1);
		} else if (command=="zombie") {
			cin>>arg1;
			if (m.find(arg1)==m.end()) {
				cout<<"unknown process: "<<arg1<<endl;
				continue;
			}
			shared->in.command=KILL;
			shared->in.pid=m[arg1];
			shared->in.ready=true;
			while (!shared->out.ready) 
				nanosleep(&req, NULL);
			zombies[arg1] = m[arg1];
			m.erase(arg1);
		} else if (command=="waitz") {
			cin>>arg1;
			if (zombies.find(arg1)==zombies.end()) {
				cout<<"unknown zombie process: "<<arg1<<endl;
				continue;
			}
			if (arg1=="main") {
				waitpid(zombies[arg1],NULL,0);
			} else if (m.find(parent[arg1])!=m.end()) {
				shared->out.ready=false;
				shared->in.pid=m[parent[arg1]];
				shared->in.child=zombies[arg1];
				shared->in.command=WAIT;
				shared->in.ready=true;
				while (!shared->out.ready) 
					nanosleep(&req, NULL);
			} else {
				cout<<"process already dead: "<<arg1<<endl;
			}
			zombies.erase(arg1);
			if (arg1!="main") 
				parent.erase(arg1);
		} else if (command=="fork") {
			cin>>arg1>>arg2;
			if (m.find(arg1)==m.end()) {
				cout<<"unknown process: "<<arg1<<endl;
				continue;
			}
			if (m.find(arg2)!=m.end()) {
				cout<<"process "<<arg2<<" already exists"<<endl;
				continue;
			}
			shared->in.command=FORK;
			shared->in.pid=m[arg1];
			shared->in.ready=true;
			while (!shared->out.ready) 
				nanosleep(&req, NULL);
			if (shared->out.code==0) {
				m[arg2]=shared->out.result;
				parent[arg2]=arg1;
			}
		} else if (command=="count") {
			cin>>arg1;
			if (m.find(arg1)==m.end()) {
				cout<<"unknown process: "<<arg1<<endl;
				continue;
			}
			shared->in.command=COUNT;
			shared->in.pid=m[arg1];
			shared->in.ready=true;
			while (!shared->out.ready) nanosleep(&req, NULL);
			cout<<"process "<<arg1<<" has "<<shared->out.result<<" subprocesses"<<endl;
		} else if (command=="set") {
			int lim;
			cin>>arg1>>lim;
			if (m.find(arg1)==m.end()) {
				cout<<"unknown process: "<<arg1<<endl;
				continue;
			}
			shared->in.command=SET;
			shared->in.pid=m[arg1];
			shared->in.limit=lim;
			shared->in.ready=true;
			while (!shared->out.ready) nanosleep(&req, NULL);
		} else if (command=="get") {
			cin>>arg1;
			if (m.find(arg1)==m.end()) {
				cout<<"unknown process: "<<arg1<<endl;
				continue;
			}
			shared->in.command=GET;
			shared->in.pid=m[arg1];
			shared->in.ready=true;
			while (!shared->out.ready) nanosleep(&req, NULL);
			if (shared->out.result>=0) {
				cout<<"process "<<arg1<<" has max_proc value of ";
				cout<<shared->out.result<<endl;
			}
		} else {
			cout<<"command "<<command<<" not found"<<endl;
			continue;
		}
		cout<<"errno: "<<strerror(shared->out.code)<<endl;
	}
	shared->exit=true;
	shared->in.ready=true;
}

int main() {
	shared=(glob*)mmap(NULL, sizeof *shared, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	shared->in.ready=false;
	shared->out.ready=false;
	shared->exit=false;
	pid_t child=fork();
	if (child==0) {
		do_commands();
	} else {
		handle_input(child);		
	}
              
}
