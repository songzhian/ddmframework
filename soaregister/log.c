#include "log.h"
#include "util.h"

/**
 * log handle
 */
log_handle g_log_h;


int get_locktype(const char *lock_type_name) {
	if (strcmp("thread", lock_type_name) == 0) {
		return LOCK_BETWEEN_THREAD;
	}
	if (strcmp("process", lock_type_name) == 0) {
		return LOCK_BETWEEN_PROCESS;
	}
	return LOCK_NONE;
}


int get_level(const char *level_name) {
	if (strcmp("debug", level_name) == 0) {
		return LEVEL_DEBUG;
	}
	if (strcmp("info", level_name) == 0) {
		return LEVEL_INFO;
	}
	if (strcmp("notice", level_name) == 0) {
		return LEVEL_NOTICE;
	}
	if (strcmp("warn", level_name) == 0) {
		return LEVEL_WARN;
	}
	if (strcmp("error", level_name) == 0) {
		return LEVEL_ERROR;
	}
	if (strcmp("fatal", level_name) == 0) {
		return LEVEL_FATAL;
	}
	if (strcmp("none", level_name) == 0) {
		return LEVEL_NONE;
	}
	return LEVEL_NONE;
}

inline static const char* level_name(int level) {
	switch (level) {
	case LEVEL_FATAL:
		return "[FATAL] ";
	case LEVEL_ERROR:
		return "[ERROR] ";
	case LEVEL_WARN:
		return "[WARN ] ";
	case LEVEL_NOTICE:
		return "[NOTICE]";
	case LEVEL_INFO:
		return "[INFO ] ";
	case LEVEL_DEBUG:
		return "[DEBUG] ";
	}
	return "";
}

static void occur_error() {
	g_log_h.status = 0;
}

static void require_lock() {
	if (g_log_h.threadsafe) {
		pthread_mutex_lock(&g_log_h.lock);
	}
}

static void release_lock() {
	if (g_log_h.threadsafe) {
		pthread_mutex_unlock(&g_log_h.lock);
	}
}

static int prepare_log() {
	time_t time;
	struct timeval tv;
	struct tm *tm;
	gettimeofday(&tv, NULL);
	time = tv.tv_sec;
	tm = localtime(&time);
	char file_path[PATH_MAX];
	snprintf(file_path, PATH_MAX, "%s.%04d%02d%02d",
	         g_log_h.filenametpl,
	         tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	require_lock();
	if (strcmp(file_path, g_log_h.filename) != 0) {
		//first or pass the night
		if (g_log_h.fp != NULL) {
			fclose(g_log_h.fp);
		}
		g_log_h.fp = fopen(file_path, "a");
		if (g_log_h.fp == NULL) {
			fprintf(stderr, "%u open log file error (%s:%d): %s : %s\n", getpid(), __FILE__, __LINE__, strerror(errno), file_path);
			occur_error();
			release_lock();
			return -1;
		}
		//set line buffer
		setvbuf(g_log_h.fp, g_log_h.buffer, _IOLBF, LOG_BUF_LEN);
		snprintf(g_log_h.filename, PATH_MAX, "%s", file_path);
		g_log_h.inode_value = get_inode(file_path);
	} else {
		int file_deleted = 0, file_changed = 0;
		long cur_inode_value = get_inode(file_path);
		if (cur_inode_value == -1) {
			//if the file is deleted
			file_deleted = 1;
		} else {
			if (g_log_h.inode_value != cur_inode_value) {
				//if the file is changed
				file_changed = 1;
			}
		}
		if (file_deleted || file_changed) {
			//switch the file
			if (g_log_h.fp != NULL) {
				fclose(g_log_h.fp);
			}
			g_log_h.fp = fopen(file_path, "a");
			if (g_log_h.fp == NULL) {
				fprintf(stderr, "%u open log file error (%s:%d): %s : %s\n", getpid(), __FILE__, __LINE__, strerror(errno), file_path);
				occur_error();
				release_lock();
				return -1;
			}
			//set line buffer
			setvbuf(g_log_h.fp, g_log_h.buffer, _IOLBF, LOG_BUF_LEN);
			snprintf(g_log_h.filename, PATH_MAX, "%s", file_path);
			g_log_h.inode_value = get_inode(file_path);
		}
	}
	release_lock();
	return 0;
}

int open_log(const char *filename, const char * levelname, const char * lock_type, uint64_t rotate_size) {
	g_log_h.status = 1;
	g_log_h.fp = NULL;
	g_log_h.level = get_level(levelname);
	g_log_h.filenametpl[0] = '\0';
	g_log_h.filename[0] = '\0';
	g_log_h.inode_value = -1;
	g_log_h.rotate_size = 0;
	g_log_h.w_curr = 0;
	g_log_h.w_total = 0;
	//
	if (strlen(filename) > PATH_MAX - 25) {
		fprintf(stderr, "log filename too long!\n");
		occur_error();
		return -1;
	}
	if (make_sure_dir(filename, 0755)) {
		occur_error();
		return -1;
	}
	int locktype = get_locktype(lock_type);
	g_log_h.threadsafe = locktype;
	g_log_h.rotate_size = rotate_size;
	snprintf(g_log_h.filenametpl, PATH_MAX, "%s", filename );
	if (g_log_h.threadsafe) {
		switch (g_log_h.threadsafe) {
		case LOCK_BETWEEN_PROCESS:
		{
			pthread_mutexattr_t mutexattr;
			pthread_mutexattr_init(&mutexattr);
			pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
			pthread_mutex_init(&g_log_h.lock, &mutexattr);
			pthread_mutexattr_destroy(&mutexattr);
			break;
		}
		case LOCK_BETWEEN_THREAD:
		{
			// pthread_mutex_init(&g_log_h.lock, NULL);
			pthread_mutexattr_t mutexattr;
			pthread_mutexattr_init(&mutexattr);
			pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_PRIVATE);
			pthread_mutex_init(&g_log_h.lock, &mutexattr);
			pthread_mutexattr_destroy(&mutexattr);
			break;
		}
		default:
			g_log_h.threadsafe = 0;
			break;
		}
	}
	return 0;
}


int close_log() {
	if (g_log_h.status) {
		if (!g_log_h.fp) {
			return 0;
		}
		if (g_log_h.fp != stdin && g_log_h.fp != stdout) {
			fclose(g_log_h.fp);
			g_log_h.fp = NULL;
		}
		if (g_log_h.threadsafe) {
			pthread_mutex_destroy(&g_log_h.lock);
		}
	}
	g_log_h.status = 0;
	return 0;
}


void set_log_level(const char *levelname) {
	g_log_h.level = get_level(levelname);
}


int logv(int level, const char *fmt, va_list ap) {
	char buf[LOG_BUF_LEN];
	int len;
	char *ptr = buf;
	time_t time;
	struct timeval tv;
	struct tm *tm;
	gettimeofday(&tv, NULL);
	time = tv.tv_sec;
	tm = localtime(&time);
	len = sprintf(ptr, "%04d-%02d-%02d %02d:%02d:%02d.%03d %u:%x ",
	              tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	              tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
	              getpid(), pthread_self());
	if (len < 0) {
		return -1;
	}
	ptr += len;
	memcpy(ptr, level_name(level), LEVEL_NAME_LEN);
	ptr += LEVEL_NAME_LEN;
	int space = sizeof(buf) - (ptr - buf) - 2;
	len = vsnprintf(ptr, space, fmt, ap);
	if (len < 0) {
		return -1;
	}
	ptr += len > space ? space : len;
	*ptr++ = '\n';
	*ptr = '\0';
	len = ptr - buf;
	if (prepare_log()) {
		return -1;
	}
	// require_lock();
	fwrite(buf, len, 1, g_log_h.fp);
	// fflush(g_log_h.fp);
	// g_log_h.w_curr += len;
	// g_log_h.w_total += len;
	// release_lock();
	return len;
}


int log_write(int level, const char *fmt, ...) {
	if (!g_log_h.status || g_log_h.level == LEVEL_NONE || level < g_log_h.level) {
		return 0;
	}
	va_list ap;
	va_start(ap, fmt);
	int ret = logv(level, fmt, ap);
	va_end(ap);
	return ret;
}

