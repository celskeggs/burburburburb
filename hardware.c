#include "hardware.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define NEW(x) ((x*) malloc(sizeof(x)))
#define HANDLE(f,m,c) { int status = f; if (status) { errno = status; perror(m); c; } }

struct hw_ent *hw_new() {
	struct hw_ent *out = NEW(struct hw_ent);
	if (out == NULL) {
		return NULL;
	}
	out->sysbus_head = NULL;
	return out;
}

static void hw_add_sysbus(struct hw_ent *out, struct hw_sysbus *add) {
	add->next = out->sysbus_head;
	out->sysbus_head = add;
}

static struct hw_sysbus *hw_sysbus_new(uint16_t typeid) {
	struct hw_sysbus *out = NEW(struct hw_sysbus);
	if (out == NULL) {
		return NULL;
	}
	out->busid = 0xFF;
	out->typeid = typeid;
	out->entry_data = NULL;
	out->next = NULL;
	return out;
}

struct hw_notify *hw_notify_new() {
	struct hw_notify *out = NEW(struct hw_notify);
	if (out == NULL) {
		return NULL;
	}
	out->is_active = false;
	HANDLE(pthread_mutex_init(&out->lock, NULL), "allocate mutex", {
		free(out);
		return NULL;
	})
	HANDLE(pthread_cond_init(&out->cond, NULL), "allocate cond", {
		HANDLE(pthread_mutex_destroy(&out->lock), "deallocate mutex", {});
		free(out);
		return NULL;
	})
	return out;
}

void hw_notify_destroy(struct hw_notify *ent) {
	HANDLE(pthread_mutex_destroy(&ent->lock), "deallocate mutex", {})
	HANDLE(pthread_cond_destroy(&ent->cond), "deallocate cond", {})
	free(ent);
}

void hw_notify_notify(struct hw_notify *ent) {
	if (ent == NULL) { return; }
	HANDLE(pthread_mutex_lock(&ent->lock), "lock mutex", { return; })
	ent->is_active = true;
	HANDLE(pthread_mutex_unlock(&ent->lock), "unlock mutex", {})
	HANDLE(pthread_cond_broadcast(&ent->cond), "broadcast cond", {})
}

void hw_notify_wait(struct hw_notify *ent) {
	HANDLE(pthread_mutex_lock(&ent->lock), "lock mutex", { return; })
	while (!ent->is_active) {
		HANDLE(pthread_cond_wait(&ent->cond, &ent->lock), "wait cond", { return; });
	}
	ent->is_active = false;
	HANDLE(pthread_mutex_unlock(&ent->lock), "unlock mutex", {})
}

struct hw_serial_buffer *hw_serial_new() {
	struct hw_serial_buffer *out = NEW(struct hw_serial_buffer);
	if (out == NULL) {
		return NULL;
	}
	memset(out->data, '\0', sizeof(out->data));
	out->read_ptr = out->write_ptr = 0;
	out->notify_reader = out->notify_writer = NULL;
	HANDLE(pthread_mutex_init(&out->lock, NULL), "allocate mutex", {
		free(out);
		return NULL;
	})
	return out;
}

void hw_serial_delete(struct hw_serial_buffer *buffer) {
	HANDLE(pthread_mutex_destroy(&buffer->lock), "deallocate mutex", {})
	free(buffer);
}

bool hw_serial_insert(struct hw_serial_buffer *ptr, uint8_t data) {
	HANDLE(pthread_mutex_lock(&ptr->lock), "lock mutex", { return false; })
	bool could_write = (ptr->read_ptr != (uint8_t) (ptr->write_ptr + 1));
	if (could_write) {
		ptr->data[ptr->write_ptr++] = data;
	}
	HANDLE(pthread_mutex_unlock(&ptr->lock), "unlock mutex", {})
	hw_notify_notify(ptr->notify_reader);
	return could_write;
}

bool hw_serial_remove(struct hw_serial_buffer *ptr, uint8_t *data) {
	bool could_read;
	HANDLE(pthread_mutex_lock(&ptr->lock), "lock mutex", { return false; })
	could_read = (ptr->read_ptr != ptr->write_ptr);
	if (could_read) {
		*data = ptr->data[ptr->read_ptr++];
	}
	HANDLE(pthread_mutex_unlock(&ptr->lock), "unlock mutex", {})
	hw_notify_notify(ptr->notify_writer);
	return could_read;
}

void hw_add_sysbus_serial_in(struct hw_ent *out, struct hw_serial_buffer *buffer) {
	struct hw_sysbus *n = hw_sysbus_new(HW_SYSBUS_SERIAL_IN);
	n->entry_data = buffer;
	hw_add_sysbus(out, n);
}

void hw_add_sysbus_serial_out(struct hw_ent *out, struct hw_serial_buffer *buffer) {
	struct hw_sysbus *n = hw_sysbus_new(HW_SYSBUS_SERIAL_OUT);
	n->entry_data = buffer;
	hw_add_sysbus(out, n);
}

void hw_add_sysbus_null(struct hw_ent *out) {
	hw_add_sysbus(out, hw_sysbus_new(HW_SYSBUS_NULL));
}

void hw_add_sysbus_zero(struct hw_ent *out) {
	hw_add_sysbus(out, hw_sysbus_new(HW_SYSBUS_ZERO));
}

struct hw_sysbus *hw_bus_lookup(struct hw_ent *out, uint8_t busid) {
	struct hw_sysbus *active = out->sysbus_head;
	while (active != NULL && active->busid != busid) {
		active = active->next;
	}
	return active;
}

uint16_t hw_exchange_bus(struct hw_sysbus *ent, uint8_t byte) {
	uint8_t toread;
	switch (ent->typeid) {
	case HW_SYSBUS_SERIAL_IN:
		if (hw_serial_remove((struct hw_serial_buffer *) ent->entry_data, &toread)) {
			return toread;
		} else {
			return 0xFFFF;
		}
	case HW_SYSBUS_SERIAL_OUT:
		return hw_serial_insert((struct hw_serial_buffer *) ent->entry_data, byte) ? 0x0001 : 0x0000;
	case HW_SYSBUS_ZERO:
		return 0x0000;
	default:
		return 0xFFFF;
	}
}

bool hw_subscribe_bus(struct hw_sysbus *bus, struct hw_notify *ent) {
	struct hw_serial_buffer *buf;
	switch (bus->typeid) {
	case HW_SYSBUS_SERIAL_IN:
		buf = (struct hw_serial_buffer *) bus->entry_data;
		if (buf->notify_reader == NULL) {
			buf->notify_reader = ent;
			return true;
		}
		return false;
	case HW_SYSBUS_SERIAL_OUT:
		buf = (struct hw_serial_buffer *) bus->entry_data;
		if (buf->notify_writer == NULL) {
			buf->notify_writer = ent;
			return true;
		}
		return false;
	default:
		return false;
	}
}

bool hw_unsubscribe_bus(struct hw_sysbus *bus, struct hw_notify *ent) {
	struct hw_serial_buffer *buf;
	switch (bus->typeid) {
	case HW_SYSBUS_SERIAL_IN:
		buf = (struct hw_serial_buffer *) bus->entry_data;
		if (buf->notify_reader == ent) {
			buf->notify_reader = NULL;
			return true;
		}
		return false;
	case HW_SYSBUS_SERIAL_OUT:
		buf = (struct hw_serial_buffer *) bus->entry_data;
		if (buf->notify_writer == ent) {
			buf->notify_writer = NULL;
			return true;
		}
		return false;
	default:
		return false;
	}
}

