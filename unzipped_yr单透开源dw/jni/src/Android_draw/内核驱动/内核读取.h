#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

class c_driver {
	private:
	int has_upper = 0;
	int has_lower = 0;
	int has_symbol = 0;
	int has_digit = 0;
	int fd;
	pid_t pid;

	typedef struct _COPY_MEMORY {
		pid_t pid;
		uintptr_t addr;
		void* buffer;
		size_t size;
	} COPY_MEMORY, *PCOPY_MEMORY;

	typedef struct _MODULE_BASE {
		pid_t pid;
		char* name;
		uintptr_t base;
	} MODULE_BASE, *PMODULE_BASE;

    enum OPERATIONS {
        OP_READ_MEM = 601,
        OP_WRITE_MEM = 602,
        OP_MODULE_BASE = 603
    };
	
	int symbol_file(const char *filename) {
		//判断文件名是否含小写并且不含大写不含数字不含符号
		int length = strlen(filename);
		for (int i = 0; i < length; i++) {
			if (islower(filename[i])) {
				has_lower = 1;
			} else if (isupper(filename[i])) {
				has_upper = 1;
			} else if (ispunct(filename[i])) {
				has_symbol = 1;
			} else if (isdigit(filename[i])) {
				has_digit = 1;
			}
		}
		return has_lower && !has_upper && !has_symbol && !has_digit;
	}
	
	public:
    c_driver() {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd == -1) {
            perror("[-] 打开失败");
            exit(EXIT_FAILURE);
        }
    }

	~c_driver() {
		//wont be called
		if (fd > 0)
			close(fd);
	}

	void initialize(pid_t pid) {
		this->pid = pid;
	}

	bool read(uintptr_t addr, void *buffer, size_t size) {
		COPY_MEMORY cm;

		cm.pid = this->pid;
		cm.addr = addr;
		cm.buffer = buffer;
		cm.size = size;

		if (ioctl(fd, OP_READ_MEM, &cm) != 0) {
			return false;
		}
		return true;
	}

	bool write(uintptr_t addr, void *buffer, size_t size) {
		COPY_MEMORY cm;

		cm.pid = this->pid;
		cm.addr = addr;
		cm.buffer = buffer;
		cm.size = size;

		if (ioctl(fd, OP_WRITE_MEM, &cm) != 0) {
			return false;
		}
		return true;
	}

	template <typename T>
	T read(uintptr_t addr) {
		T res;
		if (this->read(addr, &res, sizeof(T)))
			return res;
		return {};
	}

	template <typename T>
	bool write(uintptr_t addr,T value) {
		return this->write(addr, &value, sizeof(T));
	}

	uintptr_t get_module_base(char* name) {
		MODULE_BASE mb;
		char buf[0x100];
		strcpy(buf,name);
		mb.pid = this->pid;
		mb.name = buf;

		if (ioctl(fd, OP_MODULE_BASE, &mb) != 0) {
			return 0;
		}
		return mb.base;
	}
};

static c_driver *driver = new c_driver();

/*--------------------------------------------------------------------------------------------------------*/

typedef char PACKAGENAME;	// 包名
pid_t pid;	// 进程ID
/*
int get_name_pid(const std::string& PackageName)
{
    FILE* fp;
    std::string cmd = "pidof " + PackageName;
    fp = popen(cmd.c_str(), "r");
    fscanf(fp, "%d", &pid);
    pclose(fp);
    if (pid > 0)
    {
        driver->initialize(pid);
    }
    return pid;
}
*/


int get_name_pid(char* PackageName)
{
	FILE* fp;
    char cmd[0x100] = "pidof ";
    strcat(cmd, PackageName);
    fp = popen(cmd,"r");
    fscanf(fp,"%d", &pid);
    pclose(fp);
	if (pid > 0)
	{
		driver->initialize(pid);
	}
    return pid;
}

bool PidExamIne()
{
	char path[128];
	sprintf(path, "/proc/%d",pid);
	if (access(path,F_OK) != 0)
	{
		printf("\033[31;1m");
		puts("获取进程PID失败!");
		exit(1);
	}
	return true;
}

/*
void* getModuleBase(char* module_name)
{
    uintptr_t base = 0;
    base = driver->get_module_base(module_name);
    return reinterpret_cast<void*>(base);
}
*/
long getModuleBase(char* module_name)
{
	uintptr_t base=0;
	base = driver->get_module_base(module_name);
	return base;
}

// 读取内存
bool vm_readv(unsigned long address, void *buffer, size_t size)
{
	return driver->read(address, buffer, size);
}

// 写入内存
bool vm_writev(unsigned long address, void *buffer, size_t size)
{
	return driver->write(address, buffer, size);
}

// 获取F类内存
float getfloat(unsigned long addr)
{
	float var = 0;
	vm_readv(addr, &var, 4);
	return (var);
}

// 获取F类内存
float getFloat(unsigned long addr)
{
	float var = 0;
	vm_readv(addr, &var, 4);
	return (var);
}

// 获取D类内存
int getdword(unsigned long addr)
{
	int var = 0;
	vm_readv(addr, &var, 4);
	return (var);
}

// 获取D类内存
int getDword(unsigned long addr)
{
	int var = 0;
	vm_readv(addr, &var, 4);
	return (var);
}

// 获取指针(32位游戏)
unsigned int getPtr32(unsigned int addr)
{
	unsigned int var = 0;
	vm_readv(addr, &var, 4);
	return (var);
}

// 获取指针(64位游戏)
unsigned long getPtr64(unsigned long addr)
{
//	if (calculate_physical_address(addr) && addr != 0x0){
	unsigned long var = 0;
	vm_readv(addr, &var, 8);
	return (var);
//	}
}

   // 写入D类内存 
void writedword(unsigned long addr, int data)
{
   vm_writev(addr, &data, 4);
} 
    


void writefloat(unsigned long addr, float data)
{
	vm_writev(addr, &data, 4);
}