#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "so_stdio.h"
#include "test_util.h"

#include "hooks.h"

int num_sys_read;
int target_fd;

ssize_t hook_read(int fd, void *buf, size_t len);

struct func_hook hooks[] = {
	[0] = { .name = "read", .addr = (unsigned long)hook_read, .orig_addr = 0 },
};


char buf[] = "Hello, World!\n";
int buf_len = sizeof(buf) - 1;

ssize_t hook_read(int fd, void *buf, size_t len)
{
	ssize_t (*orig_read)(int, void *, size_t);

	orig_read = (ssize_t (*)(int, void *, size_t))hooks[0].orig_addr;

	if (fd == target_fd)
		num_sys_read++;

	return orig_read(fd, buf, len);
}


int main(int argc, char *argv[])
{
	SO_FILE *f;
	int c;
	int ret;
	char *test_work_dir;
	char fpath[256];
	int i;

	install_hooks("libso_stdio.so", hooks, 1);

	if (argc == 2)
		test_work_dir = argv[1];
	else
		test_work_dir = "_test";

	sprintf(fpath, "%s/small_file", test_work_dir);

	ret = create_file_with_contents(fpath, (unsigned char *)buf, strlen(buf));
	FAIL_IF(ret != 0, "Couldn't create file: %s\n", fpath);


	/* --- BEGIN TEST --- */
	f = so_fopen(fpath, "r");
	FAIL_IF(!f, "Couldn't open file: %s\n", fpath);

	target_fd = so_fileno(f);

	num_sys_read = 0;

	for (i = 0; i < buf_len; i++) {
		c = so_fgetc(f);
		FAIL_IF((char)c != buf[i], "Incorrect character: got %x, expected %x\n", (unsigned char)c, buf[i]);

		ret = so_feof(f);
		FAIL_IF(ret != 0, "Incorrect return value for so_feof: got %d, expected %d\n", ret, 0);
	}

	c = so_fgetc(f);
	FAIL_IF(c != SO_EOF, "Incorrect return value for so_fgetc: got %d, expected %d\n", c, SO_EOF);

	ret = so_feof(f);
	FAIL_IF(ret == 0, "Incorrect return value for so_feof: got %d, expected != 0\n", ret);

	ret = so_fclose(f);
	FAIL_IF(ret != 0, "Incorrect return value for so_fclose: got %d, expected %d\n", ret, 0);

	return 0;
}
