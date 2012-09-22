/*
 * tcbd_api_common.h
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TCBD_API_COMMON_H__
#define __TCBD_API_COMMON_H__
#include "tcbd_drv_register.h"
#include "tcbd_diagnosis.h"

/**
 * @defgroup ProtectionType
 */
/**@{*/
#define PROTECTION_TYPE_UEP 0
#define PROTECTION_TYPE_EEP 1
/**@}*/

/**
 *  @defgroup ProtectionLevel
 *  Protection type UEP
 *  <table>
 *     <tr><td>UEP1</td><td>UEP2</td><td>UEP3</td><td>UEP4</td></tr>
 *     <tr><td>0x00</td><td>0x01</td><td>0x02</td><td>0x03</td></tr></table>
 *
 *  Protection type EEP
 *  <table>
 *     <tr><td>EEP1A</td><td>EEP2A</td><td>EEP3A</td><td>EEP4A</td>
 *     <td>EEP1B</td><td>EEP2B</td><td>EEP3B</td><td>EEP4B</td></tr>
 *     <tr><td>0x00</td><td>0x01</td><td>0x02</td><td>0x03</td>
 *     <td>0x04</td><td>0x05</td><td>0x06</td><td>0x07</td></tr>
 */

/*
 * @defgroup ServiceType
 * Service type for each sub-channel.
 */
/** @{*/
#define SERVICE_TYPE_DAB	 0x01
#define SERVICE_TYPE_DABPLUS 0x02
#define SERVICE_TYPE_DATA	0x03
#define SERVICE_TYPE_DMB	 0x04
#define SERVICE_TYPE_FIC	 0x07
#define SERVICE_TYPE_FIC_WITH_ERR 0x08
#define SERVICE_TYPE_EWS	 0x09
/**@}*/



/**
 *  @defgroup Bitrate
 *  If protection type is UEP, the values of bitrate are as following.
 *  <table>
 *  <tr><td>Kbps</td>
 *      <td>32</td><td>48</td><td>56</td><td>64</td><td>80</td>
 *      <td>96</td><td>112</td><td>128</td><td>160</td><td>192</td>
 *      <td>224</td><td>256</td><td>320</td><td>384</td></tr>
 *  <tr><td>value</td>
 *      <td>0x00</td><td>0x01</td><td>0x02</td><td>0x03</td><td>0x04</td>
 *      <td>0x05</td><td>0x06</td><td>0x07</td><td>0x08</td><td>0x09</td>
 *      <td>0x0A</td><td>0x0B</td><td>0x0C</td><td>0x0D</td></tr>
 *  </table>
 *
 *  If protection type is EEP, the values of bitrate are as following.
 *  <table>
 *  <tr><td>kbps</td>
 *      <td>1</td><td>2</td><td>3</td><td>...</td><td>126</td></tr>
 *  <tr><td>value</td>
 *      <td>0x01</td><td>0x02</td><td>0x03</td><td>...</td><td>0x7E</td><tr>
 *  </table>
 *  0x00
 */

/**
 *  @defgroup ServiceInformation
 */
struct tcbd_service {
	s32 type; /**< Type of service. Please refer to @ref ServiceType*/
	s32 ptype; /**< Type of protection Please refer to @ref ProtectionType*/
	s32 subch_id; /**< Sub-channel id */
	s32 size_cu; /**< Size of CU(Capacity Unit) or sub-channel size*/
	s32 start_cu; /**< Start address of CU(Capacity Unit) */
	s32 reconfig; /**< Must be set 0x02*/
	s32 rs_on; /**< If type is DMB, it must be 0x01, or else 0x00 */
	s32 plevel; /**< Protection level Please refer @ref ProtectionLevel*/
	s32 bitrate; /**< Bitrate. Please refer @ref Bitrate*/
};

/**
 * @defgroup PreripheralType
 * types of interface to read stream
 */
/** @{*/
enum tcbd_peri_type {
	PERI_TYPE_SPI_SLAVE = 1, /**< The stream can be read
							  *  using SPI(slave) */
	PERI_TYPE_SPI_MASTER,   /**< The stream can be read
							 *  using SPI(master) */
	PERI_TYPE_PTS,	   /**< The stream can be read using PTS */
	PERI_TYPE_STS,	   /**< The stream can be read using STS */
	PERI_TYPE_HPI,	   /**< The stream can be read using HPI */
	PERI_TYPE_SPI_ONLY	/**< The stream can be read using SPI.
						 *  And command use same SPI interface. */
};
/**@}*/

enum tcbd_band_type {
	BAND_TYPE_BAND3 = 1,  /**< band III */
	BAND_TYPE_LBAND	   /**< L band */
};

enum tcbd_div_io_type {
	DIV_IO_TYPE_SINGLE = 0,
	DIV_IO_TYPE_DUAL,
};

enum tcbd_intr_mode {
	INTR_MODE_LEVEL_HIGH,   /**< Interrupt will be trigered at high level */
	INTR_MODE_LEVEL_LOW,	/**< Interrupt will be trigered at low level */
	INTR_MODE_EDGE_RISING,  /**< Interrupt will be trigered at rising edge */
	INTR_MODE_EDGE_FALLING  /**< Interrupt will be trigered at falling edge */
};

/**
 * @defgroup OscillatorType
 * supported types of oscillator
 */
/** @{*/
enum tcbd_clock_type {
	CLOCK_19200KHZ,
	CLOCK_24576KHZ,
	CLOCK_38400KHZ,
	CLOCK_MAX
};
/**}@*/

struct tcbd_multi_service {
	s32 on_air[TCBD_MAX_NUM_SERVICE];
	s32 service_idx[TCBD_MAX_NUM_SERVICE];
	s32 service_count;
	u32 service_info[TCBD_MAX_NUM_SERVICE*2];
};

struct tcbd_device {
	u8 chip_addr; /**< Chip address for diversity or dual mode*/
	u32 main_clock;
	u8 processor;  /**< Activated interrnal processor */

	u32 selected_buff;
	u32 selected_stream;
	u8 en_cmd_fifo;
	u32 intr_threshold; /**< Interrupt threshold */
#if defined(__READ_VARIABLE_LENGTH__)
	u32 size_more_read;
#endif /*__READ_VARIABLE_LENGTH__*/
	enum tcbd_clock_type clock_type;
	enum tcbd_peri_type peri_type; /**< Peripheral type for stream */
	enum tcbd_band_type curr_band;
	enum tcbd_band_type prev_band;

	s32 frequency; /**< Last set frequency */
	s32 is_pal_irq_en;
	u8 enabled_irq;
	struct tcbd_multi_service mult_service; /**< Registered service
												 infomation */
};

/**
 *  Change the type of stream. Types are
 *  - DMB : STREAM_TYPE_DMB
 *  - DAB : STREAM_TYPE_DAB
 *  - FIC : STREAM_TYPE_FIC
 *  - STATUS : STREAM_TYPE_STATUS
 *
 *  You can combine the above types. For more information @ref TypesOfStream
 *  @param _device Instance of device
 *  @param _type type of stream
 *
 *  @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_change_stream_type(
	struct tcbd_device *_device, u32 _type);

TCBB_FUNC s32 tcbd_enable_irq(
	struct tcbd_device *_device, u8 _en_bit);
TCBB_FUNC s32 tcbd_disable_irq(
	struct tcbd_device *_device, u8 _mask);

/**
 * @brief Change the mode of interrupt.
 * @param _device Instance of device
 * @param _mode Interrupt mode. For more information of mode.
 *              Please refer @tcbd_intr_mode
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_change_irq_mode(
	struct tcbd_device *_device, enum tcbd_intr_mode _mode);

/**
 * @brief read interrupt and error status.
 * @param _device instance of device
 * @param _irq_status interrupt status of buffer. Please @ref InterruptStatus
 * @param _err_status error status of buffer. Please @ref ErrorStatus
 */
TCBB_FUNC s32 tcbd_read_irq_status(
	struct tcbd_device *_device,	u8 *_irq_status, u8 *_err_status);

/**
 * @brief clear interrupt status.
 * @param _device instance of device
 * @param _status status of interrupt. Please @ref InterruptClear
 */
TCBB_FUNC s32 tcbd_clear_irq(
	struct tcbd_device *_device, u8 _status);

/**
 * @brief select interface to read stream.
 * @param _device instance of device
 * @param _peri_type interface type @ref PreripheralType
 */
TCBB_FUNC s32 tcbd_select_peri(
	struct tcbd_device *_device, enum tcbd_peri_type _peri_type);

#define ENABLE_CMD_FIFO 1
#define DISABLE_CMD_FIFO 0

/**
 * Initialize stream configuration.
 * @param _device Instance of device.
 * @param _use_cmd_fifo
 *    Determin whether or not to use command FIFO when you read
 *    stream. If you use SPI interface to read stream, you
 *    should use ENABLE_CMD_FIFO.
 * - ENABLE_CMD_FIFO
 * - DISABLE_CMD_FIFO
 * @param _buffer_mask
 *    Mask unused buffer.
 *    For more information, please refer @ref StreamDataConfig
 * @param _threshold
 *    Threshold for triggering interrupt.	The threshold can be
 *    set up to 8Kbyes. If you use STS, this value is meaningless.
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_init_stream_data_config(
	struct tcbd_device *_device,
	u8 _use_cmd_fifo,
	u8 _buffer_mask,
	u32 _threshold);

/**
 *  Tune frequency.
 *  @param _device Instance of device
 *  @param _freq_khz frequency
 *  @param _bw_khz bandwidth
 *
 *  @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_tune_frequency(
	struct tcbd_device *_device, u32 _freq_khz, s32 _bw_khz);


/**
 * @brief wait until the frequency is tuned.
 * @param _device Instance of device
 * @param _status Status.
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_wait_tune(
	struct tcbd_device *_device, u8 *_status);


/**
 * @brief register service(short argument)
 * @param _device instance of device
 * @param _subch_id sub channel id
 * @param _data_mode 0:DAB,DATA 1:DMB
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_register_service(
	struct tcbd_device *_device, u8 _subch_id, u8 _data_mode);

/**
 * @brief register service(long argument).
 * @param _device Instance of device
 * @param _service Sub channel information. @ref ServiceInformation
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_register_service_long(
	struct tcbd_device *_device, struct tcbd_service *_service);

/**
 * @brief unregister sub channel.
 * @param _device instance of device
 * @param _subch_id sub channel information previously registered.
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_unregister_service(
	struct tcbd_device *_device, u8 _subch_id);

/**
 *  @brief read tream from device
 *  @param _device instance of device
 *  @param _buff buffer for stream
 *  @param _size size of buffer. size must bigger then
 *             interrupt threshold(TCBD_MAX_THRESHOLD)
 *
 *  @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_read_stream(
	struct tcbd_device *_device, u8 *_buff, s32 *_size);

/**
 * @brief read fic data from internal buffer(I2C/STS interface only)
 * @param _device instance of device
 * @param _buff buffer for fic data.
 * @param _size size of buffer. _size must be 384 bytes.
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_read_fic_data(
	struct tcbd_device *_device, u8 *_buff, s32 _size);

/**
 * @brief read signal quality.
 * @param _device instance of device
 * @param _status_data instance of struct tcbd_status_data
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_read_signal_info(
	struct tcbd_device *_device, struct tcbd_status_data *_status_data);

/**
 * @brief _device instance of device.
 * @param _comp_en internal processor to enable
 * @param _comp_rst internal processor to reset
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_reset_ip(
	struct tcbd_device *_device, u8 _comp_en, u8 _comp_rst);

/**
 * @brief start device.
 * @param _device instance of device
 * @param _clock_type oscillator clock type @ref OscillatorType
 * @return success 0, fail -@ref ListOfErrorCode
 */
TCBB_FUNC s32 tcbd_device_start(
	struct tcbd_device *_device, enum tcbd_clock_type _clock_type);
#endif /*__TCBD_API_COMMON_H__*/
