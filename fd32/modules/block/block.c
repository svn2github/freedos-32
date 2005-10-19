/* The FreeDOS-32 Block Device Manager version 1.0
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <dr-env.h>
#include <list.h>
#include <slabmem.h>
#include <errno.h>
#include "block.h"


/**
\defgroup block Block device manager and block interface
 
\section block_overview Overview
Block devices are facilities that provide I/O access through equally sized
blocks and can contain a file system.

To use the block device functionality:
\code #include <block/block.h> \endcode
 
Each block device is registered to the Block device manager and is identified
through a unique name, that is a UTF-8 string which is considered case
insensitive in respect to characters in the ASCII set (that is, 'a'..'z').
This allows to use a simple implementation of \c strcasecmp for name comparison,
without extensive knowledge of the UTF-8 encoding and Unicode case folding,
which is not trivial.

A block device driver shall expose its functionality through a variadic request
function, having the following prototype:
\code int request(int function, ...) \endcode
where the \a function argument specifies the kind of service requested to the
driver, and other parameters may follow depending on the requested function.
A block device driver shall support at least the following requests:
<table class="datatable">
<tr><th>mnemonic</th><th>purpose</th></tr>
<tr><td>::REQ_GET_OPERATIONS</td><td>query the driver for a structure of operations</td></tr>
<tr><td>::REQ_GET_REFERENCES</td><td>get the value of the reference count of the driver</td></tr>
<tr><td>::REQ_RELEASE</td><td>release an object</td></tr>
</table>
In addition to the request function, a driver may expose other interfaces that
can be queried using the request function.

A driver shall maintain an internal reference counter that is used to keep track
of the users of the facilities of the driver. Whenever an interface is queried,
including the generic one represented by the request function, the reference
count shall be increased. When a user does not need to use the functionality of
the driver any longer, it shall release the interface it got and the driver shall
decrease its reference count. The driver can be unloaded only if the reference
count is zero.

A driver may need to manage several devices or facilities. In this case the driver
shall associate to each device or facility a an opaque identifier, in form of a void
pointer, that the caller shall use in the request function or other interfaces to
specify what device or facility to operate on. The opaque identifier is usually
the address of a structure used by the driver to store the state of the device or
facility.
 
@{
*/


/* A registered block device */
typedef struct BlockDevice BlockDevice;
struct BlockDevice
{
	BlockDevice *prev; /* from ListItem */
	BlockDevice *next; /* from ListItem */
	int (*request)(int function, ...);
	void *handle;
	const char *name;
};

/* Linked list of registered devices */
static List list =
{
	.begin = NULL,
	.end   = NULL,
	.size  = 0
};

/* Slab cache to store registered devices */
static slabmem_t slab =
{
	.pages     = NULL,
	.obj_size  = sizeof(BlockDevice),
	.free_objs = NULL
};


/**
 * \brief Enumerates the registered block devices.
 * \param iterator opaque iterator to store the enumeration progress.
 * \return The address of the name of the next device, or NULL if there are
 *         no more devices.
 * \remarks The pointer pointed by \a iterator shall be initially set to NULL
 *          so that this function knows it has to start to enumerate. Each time
 *          this function is called, it returns the name of the next registered
 *          device (in registration time order) and updates the pointer
 *          pointed by \a iterator to let the caller continue the enumeration.
 * \remarks When all devices have been enumerated, NULL is returned. In this
 *          case the \a iterator is reinitialized with a NULL pointer so that
 *          calling this function again restarts the enumeration.
 */
const char *block_enumerate(void **iterator)
{
	BlockDevice *d = (BlockDevice *) *iterator;
	if (d) d = d->next;
	  else d = (BlockDevice *) list.begin;
	*iterator = (void *) d;
	if (d) return d->name;
	return NULL;
}


/**
 * \brief Gets operations for a block device.
 * \param name name of the requested device;
 * \param type the requested type of operations;
 * \param[out] operations address of a pointer to the returned operations;
 *             it may be NULL to just check if the requested type is available.
 * \param[out] handle to receive the opaque identifier of the device;
 *             it may be NULL if \a operations is NULL, but it should not be
 *             NULL otherwise.
 * \retval 0 success, \a operations and \a handle updated if not NULL;
 * \retval -ENOTSUP the ::REQ_GET_OPERATION request or the specified type of
 *                  operations is not available. The former should not happen
 *                  as all devices should implement ::REQ_GET_OPERATIONS.
 * \remarks The Block Device Manager uses the ::REQ_GET_OPERATIONS
 *          request to get the specified type of operations. If this request
 *          succeeds the reference count of the device shall be increased.
 */
int block_get(const char *name, int type, void **operations, void **handle)
{
	BlockDevice *d;
	if (name)
		for (d = (BlockDevice *) list.begin; d; d = d->next)
		{
			assert(d->name);
			if (strcasecmp(name, d->name) == 0)
			{
				assert(d->handle);
				assert(d->request);
				if (handle) *handle = d->handle;
				return d->request(REQ_GET_OPERATIONS, type, operations);
			}
		}
	return -EINVAL;
}


/**
 * \brief  Registers a device to the Block Device Manager.
 * \param  name    name of the device to register;
 * \param  request address of the request function for the device;
 * \param  handle  opaque identifier for the device to register, may be NULL.
 * \retval 0 success;
 * \retval -EEXIST a device with the same name already exists;
 * \retval -ENOMEM not enough memory to register the device.
 * \remarks The string pointed by \a name shall not be deallocated while the
 *          device is registered. The request function should support at least
 *          the ::REQ_GET_OPERATIONS, ::REQ_GET_REFERENCES and ::REQ_RELEASE requests. The
 *          \a handle parameter may be NULL if the block device driver does
 *          not need to distinguish between several devices.
 */
int block_register(const char *name, int (*request)(int function, ...), void *handle)
{
	BlockDevice *d;
	for (d = (BlockDevice *) list.begin; d; d = d->next)
		if (!strcasecmp(d->name, name)) return -EEXIST;
	d = (BlockDevice *) slabmem_alloc(&slab);
	if (!d) return -ENOMEM;
	d->request = request;
	d->handle  = handle;
	d->name    = name;
	list_push_back(&list, (ListItem *) d);
	message("[BLOCK] Device \"%s\" registered\n", name);
	return 0;
}


/**
 * \brief  Unregisters a device from the Block Device Manager.
 * \param  name name of the device to unregister.
 * \retval 0 success;
 * \retval -ENOENT a device with the specified name is not registered;
 * \retval -EBUSY  the reference count of the device is not zero
 *                 (somebody is using the device).
 */
int block_unregister(const char *name)
{
	BlockDevice *d;
	for (d = (BlockDevice *) list.begin; d; d = d->next)
		if (!strcasecmp(d->name, name))
		{
			assert(d->request);
			if (d->request(REQ_GET_REFERENCES) > 0) return -EBUSY;
			list_erase(&list, (ListItem *) d);
			slabmem_free(&slab, d);
			return 0;
		}
	return -ENOENT;
}


/****************************************************************************** 
 * Block Device Manager initialization.
 ******************************************************************************/


#include <kernel.h>
#include <ll/i386/error.h>
static const struct { /*const*/ char *name; DWORD address; } symbols[] =
{
	{ "block_enumerate",  (DWORD) block_enumerate  },
	{ "block_get",        (DWORD) block_get        },
	{ "block_register",   (DWORD) block_register   },
	{ "block_unregister", (DWORD) block_unregister },
	{ 0, 0 }
};


/* Module entry point */
void block_init(void)
{
	int k;
	message("Going to install the Block Device Manager... ");
	for (k = 0; symbols[k].name; k++)
		if (add_call(symbols[k].name, symbols[k].address, ADD) == -1)
			message("Cannot add %s to the symbol table\n", symbols[k].name);
	message("Done\n");
}

/* @} */
