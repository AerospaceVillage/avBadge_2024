#ifndef __SUNXI_DSP_IHEX_H
#define __SUNXI_DSP_IHEX_H

enum ihex_record_type {
	RECORD_DATA = 0x00,
	RECORD_EOF = 0x01,
	RECORD_EXT_SEG_ADDR = 0x02,
	RECORD_START_SEG_ADDR = 0x03,
	RECORD_EXT_LINEAR_ADDR = 0x04,
	RECORD_START_LINEAR_ADDR = 0x05
};
#define RECORD_TYPE_MAX RECORD_START_LINEAR_ADDR
#define RECORD_MAX_LEN 255

enum ihex_decode_state {
	STATE_START_CODE,
	STATE_BYTE_COUNT,
	STATE_ADDRESS0,
	STATE_ADDRESS1,
	STATE_RECORD_TYPE,
	STATE_DATA,
	STATE_CHECKSUM
};

typedef struct ihex_decode_cfg {
	int (*handle_data_record)(struct ihex_decode_cfg *cfg, u32 addr,
				  unsigned char *data, size_t len);
	int (*handle_start_addr_record)(struct ihex_decode_cfg *cfg, u32 addr);
	void *arg;

	// Filled in by decode_ihex function
	unsigned int decoded_record_count;
	const char *error_msg;
} ihex_decode_cfg_t;

static int decode_hexchar(unsigned char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	} else {
		return -1;
	}
}

static int decode_ihex(ihex_decode_cfg_t *cfg, const unsigned char *data,
		       size_t len)
{
	// Decode Tracking
	int ret;
	const unsigned char *end = data + len;
	enum ihex_decode_state state = STATE_START_CODE;

	// Inter-record state
	unsigned char found_eof_record = 0;
	u32 base_address = 0;

	// Per-Record State
	u16 record_addr = 0;
	enum ihex_record_type record_type = 0;
	unsigned char record_computed_checksum = 0;
	unsigned char record_data_len = 0;
	unsigned char record_data_idx = 0;
	unsigned char record_data[RECORD_MAX_LEN];

	while (data < end) {
		// Character decoding
		unsigned char found_start_code = 0;

		// Decode byte if needed
		unsigned char byte = 0;
		if (state == STATE_START_CODE) {
			// As per spec, ignore all characters before start code
			if (*data++ == ':') {
				if (found_eof_record) {
					cfg->error_msg =
						"Record after EOF record";
					return -EINVAL;
				}
				found_start_code = 1;
			}
		} else {
			// All other states just read a pair of hex characters
			int val;
			val = decode_hexchar(*data++);
			if (val < 0) {
				cfg->error_msg = "Invalid hex char";
				return -EINVAL;
			}
			byte = val << 4;

			if (data >= end) {
				cfg->error_msg = "Unexpected end of file";
				return -EINVAL;
			}

			val = decode_hexchar(*data++);
			if (val < 0) {
				cfg->error_msg = "Invalid hex char";
				return -EINVAL;
			}
			byte |= val;
		}

		// Perform action based on decoded data
		switch (state) {
		case STATE_START_CODE:
			// If we got a start code, begin decoding the record
			if (found_start_code) {
				cfg->decoded_record_count++;
				record_computed_checksum = 0;
				state = STATE_BYTE_COUNT;
			}
			break;
		case STATE_BYTE_COUNT:
			record_data_len = byte;
			record_computed_checksum += byte;
			state = STATE_ADDRESS0;
			break;
		case STATE_ADDRESS0:
			record_addr = base_address + (byte << 8);
			record_computed_checksum += byte;
			state = STATE_ADDRESS1;
			break;
		case STATE_ADDRESS1:
			record_addr += byte;
			record_computed_checksum += byte;
			state = STATE_RECORD_TYPE;
			break;
		case STATE_RECORD_TYPE:
			if (byte > RECORD_TYPE_MAX) {
				cfg->error_msg = "Invalid record type";
				return -EINVAL;
			}

			record_type = byte;
			record_data_idx = 0;
			record_computed_checksum += byte;
			if (record_data_len > 0) {
				state = STATE_DATA;
			} else {
				state = STATE_CHECKSUM;
			}

			break;
		case STATE_DATA:
			record_data[record_data_idx++] = byte;
			record_computed_checksum += byte;
			if (record_data_idx == record_data_len) {
				state = STATE_CHECKSUM;
			}
			break;
		case STATE_CHECKSUM:
			// Take twos complement of computed checksum
			record_computed_checksum = ~record_computed_checksum;
			record_computed_checksum++;
			if (record_computed_checksum != byte) {
				cfg->error_msg = "Invalid checksum";
				return -EINVAL;
			}

			// Handle the record
			switch (record_type) {
			case RECORD_DATA:
				// Only process non-empty data records
				if (record_data_len > 0) {
					ret = cfg->handle_data_record(
						cfg, base_address + record_addr,
						record_data, record_data_len);
					if (ret) {
						return ret;
					}
				}
				break;
			case RECORD_EOF:
				if (record_data_len != 0) {
					cfg->error_msg =
						"Invalid EOF Record Data Len";
					return -EINVAL;
				}
				found_eof_record = 1;
				break;
			case RECORD_EXT_SEG_ADDR:
				if (record_data_len != 2) {
					cfg->error_msg =
						"Invalid Extended Segment Address Record Length";
					return -EINVAL;
				}
				// Extended Segment Address is the 16-bit segment base address
				// This is the 16-bit data record (in MSB form) multiplied by 16
				// This is added to all subsequent data records allowing addressing up to 1MB
				base_address = (((u32)record_data[0]) << 8) +
					       ((u32)record_data[1]);
				base_address *= 16;
				break;
			case RECORD_START_SEG_ADDR:
				cfg->error_msg =
					"Unsupported Record Type: Start Segment Addr";
				return -EINVAL;
			case RECORD_EXT_LINEAR_ADDR:
				if (record_data_len != 2) {
					cfg->error_msg =
						"Invalid Extended Linear Address Record Length";
					return -EINVAL;
				}
				// The 16-bit data record is the upper 16-bits of the 32-bit absolute address
				base_address = (((u32)record_data[0]) << 24) +
					       (((u32)record_data[1]) << 16);
				break;
			case RECORD_START_LINEAR_ADDR:
				if (record_data_len != 4) {
					cfg->error_msg =
						"Invalid Start Linear Address Record Length";
					return -EINVAL;
				}
				u32 start_addr = (((u32)record_data[0]) << 24) +
						 (((u32)record_data[1]) << 16) +
						 (((u32)record_data[2]) << 8) +
						 ((u32)record_data[3]);
				ret = cfg->handle_start_addr_record(cfg,
								    start_addr);
				if (ret) {
					return ret;
				}
				break;
			};

			state = STATE_START_CODE;
		}
	}

	// Make sure we saw the end of record
	if (!found_eof_record) {
		cfg->error_msg = "Unexpected end of file";
		return -EINVAL;
	}

	return 0;
}

#endif
