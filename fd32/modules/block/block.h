/* Block device interface for FreeDOS-32
 * Copyright (C) 2005  Salvatore ISAJA
 * Permission to use, copy, modify, and distribute this program is hereby
 * granted, provided this copyright and disclaimer notice is preserved.
 * THIS PROGRAM IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * This is meant to be compatible with the GNU General Public License.
 */
#ifndef __FD32_BLOCK_H
#define __FD32_BLOCK_H


/**
 * \addtogroup block
 * @{
 */


/**
 * \brief Error category code for block devices.
 */
#define BLOCK_ERROR_CATEGORY 2
/**
 * \brief Formats a block device error value.
 * \param sense the sense key identifying the error type;
 * \param code  a code to provide detailed error information.
 * \remarks Block device drivers should use this macro to format any error code
 *          they return. This macro formats a non-negative value (for human
 *          readability), thus block device driver should explicitly invert its
 *          sign before returning it.
 * \sa enum BlockSenseKeys
 */
#define BLOCK_ERROR(sense, code) (((BLOCK_ERROR_CATEGORY & 0x1FF) << 22) | (sense) | ((code) & 0xFFFF))


/**
 * \brief Sense key values for block device error values.
 * \sa \ref block_error_reporting section, BLOCK_ERROR macro.
 */
enum BlockSenseKeys
{
	BLOCK_SENSE_NONE       =  0 << 16, ///< No sense
	BLOCK_SENSE_RECOVERED  =  1 << 16, ///< Recovered error
	BLOCK_SENSE_NOTREADY   =  2 << 16, ///< Not ready
	BLOCK_SENSE_MEDIUM     =  3 << 16, ///< Medium error
	BLOCK_SENSE_HW         =  4 << 16, ///< Hardware error
	BLOCK_SENSE_ILLREQ     =  5 << 16, ///< Illegal request
	BLOCK_SENSE_ATTENTION  =  6 << 16, ///< Unit attention
	BLOCK_SENSE_DATAPROT   =  7 << 16, ///< Data protect
	BLOCK_SENSE_BLANK      =  8 << 16, ///< Blank check
	BLOCK_SENSE_ABORTED    = 11 << 16, ///< Aborted command
	BLOCK_SENSE_MISCOMPARE = 14 << 16  ///< Miscompare
};


/**
 * \brief Block device information flags.
 *
 * This enum provides mnemonics for the \a flags field of BlockDeviceInfo,
 * that is a combination of the following bits:
 * <table class="datatable">
 * <tr><td>0..7</td><td>partition identifier (0 if not a partition)</td></tr>
 * <tr><td>8..11</td><td>device type, useful for drive letter assignment</td></tr>
 * <tr><td>12</td><td>device with removable media</td></tr>
 * <tr><td>13</td><td>can check for media change (only meaningful if removable)</td></tr>
 * <tr><td>14</td><td>device is writable</td></tr>
 * <tr><td>15..31</td><td>reserved (zero)</td></tr>
 * </table>
 */
enum BlockDeviceInfoFlags
{
	
	BLOCK_DEVICE_INFO_PARTID    = 0xFF,    ///< Bit mask to get the partition identifier.
	BLOCK_DEVICE_INFO_TYPEMASK  = 15 << 8, ///< Bit mask to get the device type.
	BLOCK_DEVICE_INFO_TGENERIC  = 0 << 8,  ///< Device type: generic block device.
	BLOCK_DEVICE_INFO_TACTIVE   = 1 << 8,  ///< Device type: active or first primary partition.
	BLOCK_DEVICE_INFO_TLOGICAL  = 2 << 8,  ///< Device type: logical drive or removable drive.
	BLOCK_DEVICE_INFO_TPRIMARY  = 3 << 8,  ///< Device type: other primary partition.
	BLOCK_DEVICE_INFO_TFLOPPY   = 4 << 8,  ///< Device type: floppy drive.
	BLOCK_DEVICE_INFO_TCDDVD    = 5 << 8,  ///< Device type: CD or DVD drive.
	BLOCK_DEVICE_INFO_REMOVABLE = 1 << 12, ///< Removable media.
	BLOCK_DEVICE_INFO_MEDIACHG  = 1 << 13, ///< Can check for media change.
	BLOCK_DEVICE_INFO_WRITABLE  = 1 << 14  ///< Writable.
};


/** \brief Typedef shortcut for the struct BlockDeviceInfo. */
typedef struct BlockDeviceInfo BlockDeviceInfo;


/**
 * \brief Structure of information about a block device.
 *
 * Pass a pointer to a structure of this type to the BlockOperations::get_device_info()
 * block device operation to gather information about a block device.
 */
struct BlockDeviceInfo
{
	/// A combination of bits defined in the ::BlockDeviceInfoFlags enum.
	unsigned flags;
	/// The device number as specified by the MultiBoot standard.
	uint32_t multiboot_id;
};


/** \brief Typedef shortcut for the struct BlockMediumInfo. */
typedef struct BlockMediumInfo BlockMediumInfo;


/**
 * \brief Structure of information about a block device medium.
 *
 * Pass a pointer to a structure of this type to the BlockOperations::get_medium_info()
 * block device operation to gather information about the medium loaded in a block device.
 */
struct BlockMediumInfo
{
	/// Number of blocks in the device.
	uint64_t blocks_count;
	/// Size in bytes of a block.
	unsigned block_bytes;
};


/**
 * \brief Flags for the read and write block device operations.
 */
enum BlockReadWriteFlags
{
	BLOCK_RW_NOCACHE = 1 << 0 ///< Transfer data bypassing the device cache, if any.
};


/**
 * \brief Operations type for struct BlockOperations.
 */
#define BLOCK_OPERATIONS_TYPE 1


/** \brief Typedef shortcut for the struct BlockOperations. */
typedef struct BlockOperations BlockOperations;


/**
 * \brief Structure of operations for block devices.
 */
struct BlockOperations
{
	/**
	 * \brief Requests an operation not included in this structure.
	 * \remarks In general it is not required to claim the device using
	 *          the \a open operation before calling the request function.
	 * \remarks The caller shall release the device when it is no longer
	 *          needed using ::REQ_RELEASE.
	 */
	int (*request)(int function, ...);
	/**
	 * \brief Opens a block device.
	 * \param handle opaque identifier of the block device.
	 * \remarks This operation shall be called before using any other
	 *          operation on a block device (unless the operation is marked
	 *          with a specific exception) to claim the use of the device.
	 * \remarks The block device driver shall perform any required
	 *          initialization, such as probing the medium format and
	 *          initializing its cache.
	 * \remarks This operation shall try to acquire an exclusive advisory
	 *          lock, so that the caller can gain exclusive access to the
	 *          block device. If the lock cannot be successfully acquired,
	 *          the caller shall not use the block device.
	 * \remarks This operation shall be atomic.
	 */
	int (*open)(void *handle);
	/**
	 * \brief Revalidates a block device.
	 * \param handle opaque identifier of the block device.
	 * \remarks This operation shall be called when an error condition
	 *          requiring to revalidate the device is reported, that is an
	 *          "attention" sense key (this could happen, for example, due to media change).
	 * \remarks The block device driver must perform any required
	 *          reinitialization, such as probing the medium format, and
	 *          should keep its cache if it has one.
	 * \remarks As a typical consequential action, the caller may read some data
	 *          structure stored on the block device (such as a serial number)
	 *          to check for media change, of course disabling any cache to
	 *          do this read. The caller may cause any subsequent action requiring
	 *          the old media (for example, a mounted file system) to fail, or
	 *          may eventually invalidate any cache by closing the device. The
	 *          caller must assure that these steps are performed atomically
	 *          in response to an error with an "attention" sense key.
	 */
	int (*revalidate)(void *handle);
	/**
	 * \brief Closes a block device.
	 * \param handle opaque identifier of the block device.
	 * \remarks This operation must be called when the block device is
	 *          no longer needed to unclaim it.
	 * \remarks The block device driver must perform any required cleanup,
	 *          release its exclusive lock and discard any pending
	 *          cached data.
	 */
	int (*close)(void *handle);
	/**
	 * \brief Gets information about a block device.
	 * \param handle opaque identifier of the block device;
	 * \param buf    address of a buffer to obtain device information.
	 * \remarks Information returned by this operation is not dependent on
	 *          the medium currently loaded (for example, the drive type), thus
	 *          this function may be called even if the device has not been
	 *          claimed using the \a open operation.
	 */
	int (*get_device_info)(void *handle, BlockDeviceInfo *buf);
	/**
	 * \brief Gets information about the medium implementing the block device.
	 * \param handle opaque identifier of the block device;
	 * \param buf    address of a buffer to obtain medium information.
	 * \remarks Information returned by this operation depends on the medium
	 *          currently loaded, such as its capacity. Thus the medium must
	 *          be first probed using the \a open or \a revalidate operations.
	 *          In fact, this operation is often called right after \a open
	 *          or \a revalidate.
	 */
	int (*get_medium_info)(void *handle, BlockMediumInfo *buf);
	/**
	 * \brief Reads data from a block device.
	 * \param handle opaque identifier of the block device;
	 * \param buffer address of a buffer to obtain the data read;
	 * \param start  block number of the first block to transfer;
	 * \param count  number of block to transfer;
	 * \param flags  a combination of flags specified by enum ::BlockReadWriteFlags.
	 */
	ssize_t (*read)(void *handle, void *buffer, uint64_t start, size_t count, int flags);
	/**
	 * \brief Writes data to a block device.
	 * \param handle opaque identifier of the block device;
	 * \param buffer address of a buffer containing the data to write;
	 * \param start  block number of the first block to transfer;
	 * \param count  number of block to transfer.
	 * \param flags  a combination of flags specified by enum ::BlockReadWriteFlags.
	 */
	ssize_t (*write)(void *handle, const void *buffer, uint64_t start, size_t count, int flags);
	/**
	 * \brief Flushes any cached data to the block device.
	 * \param handle opaque identifier of the block device.
	 * \remarks The block device driver must write any pending cached data
	 *          to the block device.
	 * \remarks In particular this operation should be called before closing
	 *          the device in order to save any cached data that would be
	 *          otherwise discarded. The caller must assure that no other writes
	 *          occur between the calls to \a sync and \a close.
	 */
	int (*sync)(void *handle);
};


/**
 * \brief Request to get a structure of operations.
 *
 * Call the request function as follows:
 * \code int request(REQ_GET_OPERATIONS, int type, void **operations); \endcode
 * where \a type is the type of structure of operations to obtain, and \a operations
 * is the address of a pointer to receive the structure of operations. The
 * \a operations pointer may be NULL if the caller is only interested in checking
 * if the specified \a type is supported. If \a operations is not NULL, on success
 * the reference count of the driver is incremented.
 */
#define REQ_GET_OPERATIONS 0

/**
 * \brief Request to get the reference count of an object.
 *
 * Call the request function as follows:
 * \code int request(REQ_GET_REFERENCES); \endcode
 * The current non-negative reference count of the object is returned.
 */
#define REQ_GET_REFERENCES 1

/**
 * \brief Request to release an object.
 *
 * Call the request function as follows:
 * \code int request(REQ_RELEASE, void *handle); \endcode
 * where \a handle is the opaque identifier of the object to release.
 * The reference count of the object must be decreased and the caller must
 * not use any longer the interface used to operate on the object.
 */
#define REQ_RELEASE 2


/* Nils: these belongs in this file? */
static inline int req_get_operations(int (*r)(int function, ...), int type, void **operations)
{
    return r(REQ_GET_OPERATIONS, type, operations);
}

static inline int req_get_references(int (*r)(int function, ...))
{
    return r(REQ_GET_REFERENCES);
}

static inline int req_release(int (*r)(int function, ...), void *handle)
{
    return r(REQ_RELEASE, handle);
}

const char *block_enumerate (void **iterator);
int         block_get       (const char *name, int type, void **operations, void **handle);
int         block_register  (const char *name, int (*request)(int function, ...), void *handle);
int         block_unregister(const char *name);

/* @} */

#endif /* #ifndef __FD32_BLOCK_H */
