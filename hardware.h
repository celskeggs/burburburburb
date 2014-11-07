#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define HW_SYSBUS_NULL 0x0000
#define HW_SYSBUS_SERIAL_IN 0x9C92
#define HW_SYSBUS_SERIAL_OUT 0xCE43
#define HW_SYSBUS_ZERO 0xEA17
#define HW_SYSBUS_NO_DEVICE 0xFFFF

struct hw_sysbus {
	uint8_t busid;
	uint16_t typeid;
	void *entry_data;
	struct hw_sysbus *next;
};

struct hw_notify {
	bool is_active;
	pthread_mutex_t lock;
	pthread_cond_t cond;
};

struct hw_serial_buffer {
	uint8_t data[256]; // serial input buffer
	uint8_t read_ptr;
	uint8_t write_ptr;
	pthread_mutex_t lock;
	struct hw_notify *notify_reader;
	struct hw_notify *notify_writer;
};

struct hw_ent {
	struct hw_sysbus *sysbus_head;
};

struct hw_ent *hw_new();

struct hw_notify *hw_notify_new();
void hw_notify_notify(struct hw_notify *ent);
void hw_notify_wait(struct hw_notify *ent);
void hw_notify_destroy(struct hw_notify *ent);

struct hw_serial_buffer *hw_serial_new();
void hw_serial_delete(struct hw_serial_buffer *buffer);
bool hw_serial_insert(struct hw_serial_buffer *ptr, uint8_t data);
bool hw_serial_remove(struct hw_serial_buffer *ptr, uint8_t *data);

void hw_add_sysbus_serial_in(struct hw_ent *out, struct hw_serial_buffer *buffer);
void hw_add_sysbus_serial_out(struct hw_ent *out, struct hw_serial_buffer *buffer);
void hw_add_sysbus_null(struct hw_ent *out);
void hw_add_sysbus_zero(struct hw_ent *out);

struct hw_sysbus *hw_bus_lookup(struct hw_ent *out, uint8_t busid);
uint16_t hw_exchange_bus(struct hw_sysbus *bus, uint8_t byte);
bool hw_subscribe_bus(struct hw_sysbus *bus, struct hw_notify *ent);
bool hw_unsubscribe_bus(struct hw_sysbus *bus, struct hw_notify *ent);

#endif
