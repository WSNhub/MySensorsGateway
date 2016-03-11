//#if defined(ARDUINO_ARCH_AVR)

#include "Arduino.h"
#include "ATSHA204.h"

#if (!ATSHA204I2C)
// atsha204Class Constructor
// Feed this function the Arduino-ized pin number you want to assign to the ATSHA204's SDA pin
// This will find the DDRX, PORTX, and PINX registrs it'll need to point to to control that pin
// As well as the bit value for each of those registers
ATSHA204Class::ATSHA204Class(uint8_t pin)
{	
	device_pin = digitalPinToBitMask(pin);	// Find the bit value of the pin
	uint8_t port = digitalPinToPort(pin);	// temoporarily used to get the next three registers
	
	// Point to data direction register port of pin
	device_port_DDR = portModeRegister(port);
	// Point to output register of pin
	device_port_OUT = portOutputRegister(port);
	// Point to input register of pin
	device_port_IN = portInputRegister(port);
}
#else
ATSHA204Class::ATSHA204Class()
{	
	//
}
#endif

void ATSHA204Class::getSerialNumber(uint8_t * response)
{
  uint8_t readCommand[READ_COUNT];
  uint8_t readResponse[READ_4_RSP_SIZE];

  /* read from bytes 0->3 of config zone */
  uint8_t returnCode = sha204m_read(readCommand, readResponse, SHA204_ZONE_CONFIG, ADDRESS_SN03);
  if (!returnCode)
  {
    for (int i=0; i<4; i++) // store bytes 0-3 into respones array
    response[i] = readResponse[SHA204_BUFFER_POS_DATA+i];

    /* read from bytes 8->11 of config zone */
    returnCode = sha204m_read(readCommand, readResponse, SHA204_ZONE_CONFIG, ADDRESS_SN47);

    for (int i=4; i<8; i++) // store bytes 4-7 of SN into response array
      response[i] = readResponse[SHA204_BUFFER_POS_DATA+(i-4)];

    if (!returnCode)
    { /* Finally if last two reads were successful, read byte 8 of the SN */
      returnCode = sha204m_read(readCommand, readResponse, SHA204_ZONE_CONFIG, ADDRESS_SN8);
      response[8] = readResponse[SHA204_BUFFER_POS_DATA]; // Byte 8 of SN should always be 0xEE
    }
  }

  return;
}

/* SWI bit bang functions */

#if (!ATSHA204I2C)
void ATSHA204Class::swi_set_signal_pin(uint8_t is_high)
{
  *device_port_DDR |= device_pin;

  if (is_high)
    *device_port_OUT |= device_pin;
  else
    *device_port_OUT &= ~device_pin;
}
#endif

uint8_t ATSHA204Class::swi_send_bytes(uint8_t count, uint8_t *buffer)
{
#if (ATSHA204I2C)
	int sent_bytes = Wire.write(buffer, count);

	if (count > 0 && sent_bytes == count) {
		return SWI_FUNCTION_RETCODE_SUCCESS;
	}

	return SWI_FUNCTION_RETCODE_TIMEOUT;

#else
  uint8_t i, bit_mask;

  // Disable interrupts while sending.
  noInterrupts();  //swi_disable_interrupts();

  // Set signal pin as output.
  *device_port_OUT |= device_pin;
  *device_port_DDR |= device_pin;

  // Wait turn around time.
  delayMicroseconds(RX_TX_DELAY);  //RX_TX_DELAY;

  for (i = 0; i < count; i++) 
  {
    for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1) 
    {
      if (bit_mask & buffer[i]) 
      {
        *device_port_OUT &= ~device_pin;
        delayMicroseconds(BIT_DELAY);  //BIT_DELAY_1;
        *device_port_OUT |= device_pin;
        delayMicroseconds(7*BIT_DELAY);  //BIT_DELAY_7;
      }
      else 
      {
        // Send a zero bit.
        *device_port_OUT &= ~device_pin;
        delayMicroseconds(BIT_DELAY);  //BIT_DELAY_1;
        *device_port_OUT |= device_pin;
        delayMicroseconds(BIT_DELAY);  //BIT_DELAY_1;
        *device_port_OUT &= ~device_pin;
        delayMicroseconds(BIT_DELAY);  //BIT_DELAY_1;
        *device_port_OUT |= device_pin;
        delayMicroseconds(5*BIT_DELAY);  //BIT_DELAY_5;
      }
    }
  }
  interrupts();  //swi_enable_interrupts();
  return SWI_FUNCTION_RETCODE_SUCCESS;
#endif
}

uint8_t ATSHA204Class::swi_send_byte(uint8_t value)
{
  return swi_send_bytes(1, &value);
}

uint8_t ATSHA204Class::swi_receive_bytes(uint8_t count, uint8_t *buffer)
{
#if (ATSHA204I2C)
	uint8_t i;

        delay(10);

	int available_bytes = Wire.requestFrom(0x64, count);
	if (available_bytes != count) {
		return SWI_FUNCTION_RETCODE_TIMEOUT;
	}

	for (i = 0; i < count; i++) {
		while (!Wire.available()); // Wait for byte that is going to be read next
		*buffer++ = Wire.read(); // Store read value
	}

	return SWI_FUNCTION_RETCODE_SUCCESS;

#else
  uint8_t status = SWI_FUNCTION_RETCODE_SUCCESS;
  uint8_t i;
  uint8_t bit_mask;
  uint8_t pulse_count;
  uint8_t timeout_count;

  // Disable interrupts while receiving.
  noInterrupts(); //swi_disable_interrupts();

  // Configure signal pin as input.
  *device_port_DDR &= ~device_pin;

  // Receive bits and store in buffer.
  for (i = 0; i < count; i++)
  {
    for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1) 
    {
      pulse_count = 0;

      // Make sure that the variable below is big enough.
      // Change it to uint16_t if 255 is too small, but be aware that
      // the loop resolution decreases on an 8-bit controller in that case.
      timeout_count = START_PULSE_TIME_OUT;

      // Detect start bit.
      while (--timeout_count > 0) 
      {
        // Wait for falling edge.
        if ((*device_port_IN & device_pin) == 0)
          break;
      }

      if (timeout_count == 0) 
      {
        status = SWI_FUNCTION_RETCODE_TIMEOUT;
        break;
      }

      do 
      {
        // Wait for rising edge.
        if ((*device_port_IN & device_pin) != 0) 
        {
          // For an Atmel microcontroller this might be faster than "pulse_count++".
          pulse_count = 1;
          break;
        }
      } while (--timeout_count > 0);

      if (pulse_count == 0) 
      {
        status = SWI_FUNCTION_RETCODE_TIMEOUT;
        break;
      }

      // Trying to measure the time of start bit and calculating the timeout
      // for zero bit detection is not accurate enough for an 8 MHz 8-bit CPU.
      // So let's just wait the maximum time for the falling edge of a zero bit
      // to arrive after we have detected the rising edge of the start bit.
      timeout_count = ZERO_PULSE_TIME_OUT;

      // Detect possible edge indicating zero bit.
      do 
      {
        if ((*device_port_IN & device_pin) == 0) 
        {
          // For an Atmel microcontroller this might be faster than "pulse_count++".
          pulse_count = 2;
          break;
        }
      } while (--timeout_count > 0);

      // Wait for rising edge of zero pulse before returning. Otherwise we might interpret
      // its rising edge as the next start pulse.
      if (pulse_count == 2) 
      {
        do 
        {
          if ((*device_port_IN & device_pin) != 0)
            break;
        } while (timeout_count-- > 0);
      }

      // Update byte at current buffer index.
      else
        buffer[i] |= bit_mask;  // received "one" bit
    }

    if (status != SWI_FUNCTION_RETCODE_SUCCESS)
      break;
  }
  interrupts(); //swi_enable_interrupts();

  if (status == SWI_FUNCTION_RETCODE_TIMEOUT) 
  {
    if (i > 0)
    // Indicate that we timed out after having received at least one byte.
    status = SWI_FUNCTION_RETCODE_RX_FAIL;
  }
  return status;
#endif
}

#if (ATSHA204I2C)
enum i2c_read_write_flag {
        FLAG_I2C_WRITE = (uint8_t) 0x00,  //!< write command flag
        FLAG_I2C_READ  = (uint8_t) 0x01   //!< read command flag
};

enum i2c_word_address {
        SHA204_I2C_PACKET_FUNCTION_RESET,  //!< Reset device.
        SHA204_I2C_PACKET_FUNCTION_SLEEP,  //!< Put device into Sleep mode.
        SHA204_I2C_PACKET_FUNCTION_IDLE,   //!< Put device into Idle mode.
        SHA204_I2C_PACKET_FUNCTION_NORMAL  //!< Write / evaluate data that follow this word address byte.
};

#define I2C_FUNCTION_RETCODE_SUCCESS     ((uint8_t) 0x00) //!< Communication with device succeeded.
#define I2C_FUNCTION_RETCODE_COMM_FAIL   ((uint8_t) 0xF0) //!< Communication with device failed.
#define I2C_FUNCTION_RETCODE_TIMEOUT     ((uint8_t) 0xF1) //!< Communication timed out.
#define I2C_FUNCTION_RETCODE_NACK        ((uint8_t) 0xF8) //!< TWI nack

int ATSHA204Class::start_operation(uint8_t readWrite) {
	//Serial.println("start_operation(uint8_t readWrite)");
	int written = Wire.write(&readWrite, (uint8_t)1);
	
	return written > 0;
}

uint8_t ATSHA204Class::send(uint8_t word_address, uint8_t count, uint8_t *buffer) {
	//Serial.println("send(uint8_t word_address, uint8_t count, uint8_t *buffer)");
	uint8_t i2c_status;

	Wire.beginTransmission(0x64);

	start_operation(FLAG_I2C_WRITE);

	i2c_status = swi_send_bytes(1, &word_address);
	if (i2c_status != I2C_FUNCTION_RETCODE_SUCCESS) {
		Serial.println("send -- fail 1");
		return SHA204_COMM_FAIL;
	}

	if (count == 0) {
		return SHA204_SUCCESS;
	}

	i2c_status = swi_send_bytes(count, buffer);

	if (i2c_status != I2C_FUNCTION_RETCODE_SUCCESS) {
		Serial.println("send -- fail 2");
		return SHA204_COMM_FAIL;
	}

	Wire.endTransmission();

	return SHA204_SUCCESS;
}

uint8_t ATSHA204Class::send_command(uint8_t count, uint8_t *command) {
	//Serial.println("send_command(uint8_t count, uint8_t *command)");
	return send(SHA204_I2C_PACKET_FUNCTION_NORMAL, count, command);
}

uint8_t ATSHA204Class::receive_byte(uint8_t *data) {

	int available_bytes = Wire.requestFrom(0x64, (uint8_t)1);
	if (available_bytes != 1) {
		return I2C_FUNCTION_RETCODE_COMM_FAIL;
	}
	while (!Wire.available()); // Wait for byte that is going to be read next
	*data++ = Wire.read(); // Store read value

	return I2C_FUNCTION_RETCODE_SUCCESS;
}

#endif

/* Physical functions */

void ATSHA204Class::sha204c_sleep()
{
  swi_send_byte(SHA204_SWI_FLAG_SLEEP);
}

uint8_t ATSHA204Class::sha204p_receive_response(uint8_t size, uint8_t *response)
{
#if (ATSHA204I2C)
	uint8_t count;
	uint8_t i2c_status;

        for (int i = 0; i < size; i++)
            response[i] = 0;
  
        //(void) swi_send_byte(SHA204_SWI_FLAG_TX);

	// Receive count byte.
	i2c_status = swi_receive_bytes(size, response);
	if (i2c_status == SWI_FUNCTION_RETCODE_SUCCESS ||
            i2c_status == SWI_FUNCTION_RETCODE_RX_FAIL) 
        {
            count = response[SHA204_BUFFER_POS_COUNT];
            if ((count < SHA204_RSP_SIZE_MIN) || (count > size))
                return SHA204_INVALID_SIZE;

            return SHA204_SUCCESS;
        }
        // Translate error so that the Communication layer
        // can distinguish between a real error or the
        // device being busy executing a command.
        if (i2c_status == SWI_FUNCTION_RETCODE_TIMEOUT)
            return SHA204_RX_NO_RESPONSE;
        else
            return SHA204_RX_FAIL;
#else
  uint8_t count_byte;
  uint8_t i;
  uint8_t ret_code;

  for (i = 0; i < size; i++)
    response[i] = 0;

  (void) swi_send_byte(SHA204_SWI_FLAG_TX);

  ret_code = swi_receive_bytes(size, response);
  if (ret_code == SWI_FUNCTION_RETCODE_SUCCESS || ret_code == SWI_FUNCTION_RETCODE_RX_FAIL) 
  {
    count_byte = response[SHA204_BUFFER_POS_COUNT];
    if ((count_byte < SHA204_RSP_SIZE_MIN) || (count_byte > size))
      return SHA204_INVALID_SIZE;

    return SHA204_SUCCESS;
  }

  // Translate error so that the Communication layer
  // can distinguish between a real error or the
  // device being busy executing a command.
  if (ret_code == SWI_FUNCTION_RETCODE_TIMEOUT)
    return SHA204_RX_NO_RESPONSE;
  else
    return SHA204_RX_FAIL;
#endif
}

/* Communication functions */

uint8_t ATSHA204Class::sha204c_wakeup(uint8_t *response)
{
#if (ATSHA204I2C)
	// This was the only way short of manually adjusting the SDA pin to wake up the device
	Wire.beginTransmission(0x64);
	delay(10);
        int i2c_status = Wire.endTransmission();
	if (i2c_status != 0) {
		//Serial.println("chip_wakeup() FAIL");
		return SHA204_COMM_FAIL;
	}

	return SHA204_SUCCESS;
#else
  swi_set_signal_pin(0);
  delayMicroseconds(10*SHA204_WAKEUP_PULSE_WIDTH);
  swi_set_signal_pin(1);
  delay(SHA204_WAKEUP_DELAY);

  uint8_t ret_code = sha204p_receive_response(SHA204_RSP_SIZE_MIN, response);
  if (ret_code != SHA204_SUCCESS)
    return ret_code;

  // Verify status response.
  if (response[SHA204_BUFFER_POS_COUNT] != SHA204_RSP_SIZE_MIN)
    ret_code = SHA204_INVALID_SIZE;
  else if (response[SHA204_BUFFER_POS_STATUS] != SHA204_STATUS_BYTE_WAKEUP)
    ret_code = SHA204_COMM_FAIL;
  else 
  {
    if ((response[SHA204_RSP_SIZE_MIN - SHA204_CRC_SIZE] != 0x33)
      || (response[SHA204_RSP_SIZE_MIN + 1 - SHA204_CRC_SIZE] != 0x43))
      ret_code = SHA204_BAD_CRC;
  }
  if (ret_code != SHA204_SUCCESS)
    delay(SHA204_COMMAND_EXEC_MAX);

  return ret_code;
#endif
}

uint8_t ATSHA204Class::sha204c_resync(uint8_t size, uint8_t *response)
{
#if (ATSHA204I2C)
	// Try to re-synchronize without sending a Wake token
	// (step 1 of the re-synchronization process).
	uint8_t nine_clocks = 0xFF;
	swi_send_bytes(1, &nine_clocks);
	Wire.beginTransmission(0x64);
	Wire.endTransmission();

	// Try to send a Reset IO command if re-sync succeeded.
	int ret_code = send(SHA204_I2C_PACKET_FUNCTION_RESET, 0, NULL); //reset_io();

	if (ret_code == SHA204_SUCCESS) {
		return ret_code;
	}

	// We lost communication. Send a Wake pulse and try
	// to receive a response (steps 2 and 3 of the
	// re-synchronization process).
	send(SHA204_I2C_PACKET_FUNCTION_SLEEP, 0, NULL); //sleep();
	ret_code = sha204c_wakeup(response); //wakeup(response);

	// Translate a return value of success into one
	// that indicates that the device had to be woken up
	// and might have lost its TempKey.
	return (ret_code == SHA204_SUCCESS ? SHA204_RESYNC_WITH_WAKEUP : ret_code);
#else
  // Try to re-synchronize without sending a Wake token
  // (step 1 of the re-synchronization process).
  delay(SHA204_SYNC_TIMEOUT);
  uint8_t ret_code = sha204p_receive_response(size, response);
  if (ret_code == SHA204_SUCCESS)
    return ret_code;

  // We lost communication. Send a Wake pulse and try
  // to receive a response (steps 2 and 3 of the
  // re-synchronization process).
  sha204c_sleep();
  ret_code = sha204c_wakeup(response);

  // Translate a return value of success into one
  // that indicates that the device had to be woken up
  // and might have lost its TempKey.
  return (ret_code == SHA204_SUCCESS ? SHA204_RESYNC_WITH_WAKEUP : ret_code);
#endif
}

uint8_t ATSHA204Class::sha204c_send_and_receive(uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer, uint8_t execution_delay, uint8_t execution_timeout)
{
  uint8_t ret_code = SHA204_FUNC_FAIL;
  uint8_t ret_code_resync;
  uint8_t n_retries_send;
  uint8_t n_retries_receive;
  uint8_t i;
  uint8_t status_byte;
  uint8_t count = tx_buffer[SHA204_BUFFER_POS_COUNT];
  uint8_t count_minus_crc = count - SHA204_CRC_SIZE;
  uint16_t execution_timeout_us = (uint16_t) (execution_timeout * 1000) + SHA204_RESPONSE_TIMEOUT;
  volatile uint16_t timeout_countdown;

  // Append CRC.
  sha204c_calculate_crc(count_minus_crc, tx_buffer, tx_buffer + count_minus_crc);

  // Retry loop for sending a command and receiving a response.
  n_retries_send = SHA204_RETRY_COUNT + 1;

  while ((n_retries_send-- > 0) && (ret_code != SHA204_SUCCESS)) 
  {
    // Send command.
#if (ATSHA204I2C)
    ret_code = send_command(count, tx_buffer);
#else
    ret_code = swi_send_byte(SHA204_SWI_FLAG_CMD);
    if (ret_code != SWI_FUNCTION_RETCODE_SUCCESS)
      ret_code = SHA204_COMM_FAIL;
    else
      ret_code = swi_send_bytes(count, tx_buffer);
#endif

    if (ret_code != SHA204_SUCCESS) 
    {
      if (sha204c_resync(rx_size, rx_buffer) == SHA204_RX_NO_RESPONSE)
        return ret_code; // The device seems to be dead in the water.
      else
        continue;
    }

    // Wait minimum command execution time and then start polling for a response.
    delay(execution_delay);

    // Retry loop for receiving a response.
    n_retries_receive = SHA204_RETRY_COUNT + 1;
    while (n_retries_receive-- > 0) 
    {
      // Reset response buffer.
      for (i = 0; i < rx_size; i++)
        rx_buffer[i] = 0;

      // Poll for response.
      timeout_countdown = execution_timeout_us;
      do 
      {
        ret_code = sha204p_receive_response(rx_size, rx_buffer);
        timeout_countdown -= SHA204_RESPONSE_TIMEOUT;
      } 
      while ((timeout_countdown > SHA204_RESPONSE_TIMEOUT) && (ret_code == SHA204_RX_NO_RESPONSE));

      if (ret_code == SHA204_RX_NO_RESPONSE) 
      {
        // We did not receive a response. Re-synchronize and send command again.
        if (sha204c_resync(rx_size, rx_buffer) == SHA204_RX_NO_RESPONSE)
        {
          // The device seems to be dead in the water.
          return ret_code;
        }
        else
        {
          delay(10);
          n_retries_send += 1;
          goto next_send_recv_cycle; //break;
        }
      }

      // Check whether we received a valid response.
      if (ret_code == SHA204_INVALID_SIZE)
      {
        // We see 0xFF for the count when communication got out of sync.
        ret_code_resync = sha204c_resync(rx_size, rx_buffer);
        if (ret_code_resync == SHA204_SUCCESS)
          // We did not have to wake up the device. Try receiving response again.
          continue;
        if (ret_code_resync == SHA204_RESYNC_WITH_WAKEUP)
          // We could re-synchronize, but only after waking up the device.
          // Re-send command.
          break;
        else
          // We failed to re-synchronize.
          return ret_code;
      }

      // We received a response of valid size.
      // Check the consistency of the response.
      ret_code = sha204c_check_crc(rx_buffer);
      if (ret_code == SHA204_SUCCESS) 
      {
        // Received valid response.
        if (rx_buffer[SHA204_BUFFER_POS_COUNT] > SHA204_RSP_SIZE_MIN)
          // Received non-status response. We are done.
          return ret_code;

        // Received status response.
        status_byte = rx_buffer[SHA204_BUFFER_POS_STATUS];

        // Translate the three possible device status error codes
        // into library return codes.
        if (status_byte == SHA204_STATUS_BYTE_PARSE)
          return SHA204_PARSE_ERROR;
        if (status_byte == SHA204_STATUS_BYTE_EXEC)
          return SHA204_CMD_FAIL;
        if (status_byte == SHA204_STATUS_BYTE_COMM) 
        {
          // In case of the device status byte indicating a communication
          // error this function exits the retry loop for receiving a response
          // and enters the overall retry loop
          // (send command / receive response).
          ret_code = SHA204_STATUS_CRC;
          break;
        }

        // Received status response from CheckMAC, DeriveKey, GenDig,
        // Lock, Nonce, Pause, UpdateExtra, or Write command.
        return ret_code;
      }

      else 
      {
        // Received response with incorrect CRC.
        ret_code_resync = sha204c_resync(rx_size, rx_buffer);
        if (ret_code_resync == SHA204_SUCCESS)
          // We did not have to wake up the device. Try receiving response again.
          continue;
        if (ret_code_resync == SHA204_RESYNC_WITH_WAKEUP)
          // We could re-synchronize, but only after waking up the device.
          // Re-send command.
          break;
        else
          // We failed to re-synchronize.
          return ret_code;
      } // block end of check response consistency

    } // block end of receive retry loop

next_send_recv_cycle:
	ret_code = ret_code; //dummy to make goto work
  } // block end of send and receive retry loop

  return ret_code;
}


/* Marshaling functions */
uint8_t ATSHA204Class::sha204m_read(uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t zone, uint16_t address)
{
  uint8_t rx_size;

  address >>= 2;

  tx_buffer[SHA204_COUNT_IDX] = READ_COUNT;
  tx_buffer[SHA204_OPCODE_IDX] = SHA204_READ;
  tx_buffer[READ_ZONE_IDX] = zone;
  tx_buffer[READ_ADDR_IDX] = (uint8_t) (address & SHA204_ADDRESS_MASK);
  tx_buffer[READ_ADDR_IDX + 1] = 0;

  rx_size = (zone & SHA204_ZONE_COUNT_FLAG) ? READ_32_RSP_SIZE : READ_4_RSP_SIZE;

  return sha204c_send_and_receive(&tx_buffer[0], rx_size, &rx_buffer[0], READ_DELAY, READ_EXEC_MAX - READ_DELAY);
}

uint8_t ATSHA204Class::sha204m_execute(uint8_t op_code, uint8_t param1, uint16_t param2,
			uint8_t datalen1, uint8_t *data1,	uint8_t tx_size, uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer)
{
	uint8_t poll_delay, poll_timeout, response_size;
	uint8_t *p_buffer;
	uint8_t len;
  //(void)tx_size;
  
	/*
	 * Just send the wakeup command to be sure.
	 * If it fails the chip is either broken or already awake.
	 */
	int ret_code = sha204c_wakeup(rx_buffer);
	if (ret_code != SHA204_SUCCESS)
  	{
    		//Serial.print("Failed to wake device. Response: ");
    		//Serial.println(ret_code, HEX);
    		//return; //halt();
                delay(3);
  	}

	// Supply delays and response size.
	switch (op_code) 
	{
		case SHA204_GENDIG:
			poll_delay = GENDIG_DELAY;
			poll_timeout = GENDIG_EXEC_MAX - GENDIG_DELAY;
			response_size = GENDIG_RSP_SIZE;
			break;

		case SHA204_HMAC:
			poll_delay = HMAC_DELAY;
			poll_timeout = HMAC_EXEC_MAX - HMAC_DELAY;
			response_size = HMAC_RSP_SIZE;
			break;

		case SHA204_NONCE:
			poll_delay = NONCE_DELAY;
			poll_timeout = NONCE_EXEC_MAX - NONCE_DELAY;
			response_size = param1 == NONCE_MODE_PASSTHROUGH
								? NONCE_RSP_SIZE_SHORT : NONCE_RSP_SIZE_LONG;
			break;

		case SHA204_RANDOM:
			poll_delay = RANDOM_DELAY;
			poll_timeout = RANDOM_EXEC_MAX - RANDOM_DELAY;
			response_size = RANDOM_RSP_SIZE;
			break;

		case SHA204_SHA:
			poll_delay = SHA_DELAY;
			poll_timeout = SHA_EXEC_MAX - SHA_DELAY;
      response_size = param1 == SHA_INIT
                ? SHA_RSP_SIZE_SHORT : SHA_RSP_SIZE_LONG;
			break;

    case SHA204_WRITE:
      poll_delay = WRITE_DELAY;
      poll_timeout = WRITE_EXEC_MAX - WRITE_DELAY;
      response_size = WRITE_RSP_SIZE;
      break;

		default:
			poll_delay = 0;
			poll_timeout = SHA204_COMMAND_EXEC_MAX;
			response_size = rx_size;
	}

	// Assemble command.
	len = datalen1 + SHA204_CMD_SIZE_MIN;
	p_buffer = tx_buffer;
	*p_buffer++ = len;
	*p_buffer++ = op_code;
	*p_buffer++ = param1;
	*p_buffer++ = param2 & 0xFF;
	*p_buffer++ = param2 >> 8;

	if (datalen1 > 0) {
		memcpy(p_buffer, data1, datalen1);
		p_buffer += datalen1;
	}

	sha204c_calculate_crc(len - SHA204_CRC_SIZE, tx_buffer, p_buffer);

	// Send command and receive response.
	return sha204c_send_and_receive(&tx_buffer[0], response_size,
	 				&rx_buffer[0], poll_delay, poll_timeout);
}

/* CRC Calculator and Checker */

void ATSHA204Class::sha204c_calculate_crc(uint8_t length, uint8_t *data, uint8_t *crc)
{
  uint8_t counter;
  uint16_t crc_register = 0;
  uint16_t polynom = 0x8005;
  uint8_t shift_register;
  uint8_t data_bit, crc_bit;

  for (counter = 0; counter < length; counter++)
  {
    for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) 
    {
      data_bit = (data[counter] & shift_register) ? 1 : 0;
      crc_bit = crc_register >> 15;

      // Shift CRC to the left by 1.
      crc_register <<= 1;

      if ((data_bit ^ crc_bit) != 0)
        crc_register ^= polynom;
    }
  }
  crc[0] = (uint8_t) (crc_register & 0x00FF);
  crc[1] = (uint8_t) (crc_register >> 8);
}

uint8_t ATSHA204Class::sha204c_check_crc(uint8_t *response)
{
  uint8_t crc[SHA204_CRC_SIZE];
  uint8_t count = response[SHA204_BUFFER_POS_COUNT];

  count -= SHA204_CRC_SIZE;
  sha204c_calculate_crc(count, response, crc);

  return (crc[0] == response[count] && crc[1] == response[count + 1])
    ? SHA204_SUCCESS : SHA204_BAD_CRC;
}

#if (ATSHA204I2C)
void ATSHA204Class::dump_configuration()
{
  uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;

  for (int i=0; i < 88; i += 4)
  {
    ret_code = sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, i);
    if (ret_code != 0)
    {
      Serial.print("Failed to read config. Response: "); Serial.println(ret_code, HEX);
      break;
    }
    if (i == 0x00)
    {
      Serial.print("           SN[0:1]           |         SN[2:3]           | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x04)
    {
      Serial.print("                          Revnum                         | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print("   ");
      }
      Serial.print("\n");
    }
    else if (i == 0x08)
    {
      Serial.print("                          SN[4:7]                        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print("   ");
      }
      Serial.print("\n");
    }
    else if (i == 0x0C)
    {
      Serial.print("    SN[8]    |  Reserved13   | I2CEnable | Reserved15    | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x10)
    {
      Serial.print("  I2CAddress |  TempOffset   |  OTPmode  | SelectorMode  | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x14)
    {
      Serial.print("         SlotConfig00        |       SlotConfig01        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x18)
    {
      Serial.print("         SlotConfig02        |       SlotConfig03        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x1C)
    {
      Serial.print("         SlotConfig04        |       SlotConfig05        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x20)
    {
      Serial.print("         SlotConfig06        |       SlotConfig07        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x24)
    {
      Serial.print("         SlotConfig08        |       SlotConfig09        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x28)
    {
      Serial.print("         SlotConfig0A        |       SlotConfig0B        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x2C)
    {
      Serial.print("         SlotConfig0C        |       SlotConfig0D        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x30)
    {
      Serial.print("         SlotConfig0E        |       SlotConfig0F        | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x34)
    {
      Serial.print("  UseFlag00  | UpdateCount00 | UseFlag01 | UpdateCount01 | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x38)
    {
      Serial.print("  UseFlag02  | UpdateCount02 | UseFlag03 | UpdateCount03 | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x3C)
    {
      Serial.print("  UseFlag04  | UpdateCount04 | UseFlag05 | UpdateCount05 | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x40)
    {
      Serial.print("  UseFlag06  | UpdateCount06 | UseFlag07 | UpdateCount07 | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
    else if (i == 0x44)
    {
      Serial.print("                      LastKeyUse[0:3]                    | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print("   ");
      }
      Serial.print("\n");
    }
    else if (i == 0x48)
    {
      Serial.print("                      LastKeyUse[4:7]                    | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print("   ");
      }
      Serial.print("\n");
    }
    else if (i == 0x4C)
    {
      Serial.print("                      LastKeyUse[8:B]                    | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print("   ");
      }
      Serial.print("\n");
    }
    else if (i == 0x50)
    {
      Serial.print("                      LastKeyUse[C:F]                    | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print("   ");
      }
      Serial.print("\n");
    }
    else if (i == 0x54)
    {
      Serial.print("  UserExtra  |    Selector   | LockValue |  LockConfig   | ");
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(" | ");
        }
        else
        {
          Serial.print("   ");
        }
      }
      Serial.print("\n");
    }
  }
}

// Uncomment this to enable locking the configuration zone.
// *** BE AWARE THAT THIS PREVENTS ANY FUTURE CONFIGURATION CHANGE TO THE CHIP ***
// It is still possible to change the key, and this also enable random key generation
#define LOCK_CONFIGURATION

// Uncomment this to enable locking the data zone.
// *** BE AWARE THAT THIS PREVENTS THE KEY TO BE CHANGED ***
// It is not required to lock data, key cannot be retrieved anyway, but by locking
// data, it can be guaranteed that nobody even with physical access to the chip,
// will be able to change the key.
//#define LOCK_DATA

// Uncomment this to skip key storage (typically once key has been written once)
//#define SKIP_KEY_STORAGE

// Uncomment this to skip key data storage (once configuration is locked, key
// will aways randomize)
// Uncomment this to skip key generation and use 'user_key_data' as key instead.
#define USER_KEY_DATA

#ifdef USER_KEY_DATA
const uint8_t user_key_data[32] = SIGNING_HMAC;
#endif

uint16_t ATSHA204Class::calculateAndUpdateCrc(uint8_t length, uint8_t *data, uint16_t current_crc)
{
  uint8_t counter;
  uint16_t crc_register = current_crc;
  uint16_t polynom = 0x8005;
  uint8_t shift_register;
  uint8_t data_bit, crc_bit;

  for (counter = 0; counter < length; counter++)
  {
    for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) 
    {
      data_bit = (data[counter] & shift_register) ? 1 : 0;
      crc_bit = crc_register >> 15;

      // Shift CRC to the left by 1.
      crc_register <<= 1;

      if ((data_bit ^ crc_bit) != 0)
        crc_register ^= polynom;
    }
  }
  return crc_register;
}

uint16_t ATSHA204Class::write_config_and_get_crc()
{
  uint16_t crc = 0;
  uint8_t config_word[4];
  uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;
  bool do_write;

  // We will set default settings from datasheet on all slots. This means that we can use slot 0 for the key
  // as that slot will not be readable (key will therefore be secure) and slot 8 for the payload digest
  // calculationon as that slot can be written in clear text even when the datazone is locked.
  // Other settings which are not relevant are kept as is.

  for (int i=0; i < 88; i += 4)
  {
    do_write = true;
    if (i == 20)
    {
      config_word[0] = 0x8F;
      config_word[1] = 0x80;
      config_word[2] = 0x80;
      config_word[3] = 0xA1;
    }
    else if (i == 24)
    {
      config_word[0] = 0x82;
      config_word[1] = 0xE0;
      config_word[2] = 0xA3;
      config_word[3] = 0x60;
    }
    else if (i == 28)
    {
      config_word[0] = 0x94;
      config_word[1] = 0x40;
      config_word[2] = 0xA0;
      config_word[3] = 0x85;
    }
    else if (i == 32)
    {
      config_word[0] = 0x86;
      config_word[1] = 0x40;
      config_word[2] = 0x87;
      config_word[3] = 0x07;
    }
    else if (i == 36)
    {
      config_word[0] = 0x0F;
      config_word[1] = 0x00;
      config_word[2] = 0x89;
      config_word[3] = 0xF2;
    }
    else if (i == 40)
    {
      config_word[0] = 0x8A;
      config_word[1] = 0x7A;
      config_word[2] = 0x0B;
      config_word[3] = 0x8B;
    }
    else if (i == 44)
    {
      config_word[0] = 0x0C;
      config_word[1] = 0x4C;
      config_word[2] = 0xDD;
      config_word[3] = 0x4D;
    }
    else if (i == 48)
    {
      config_word[0] = 0xC2;
      config_word[1] = 0x42;
      config_word[2] = 0xAF;
      config_word[3] = 0x8F;
    }
    else if (i == 52 || i == 56 || i == 60 || i == 64)
    {
      config_word[0] = 0xFF;
      config_word[1] = 0x00;
      config_word[2] = 0xFF;
      config_word[3] = 0x00;
    }
    else if (i == 68 || i == 72 || i == 76 || i == 80)
    {
      config_word[0] = 0xFF;
      config_word[1] = 0xFF;
      config_word[2] = 0xFF;
      config_word[3] = 0xFF;
    }
    else
    {
      // All other configs are untouched
      ret_code = sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, i);
      if (ret_code != SHA204_SUCCESS)
      {
        Serial.print("Failed to read config. Response: ");
        Serial.println(ret_code, HEX);
        return 0; //halt();
      }
      // Set config_word to the read data
      config_word[0] = rx_buffer[SHA204_BUFFER_POS_DATA+0];
      config_word[1] = rx_buffer[SHA204_BUFFER_POS_DATA+1];
      config_word[2] = rx_buffer[SHA204_BUFFER_POS_DATA+2];
      config_word[3] = rx_buffer[SHA204_BUFFER_POS_DATA+3];
      do_write = false;
    }

    // Update crc with CRC for the current word
    crc = calculateAndUpdateCrc(4, config_word, crc);

    // Write config word
    if (do_write)
    {
      ret_code = sha204m_execute(SHA204_WRITE, SHA204_ZONE_CONFIG,
                                i >> 2, 4, config_word,                                 WRITE_COUNT_SHORT, tx_buffer, WRITE_RSP_SIZE, rx_buffer);
      if (ret_code != SHA204_SUCCESS)
      {
        Serial.print("Failed to write config word at address ");
        Serial.print(i);
        Serial.print(". Response: ");
        Serial.println(ret_code, HEX);
        return 0; //halt();
      }
    }
  }
  return crc;
}

void ATSHA204Class::write_key(uint8_t* key)
{
  uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;

  // Write key to slot 0
  ret_code = sha204m_execute(SHA204_WRITE, SHA204_ZONE_DATA | SHA204_ZONE_COUNT_FLAG,
                            0, SHA204_ZONE_ACCESS_32, key,                            WRITE_COUNT_LONG, tx_buffer, WRITE_RSP_SIZE, rx_buffer);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print("Failed to write key to slot 0. Response: ");
    Serial.println(ret_code, HEX);
    return; //halt();
  }
}

#define SHA204_CMD_SIZE_MIN          ((uint8_t)  7)  //! minimum number of bytes in command (from count byte to second CRC byte)
// DevRev command definitions
#define DEVREV_PARAM1_IDX               SHA204_PARAM1_IDX      //!< DevRev command index for 1. parameter (ignored)
#define DEVREV_PARAM2_IDX               SHA204_PARAM2_IDX      //!< DevRev command index for 2. parameter (ignored)
#define DEVREV_COUNT                    SHA204_CMD_SIZE_MIN    //!< DevRev command packet size
#define SHA204_DEVREV                   ((uint8_t) 0x30)       //!< DevRev command op-code
#define DEVREV_DELAY                    ((uint8_t) 0.4)
#define DEVREV_EXEC_MAX                  ((uint8_t) 2.0)
#define SHA204_RSP_SIZE_VAL             ((uint8_t)  7)         //!< size of response packet containing four bytes of data
#define DEVREV_RSP_SIZE                 SHA204_RSP_SIZE_VAL    //!< response size of DevRev command returns 4 bytes
#define SHA204_LOCK                     ((uint8_t) 0x17)       //!< Lock command op-code
#define LOCK_COUNT                      SHA204_CMD_SIZE_MIN    //!< Lock command packet size
#define LOCK_RSP_SIZE                   SHA204_RSP_SIZE_MIN    //!< response size of Lock command

uint8_t ATSHA204Class::dev_rev(uint8_t *tx_buffer, uint8_t *rx_buffer) {
    if (!tx_buffer || !rx_buffer)
        return SHA204_BAD_PARAM;

    tx_buffer[SHA204_COUNT_IDX] = DEVREV_COUNT;
    tx_buffer[SHA204_OPCODE_IDX] = SHA204_DEVREV;

    // Parameters are 0.
    tx_buffer[DEVREV_PARAM1_IDX] =
        tx_buffer[DEVREV_PARAM2_IDX] =
            tx_buffer[DEVREV_PARAM2_IDX + 1] = 0;

    return sha204c_send_and_receive(&tx_buffer[0], DEVREV_RSP_SIZE, &rx_buffer[0],
                            DEVREV_DELAY, DEVREV_EXEC_MAX - DEVREV_DELAY);
}

void ATSHA204Class::personalize(void)
{
  uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t key[32];
  uint8_t ret_code;
  uint8_t lockConfig = 0;
  uint8_t lockValue = 0;
  uint16_t crc;

  Serial.println("ATSHA204 personalization sketch for MySensors usage.");
  Serial.println("----------------------------------------------------");

  // Wake device before starting operations
  ret_code = sha204c_wakeup(rx_buffer);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print("Failed to wake device. Response: ");
    Serial.println(ret_code, HEX);
    //return; //halt();
  }
  
  // Output device revision on console
  ret_code = dev_rev(tx_buffer, rx_buffer);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print("Failed to determine device revision. Response: ");
    Serial.println(ret_code, HEX);
    //return; //halt();
  }
  else
  {
    Serial.print("Device revision: ");
    for (int i=0; i<4; i++)
    {
      if (rx_buffer[SHA204_BUFFER_POS_DATA+i] < 0x10)
      {
        Serial.print('0'); // Because Serial.print does not 0-pad HEX
      }
      Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+i], HEX);
    }
    Serial.println();
  }

  // Output serial number on console
  getSerialNumber(rx_buffer);
  {
    Serial.print("Device serial:   ");
    Serial.print('{');
    for (int i=0; i<9; i++)
    {
      Serial.print("0x");
      if (rx_buffer[i] < 0x10)
      {
        Serial.print('0'); // Because Serial.print does not 0-pad HEX
      }
      Serial.print(rx_buffer[i], HEX);
      if (i < 8) Serial.print(',');
    }
    Serial.print('}');
    Serial.println();
    for (int i=0; i<9; i++)
    {
      if (rx_buffer[i] < 0x10)
      {
        Serial.print('0'); // Because Serial.print does not 0-pad HEX
      }
      Serial.print(rx_buffer[i], HEX);
    }
    Serial.println();
  }

  // Read out lock config bits to determine if locking is possible
  ret_code = sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print("Failed to determine device lock status. Response: "); Serial.println(ret_code, HEX);
    //return; //halt();
  }
  else
  {
    lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
    lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
  }

    //TODO List current configuration before attempting to lock
    Serial.println("Old chip configuration:");
    dump_configuration();

  if (lockConfig != 0x00)
  {
    // Write config and get CRC for the updated config
    crc = write_config_and_get_crc();

    // List current configuration before attempting to lock
    Serial.println("Chip configuration:");
    dump_configuration();

#ifdef LOCK_CONFIGURATION
    {
      Serial.println("Locking configuration...");

      // Correct sequence, resync chip
      ret_code = sha204c_resync(SHA204_RSP_SIZE_MAX, rx_buffer);
      if (ret_code != SHA204_SUCCESS && ret_code != SHA204_RESYNC_WITH_WAKEUP)
      {
        Serial.print("Resync failed. Response: "); Serial.println(ret_code, HEX);
        return; //halt();
      }

      // Lock configuration zone
      ret_code = sha204m_execute(SHA204_LOCK, SHA204_ZONE_CONFIG,
                                crc, 0, NULL, 
                                LOCK_COUNT, tx_buffer, LOCK_RSP_SIZE, rx_buffer);
      if (ret_code != SHA204_SUCCESS)
      {
        Serial.print("Configuration lock failed. Response: "); Serial.println(ret_code, HEX);
        return; //halt();
      }
      else
      {
        Serial.println("Configuration locked.");

        // Update lock flags after locking
        ret_code = sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
        if (ret_code != SHA204_SUCCESS)
        {
          Serial.print("Failed to determine device lock status. Response: "); Serial.println(ret_code, HEX);
          return; //halt();
        }
        else
        {
          lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
          lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
        }
      }
    }
#else //LOCK_CONFIGURATION
    Serial.println("Configuration not locked. Define LOCK_CONFIGURATION to lock for real.");
#endif
  }
  else
  {
    Serial.println("Skipping configuration write and lock (configuration already locked).");
    Serial.println("Chip configuration:");
    dump_configuration();
  }

#ifdef SKIP_KEY_STORAGE
  Serial.println("Disable SKIP_KEY_STORAGE to store key.");
#else
#ifdef USER_KEY_DATA
  memcpy(key, user_key_data, 32);
  Serial.println("Using this user supplied key:");
#else
  // Retrieve random value to use as key
  ret_code = sha204m_random(tx_buffer, rx_buffer, RANDOM_SEED_UPDATE);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print("Random key generation failed. Response: "); Serial.println(ret_code, HEX);
    //return; //halt();
  }
  else
  {
    memcpy(key, rx_buffer+SHA204_BUFFER_POS_DATA, 32);
  }
  if (lockConfig == 0x00)
  {
    Serial.println("Take note of this key, it will never be the shown again:");
  }
  else
  {
    Serial.println("Key is not randomized (configuration not locked):");
  }
#endif
  Serial.print("#define MY_HMAC_KEY ");
  for (int i=0; i<32; i++)
  {
    Serial.print("0x");
    if (key[i] < 0x10)
    {
      Serial.print('0'); // Because Serial.print does not 0-pad HEX
    }
    Serial.print(key[i], HEX);
    if (i < 31) Serial.print(',');
    if (i+1 == 16) Serial.print("\\\n                    ");
  }
  Serial.println();

  // It will not be possible to write the key if the configuration zone is unlocked
  if (lockConfig == 0x00)
  {
    // Write the key to the appropriate slot in the data zone
    Serial.println("Writing key to slot 0...");
    write_key(key);
  }
  else
  {
    Serial.println("Skipping key storage (configuration not locked).");
    Serial.println("The configuration must be locked to be able to write a key.");
  }  
#endif

  if (lockValue != 0x00)
  {
#ifdef LOCK_DATA
    {
      // Correct sequence, resync chip
      ret_code = sha204c_resync(SHA204_RSP_SIZE_MAX, rx_buffer);
      if (ret_code != SHA204_SUCCESS && ret_code != SHA204_RESYNC_WITH_WAKEUP)
      {
        Serial.print("Resync failed. Response: "); Serial.println(ret_code, HEX);
        return; //halt();
      }

      // If configuration is unlocked, key is not updated. Locking data in this case will cause
      // slot 0 to contain an unknown (or factory default) key, and this is in practically any
      // usecase not the desired behaviour, so ask for additional confirmation in this case.
      if (lockConfig != 0x00)
      {
        while (Serial.available())
        {
          Serial.read();
        }
        Serial.println("*** ATTENTION ***");
        Serial.println("Configuration is not locked. Are you ABSULOUTELY SURE you want to lock data?");
        Serial.println("Locking data at this stage will cause slot 0 to contain a factory default key");
        Serial.println("which cannot be change after locking is done. This is in practically any usecase");
        Serial.println("NOT the desired behavour. Send SPACE character now to lock data anyway...");
        while (Serial.available() == 0);
        if (Serial.read() != ' ')
        {
          Serial.println("Unexpected answer. Skipping lock.");
          return; //halt();
        }
      }

      // Lock data zone
      ret_code = sha204m_execute(SHA204_LOCK, SHA204_ZONE_DATA | LOCK_ZONE_NO_CRC,
                                        0x0000, 0, NULL, 0, NULL, 0, NULL,
                                        LOCK_COUNT, tx_buffer, LOCK_RSP_SIZE, rx_buffer);
      if (ret_code != SHA204_SUCCESS)
      {
        Serial.print("Data lock failed. Response: "); Serial.println(ret_code, HEX);
        return; //halt();
      }
      else
      {
        Serial.println("Data locked.");

        // Update lock flags after locking
        ret_code = sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
        if (ret_code != SHA204_SUCCESS)
        {
          Serial.print("Failed to determine device lock status. Response: "); Serial.println(ret_code, HEX);
          return; //halt();
        }
        else
        {
          lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
          lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
        }
      }
    }
#else //LOCK_DATA
    Serial.println("Data not locked. Define LOCK_DATA to lock for real.");
#endif
  }
  else
  {
    Serial.println("Skipping OTP/data zone lock (zone already locked).");
  }

  Serial.println("--------------------------------");
  Serial.println("Personalization is now complete.");
  Serial.print("Configuration is ");
  if (lockConfig == 0x00)
  {
    Serial.println("LOCKED");
  }
  else
  {
    Serial.println("UNLOCKED");
  }
  Serial.print("Data is ");
  if (lockValue == 0x00)
  {
    Serial.println("LOCKED");
  }
  else
  {
    Serial.println("UNLOCKED");
  }
}

#endif

//#endif // defined(ARDUINO_ARCH_AVR)
