/***************************************************************************//**
 *   @file   AD9250.c
 *   @brief  Implementation of AD9250 Driver.
 *   @author DNechita (Dan.Nechita@analog.com)
 ********************************************************************************
 * Copyright 2013(c) Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/

#include <stdlib.h>
#include "ad9250.h"
#include "no_os_error.h"
#include "no_os_alloc.h"

const int32_t shadow_regs[SHADOW_REGISTER_COUNT] = {
	0,
	0x01, // AD9250_SHD_REG_CLOCK
	0x00, // AD9250_SHD_REG_CLOCK_DIV
	0x00, // AD9250_SHD_REG_TEST
	0x00, // AD9250_SHD_REG_BIST
	0x00, // AD9250_SHD_REG_OFFSET
	0x01, // AD9250_SHD_REG_OUT_MODE
	0x00, // AD9250_SHD_REG_VREF
	0x00, // AD9250_SHD_REG_SYS_CTRL
	0x00, // AD9250_SHD_REG_DCC_CTRL
	0x00, // AD9250_SHD_REG_DCC_VAL
	0x00, // AD9250_SHD_REG_FAST_DETECT
	0x00, // AD9250_SHD_REG_FD_UPPER_THD
	0x00, // AD9250_SHD_REG_FD_LOWER_THD
	0x00  // AD9250_SHD_REG_FD_DWELL_TIME
};

int32_t ad9250_set_bits_to_reg(struct ad9250_dev *dev,
			       uint32_t register_address,
			       uint8_t bits_value,
			       uint8_t mask);

int32_t ad9250_is_shadow_register(int32_t register_address);

/***************************************************************************//**
 * @brief Configures the device.
 *
 * @param device     - The device structure.
 * @param init_param - The structure that contains the device initial
 * 		       parameters.
 *
 * @return Returns negative error code or 0 in case of success.
 *******************************************************************************/
int32_t ad9250_setup(struct ad9250_dev **device,
		     struct ad9250_init_param init_param)
{
	struct ad9250_dev *dev;
	int32_t ret, i;

	dev = (struct ad9250_dev *)no_os_malloc(sizeof(*dev));
	if (!dev)
		return -1;

	/* Initializes registers */
	for (i = 0; i < SHADOW_REGISTER_COUNT; i++)
		dev->shadow_regs[i] = shadow_regs[i];

	/* Initializes the SPI peripheral */
	ret = no_os_spi_init(&dev->spi_desc, &init_param.spi_init);
	if (ret == -1) {
		return ret;
	}

	/* Reset ad9250 registers to their default values. */
	ad9250_soft_reset(dev);

	/* Configure the AD9250 device. */
	dev->ad9250_st = init_param.ad9250_st_init;
	ret = ad9250_write(dev,
			   AD9250_REG_SPI_CFG,
			   AD9250_SPI_CFG_SOFT_RST);
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_PDWN,
				     dev->ad9250_st.pdata->extrn_pdwnmode * AD9250_PDWN_EXTERN,
				     AD9250_PDWN_EXTERN);
	if (ret == -1) {
		return ret;
	}

	ret = ad9250_write(dev,
			   AD9250_REG_CLOCK,
			   dev->ad9250_st.pdata->en_clk_dcs * AD9250_CLOCK_DUTY_CYCLE |
			   AD9250_CLOCK_SELECTION(dev->ad9250_st.pdata->clk_selection));
	if (ret == -1) {
		return ret;
	}

	ret = ad9250_write(dev,
			   AD9250_REG_CLOCK_DIV,
			   AD9250_CLOCK_DIV_RATIO(dev->ad9250_st.pdata->clk_div_ratio) |
			   AD9250_CLOCK_DIV_PHASE(dev->ad9250_st.pdata->clk_div_phase));
	if (ret == -1) {
		return ret;
	}

	ret = ad9250_write(dev,
			   AD9250_REG_VREF,
			   AD9250_VREF_FS_ADJUST(dev->ad9250_st.pdata->adc_vref));
	if (ret == -1) {
		return ret;
	}

	ret = ad9250_write(dev,
			   AD9250_REG_PLL_ENCODE,
			   AD9250_PLL_ENCODE(dev->ad9250_st.pdata->pll_low_encode));
	if (ret == -1) {
		return ret;
	}

	/* Synchronously update registers. */
	ret = ad9250_transfer(dev);
	if (ret == -1) {
		return ret;
	}

	/* Configure the JESD204B interface. */
	ret = ad9250_jesd204b_setup(dev);
	if (ret == -1) {
		return ret;
	}

	/* Configure the Fast-detect circuit. */
	ret = ad9250_fast_detect_setup(dev);
	if (ret == -1) {
		return ret;
	}

	/* Synchronously update registers. */
	ret = ad9250_transfer(dev);

	*device = dev;

	return ret;
}

/***************************************************************************//**
 * @brief Free the resources allocated by ad9250_setup().
 *
 * @param dev - The device structure.
 *
 * @return 0 in case of success, negative error code otherwise.
*******************************************************************************/
int32_t ad9250_remove(struct ad9250_dev *dev)
{
	int32_t ret;

	ret = no_os_spi_remove(dev->spi_desc);

	no_os_free(dev);

	return ret;
}

/***************************************************************************//**
 * @brief Reads the value of the selected register.
 *
 * @param dev              - The device structure.
 * @param register_address - The address of the register to read.
 *
 * @return reg_value  - The register's value or negative error code.
 *******************************************************************************/
int32_t ad9250_read(struct ad9250_dev *dev,
		    int32_t register_address)
{
	uint32_t reg_address = 0;
	uint8_t buffer [ 3 ]   = {0, 0, 0};
	uint32_t reg_value = 0;
	uint8_t i = 0;
	int32_t ret = 0;

	reg_address = AD9250_READ + AD9250_ADDR(register_address);
	for (i = 0; i < AD9250_TRANSF_LEN(register_address); i++) {
		buffer[0] = (reg_address & 0xFF00) >> 8;
		buffer[1] = reg_address & 0x00FF;
		buffer[2] = 0;
		ret = no_os_spi_write_and_read(dev->spi_desc,
					       buffer,
					       3);
		if (ret == -1) {
			return ret;
		}
		reg_address--;
		reg_value <<= 8;
		reg_value |= buffer[2];
	}

	return reg_value;
}

/***************************************************************************//**
 * @brief Writes a value to the selected register.
 *
 * @param dev              - The device structure.
 * @param register_address - The address of the register to write to.
 * @param register_value   - The value to write to the register.
 *
 * @return Returns 0 in case of success or negative error code.
 *******************************************************************************/
int32_t ad9250_write(struct ad9250_dev *dev,
		     int32_t register_address,
		     int32_t register_value)
{
	uint8_t i = 0;
	int32_t ret = 0;
	uint16_t reg_address = 0;
	int8_t reg_value = 0;
	uint8_t tx_buffer[3] = {0, 0, 0};

	/* Check if the register is shadowed. */
	ret = ad9250_is_shadow_register(register_address);
	/* Synchronize shadow register with on-chip register. */
	if (ret > 0) {
		dev->shadow_regs[ret] = register_value;
	}
	reg_address = AD9250_WRITE + AD9250_ADDR(register_address);
	for (i = 0; i < AD9250_TRANSF_LEN(register_address); i++) {
		reg_value = (register_value >>
			     ((AD9250_TRANSF_LEN(register_address) - i - 1) * 8)) & 0xFF;
		tx_buffer[0] = (reg_address & 0xFF00) >> 8;
		tx_buffer[1] = reg_address & 0x00FF;
		tx_buffer[2] = reg_value;
		ret = no_os_spi_write_and_read(dev->spi_desc,
					       tx_buffer,
					       3);
		if (ret == -1) {
			return ret;
		}
		reg_address--;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Initiates a transfer and waits for the operation to end.
 *	  Note: This function may be called after a shadowed register was written,
 *		so that the internal update can actually take place.
 *
 * @param dev - The device structure.
 *
 * @return  Negative error code or 0 in case of success.
 *******************************************************************************/
int32_t ad9250_transfer(struct ad9250_dev *dev)
{
	int32_t timeout = 0xFFFF;
	int32_t ret = 0;
	int8_t sw_bit = 0;

	ret = ad9250_write(dev,
			   AD9250_REG_DEVICE_UPDATE,
			   AD9250_DEVICE_UPDATE_SW);
	if (ret == -1) {
		return ret;
	}
	do {
		ret = ad9250_read(dev,
				  AD9250_REG_DEVICE_UPDATE);
		if (ret == -1) {
			return ret;
		}
		sw_bit = ret & AD9250_REG_DEVICE_UPDATE;
		timeout--;
	} while ((sw_bit == 1) && (timeout != 0));

	return 0;
}

/***************************************************************************//**
 * @brief Resets all registers to their default values.
 *
 * @param dev - The device structure.
 *
 * @return Returns negative error code or 0 in case of success.
 *******************************************************************************/
int32_t ad9250_soft_reset(struct ad9250_dev *dev)
{
	int32_t timeout = 0xFFFF;
	int32_t ret = 0;

	/* Software reset to default SPI values. */
	ret = ad9250_write(dev,
			   AD9250_REG_SPI_CFG,
			   AD9250_SPI_CFG_SOFT_RST);
	if (ret == -1) {
		return ret;
	}
	do {
		ret = ad9250_read(dev,
				  AD9250_REG_SPI_CFG);
		if (ret == -1) {
			return ret;
		}
		timeout--;
	} while (((ret & AD9250_SPI_CFG_SOFT_RST) != 0) && (timeout != 0));

	return ret;
}

/***************************************************************************//**
 * @brief Sets a bit/group of bits inside a register without modifying other
 *	  bits.
 *
 * @param dev              - The device structure.
 * @param register_address - The address of the register to be written.
 * @param bits_value	   - The value of the bit/bits.
 * @param mask		   - The bit/bits position in the register.
 *
 * @return Returns negative error code or 0 in case of success.
 *******************************************************************************/
int32_t ad9250_set_bits_to_reg(struct ad9250_dev *dev,
			       uint32_t register_address,
			       uint8_t  bits_value,
			       uint8_t  mask)
{
	uint8_t reg_value = 0;
	int32_t ret = 0;

	/* Read from the shadow register instead of the on-chip register when
	   shadowed register is discovered. */
	ret = ad9250_is_shadow_register(register_address);
	if (ret > 0) {
		reg_value = dev->shadow_regs[ret];
	} else {
		ret = ad9250_read(dev,
				  register_address);
		if (ret == -1) {
			return ret;
		}
		reg_value = ret;
	}
	reg_value &= (~mask);
	reg_value |= bits_value;
	ret = ad9250_write(dev,
			   register_address,
			   reg_value);
	if (ret == -1) {
		return ret;
	}

	return 0;
}

/***************************************************************************//**
 * @brief Checks if the register is shadowed.
 *
 * @param register_address - The address of the register to be checked.
 *
 * @return Returns the index of the shadow register or 0 if the register is not
 *	   shadowed.
 *******************************************************************************/
int32_t ad9250_is_shadow_register(int32_t register_address)
{
	return ((SHADOW(0xFF) & register_address) >> 16);
}

/***************************************************************************//**
 * @brief Configures the power mode of the chip.
 *
 * @param dev  - The device structure.
 * @param mode - The power mode.
 *		   Example: 00 � normal operation(default);
 *			    01 � power-down;
 *			    10 - standby.
 *
 * @return Returns negative error code or the set power mode.
 *******************************************************************************/
int32_t ad9250_chip_pwr_mode(struct ad9250_dev *dev,
			     int32_t mode)
{
	uint32_t ret = 0;

	if ((mode >= 0) && (mode < 3)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_PDWN,
					     AD9250_PDWN_CHIP(mode),
					     AD9250_PDWN_CHIP(0x3));
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_PDWN);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_PDWN_CHIP(0x3)) >> 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Selects a channel as the current channel for further configurations.
 *
 * @param dev     - The device structure.
 * @param channel - Channel option.
 *		    Example 1 - channel A
 *			    2 - channel B
 *			    3 - channel A and channel B
 *
 * @return Returns negative error code or the selected channel.
 *******************************************************************************/
int32_t ad9250_select_channel_for_config(struct ad9250_dev *dev,
		int32_t channel)
{
	int32_t ret = 0;

	if ((channel > 0) && (channel <= 3)) {
		ret = ad9250_write(dev,
				   AD9250_REG_CH_INDEX,
				   channel);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_CH_INDEX);
		if (ret == -1) {
			return ret;
		}
		return ret & (AD9250_CH_INDEX_ADC_A | AD9250_CH_INDEX_ADC_B);
	}

	return ret;
}

/***************************************************************************//**
 * @brief Sets the ADC's test mode.
 *
 * @param dev  - The device structure.
 * @param mode - ADC test mode.
 *		 Example: 0 -> off(default)
 *			  1 -> midscale short
 *			  2 -> +FS short
 *			  3 -> -FS short
 *			  4 -> checkerboard output
 *			  5 -> PN sequence long
 *			  6 -> PN 9 short
 *			  7 -> one/zero word toggle
 *			  8 -> user test mode
 *		    9 to 14 -> unused
 *			 15 -> ramp output
 *
 * @return Returns the set test mode or negative error code.
 *******************************************************************************/
int32_t ad9250_test_mode(struct ad9250_dev *dev,
			 int32_t mode)
{
	int32_t ret = 0;

	if ((mode >= 0) && (mode < 16)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_TEST,
					     AD9250_TEST_OUTPUT_TEST(mode),
					     AD9250_TEST_OUTPUT_TEST(0xF));
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_TEST);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_TEST_OUTPUT_TEST(0xF)) >> 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Sets the offset adjustment.
 *
 * @param dev - The device structure.
 * @param adj - The offset adjust value in LSBs from +31 to -32.
 *
 * @return Returns negative error code or the set offset adjustment.
 *******************************************************************************/
int32_t ad9250_offset_adj(struct ad9250_dev *dev,
			  int32_t adj)
{
	int32_t ret = 0;

	if ((adj >= -32) && (adj <= 31)) {
		ret = ad9250_write(dev,
				   AD9250_REG_OFFSET,
				   AD9250_REG_OFFSET_ADJUST(adj));
	} else {
		return (ad9250_read(dev,
				    AD9250_REG_OFFSET));
	}

	return ret;
}

/***************************************************************************//**
 * @brief Disables (1) or enables (0) the data output.
 *	  Note: This function modifies a shadowed register, therefore a call of
 *		ad9250_transfer() is required for the internal update to take
 *		place.
 *
 * @param dev - The device structure.
 * @param en  - Enable option.
 *		 Example: 1 - Disables the data output;
 *			  0 - Enables the data output(default).
 *
 * @return  Returns negative error code or the output disable state.
 *******************************************************************************/
int32_t ad9250_output_disable(struct ad9250_dev *dev,
			      int32_t en)
{
	uint32_t ret = 0;

	if ((en == 0) || (en == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_OUT_MODE,
					     en * AD9250_OUT_MODE_DISABLE,
					     AD9250_OUT_MODE_DISABLE);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_OUT_MODE);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_OUT_MODE_DISABLE) != 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Activates the inverted (1) or normal (0) output mode.
 *	  Note: This function modifies a shadowed register, therefore a call of
 *		ad9250_transfer() is required for the internal update to take
 *		place.
 *
 * @param dev    - The device structure.
 * @param invert - Invert option.
 *		     Example: 1 - Activates the inverted output mode;
 *			      0 - Activates the normal output mode(default).
 *
 * @return Returns negative error code or the set output mode.
 *******************************************************************************/
int32_t ad9250_output_invert(struct ad9250_dev *dev,
			     int32_t invert)
{
	uint32_t ret = 0;

	if ((invert == 0) || (invert == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_OUT_MODE,
					     invert * AD9250_OUT_MODE_INVERT_DATA,
					     AD9250_OUT_MODE_INVERT_DATA);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_OUT_MODE);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_OUT_MODE_INVERT_DATA) != 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Specifies the output format.
 *	  Note: This function modifies a shadowed register, therefore a call of
 *		ad9250_transfer() is required for the internal update to take
 *		place.
 *
 * @param dev    - The device structure.
 * @param format - The output format.
 *		     Example: 0 � offset binary(default);
 *			      1 � two's complement;
 *
 * @return Returns negative error code or the set output format.
 *******************************************************************************/
int32_t ad9250_output_format(struct ad9250_dev *dev,
			     int32_t format)
{
	uint32_t ret = 0;

	if ((format == 0) || (format == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_OUT_MODE,
					     AD9250_OUT_MODE_DATA_FORMAT(format),
					     AD9250_OUT_MODE_DATA_FORMAT(1));
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_OUT_MODE);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_OUT_MODE_DATA_FORMAT(1)) >> 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Sets (1) or clears (0) the reset short PN sequence bit(PN9).
 *
 * @param dev - The device structure.
 * @param rst  - Enable option.
 *	       Example: 1 - The PN sequence is held in reset;
 *			0 - The PN sequence resumes from the seed value(0x92).
 *
 * @return Returns negative error code or the set PN9 status.
 *******************************************************************************/
int32_t ad9250_reset_pn9(struct ad9250_dev *dev,
			 int32_t rst)
{
	int32_t ret = 0;

	if ((rst == 0) || (rst == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_TEST,
					     rst * AD9250_TEST_RST_PN_SHOR,
					     AD9250_TEST_RST_PN_SHOR);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_TEST);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_TEST_RST_PN_SHOR) != 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Sets (1) or clears (0) the reset long PN sequence bit(PN23).
 *
 * @param dev - The device structure.
 * @param rst - Enable option.
 *	       		Example: 1 - The PN sequence is held in reset;
 *				0 - The PN sequence resumes from the seed value(0x3AFF).
 *
 * @return Returns negative error code or the set PN23 status.
 *******************************************************************************/
int32_t ad9250_reset_pn23(struct ad9250_dev *dev,
			  int32_t rst)
{
	int32_t ret = 0;

	if ((rst == 0) || (rst == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_TEST,
					     rst * AD9250_TEST_RST_PN_LONG,
					     AD9250_TEST_RST_PN_LONG);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_TEST);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_TEST_RST_PN_LONG) != 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Configures a User Test Pattern.
 *
 * @param dev           - The device structure.
 * @param pattern_no    - Selects the patterns to be configured. Range 1..4.
 * @param user_pattern  - Users's pattern.
 *
 * @return Returns negative error code or the selected user pattern.
 *******************************************************************************/
int32_t ad9250_set_user_pattern(struct ad9250_dev *dev,
				int32_t pattern_no,
				int32_t user_pattern)
{
	int32_t pattern_address = 0;;
	int32_t ret = 0;

	pattern_address = AD9250_REG_USER_TEST1 + (2 * pattern_no);
	ret = ad9250_write(dev,
			   pattern_address,
			   user_pattern);

	return ret;
}

/***************************************************************************//**
 * @brief Enables the Build-In-Self-Test.
 *
 * @param dev    - The device structure.
 * @param enable - enable option.
 *
 * @return Returns negative error code or the state of the enable bit.
 *******************************************************************************/
int32_t ad9250_bist_enable(struct ad9250_dev *dev,
			   int32_t enable)
{
	int32_t ret = 0;

	if ((enable == 0) || (enable == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_BIST,
					     enable * AD9250_BIST_ENABLE,
					     AD9250_BIST_ENABLE);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_BIST);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_BIST_ENABLE) >> 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Resets the Build-In-Self-Test.
 *
 * @param dev    - The device structure.
 * @param reset - reset option.
 *
 * @return Returns negative error code or the state of the reset bit.
 *******************************************************************************/
int32_t ad9250_bist_reset(struct ad9250_dev *dev,
			  int32_t reset)
{
	int32_t ret = 0;

	if ((reset == 0) || (reset == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_BIST,
					     reset * AD9250_BIST_RESET,
					     AD9250_BIST_RESET);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_BIST);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_BIST_RESET) >> 2;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Sets number of frames per multiframe (K).
 *
 * @param dev      - The device structure.
 * @param k_frames - Number of frames. Must be a multiple of 4.
 *		     Range: Ceil(17/F) to 32,
 *			    where F - is the number of octets per frame.
 *
 * @return Returns negative error code or actual number of frames that can be set.
 *******************************************************************************/
int32_t ad9250_jesd204b_set_frames(struct ad9250_dev *dev,
				   int32_t k_frames)
{
	int32_t k_reg_val = k_frames;
	int32_t mod4 = 0;
	int32_t ret = 0;

	if (k_frames >= 0 && k_frames <= 32) {
		mod4 = k_frames % 4;
		if (mod4 != 0) {
			if (mod4 < 2) {
				k_reg_val -= mod4;
			} else {
				k_reg_val += 4 - mod4;
			}
		}
		ret = ad9250_write(dev,
				   AD9250_REG_204B_PARAM_K,
				   k_reg_val - 1);
		if (ret == -1) {
			return ret;
		}
		return k_reg_val;
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_204B_PARAM_K);
		return ret;
	}
}

/***************************************************************************//**
 * @brief Configures the JESD204B interface.
 *
 * @param dev - The device structure.
 *
 * @return Returns negative error code or 0 in case of success.
 *******************************************************************************/
int32_t ad9250_jesd204b_setup(struct ad9250_dev *dev)
{
	int32_t ret = 0;

	/* Disable lanes before changing configuration */
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_204B_CTRL1,
				     AD9250_204B_CTRL1_POWER_DOWN,
				     AD9250_204B_CTRL1_POWER_DOWN);
	if (ret == -1) {
		return ret;
	}
	/* Select quick configuration option */
	ret = ad9250_write(dev,
			   AD9250_REG_204B_QUICK_CFG,
			   AD9250_204B_QUICK_CFG(dev->ad9250_st.p_jesd204b->quick_cfg_option));
	if (ret == -1) {
		return ret;
	}
	/* Configure detailed options */
	/* CML differential output drive level adjustment */
	ret = ad9250_write(dev,
			   AD9250_REG_CML,
			   AD9250_CML_DIFF_OUT_LEVEL(dev->ad9250_st.p_jesd204b->cml_level));
	if (ret == -1) {
		return ret;
	}
	/* Select the behavioral of the 204B core when in standby. */
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_PDWN,
				     dev->ad9250_st.p_jesd204b->jtx_in_standby * AD9250_PDWN_JTX,
				     AD9250_PDWN_JTX);
	if (ret == -1) {
		return ret;
	}

	/* Select subclass. */
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_204B_PARAM_NP,
				     AD9250_204B_PARAM_NP_JESD_SUBCLASS(dev->ad9250_st.p_jesd204b->subclass),
				     AD9250_204B_PARAM_NP_JESD_SUBCLASS(-1));
	if (ret == -1) {
		return ret;
	}
	/* Configure the tail bits and control bits. */
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_204B_PARAM_CS_N,
				     AD9250_204B_PARAM_CS_N_NR_CTRL_BITS(dev->ad9250_st.p_jesd204b->ctrl_bits_no),
				     AD9250_204B_PARAM_CS_N_NR_CTRL_BITS(-1));
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_OUT_MODE,
				     AD9250_OUT_MODE_JTX_BIT_ASSIGN(dev->ad9250_st.p_jesd204b->ctrl_bits_assign),
				     AD9250_OUT_MODE_JTX_BIT_ASSIGN(-1));
	if (ret == -1) {
		return ret;
	}
	/* A transfer operation is needed because AD9250_REG_OUT_MODE is a shadowed register. */
	ret = ad9250_transfer(dev);
	if (ret == -1) {
		return ret;
	}
	if (dev->ad9250_st.p_jesd204b->ctrl_bits_no == 0) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_204B_CTRL1,
					     AD9250_204B_CTRL1_TAIL_BITS * dev->ad9250_st.p_jesd204b->tail_bits_mode,
					     AD9250_204B_CTRL1_TAIL_BITS);
		if (ret == -1) {
			return ret;
		}
	}
	/* Set lane identification values. */
	ret = ad9250_write(dev,
			   AD9250_REG_204B_DID_CFG,
			   dev->ad9250_st.p_jesd204b->did);
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_write(dev,
			   AD9250_REG_204B_BID_CFG,
			   dev->ad9250_st.p_jesd204b->bid);
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_write(dev,
			   AD9250_REG_204B_LID_CFG0,
			   dev->ad9250_st.p_jesd204b->lid0);
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_write(dev,
			   AD9250_REG_204B_LID_CFG1,
			   dev->ad9250_st.p_jesd204b->lid1);
	if (ret == -1) {
		return ret;
	}
	/* Set number of frames per multiframe, K */
	ret = ad9250_jesd204b_set_frames(dev,
					 dev->ad9250_st.p_jesd204b->k);
	if (ret == -1) {
		return ret;
	}
	/* Scramble, SCR. */
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_204B_PARAM_SCR_L,
				     AD9250_204B_PARAM_SCR_L_SCRAMBLING * dev->ad9250_st.p_jesd204b->scrambling,
				     AD9250_204B_PARAM_SCR_L_SCRAMBLING);
	if (ret == -1) {
		return ret;
	}
	/* Select lane synchronization options */
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_204B_CTRL1,
				     AD9250_204B_CTRL1_ILAS_MODE(dev->ad9250_st.p_jesd204b->ilas_mode),
				     AD9250_204B_CTRL1_ILAS_MODE(-1));
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_204B_CTRL1,
				     dev->ad9250_st.p_jesd204b->en_ilas_test * AD9250_204B_CTRL1_TEST_SAMPLE_EN,
				     AD9250_204B_CTRL1_TEST_SAMPLE_EN);
	if (ret == -1) {
		return ret;
	}

	/* Set additional digital output configuration options */
	/* Set polarity of serial output data */
	ret = ad9250_jesd204b_invert_logic(dev,
					   dev->ad9250_st.p_jesd204b->invert_logic_bits);
	if (ret == -1) {
		return ret;
	}
	/* Options for interpreting single on SYSREF+- and SYNCINB+- */
	ret = ad9250_write(dev,
			   AD9250_REG_SYS_CTRL,
			   dev->ad9250_st.p_jesd204b->en_sync_in_b * AD9250_SYS_CTRL_SYNCINB_EN |
			   dev->ad9250_st.p_jesd204b->en_sys_ref * AD9250_SYS_CTRL_SYSREF_EN |
			   dev->ad9250_st.p_jesd204b->sys_ref_mode * AD9250_SYS_CTRL_SYSREF_MODE |
			   dev->ad9250_st.p_jesd204b->align_sys_ref * AD9250_SYS_CTRL_REALIGN_ON_SYSREF |
			   dev->ad9250_st.p_jesd204b->align_sync_in_b *
			   AD9250_SYS_CTRL_REALIGN_ON_SYNCINB);
	if (ret == -1) {
		return ret;
	}
	/* A transfer operation is needed, because AD9250_REG_SYS_CTRL is a shadowed register. */
	ret = ad9250_transfer(dev);
	if (ret == -1) {
		return ret;
	}
	/* Option to remap converter and lane assignments */
	ret = ad9250_write(dev,
			   AD9250_REG_204B_LANE_ASSGN1,
			   AD9250_204B_LANE_ASSGN1(dev->ad9250_st.p_jesd204b->lane0_assign) |
			   0x02);
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_write(dev,
			   AD9250_REG_204B_LANE_ASSGN2,
			   AD9250_204B_LANE_ASSGN2(dev->ad9250_st.p_jesd204b->lane1_assign) |
			   0x30);
	if (ret == -1) {
		return ret;
	}
	/* Re-enable lane(s) */
	ret = ad9250_set_bits_to_reg(dev,
				     AD9250_REG_204B_CTRL1,
				     0,
				     AD9250_204B_CTRL1_POWER_DOWN);
	if (ret == -1) {
		return ret;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Configures the power mode of the JESD204B data transmit block.
 *
 * @param dev  - The device structure.
 * @param mode - The power mode.
 *		   Example: 00 � normal operation(default);
 *			    01 � power-down;
 *			    10 - standby.
 *
 * @return Returns negative error code or the set power mode.
 *******************************************************************************/
int32_t ad9250_jesd204b_pwr_mode(struct ad9250_dev *dev,
				 int32_t mode)
{
	uint32_t ret = 0;

	if ((mode >= 0) && (mode < 3)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_PDWN,
					     AD9250_PDWN_JESD204B(mode),
					     AD9250_PDWN_JESD204B(0x3));
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_PDWN);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_PDWN_JESD204B(0x3)) >> 2;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Selects the point in the processing path of a lane, where the test
 *	  data will be inserted.
 *
 * @param dev       - The device structure.
 * @param inj_point - The point in the processing path of a lane.
 *		     Example:
 *			  1 - a 10-bit data is inserted at 8B/10B ENCODER output
 *			  2 - a 8-bit at scrambler input
 *
 * @return Returns negative error code or the status of the data injection point bit.
 *******************************************************************************/
int32_t ad9250_jesd204b_select_test_injection_point(struct ad9250_dev *dev,
		int32_t inj_point)
{
	int32_t ret = 0;

	if ((inj_point == 1) || (inj_point == 2)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_204B_CTRL3,
					     AD9250_204B_CTRL3_TEST_DATA_INJ_PT(inj_point),
					     AD9250_204B_CTRL3_TEST_DATA_INJ_PT(-1));
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_204B_CTRL3);
		if (ret == -1) {
			return ret;
		}
		return (ret &  AD9250_204B_CTRL3_TEST_DATA_INJ_PT(-1)) >> 4;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Selects a JESD204B test mode.
 *
 * @param dev       - The device structure.
 * @param test_mode - mode option.
 *		     Example: 0 - test mode disabled
 *			      1 - alternating checker board
 *			      2 - 1/0 word toggle
 *			      3 - PN23 sequence
 *			      4 - PN9 sequence
 *			      5 - continuous/repeat user test mode
 *			      6 - single user test mode
 *			      7 - reserved
 *			      8 - modified RPAT test sequence
 *			     12 - PN7 sequence
 *			     13 - PN15 sequence
 *
 * @return Returns the set test mode or negative error code.
 *******************************************************************************/
int32_t ad9250_jesd204b_test_mode(struct ad9250_dev *dev,
				  int32_t test_mode)
{
	int32_t ret = 0;

	if ((test_mode >= 0) && (test_mode < 14)) {
		ad9250_set_bits_to_reg(dev,
				       AD9250_REG_204B_CTRL3,
				       AD9250_204B_CTRL3_JESD_TEST_MODE(test_mode),
				       AD9250_204B_CTRL3_JESD_TEST_MODE(-1));
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_204B_CTRL3);
		if (ret == -1) {
			return ret;
		}
		return (ret &  AD9250_204B_CTRL3_JESD_TEST_MODE(-1)) >> 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Inverts the logic of JESD204B bits.
 *
 * @param dev    - The device structure.
 * @param invert - Invert option.
 *		     Example: 1 - Activates the inverted mode
 *			      0 - Activates the normal mode
 *
 * @return Returns negative error code or the set mode.
 *******************************************************************************/
int32_t ad9250_jesd204b_invert_logic(struct ad9250_dev *dev,
				     int32_t invert)
{
	uint32_t ret = 0;

	if ((invert == 0) || (invert == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_204B_CTRL2,
					     invert * AD9250_204B_CTRL2_INVERT_JESD_BITS,
					     AD9250_204B_CTRL2_INVERT_JESD_BITS);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_204B_CTRL2);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_204B_CTRL2_INVERT_JESD_BITS) != 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Configures the Fast-Detect module.
 *
 * @param dev - The device structure.
 *
 * @return Returns negative error code or 0 in case of success.
 *******************************************************************************/
int32_t ad9250_fast_detect_setup(struct ad9250_dev *dev)
{
	int32_t ret = 0;

	ret = ad9250_write(dev,
			   AD9250_REG_FAST_DETECT,
			   AD9250_FAST_DETECT_OUTPUT_ENABLE * dev->ad9250_st.p_fd->en_fd |
			   AD9250_FAST_DETECT_FORCE_FDA_FDB_VAL * dev->ad9250_st.p_fd->pin_force_value |
			   AD9250_FAST_DETECT_FORCE_FDA_FDB_PIN * dev->ad9250_st.p_fd->force_pins |
			   AD9250_FAST_DETECT_PIN_FCT * dev->ad9250_st.p_fd->pin_function);
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_write(dev,
			   AD9250_REG_FD_UPPER_THD,
			   dev->ad9250_st.p_fd->fd_upper_tresh);
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_write(dev,
			   AD9250_REG_FD_LOWER_THD,
			   dev->ad9250_st.p_fd->fd_lower_tresh);
	if (ret == -1) {
		return ret;
	}
	ret = ad9250_write(dev,
			   AD9250_REG_FD_DWELL_TIME,
			   dev->ad9250_st.p_fd->df_dwell_time);
	if (ret == -1) {
		return ret;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Enables DC correction for use in the output data signal path.
 *
 * @param dev    - The device structure.
 * @param enable - enable option.
 *		   Example: 0 - correction off
 *			    1 - correction on
 *
 * @return Returns negative error code or the status of the enable bit.
 *******************************************************************************/
int32_t ad9250_dcc_enable(struct ad9250_dev *dev,
			  int32_t enable)
{
	int32_t ret = 0;

	if ((enable == 0) || (enable == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_DCC_CTRL,
					     AD9250_DCC_CTRL_DCC_EN * enable,
					     AD9250_DCC_CTRL_DCC_EN);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_DCC_CTRL);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_DCC_CTRL_DCC_EN) >> 0;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Selects the bandwidth value for the DC correction circuit.
 *
 * @param dev - The device structure.
 * @param bw  - DC correction bandwidth.
 *	       Example: 0 - 2387.32 Hz at Sample Rate of 245.76 MSPS
 *			1 - 1193.66 Hz at Sample Rate of 245.76 MSPS
 *			...
 *			13 - 0.29 Hz at Sample Rate of 245.76 MSPS
 *
 * @return Returns negative error code or the state of the bandwidth bits.
 *******************************************************************************/
int32_t ad9250_dcc_bandwidth(struct ad9250_dev *dev,
			     int32_t bw)
{
	int32_t ret = 0;

	if ((bw >= 0) && (bw <= 13)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_DCC_CTRL,
					     AD9250_DCC_CTRL_DCC_BW(bw),
					     AD9250_DCC_CTRL_DCC_BW(-1));
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_DCC_CTRL);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_DCC_CTRL_DCC_BW(-1)) >> 2;
	}

	return ret;
}

/***************************************************************************//**
 * @brief Freezes DC correction value.
 *
 * @param dev    - The device structure.
 * @param freeze - freeze option.
 *		   Example: 0 - calculates the correction value
 *			    1 - freezes the DC correction at its current state
 *
 * @return Returns negative error code or the status of the freeze bit.
 *******************************************************************************/
int32_t ad9250_dcc_freeze(struct ad9250_dev *dev,
			  int32_t freeze)
{
	int32_t ret = 0;

	if ((freeze == 0) || (freeze == 1)) {
		ret = ad9250_set_bits_to_reg(dev,
					     AD9250_REG_DCC_CTRL,
					     AD9250_DCC_CTRL_FREEZE_DCC * freeze,
					     AD9250_DCC_CTRL_FREEZE_DCC);
	} else {
		ret = ad9250_read(dev,
				  AD9250_REG_DCC_CTRL);
		if (ret == -1) {
			return ret;
		}
		return (ret & AD9250_DCC_CTRL_FREEZE_DCC) >> 6;
	}

	return ret;
}
