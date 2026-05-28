#include "STM8S003F3.h"
#include "ds3231.h"
#include "i2c.h"

void ds3231Init(void)
{
	i2c_init();
	// дописать инициализацию, особенно часов, чтобы был только 24 часа формат времени
}

void ds3231_read_time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours)
{
	if (i2c_busy_check() == OK){
		i2c_start();
		i2c_shift(DS3231_ADDRESS);
		if (i2c_ack(READ_ANSWER)==ACK)
		{
			i2c_shift(SECONDS_ADDR);
			if (i2c_ack(READ_ANSWER)==ACK)
			{
				i2c_start();
				i2c_shift(DS3231_ADDRESS | 0x01);
				if (i2c_ack(READ_ANSWER)==ACK)
				{
					*seconds = i2c_shift(0xFF); i2c_ack(SET_ACK);
					*minutes = i2c_shift(0xFF); i2c_ack(SET_ACK);
					*hours = i2c_shift(0xFF); 	i2c_ack(SET_NAK);
					i2c_stop();
					return;
				}
			}
		}
	}else{
		i2c_slave_unlock();
		return;
	}
	i2c_stop();
}

void ds3231_write_time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours)
{
	if (i2c_busy_check() == OK){
		i2c_start();
		i2c_shift(DS3231_ADDRESS);
		if (i2c_ack(READ_ANSWER)==ACK)
		{
			i2c_shift(SECONDS_ADDR);
			if (i2c_ack(READ_ANSWER)==ACK)
			{
				i2c_shift(*seconds); 
				if (i2c_ack(READ_ANSWER)==ACK)
				{
					i2c_shift(*minutes); 
					if (i2c_ack(READ_ANSWER)==ACK)
					{
						i2c_shift(*hours); 
						if (i2c_ack(READ_ANSWER)==ACK)
						{
							i2c_stop();
							return;
						}
					}
				}
			}
		}
	}else{
		i2c_slave_unlock();
		return;
	}
	i2c_stop();
}