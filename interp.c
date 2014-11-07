#include "cpu.h"
#include "hardware.h"
#include <stdlib.h>

int main() {
	struct cpu_ent *ent = cpu_new();
	if (ent == NULL) {
		exit(1);
	}
	if (ent->hardware != NULL) {
		struct hw_serial_buffer *buf = hw_serial_new();
		hw_add_sysbus_serial_in(ent->hardware, buf);
		hw_add_sysbus_serial_out(ent->hardware, buf);
		hw_add_sysbus_null(ent->hardware);
	}
	if (!cpu_start(ent)) {
		exit(2);
	}
	if (!cpu_collect(ent, true)) {
		exit(3);
	}
	exit(0);
}

