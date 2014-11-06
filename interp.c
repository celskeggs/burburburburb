#include "cpu.h"
#include "hardware.h"
#include <stdlib.h>

int main() {
	struct cpu_ent *ent = cpu_new();
	hw_add_sysbus_null(ent->hardware);
	if (!cpu_start(ent)) {
		exit(1);
	}
	if (!cpu_collect(ent, true)) {
		exit(2);
	}
	exit(0);
}

