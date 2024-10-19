#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

#define DEVICE_NAME "/dev/nvbyby"

class MemKDriver {
private:
	int fd;
	pid_t pid;

	struct CopyMemory
	{
		pid_t pid;
		uintptr_t addr;
		void *buffer;
		size_t size;
	};

	struct ModuleBase
	{
		pid_t pid;
		char *name;
		uintptr_t base;
	};

	enum Operations {
		OP_READ_MEM = 0x801,
		OP_WRITE_MEM = 0x802,
		OP_MODULE_BASE = 0x803,
	};

public:
	MemKDriver(const char* deviceName) : fd(open(deviceName, O_RDWR)) {
		if (fd == -1) {
			perror("[-] Failed to open driver");
		}
	}

	~MemKDriver() {
		if (fd > 0) {
			close(fd);
		}
	}

	inline void initialize(const pid_t new_pid) {
		pid = new_pid;
	}

	ssize_t read(const uintptr_t addr, void *buffer, const size_t size) const {
		if (!buffer || fd == -1) return -1;

		CopyMemory cm = { pid, addr, buffer, size };
		return ioctl(fd, OP_READ_MEM, &cm);
	}

	ssize_t write(const uintptr_t addr, const void *buffer, const size_t size) const {
		if (!buffer || fd == -1) return -1;

		CopyMemory cm = { pid, addr, const_cast<void *>(buffer), size };
		return ioctl(fd, OP_WRITE_MEM, &cm);
	}

	template <typename T>
	inline T read(const uintptr_t addr) const {
		T result{};
		read(addr, &result, sizeof(T));
		return result;
	}

	template <typename T>
	inline bool write(const uintptr_t addr, const T &value) const {
		return write(addr, &value, sizeof(T)) == sizeof(T);
	}

	inline uintptr_t get_module_base(const char *name) const {
		if (!name || fd == -1) return 0;

		ModuleBase mb = { pid, nullptr, 0 };
		mb.name = const_cast<char *>(name);

		return (ioctl(fd, OP_MODULE_BASE, &mb) == 0) ? mb.base : 0;
	}
};

static MemKDriver *driver = new MemKDriver(DEVICE_NAME);
