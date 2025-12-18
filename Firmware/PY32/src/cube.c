#include "cube.h"

static inline void cube_handle_incoming_data(cube_side_t cube_side);

static uint8_t connected_cubes = 0;
static uint8_t data_transfer_expecting_response = 0; // Prevents initializing transfers multiple times for requests

static cube_data_callback_t data_callback = NULL;
static cube_connected_callback_t connected_callback = NULL;
static cube_disconnected_callback_t disconnected_callback = NULL;
static cube_error_callback_t error_callback = NULL;

/**
 * Set the callback that is called when data is received from another cube.
 */
void cube_set_data_callback(cube_data_callback_t callback)
{
  data_callback = callback;
}

/**
 * Set the callback that is called when a cube is connected.
 */
void cube_set_connected_callback(cube_connected_callback_t callback)
{
  connected_callback = callback;
}

/**
 * Set the callback that is called when a cube is disconnected.
 */
void cube_set_disconnected_callback(cube_disconnected_callback_t callback)
{
  disconnected_callback = callback;
}

/**
 * Set the callback that is called when an error occurs during communication.
 */
void cube_set_error_callback(cube_error_callback_t callback)
{
  error_callback = callback;
}

/**
 * Send a request to another cube and handle incoming data.
 *
 * Needs to be called after cube_init_data_transfer().
 *
 * @param cube_side The side of the cube to communicate with
 * @param data Pointer to the data to send
 * @param length Length of the data in bytes
 * The received data is passed to the registered data callback.
 */
static void cube_request_data(cube_side_t cube_side, uint8_t *data, uint32_t length)
{
  cube_send_data(cube_side, data, length);
  cube_handle_incoming_data(cube_side);
}

/**
 * Called when a cube is connected.
 */
static inline void cube_handle_connection(cube_side_t cube_side)
{
  if (connected_callback)
  {
    connected_callback(cube_side);
  }
  connected_cubes |= cube_side;
}

/**
 * Called when a cube is disconnected.
 */
static inline void cube_handle_disconnection(cube_side_t cube_side)
{
  if (disconnected_callback)
  {
    disconnected_callback(cube_side);
  }
  connected_cubes &= ~cube_side;
}

/**
 * Called when a communication error occurs.
 * @param cube_side The side of the cube where the error occurred
 * @param status The error status code
 * @param packet The data packet involved in the error, if any, else NULL
 */
static inline void cube_handle_communication_error(cube_side_t cube_side, cube_status_t status, cube_data_packet_t *packet)
{
  if (error_callback)
  {
    error_callback(cube_side, status, packet);
  }
}

/**
 * Handle incoming data from a connected cube.
 * @param cube_side The side of the cube to receive data from
 * The received data is passed to the registered data callback.
 */
static inline void cube_handle_incoming_data(cube_side_t cube_side)
{
  uint32_t length = 0;
  uint8_t cube_data_buffer[DATA_BUFFER_SIZE];
  cube_status_t cube_status = cube_receive_data(cube_side, cube_data_buffer, DATA_BUFFER_SIZE, &length);
  if (cube_status != CUBE_OK)
  {
    cube_handle_communication_error(cube_side, cube_status, NULL);
    return;
  }
  if (!data_callback)
    return;
  cube_data_packet_t *packet = deserialize_cube_data(cube_data_buffer, length);
  if (!packet)
  {
    cube_handle_communication_error(cube_side, CUBE_ERROR_DESERIALIZATION, NULL);
    return;
  }
  if (data_type_expects_response(packet->type))
  {
    data_transfer_expecting_response = 1;
  }
  data_callback(cube_side, packet);
  free(packet);
}

/**
 * Called when data disconnection is detected. The cube is either disconnected or wants to start communication.
 */
static inline void cube_handle_data_disconnection(cube_side_t cube_side)
{
  cube_status_t cube_status = cube_handle_disconnect_or_communication_request(cube_side);
  if (cube_status == CUBE_DISCONNECTED)
  {
    cube_handle_disconnection(cube_side);
  }
  else if (cube_status == CUBE_OK)
  {
    cube_handle_incoming_data(cube_side);
  }
  else
  {
    cube_handle_communication_error(cube_side, cube_status, NULL);
  }
  cube_set_side_idle(cube_side);
}

/**
 * Main loop to be called periodically to handle cube connections and data transfers.
 */
void cube_loop()
{
  for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
  {
    uint8_t is_newly_connected = !(connected_cubes & cube_side) && cube_is_connected(cube_side);
    uint8_t is_data_disconnected = (connected_cubes & cube_side) && !cube_is_connected(cube_side);
    if (is_newly_connected)
    {
      cube_handle_connection(cube_side);
    }
    else if (is_data_disconnected)
    {
      cube_handle_data_disconnection(cube_side);
    }
  }
}

/**
 * Sends a cube data packet to another cube.
 * Automatically handles serialization, data transfer, and whether to request data after sending.
 * @param cube_side The side of the cube to send the data to
 * @param packet The cube data packet to send
 */
void cube_send_data_packet(cube_side_t cube_side, cube_data_packet_t *packet)
{
  uint32_t length = 0;
  uint8_t *data = serialize_cube_data(packet, &length);
  if (data)
  {
    if (!data_transfer_expecting_response)
    {
      cube_status_t status = cube_init_data_transfer(cube_side);
      if (status != CUBE_OK)
      {
        cube_handle_communication_error(cube_side, status, packet);
        goto end;
      }
    }
    if (data_type_expects_response(packet->type))
    {
      cube_request_data(cube_side, data, length);
    }
    else
    {
      cube_send_data(cube_side, data, length);
    }
  end:
    free(data);
  }
  cube_set_side_idle(cube_side);
  data_transfer_expecting_response = 0;
}