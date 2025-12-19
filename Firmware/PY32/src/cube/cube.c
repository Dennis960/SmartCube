#include "cube.h"

static inline void cube_handle_incoming_data(cube_side_t cube_side);
cube_side_t cube_find_new_parent();

static uint8_t connected_cubes = 0;
static uint8_t data_transfer_expecting_response = 0; // Prevents initializing transfers multiple times for requests

static cube_data_callback_t data_callback = NULL;
static cube_connected_callback_t connected_callback = NULL;
static cube_disconnected_callback_t disconnected_callback = NULL;
static cube_error_callback_t error_callback = NULL;

cube_side_t parent_cube = 0; // In the tree of cube connections, this is the side of the parent cube for this cube

/**
 * Get the side of the parent cube this cube is connected to.
 * The parent cube is the cube in the tree of cubes that is the root of this cube's connections.
 * @return The side of the parent cube
 */
cube_side_t cube_get_parent_cube()
{
  return parent_cube;
}

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
  if (parent_cube == 0)
  {
    parent_cube = cube_side;
  }
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
  if (cube_side == parent_cube)
  {
      parent_cube = 0;
      parent_cube = cube_find_new_parent();
  }
}

/**
 * Called when a communication error occurs.
 * @param cube_side The side of the cube where the error occurred
 * @param status The error status code, one of CUBE_DATA_TRANSMISSION_ERROR_DESERIALIZATION, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_TIMEOUT, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_RECEIVE_TIMEOUT, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_FINISH_TIMEOUT, CUBE_DATA_TRANSMISSION_ERROR_CLOCK_TIMEOUT
 * @param packet The data packet involved in the error, if any, else NULL
 */
static inline void cube_handle_communication_error(cube_side_t cube_side, cube_data_transmission_status_t status, cube_data_packet_t *packet)
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
  cube_data_transmission_status_t status = cube_receive_data(cube_side, cube_data_buffer, DATA_BUFFER_SIZE, &length);
  if (status != CUBE_DATA_TRANSMISSION_OK)
  {
    cube_handle_communication_error(cube_side, status, NULL);
    return;
  }
  if (!data_callback)
    return;
  cube_data_packet_t *packet = deserialize_cube_data(cube_data_buffer, length);
  if (!packet)
  {
    cube_handle_communication_error(cube_side, CUBE_DATA_TRANSMISSION_ERROR_DESERIALIZATION, NULL);
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
  cube_data_transmission_status_t status = cube_handle_disconnect_or_communication_request(cube_side);
  if (status == CUBE_DATA_TRANSMISSION_DISCONNECTED)
  {
    cube_set_side_disconnected(cube_side);
    // TODO: if the cube that this cube was first connected to gets disconnected, connect to another cube
    // TODO: Store the connected cube for future reference to know where to transmit data in the future
    // TODO: Store the cubes that are connected to this cube to know where to receive data from in the future
    cube_handle_disconnection(cube_side);
  }
  else if (status == CUBE_DATA_TRANSMISSION_OK)
  {
    cube_handle_incoming_data(cube_side);
    cube_set_side_idle(cube_side);
  }
  else
  {
    cube_handle_communication_error(cube_side, status, NULL);
    cube_set_side_idle(cube_side);
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
      cube_data_transmission_status_t status = cube_init_data_transfer(cube_side);
      if (status != CUBE_DATA_TRANSMISSION_OK)
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

/**
 * Waits until any cube is connected and returns the side of the connected cube.
 * @return The side of the connected cube
 */
cube_side_t wait_for_cube_connection()
{
  while (1)
  {
    for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
    {
      if (cube_connection_announce_presence(cube_side))
      {
        cube_handle_connection(cube_side);
        return cube_side;
      }
    }
  }
}

/**
 * Disconnects all child cubes, waits a short delay for them to process the disconnection,
 * and then finds a new parent cube among the connected cubes.
 * Blocks until a new parent cube is found.
 * @return The side of the new parent cube
 */
cube_side_t cube_find_new_parent()
{
  // Disconnect all child cubes
  for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
  {
    if (connected_cubes & cube_side)
    {
      cube_set_side_disconnected(cube_side);
    }
  }

  // Wait a short delay to allow child cubes to process the disconnection
  // TODO: It might be faster or better and more reliable to send a disconnection packet instead of just waiting
  for (volatile uint32_t i = 0; i < 100000; i++)
    ;

  // Find a new parent cube among the connected cubes
  return wait_for_cube_connection();
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
      if (cube_connection_respond_to_announcement(cube_side))
      {
        cube_handle_connection(cube_side);
      }
    }
    else if (is_data_disconnected)
    {
      cube_handle_data_disconnection(cube_side);
    }
  }
}
