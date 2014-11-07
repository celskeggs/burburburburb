#include "cpu.h"
#include "hardware.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define HANDLE(f,m,c) { int status = f; if (status) { errno = status; perror(m); c; } }

static void *drain_serial(void *in) {
	struct hw_serial_buffer *buf = (struct hw_serial_buffer *) in;
	if (buf->notify_reader != NULL) {
		fputs("notify_reader already exists", stderr);
		return NULL;
	}
	uint8_t data;
	struct hw_notify *not = hw_notify_new();
	buf->notify_reader = not;
	while (1) {
		while (hw_serial_remove(buf, &data)) {
			putchar(data);
		}
		hw_notify_wait(not);
	}
	// TODO: hw_notify_destroy(not);
	return NULL;
}

static void *fill_serial(void *in) {
	struct hw_serial_buffer *buf = (struct hw_serial_buffer *) in;
	if (buf->notify_writer != NULL) {
		fputs("notify_writer already exists", stderr);
		return NULL;
	}
	struct hw_notify *not = hw_notify_new();
	buf->notify_writer = not;
	while (1) {
		uint8_t data = getchar();
		while (!hw_serial_insert(buf, data)) {
			hw_notify_wait(not);
		}
	}
	// TODO: hw_notify_destroy(not);
	return NULL;
}

int main() {
	struct cpu_ent *ent = cpu_new();
	if (ent == NULL) {
		exit(1);
	}
	struct hw_serial_buffer *buf_in = hw_serial_new();
	struct hw_serial_buffer *buf_out = hw_serial_new();
	if (ent->hardware != NULL) {
		hw_add_sysbus_serial_in(ent->hardware, buf_in);
		hw_add_sysbus_serial_out(ent->hardware, buf_out);
		hw_add_sysbus_null(ent->hardware);
	}
	if (!cpu_start(ent)) {
		exit(2);
	}
	pthread_t drain, fill;
	HANDLE(pthread_create(&drain, NULL, drain_serial, buf_out), "starting i/o worker", {})
	HANDLE(pthread_create(&fill, NULL, fill_serial, buf_in), "starting i/o worker", {})
	if (!cpu_collect(ent, true)) {
		exit(3);
	}
	HANDLE(pthread_cancel(drain), "cancelling i/o worker", {})
	HANDLE(pthread_cancel(fill), "cancelling i/o worker", {})
	HANDLE(pthread_join(drain, NULL), "joining i/o worker", {})
	HANDLE(pthread_join(fill, NULL), "joining i/o worker", {})
	exit(0);
}

