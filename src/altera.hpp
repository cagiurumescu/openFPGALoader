// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (C) 2019 Gwenhael Goavec-Merou <gwenhael.goavec-merou@trabucayre.com>
 */

#ifndef SRC_ALTERA_HPP_
#define SRC_ALTERA_HPP_

#include <string>

#include "device.hpp"
#include "jtag.hpp"
#include "rawParser.hpp"
#include "spiInterface.hpp"
#include "svf_jtag.hpp"

class Altera: public Device, SPIInterface {
	public:
		Altera(Jtag *jtag, const std::string &filename,
				const std::string &file_type,
				Device::prog_type_t prg_type,
				const std::string &device_package,
				const std::string &spiOverJtagPath,
				bool verify, int8_t verbose,
				const std::string &flash_sectors,
				bool skip_load_bridge, bool skip_reset);
		~Altera();

		void programMem(RawParser &_bit);
		void program(unsigned int offset, bool unprotect_flash) override;
		/*!
		 * \brief read len Byte starting at base_addr and store
		 *        into filename
		 * \param[in] filename: file name
		 * \param[in] base_addr: starting address in flash memory
		 * \param[in] len: length (in Byte)
		 * \return false if read fails or filename can't be open, true otherwise
		 */
		bool dumpFlash(uint32_t base_addr, uint32_t len) override {
			if (_fpga_family == MAX10_FAMILY)
				return max10_dump();
			return SPIInterface::dump(base_addr, len);
		}

		uint32_t idCode() override;
		void reset() override;

		/*************************/
		/*     spi interface     */
		/*************************/

		/*!
		 * \brief display SPI flash ID and status register
		 */
		bool detect_flash() override {
			return SPIInterface::detect_flash();
		}
		/*!
		 * \brief protect SPI flash blocks
		 */
		bool protect_flash(uint32_t len) override {
			return SPIInterface::protect_flash(len);
		}
		/*!
		 * \brief unprotect SPI flash blocks
		 */
		bool unprotect_flash() override {
			return SPIInterface::unprotect_flash();
		}
		/*!
		 * \brief bulk erase SPI flash
		 */
		bool bulk_erase_flash() override {
			return SPIInterface::bulk_erase_flash();
		}

		int spi_put(uint8_t cmd, const uint8_t *tx, uint8_t *rx,
				uint32_t len) override;
		int spi_put(const uint8_t *tx, uint8_t *rx, uint32_t len) override;
		int spi_wait(uint8_t cmd, uint8_t mask, uint8_t cond,
				uint32_t timeout, bool verbose = false) override;

	protected:
		bool prepare_flash_access() override;
		bool post_flash_access() override;

	private:
		enum altera_family_t {
			MAX2_FAMILY      = 0,
			MAX10_FAMILY     = 1,
			CYCLONE5_FAMILY  = 2,
			CYCLONE10_FAMILY = 3,
			STRATIXV_FAMILY  = 3,
			CYCLONE_MISC     = 10, // Fixme: idcode shared
			UNKNOWN_FAMILY   = 999
		};
		/*************************/
		/*     max10 specific    */
		/*************************/
public:
		struct max10_mem_t;
private:
		static const std::map<uint32_t, Altera::max10_mem_t> max10_memory_map;

		/* Write a full POF file, or updates UFM with an arbitrary binary file */
		void max10_program(uint32_t offset);
		/* Write something in UFMx sections after erase */
		bool max10_program_ufm(const max10_mem_t *mem, uint32_t offset,
			uint8_t update_sectors);
		/* Write len Word from cfg_data at a specific address */
		void writeXFM(const uint8_t *cfg_data, uint32_t base_addr, uint32_t offset, uint32_t len);
		/* Compare cfg_data with data stored at base_addr */
		uint32_t verifyxFM(const uint8_t *cfg_data, uint32_t base_addr, uint32_t offset,
			uint32_t len);
		void max10_dsm_program_success(const uint32_t pgm_success_addr);
		void max10_flow_program_donebit(const uint32_t done_bit_addr);
		void max10_addr_shift(uint32_t addr);
		void max10_flow_enable();
		void max10_flow_disable();
		/* Performs a full internal flash erase or sectors per sectors erase */
		void max10_flow_erase(const max10_mem_t *mem, const uint8_t erase_sectors=0x1f);
		void max10_dsm_program(const uint8_t *dsm_data, const uint32_t dsm_len);
		bool max10_dsm_verify();
		bool max10_dump();
		bool max10_read_section(FILE *fd, const uint32_t base_addr, const uint32_t addr);
		/* Utils methods */
public:
		static uint8_t max10_flash_sectors_to_mask(std::string flash_sectors);
		static bool sectors_mask_start_end_addr(const Altera::max10_mem_t *mem,
			const uint8_t update_sectors, uint32_t *start, uint32_t *end,
			uint8_t *sectors_mask);
private:
		/*!
		 * \brief with intel devices SPI flash direct access is not possible
		 * 		so a bridge must be loaded in RAM to access flash
		 * 	\return false if missing device mode, true otherwise
		 */
		bool load_bridge();
		/* virtual JTAG access */
		/*!
		 * \brief virtual IR: send USER0 IR followed, in DR, by
		 *         address (_vir_addr) in a burst of _vir_length
		 * \param[in] reg: data to send in shiftDR mode with addr
		 */
		void shiftVIR(uint32_t reg);
		/*!
		 * \brief virtual IR: send USER1 IR followed by
		 *         data in DR with an optional read
		 * \param[in] tx: data to send in shiftDR mode
		 * \param[in] rx: data to read in shiftDR mode
		 * \param[in] len: len of tx & rx
		 * \param[in] end_state: next state at the end of xfer
		 */
		void shiftVDR(uint8_t * tx, uint8_t * rx, uint32_t len,
				Jtag::tapState_t end_state = Jtag::UPDATE_DR, bool debug = false);

		std::string _device_package;
		std::string _spiOverJtagPath; /**< spiOverJtag explicit path */
		uint32_t _vir_addr; /**< addr affected to virtual jtag */
		uint32_t _vir_length; /**< length of virtual jtag IR */
		uint32_t _clk_period; /**< JTAG clock period */

		altera_family_t _fpga_family;
		uint32_t _idcode;
		std::string _flash_sectors; /**< MAX10 Only: list of sectors to erase/write */
};

#endif  // SRC_ALTERA_HPP_
