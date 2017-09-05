#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>  //sigaction
#include <pthread.h>
#include <unistd.h>  //sleep
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "log.h"
#include "server.h"
#include "invoke_stub.h"
#include "util.h"


static void sig_handle (int signo)
{
	INFO ("======received signal : %d", signo);
	if (SIGTERM == signo) {
		exit(0);
	}
	if (SIGUSR1 == signo) {
		INFO ("======received signal : SIGUSR1");
		show_resource_status();
	}
	if (SIGUSR2 == signo) {
		INFO ("======received signal : SIGUSR2");
		refresh_declared_services();
	}
}

static void register_sig()
{
	//signal handle
	struct sigaction sa;
	sa.sa_handler = sig_handle;
	sa.sa_flags = 0;
	sa.sa_flags |= SA_RESTART;
	sigemptyset(&sa.sa_mask);
	if ( sigaction(SIGUSR1, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGUSR1 failed");
	}
	if ( sigaction(SIGUSR2, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGUSR2 failed");
	}
	if ( sigaction(SIGALRM, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGALRM failed");
	}
	if ( sigaction(SIGINT, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGINT failed");
	}
	if ( sigaction(SIGPIPE, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGPIPE failed");
	}
	if ( sigaction(SIGPOLL, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGPOLL failed");
	}
	if ( sigaction(SIGPROF, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGPROF failed");
	}
	if ( sigaction(SIGPWR, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGPWR failed");
	}
	if ( sigaction(SIGSTKFLT, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGSTKFLT failed");
	}
	if ( sigaction(SIGTERM, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGTERM failed");
	}
	if ( sigaction(SIGVTALRM, &sa, NULL) < 0 )
	{
		ERROR ("catch signal SIGVTALRM failed");
	}
}

static void * signal_handle_thread_func(void * arg) {
	INFO("======signal handle thread startup ...");
	sigset_t   waitset, oset;
	siginfo_t  info;
	int        rc;
	pthread_t  ppid = pthread_self();

	pthread_detach(ppid);

	sigemptyset(&waitset);
	sigaddset(&waitset, SIGUSR1);
	sigaddset(&waitset, SIGUSR2);
	sigaddset(&waitset, SIGPIPE);

	while (1)  {
		rc = sigwaitinfo(&waitset, &info);
		if (rc != -1) {
			INFO("======signal_handle_thread_func fetch the signal - %d", rc);
			sig_handle(info.si_signo);
		} else {
			ERROR("======signal_handle_thread_func fetch err: %d, %s", errno, strerror(errno));
		}
	}
	INFO("======signal handle thread shutdown");
}

static void print_help() {
	const char * help = "Usage: agent [OPTIONS]\n"
	                    "     -c    the path of core config file\n"
	                    "     -d    daemonize the program\n"
	                    "     -h    print help\n"
	                    "     \n"
	                    "version : 0.1, Written by Kevin , xuhuahai@dangdang.com\n"
	                    "     \n"
	                    "example : \n"
	                    "          ./agent -c ./conf/core.conf -d\n"
	                    ;
	printf("%s\n", help);
}

int main(int argc, char** argv) {

	if (argc == 1)
	{
		print_help();
		exit(0);
	}
	int bDemaeon = 0;
	char config_file_path[PATH_MAX] = {0};
	int opt;
	while ( (opt = getopt(argc, argv, "c:dh")) != -1 ) {
		switch (opt)
		{
		case 'c':
		{
			snprintf(config_file_path, PATH_MAX, "%s", optarg);
			break;
		}
		case 'd':
		{
			bDemaeon = 1;
			break;
		}
		case 'h':
		{
			print_help();
			exit(0);
		}
		}
	}

	if ( config_file_path[0] == 0 ) {
		print_help();
		exit(0);
	}

	//init config
	if (config_init(config_file_path)) {
		exit(0);
	}

	//daemonize
	char cwd_path[PATH_MAX] = {0};
	get_config_item("server:cwd", cwd_path, PATH_MAX, "/tmp");
	if (bDemaeon) {
		daemonize(cwd_path);
	}

	//init log
	get_config_item("log:config_file", config_file_path, PATH_MAX, "./conf/log.conf");
	if ( log_init(config_file_path) ) {
		exit(0);
	}

	//register_sig();

	// Block SIGRTMIN and SIGUSR1 which will be handled in dedicated thread signal_handle_thread_func()
	// Newly created threads will inherit the pthread mask from its creator
	sigset_t bset, oset;
	sigemptyset(&bset);
	sigaddset(&bset, SIGUSR1);
	sigaddset(&bset, SIGUSR2);
	sigaddset(&bset, SIGPIPE);
	//主线程处理信号
	if (pthread_sigmask(SIG_BLOCK, &bset, &oset) != 0) {
		ERROR("Set pthread mask failed");
		exit(0);
	}
	pthread_t tid1;
	if (pthread_create(&tid1, NULL, (void*)signal_handle_thread_func, NULL)) {
		ERROR("signal_handle_thread_func create error : %s", strerror(errno) );
		exit(0);
	}
	INFO("signal_handle_thread_func create successfully");


	//write pid
	char pid_file_path[PATH_MAX] = {0};
	get_config_item("server:pid_file", pid_file_path, PATH_MAX, "./logs/soa_proxy.pid");
	write_pid(pid_file_path);
	INFO("Written pid to %s", pid_file_path);

	//init invoke stub
	char services_declare_file_path[PATH_MAX] = {0};
	char zk_addrs[256] = {0};
	get_config_item("server:services_declare_file", services_declare_file_path, PATH_MAX, "./conf/declares.json");
	get_config_item("server:zk_addrs", zk_addrs, 256, "127.0.0.1:2181");
	if (invoke_stub_init(zk_addrs, services_declare_file_path)) {
		exit(0);
	}

	//startup server
	char unix_sock_path[PATH_MAX] = {0};
	get_config_item("server:unix_sock_path", unix_sock_path, PATH_MAX, "/tmp/sock_proxy.sock");
	make_sure_dir(unix_sock_path, 0755);
	if (server_startup_multibase(unix_sock_path)) {
		exit(0);
	}

	INFO("Shutdown !");
	return 0;
}