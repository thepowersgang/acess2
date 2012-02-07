/**
 * \file apidoc_mainpage.h
 * \brief API Documentation Home Page
 * \author John Hodge (thePowersGang)
 * 
 * \mainpage Acess 2 Kernel API Documentation
 * 
 * \section intro Introduction
 * These documents attempt to describe the standard Acess 2 (and hopefully
 * future versions) Kernel mode API.
 * The documentation covers filesystem drivers, binary formats and the
 * various device driver interface standards.
 * 
 * \section index	"Sections"
 * - \ref modules.h "Module Definitions"
 *  - Describes how a module is defined in Acess
 * - \ref binary.h	"Binary Formats"
 *  - Explains how to register a new binary format with the kernel
 * - \ref vfs.h	"VFS - The Virtual File System"
 *  - The VFS is the core of Acess's driver architecture
 * - \ref drivers	"Device Drivers"
 *  - Describes how drivers should use the VFS to expose themselves to the
 *    user.
 *  - Drivers for specific types of hardware must behave in the specific
 *    way described here.
 * 
 * \page drivers Device Drivers
 * 
 * \section toc	Contents
 * - \ref drvintro	"Introduction"
 * - \ref drv_misc "Miscelanious Devices"
 * - \ref drv_video	"Video Drivers"
 * 
 * \section drvintro	Introduction
 * All Acess2 device drivers communicate with user-level (and other parts
 * of the greater kernel) via the VFS. They register themselves in a similar
 * way to how filesystem drivers do, however instead of registering with
 * the VFS core, they register with a special filesystem driver called the
 * \ref fs_devfs.h "Device Filesystem" (devfs). The DevFS provides the
 * ::DevFS_AddDevice function that takes a ::tDevFS_Driver structure as
 * an agument. This structure specifies the driver's name and its root
 * VFS node. This node is used to provide the user access to the
 * driver's functions via IOCtl calls and Reading or Writing to the driver
 * file. Drivers are also able to expose a readonly buffer by using
 * \ref fs_sysfs.h "ProcDev", usually to provide state information or device
 * capabilities for the the user.
 * 
 * The device driver interfaces are all based on the core specifcation
 * in api_drv_common.h (Common Device Driver definitions).
 * The following subsections define the various specific types of driver
 * interfaces. These definitions only define the bare minimum of what the
 * driver must implement, if the driver author so wants to, they can add
 * IOCtl calls and/or files (where allowed by the type specifcation) to
 * their device's VFS layout.
 * 
 * \subsection drv_misc Miscelanious Devices
 * If a device type does not have a specifcation for it, the driver can
 * identify itself as a miscelanious device by returning DRV_TYPE_MISC
 * from \ref DRV_IOCTL_TYPE.
 * A misc device must at least implement the IOCtl calls defined in the
 * \ref api_drv_common.h "Common Device Driver definitions", allowing it
 * to be identified easily by the user and for interfacing programs to
 * utilise the DRV_IOCTL_LOOKUP call.
 * 
 * \subsection drv_video Video Devices
 * Video drivers are based on a framebuffer model (unless in 3D mode,
 * which is not yet fully standardised, so should be ignored).
 * The driver will contain only one VFS node, that exposes the video
 * framebuffer (this may not be the true framebuffer, to allow for double-buffering)
 * to the user. See the full documentation in api_drv_video.h for the
 * complete specifcation.
 * 
 * \subsection drv_disk Disk/Storage Devices
 * Storage devices present themselves as a linear collection of bytes.
 * Reads and writes to the device need not be aligned to the stated block
 * size, but it is suggested that users of a storage device file align
 * their accesses to block boundaries.
 * The functions DrvUtil_ReadBlock and DrvUtil_WriteBlock are provided
 * to storage drivers to assist in handling non-alinged reads and writes.
 * 
 * \see api_drv_common.h Common Spec.
 * \see api_drv_video.h Video Device Spec.
 * \see api_drv_keyboard.h Keyboard Device Spec.
 * \see api_drv_disk.h Disk/Storage Device Spec.
 * \see api_drv_network.h Network Device Spec.
 * \see api_drv_terminal.h Virtual Terminal Spec.
 */
