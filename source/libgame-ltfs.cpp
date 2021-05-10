#include <ltfs/libltfs/tape_ops.h>

/**
 * Open a device.
 * @param devname Name of the device to open. The format of this string is
 *                implementation-dependent. For example, the ibmtape backend requires
 *                the path to an IBM tape device, e.g. /dev/IBMtape0.
 * @param[out] handle Stores the handle of the device on a successful call to this function.
 *             The device handle is implementation-defined and treated as opaque by libltfs.
 * @return 0 on success or a negative value on error.
 */
int libgame_open(const char *devname, void **handle)
{
	*handle = new skystream(devname);
	return 0;
}

/**
 * Reopen a device. If reopen is not needed, do nothing in this call. (ie. ibmtape backend)
 * @param devname Name of the device to open. The format of this string is
 *                implementation-dependent. For example, the ibmtape backend requires
 *                the path to an IBM tape device, e.g. /dev/IBMtape0.
 * @param device Device handle returned by the backend's open().
 * @return 0 on success or a negative value on error.
 */
int libgame_reopen(const char *devname, void *handle)
{
	return 0;
}

/**
 * Close a previously opened device.
 * @param device Device handle returned by the backend's open(). The handle is invalidated
 *               and will not be reused after this function is called.
 * @return 0 on success or a negative value on error.
 */
int   libgame_close(void *device)
{
	delete (skystream*) device;
	return 0;
}

/**
 * Close only file descriptor
 * @param device a pointer to the ibmtape backend
 * @return 0 on success or a negative value on error
 */
int   libgame_close_raw(void *device) { return 0; }

/**
 * Verify if a tape device is connected to the host.
 * @param devname Name of the device to check. The format of this string is the same one
 *                used in the open() operation.
 * @return 0 to indicate that the tape device is connected and a negative value otherwise.
 */
int   libgame_is_connected(const char *devname)
{
	skystream s(devname);
	s.length("bytes");
	return 0;
}

/**
 * Retrieve inquiry data from a device.
 * This function is not currently used by libltfs. Backends not implementing it should
 * zero out the inq parameter and return 0.
 * @param device Device handle returned by the backend's open().
 * @param inq Pointer to a tc_inq structure. On success, this structure's fields will be filled
 *            using data from the device. Any fields which do not make sense for the device
 *            are zero-filled.
 * @return 0 on success or a negative value on error.
 */
int   libgame_inquiry(void *device, struct tc_inq *inq)
{
	memset(inq, 0, sizeof(*inq));
	return 0;
}

/**
 * Retrieve inquiry data from a specific page.
 * @param device Device handle returned by the backend's open().
 * @param page Page to inquiry data from
 * @param inq Pointer to a tc_inq_page structure. On success, this structure's fields will
 *            be filled using data from the device.
 * @return 0 on success or negative value on error
 */
int   libgame_inquiry_page(void *device, unsigned char page, struct tc_inq_page *inq)
{
	memset(inq, 0, sizeof(*inq));
	return 0;
}

/**
 * Check whether a device is ready to accept commands.
 * Some devices may indicate their readiness but still fail certain commands if a load() is
 * not performed. Therefore, load() will be issued before any calls to this function.
 * @param device Device handle returned by the backend's open().
 * @return 0 if the device is ready, or a negative value otherwise.
 */
int   libgame_test_unit_ready(void *device)
{
	return libgame_is_connected(device);
}

/**
 * Read exactly one block from a device, of at most the specified size.
 * libltfs will break badly if this function reads more or less than one logical block.
 * @param device Device handle returned by the backend's open().
 * @param buf Buffer to receive data read from the device.
 * @param count Buffer size (maximum block size to read).
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 *            libltfs expects the block position to increment by 1 on success; violating this
 *            assumption may harm performance.
 * @param unusual_size True if libltfs expects the actual block size to be smaller than
 *                     the requested count. This is purely a hint: the backend must always
 *                     correctly handle any block size up to the value of the count argument,
 *                     regardless of the value of this flag.
 * @return Number of bytes read on success, or a negative value on error. If a file mark is
 *         encountered during reading, this function must return 0 and position the device
 *         immediately after the file mark.
 */
int   libgame_read(void *device, char *buf, size_t count, struct tc_position *pos, const bool unusual_size)
{
	skystream * s = device;
	auto real_size = s->block_spans("index", pos->block)["bytes"];
	if (real_size > count) {
		return -LTFS_SMALL_BLOCKSIZE;
	}
	auto data = s->read("index", pos->block);
	memcpy(buf, data.data(), data.size());
	++ pos->block;
	return data.size();
}

/**
 * Write the given bytes to a device in exactly one logical block.
 * libltfs will break badly if this function writes only some of the given bytes, or if it
 * splits them across multiple logical blocks.
 * @param device Device handle returned by the backend's open().
 * @param buf Buffer containing data to write to the device.
 * @param count Buffer size (number of bytes to write).
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 *            libltfs expects the block position to increment by 1 on success; violating
 *            this assumption may cause data corruption.
 *
 *            On success, libltfs also inspects the early_warning and
 *            programmable_early_warning flags. These flags must be set by the backend when
 *            low space (programmable_early_warning) or very low space (early_warning)
 *            conditions are encountered. These flags are used by LTFS to decide when it should
 *            stop accepting user data writes (programmable_early_warning) and when free space
 *            is low enough to start panicking (early_warning).
 *
 *            Implementation of the early_warning flag is optional. If the backend can only
 *            support a single low space warning, it should set programmable_early_warning
 *            when it reaches that condition. The amount of space between
 *            programmable_early_warning and the next "low space" state (either early_warning
 *            or out of space) must be sufficient to write an index, preferably two indexes.
 *            At least 10 GB is recommended, but values as low as 0.5 GB are safe for many
 *            common use cases.
 *
 *            If a backend does not (or cannot) implement a low space warning, it must set
 *            early_warning and programmable_early_warning to false (0). Note, however, that
 *            data loss may occur with such backends when the medium runs out of space.
 *            Therefore, any backend which is targeted at end users must support
 *            the programmable_early_warning flag in this function if at all possible.
 *            Support for programmable_early_warning in the writefm(), locate() and space()
 *            functions is desirable, but not absolutely required.
 * @return 0 on success or a negative value on error.
 */
int libgame_write(void *device, const char *buf, size_t count, struct tc_position *pos)
{
	skystream * s = device;
	s->write({buf, buf + count}, "index", pos->block);
	++ pos->block;
	pos->early_warning = 0;
	pos->programmable_early_warning = 0;
	return 0;
}

/**
 * Write one or more file marks to a device.
 * @param device Device handle returned by the backend's open().
 * @param count Number of file marks to write. This function will not be called with a zero
 *              argument. Currently libltfs only writes 1 file mark at a time, but this
 *              function must correctly handle larger values.
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 *
 *            On success, the programmable_early_warning and early_warning flags should be
 *            set appropriately. These flags are optional for this function; libltfs will
 *            function correctly if the backend always sets them to false (0).
 *            See the documentation for write() for more information on the early
 *            warning flags.
 * @param immed Set immediate bit on
 * @return 0 on success or a negative value on error.
 */
int libgame_writefm(void *device, size_t count, struct tc_position *pos, bool immed)
{
	pos->early_warning = 0;
	pos->programmable_early_warning = 0;
	return 0;
}

/**
 * Rewind a device.
 * Ideally the backend should position the device at partition 0, block 0. But libltfs
 * does not depend on this behavior; for example, the file backend sets the position to
 * block 0 of the current partition.
 * This function is called immediately before unload().
 * @param device Device handle returned by the backend's open().
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 * @return 0 on success or a negative value on error.
 */
int   libgame_rewind(void *device, struct tc_position *pos)
{
	struct tc_position dest = *pos;
	dest->block = 0;
	return libgame_locate(device, dest, pos);
}

/**
 * Seek to the specified position on a device.
 * @param device Device handle returned by the backend's open().
 * @param dest Destination position, specified as a partition and logical block. The filemarks
 *             field must be ignored by the backend.
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 *            The backend should ensure that on success, pos matches dest. libltfs considers
 *            pos != dest as an error, even if this function returns 0.
 *
 *            On success, the programmable_early_warning and early_warning flags should be
 *            set appropriately. These flags are optional for this function; libltfs will
 *            function correctly if the backend always sets them to false (0).
 *            See the documentation for write() for more information on the early
 *            warning flags.
 * @return 0 on success or a negative value on error.
 */
int libgame_locate(void *device, struct tc_position dest, struct tc_position *pos)
{
	skystream * s = device;
	s->block_spans("index", dest->block);
	pos->block = dest.block;
	pos->early_warning = 0;
	pos->programmable_early_warning = 0;
	return 0;
}

/**
 * Issue a space command to a device.
 * @param device Device handle returned by the backend's open().
 * @param count Number of items to space by.
 * @param type Space type. Must be one of the following.
 *             TC_SPACE_EOD: space to end of data on the current partition.
 *             TC_SPACE_FM_F: space forward by file marks.
 *             TC_SPACE_FM_B: space backward by file marks.
 *             TC_SPACE_F: space forward by records.
 *             TC_SPACE_B: space backward by records.
 *             Currently only TC_SPACE_FM_F and TC_SPACE_FM_B are used by libltfs.
 *             If TC_SPACE_FM_F is specified, the backend must skip the specified number of
 *             file marks and position the device immediately after the last skipped file mark.
 *             If TC_SPACE_FM_B is specified, the backend must skip the specified number of
 *             file marks and position the device immediately before the last skipped file mark.
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 *
 *            On success, the programmable_early_warning and early_warning flags should be
 *            set appropriately. These flags are optional for this function; libltfs will
 *            function correctly if the backend always sets them to false (0).
 *            See the documentation for write() for more information on the early
 *            warning flags.
 * @return 0 on success or a negative value on error.
 *
 *         The backend should return an error if the requested operation causes the
 *         device to cross the beginning of the current partition or the end of data
 *         on the current partition, as these conditions will not occur in a valid LTFS volume.
 */
int libgame_space(void *device, size_t count, TC_SPACE_TYPE type, struct tc_position *pos)
{
	skystream * s = device;
	switch(type)
	{
 	case TC_SPACE_EOD: // space to end of data on the current partition.
		pos->block = s->length("index").second;
		break;
	case TC_SPACE_FM_F: // space forward by file marks. // fall-thru
	case TC_SPACE_FM_B: // space backward by file marks.
		break;
	case TC_SPACE_F: // space forward by records.
		break;
	case TC_SPACE_B: // space backward by records.
		break;
	}
	pos->early_warning = 0;
	pos->programmable_early_warning = 0;
	return 0;
}

/**
 * Erase medium starting at the current position.
 * This function is currently unused by libltfs.
 * @param device Device handle returned by the backend's open().
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 * @param long_erase   Set long bit and immed bit ON
 * @return 0 on success or a negative value on error.
 */
int   libgame_erase(void *device, struct tc_position *pos, bool long_erase)
{
	return -LTFS_OP_NOT_ALLOWED;
}

/**
 * Load medium into a device.
 * libltfs calls this function after open() and reserve_unit(), but before any
 * other backend calls.
 * @param device Device handle returned by the backend's open().
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 *            libltfs does not depend on any particular position being set here.
 * @return 0 on success or a negative value on error.
 *         If no medium is present in the device, the backend must return -EDEV_NO_MEDIUM.
 *         If the medium is unsupported (for example, does not support two partitions),
 *         the backend should return -LTFS_UNSUPPORTED_MEDIUM.
 */
int   libgame_load(void *device, struct tc_position *pos)
{
	pos->block = ~0;
	return 0;
}

/**
 * Eject medium from a device.
 * @param device Device handle returned by the backend's open().
 * @param pos Pointer to a tc_position structure. The backend should zero out this structure
 *            on success. On error, it must fill this structure with the final logical
 *            block position of the device.
 * @return 0 on success or a negative value on error.
 */
int   libgame_unload(void *device, struct tc_position *pos)
{
	pos->block = ~0;
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Read logical position (partition and logical block) from a device.
 * @param device Device handle returned by the backend's open().
 * @param pos Pointer to a tc_position structure. On success, the backend must fill this
 *            structure with the current logical block position of the device. On error,
 *            its contents must be unchanged.
 * @return 0 on success or a negative value on error.
 */
int   libgame_readpos(void *device, struct tc_position *pos)
{
	skystream * s = device;
	pos->block = s->length("index");
	return 0;
}

/**
 * Set the capacity proportion of the medium.
 * This function is always preceded by a locate request for partition 0, block 0.
 * @param device Device handle returned by the backend's open().
 * @param proportion Number to specify the proportion from 0 to 0xFFFF. 0xFFFF is for full capacity.
 * @return 0 on success or a negative value on error.
 */
int   libgame_setcap(void *device, uint16_t proportion)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Format a device.
 * This function is always preceded by a locate request for partition 0, block 0.
 * @param device Device handle returned by the backend's open().
 * @param format Type of format to perform. Currently libltfs uses the following values.
 *               TC_FORMAT_DEFAULT: create a single partition on the medium.
 *               TC_FORMAT_DEST_PART: create two partitions on the medium.
 * @param vol_name Volume name, unused by libtlfs (HPE extension)
 * @param vol_name Volume barcode, unused by libtlfs (HPE extension)
 * @param vol_mam_uuid Volume UUID, unused by libtlfs (HPE extension)
 * @return 0 on success or a negative value on error.
 */
int   libgame_format(void *device, TC_FORMAT_TYPE format, const char *vol_name, const char *barcode_name, const char *vol_mam_uuid)
{
	return -UNSUPPORTED_FUNCTION;
}


/**
 * Get capacity data from a device.
 *
 * @param device Device handle returned by the backend's open().
 * @param cap On success, the backend must fill this structure with the total and remaining
 *            capacity values of the two partitions on the medium, in units of 1048576 bytes.
 * @return 0 on success or a negative value on error.
 */
int   libgame_remaining_capacity(void *device, struct tc_remaining_cap *cap)
{
	cap->remaining_p0 = ~0;
	cap->remaining_p1 = 0;
	cap->max_p0 = ~0;
	cap->max_p1 = 0;
	return 0;
}

/**
 * Send a SCSI Log Sense command to a device.
 *
 * @param device Device handle returned by the backend's open().
 * @param page Log page to query.
 * @param subpage Specify the sub page of the log page to query.
 * @param buf On success, the backend must fill this buffer with the log page's value.
 * @param size Buffer size.
 * @return Page length on success or a negative value on error. Backends for which Log Sense is
 *         meaningless should return -1.
 */
int   libgame_logsense(void *device, const uint8_t page, const uint8_t subpage,
				  unsigned char *buf, const size_t size)
{
	return -1;
}

/**
 * Send a SCSI Mode Sense(10) command to a device.
 * This is used by libltfs to query the device's current partition size settings.
 * @param device Device handle returned by the backend's open().
 * @param page Mode page to query.
 * @param pc Page control value for the command. Currently libltfs only uses
 *  		 TC_MP_PC_CURRENT (to request the current value of the mode page).
 * @param subpage Subpage of the specified mode page.
 * @param buf On success, the backend must fill this buffer with the mode page's value.
 * @param size Buffer size.
 * @return positive value or 0 on success or a negative value on error. Backend can return 0
 *         on success or positive value on success as transfered length.
 */
int   libgame_modesense(void *device, const uint8_t page, const TC_MP_PC_TYPE pc, const uint8_t subpage, unsigned char *buf, const size_t size)
{
	memset(buf, 0, size);
	//buf[16] = page;
	return 0;
}

/**
 * Send a SCSI Mode Select(10) command to a device.
 * This is used by libltfs to update the partition size settings of the device.
 * @param device Device handle returned by the backend's open().
 * @param buf Buffer containing the new mode page value to set.
 * @param size Buffer size.
 * @return 0 on success or a negative value on error. Backends for which Mode Select is
 *         meaningless should return 0.
 */
int   libgame_modeselect(void *device, unsigned char *buf, const size_t size)
{
	return 0;
}

/**
 * Send a SCSI Reserve Unit command to a device.
 * libltfs calls this function immediately after opening the device to prevent contention
 * with other initiators.
 * @param device Device handle returned by the backend's open().
 * @return 0 on success or a negative value on error.
 */
int   libgame_reserve_unit(void *device)
{
	return 0;
}

/**
 * Send a SCSI Release Unit command to a device.
 * libltfs calls this function immediately before closing the device to allow access by
 * other initiators.
 * @param device Device handle returned by the backend's open().
 * @return 0 on success or a negative value on error.
 */
int   libgame_release_unit(void *device)
{
	return 0;
}

/**
 * Lock the medium in a device, preventing manual removal.
 * libltfs calls this function immediately after load().
 * @param device Device handle returned by the backend's open().
 * @return 0 on success or a negative value on error.
 */
int   libgame_prevent_medium_removal(void *device)
{
	return 0;
}

/**
 * Unlock the medium in a device, allowing manual removal.
 * @param device Device handle returned by the backend's open().
 * @return 0 on success or a negative value on error.
 */
int   libgame_allow_medium_removal(void *device)
{
	return 0;
}

/**
 * Read a MAM parameter from a device.
 * For performance reasons, it is recommended that all backends implement MAM parameter support.
 * However, this support is technically optional.
 * Normally the buffer doesn't include header of contents. But it includes only when the size is
 * MAXMAM_SIZE.
 * @param device Device handle returned by the backend's open().
 * @param part Partition to read the parameter from.
 * @param id Attribute ID to read. libltfs uses TC_MAM_PAGE_VCR and TC_MAM_PAGE_COHERENCY.
 * @param buf On success, the backend must place the MAM parameter in this buffer.
 *            Otherwise, the backend should zero out the buffer.
 * @param size Buffer size. The backend behavior is implementation-defined if the buffer
 *             is too small to receive the MAM parameter.
 * @return 0 on success or a negative value on error. A backend which does not implement
 *         MAM parameters must zero the output buffer and return a negative value.
 */
int   libgame_read_attribute(void *device, const tape_partition_t part, const uint16_t id, unsigned char *buf, const size_t size)
{
	memset(buf, 0, size);
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Write a MAM parameter to a device.
 * For performance reasons, it is recommended that all backends implement MAM parameter support.
 * However, this support is technically optional.
 * libltfs calls this to set cartridge coherency data from tape_set_cartridge_coherency().
 * @param device Device handle returned by the backend's open().
 * @param part Partition to write the parameter to.
 * @param buf Parameter to write. It is formatted for copying directly into the CDB, so
 *            it contains a header with the attribute ID and size.
 * @param size Buffer size.
 * @return 0 on success or a negative value on error. A backend which does not implement
 *         MAM parameters should return a negative value.
 */
int   libgame_write_attribute(void *device, const tape_partition_t part, const unsigned char *buf, const size_t size)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Set append point to the device.
 * The device will accept write commmand only on specified position or EOD, if the dvice
 * supports this feature.
 * @param device Device handle returned by the backend's open().
 * @param pos position to accept write command
 * @return 0 on success or a negative value on error.
 */
int   libgame_allow_overwrite(void *device, const struct tc_position pos)
{
	skystream * s = device;
	if (pos->block == s->length("index"))
	{
		return 0;
	}
	else
	{
		return -LTFS_OP_NOT_ALLOWED;
	}

}

/**
 * Enable or disable compression on a device.
 * @param device Device handle returned by the backend's open().
 * @param enable_compression If true, turn on compression. Otherwise, turn it off.
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 * @return 0 on success or a negative value on error. If the underlying device does not
 *         support transparent compression, the backend should always return 0.
 */
int   libgame_set_compression(void *device, const bool enable_compression, struct tc_position *pos)
{
	return 0;
}

/**
 * Set up any required default parameters for a device.
 * The effect of this function is implementation-defined. For example, the file backend
 * does nothing, while the ibmtape backend sets the device blocksize to variable and disables
 * the IBM tape driver's read past file mark option.
 * @param device Device handle returned by the backend's open().
 * @return 0 on success or a negative value on error.
 */
int   libgame_set_default(void *device)
{
	return 0;
}

/**
 * Get cartridge health data from the drive
 * @param device Device handle returned by the backend's open().
 * @param cart_health On success, the backend must fill this structure with the cartridge health
 *                    "-1" shows the unsupported value except tape alert.
 * @return 0 on success or a negative value on error.
 */
int   libgame_get_cartridge_health(void *device, struct tc_cartridge_health *cart_health)
{
	skystream * s = device;
	memset(cart_health, ~0, sizeof(*cart_health));
	auto lengths = s->lengths();
	cart_health->written_ds = lengths["index"];
	cart_health->writte_mbytes = lengths["bytes"] / 1024 / 1024;
	return 0;
}

/**
 * Get tape alert from the drive this value shall be latched by backends and shall be cleard by
 * clear_tape_alert() on write clear method
 * @param device Device handle returned by the backend's open().
 * @param tape alert On success, the backend must fill this value with the tape alert
 *                    "-1" shows the unsupported value except tape alert.
 * @return 0 on success or a negative value on error.
 */
int   libgame_get_tape_alert(void *device, uint64_t *tape_alert)
{
	*tape_alert = -1;
	return 0;
}

/**
 * clear latched tape alert from the drive
 * @param device Device handle returned by the backend's open().
 * @param tape_alert value to clear tape alert. Backend shall be clear the specicied bits in this value.
 * @return 0 on success or a negative value on error.
 */
int   libgame_clear_tape_alert(void *device, uint64_t tape_alert)
{
	return 0;
}

/**
 * Get vendor unique backend xattr
 * @param device Device handle returned by the backend's open().
 * @param name   Name of xattr
 * @param buf    On success, the backend must fill this value with the pointer of data buffer for xattr
 * @return 0 on success or a negative value on error.
 */
int   libgame_get_xattr(void *device, const char *name, char **buf)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Get vendor unique backend xattr
 * @param device Device handle returned by the backend's open().
 * @param name   Name of xattr
 * @param buf    Data buffer to set the value
 * @param size   Length of data buffer
 * @return 0 on success or a negative value on error.
 */
int   libgame_set_xattr(void *device, const char *name, const char *buf, size_t size)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Get operational parameters of a device. These parameters include such things as the
 * maximum supported blocksize and medium write protect state.
 * @param device Device handle returned by the backend's open().
 * @param params On success, the backend must fill this structure with the device
 *                    parameters.
 * @return 0 on success or a negative value on error.
 */
int   libgame_get_parameters(void *device, struct tc_drive_param *params)
{
	memset(params, 0, sizeof(*params));
	params->max_blksize = 1024 * 1024 * 128;
	return 0;
}

/**
 * Get EOD status of a partition.
 * @param device Device handle returned by the backend's open().
 * @param part Partition to read the parameter from.
 * @param pos Pointer to a tc_position structure. The backend must fill this structure with
 *            the final logical block position of the device, even on error.
 * @return enum eod_status or UNSUPPORTED_FUNCTION if not supported.
 */
int   libgame_get_eod_status(void *device, int part)
{
	return UNSUPPORTED_FUNCTION;
}


/**
 * Get a list of available tape devices for LTFS found in the host. The caller is
 * responsible from allocating the buffer to contain the tape drive information
 * by get_device_count() call.
 * When buf is NULL, this function just returns an available tape device count.
 * @param[out] buf Pointer to tc_drive_info structure array.
 *             The backend must fill this structure when this paramater is not NULL.
 * @param count size of array in buf.
 * @return on success, available device count on this system or a negative value on error.
 */
int   libgame_get_device_list(struct tc_drive_info *buf, int count)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Print a help message for the backend.
 * This function should print options specific to the backend. For example, the IBM
 * backends print their default device names.
 * @param progname The program name
 */
void  libgame_help_message(const char *progname)
{
	std::cout << "(todo: libgame ltfs help message)" << std::endl;
}

/**
 * Parse backend-specific options.
 * For example: the file backend takes an argument to write protect its
 * simulated tape cartridge.
 * @param device Device handle returned by the backend's open().
 * @param opt_args Pointer to a FUSE argument structure, suitable for passing to
 *                 fuse_opt_parse(). See the file backend for an example of argument parsing.
 * @return 0 on success or a negative value on error.
 */
int   libgame_parse_opts(void *device, void *opt_args)
{
	return 0;
}

/**
 * Get the default device name for the backend.
 * @return A pointer to the default device name string. This pointer is not freed on exit
 *         by the IBM LTFS utilities. It may be NULL if the backend has no default device.
 */
const char *libgame_default_device_name(void)
{
	return 0;
}

/**
 * Set the data key for application-managed encryption.
 * @param device Device handle returned by the backend's open().
 * @param keyalias A pointer to Data Key Identifier (DKi).
 *                 DKi compounded from 3 bytes ASCII characters and 9 bytes binary data.
 * @param key A pointer to Data Key (DK). DK is 32 bytes binary data.
 * @return 0 on success or a negative value on error.
 */
int   libgame_set_key(void *device, const unsigned char *keyalias, const unsigned char *key)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Get the key alias of the next block for application-managed encryption.
 * @param device Device handle returned by the backend's open().
 * @param[out] keyalias A pointer to Data Key Identifier (DKi).
 *                      DKi compounded from 3 bytes ASCII characters and 9 bytes binary data.
 * @return 0 on success or a negative value on error.
 */
int   libgame_get_keyalias(void *device, unsigned char **keyalias)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Take a dump from the tape drive.
 * @param device Device handle returned by the backend's open().
 * @return 0 on success or a negative value on error.
 */
int   libgame_takedump_drive(void *device, bool capture_unforced)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Check if the tape drive can mount the medium.
 * @param device Device handle returned by the backend's open().
 * @param barcode Bar code of the medium
 * @param cart_type Cartridge type of the medium (in the CM on LTO)
 *                  0x00 when this tape never be loaded to a drive
 * @param density Density code of the medium
 *                  0x00 when this tape never be loaded to a drive
 * @return MEDIUM_PERFECT_MATCH when this drive support the tape naively,
 *         MEDIUM_WRITABLE when this drive can read/write the medium,
 *         MEDIUM_READONLY when this drive can only read the medium,
 *         MEDIUM_CANNOT_ACCESS when this drive cannot read/write the medium,
 *         MEDIUM_PROBABLY_WRITABLE when this drive may read/write the medium like
 *         JC cartridge which never be loaded yet (no density_code information) and
 *         the drive is TS1140
 */
int   libgame_is_mountable(void *device,
					  const char *barcode,
					  const unsigned char cart_type,
					  const unsigned char density)
{
	return 0;
}

/**
 * Check if the loaded carridge is WORM.
 * @param device Device handle returned by the backend's open().
 * @param is_worm Pointer to worm status.
 * @return 0 on success or a negative value on error.
 */
int   libgame_get_worm_status(void *device, bool *is_worm)
{
	*is_worm = 1;
	return 0;
}

/**
 * Get the tape device's serial number
 * @param device a pointer to the tape device
 * @param[out] result On success, contains the serial number of the changer device.
 *                    The memory is allocated on demand and must be freed by the user
 *                    once it's been used.
 * @return 0 on success or a negative value on error
 */
int   libgame_get_serialnumber(void *device, char **result)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Get the tape device's information
 * This function must not issue any scsi command to the device.
 * @param device a pointer to the tape device
 * @param[out] info On success, contains device information.
 * @return 0 on success or a negative value on error
 */
int   libgame_get_info(void *device, struct tc_drive_info *info)
{
	memset(info, 0, sizeof(info));
	return 0;
}

/**
 * Enable profiler function
 * @param device a pointer to the tape device
 * @param work_dir work directory to store profiler data
 * @paran enable enable or disable profiler function of this backend
 * @return 0 on success or a negative value on error
 */
int   libgame_set_profiler(void *device, char *work_dir, bool enable)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Get block number stored in the drive buffer
 * @param device A pointer to the tape device
 * @param block Number of blocks stored in the drive buffer
 * @return 0 on success or a negative value on error
 */
int   libgame_get_block_in_buffer(void *device, unsigned int *block)
{
	return -UNSUPPORTED_FUNCTION;
}

/**
 * Check if the generation of tape drive and the current loaded cartridge is read-only combination
 * @param device Device handle returned by the backend's open().
 */
bool   libgame_is_readonly(void *device)
{
	return 0;
}

struct tape_ops libgame_handler = {
	.open                   = libgame_open,
	.reopen                 = libgame_reopen,
	.close                  = libgame_close,
	.close_raw              = libgame_close_raw,
	.is_connected           = libgame_is_connected,
	.inquiry                = libgame_inquiry,
	.inquiry_page           = libgame_inquiry_page,
	.test_unit_ready        = libgame_test_unit_ready,
	.read                   = libgame_read,
	.write                  = libgame_write,
	.writefm                = libgame_writefm,
	.rewind                 = libgame_rewind,
	.locate                 = libgame_locate,
	.space                  = libgame_space,
	.erase                  = libgame_erase,
	.load                   = libgame_load,
	.unload                 = libgame_unload,
	.readpos                = libgame_readpos,
	.setcap                 = libgame_setcap,
	.format                 = libgame_format,
	.remaining_capacity     = libgame_remaining_capacity,
	.logsense               = libgame_logsense,
	.modesense              = libgame_modesense,
	.modeselect             = libgame_modeselect,
	.reserve_unit           = libgame_reserve_unit,
	.release_unit           = libgame_release_unit,
	.prevent_medium_removal = libgame_prevent_medium_removal,
	.allow_medium_removal   = libgame_allow_medium_removal,
	.read_attribute         = libgame_read_attribute,
	.write_attribute        = libgame_write_attribute,
	.allow_overwrite        = libgame_allow_overwrite,
	.set_compression        = libgame_set_compression,
	.set_default            = libgame_set_default,
	.get_cartridge_health   = libgame_get_cartridge_health,
	.get_tape_alert         = libgame_get_tape_alert,
	.clear_tape_alert       = libgame_clear_tape_alert,
	.get_xattr              = libgame_get_xattr,
	.set_xattr              = libgame_set_xattr,
	.get_parameters         = libgame_get_parameters,
	.get_eod_status         = libgame_get_eod_status,
	.get_device_list        = libgame_get_device_list,
	.help_message           = libgame_help_message,
	.parse_opts             = libgame_parse_opts,
	.default_device_name    = libgame_default_device_name,
	.set_key                = libgame_set_key,
	.get_keyalias           = libgame_get_keyalias,
	.takedump_drive         = libgame_takedump_drive,
	.is_mountable           = libgame_is_mountable,
	.get_worm_status        = libgame_get_worm_status,
	.get_serialnumber       = libgame_get_serialnumber,
	.get_info               = libgame_get_info,
	.set_profiler           = libgame_set_profiler,
	.get_block_in_buffer    = libgame_get_block_in_buffer,
	.is_readonly            = libgame_is_readonly
};

struct tape_ops *tape_dev_get_ops()
{
	return &libgame_handler;
}

const char *tape_dev_get_message_bundle_name(void **message_data)
{
	*message_data = 0;
	return "libgame_skystream";
}
