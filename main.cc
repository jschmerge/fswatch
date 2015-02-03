#include <sys/inotify.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include <map>
#include <string>
#include <utility>

#include "util.h"

class descriptor
{
 public:
	descriptor(int _fd = -1) : fd(_fd) { }

	descriptor(const descriptor & d) = delete;

	descriptor & operator = (const descriptor & d) = delete;

	descriptor(descriptor && other) : fd(other.fd) { other.fd = -1; }

	descriptor & operator = (descriptor && other)
		{ std::swap(fd, other.fd); return *this; }

	descriptor & operator = (int _fd) { fd = _fd; return *this; }

	~descriptor() { if (fd >= 0) close(fd); }

	operator int() { return fd; }

 protected:
	int fd;
};


std::string verb_string(int mask)
{
	std::string s;

	if (mask & IN_ISDIR)
		s += "directory ";
	else
		s += "file ";

	if (mask & IN_ACCESS)
		s += "accessed ";
	if (mask & IN_ATTRIB)
		s += "metadata accessed ";
	if (mask & IN_CLOSE_WRITE)
		s += "(writable) closed ";
	if (mask & IN_CLOSE_NOWRITE)
		s += "(non-writable) closed ";
	if (mask & IN_CREATE)
		s += "created ";
	if (mask & IN_DELETE)
		s += "deleted ";
	if (mask & IN_DELETE_SELF)
		s += "(watched) deleted ";
	if (mask & IN_MODIFY)
		s += "modified ";
	if (mask & IN_MOVE_SELF)
		s += "(watched) moved ";
	if (mask & IN_MOVED_FROM)
		s += "moved from ";
	if (mask & IN_MOVED_TO)
		s += "moved to ";
	if (mask & IN_OPEN)
		s += "opened ";
	if (mask & IN_IGNORED)
		s += "Watch on file terminated ";
	if (mask & IN_Q_OVERFLOW)
		s += "**** event queue overflow ****";
	if (mask & IN_UNMOUNT)
		s += "owning filesystem unmounted ";

	return s;
}

/*
 * inotify event mask values:
 * IN_ACCESS
 * IN_ATTRIB
 * IN_CLOSE_WRITE
 * IN_CLOSE_NOWRITE
 * IN_CREATE
 * IN_DELETE
 * IN_DELETE_SELF
 * IN_MODIFY
 * IN_MOVE_SELF
 * IN_MOVED_FROM
 * IN_MOVED_TO
 * IN_OPEN
 *
 * IN_ALL_EVENTS (or of the above values)
 */

std::map<int, std::string> watches;
const size_t ev_buf_size = 32 * (sizeof(struct inotify_event) + NAME_MAX + 1);

//////////////////////////////////////////////////////////////////////
size_t process_event_buffer(const char * buffer, size_t len)
{
	const char * buf_ptr = buffer;
	size_t num_processed = 0;

	while (buf_ptr < (buffer + len))
	{
		const inotify_event * ev =
		  reinterpret_cast<const inotify_event*>(buf_ptr);

		printf("Got event for watch on directory %s:\n",
		       watches[ev->wd].c_str());
		printf("\t%s\n", verb_string(ev->mask).c_str());
		printf("\tpath in event is %s\n", ev->name);

		buf_ptr += (sizeof(struct inotify_event) + ev->len);

		++num_processed;
	}

	return num_processed;
}

//////////////////////////////////////////////////////////////////////
int main(int argc, char ** argv)
{
	descriptor watchfd = inotify_init1(IN_CLOEXEC);

	for (int i = 1; i < argc; ++i)
	{
		int id = inotify_add_watch(watchfd, argv[i], IN_ALL_EVENTS);
		if (id < 0)
			throw make_syserr("inotify_add_watch failed");
		watches[id] = argv[i];
	}

	char event_buffer[ev_buf_size];
	memset(event_buffer, 0, ev_buf_size);

	ssize_t n = 0;
	while ((n = read(watchfd, event_buffer, ev_buf_size)) > 0)
	{
		printf("Read %zd bytes\n", n);
		process_event_buffer(event_buffer, n);
	}

	return 0;
}
