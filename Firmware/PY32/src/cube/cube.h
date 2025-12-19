#ifndef CUBE_H
#define CUBE_H

#include "cube_data.h"
#include "cube_data_transmission.h"
#include "cube_connection.h"

#define DATA_BUFFER_SIZE 256

typedef void (*cube_data_callback_t)(cube_side_t cube_side, cube_data_packet_t *packet);
typedef void (*cube_connected_callback_t)(cube_side_t cube_side);
typedef void (*cube_disconnected_callback_t)(cube_side_t cube_side);
typedef void (*cube_error_callback_t)(cube_side_t cube_side, cube_data_transmission_status_t error_code, cube_data_packet_t *packet);

void cube_loop();

void cube_set_data_callback(cube_data_callback_t callback);
void cube_set_connected_callback(cube_connected_callback_t callback);
void cube_set_disconnected_callback(cube_disconnected_callback_t callback);
void cube_set_error_callback(cube_error_callback_t callback);

void cube_send_data_packet(cube_side_t cube_side, cube_data_packet_t *packet);
cube_side_t wait_for_cube_connection();
cube_side_t cube_get_parent_cube();
#endif
