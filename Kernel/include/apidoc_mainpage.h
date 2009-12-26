/**
 * \file apidoc_mainpage.h
 * \brief API Documentation Home Page
 * \author John Hodge (thePowersGang)
 */

/**
 * \mainpage Acess 2 Kernel API Documentation
 * 
 * \section intro Introduction
 * These documents attempt to describe the standard Acess 2 (and hopefully
 * future versions) Kernel mode API.
 * The documentation covers filesystem drivers, binary formats and the
 * various device driver interface standards.
 * 
 * \section VFS
 * The core of Acess is the VFS, or Virtual File System. The VFS abstracts
 * away from the user the differences between different filesystems,
 * network protocols and types of hardware.
 * The core of the VFS is the concept of a VFS Node (represented by the
 * ::tVFS_Node type). A node defines a file (either a normal file, directory
 * or some device abstraction), it's attributes (size, flags, timestamps)
 * and the functions used to access and modify it.
 * 
 * \subsection filesystems Filesystem Drivers
 * Acess filesystems register themselves with the VFS by calling
 * ::VFS_AddDriver with a ::tVFS_Driver structure that defines the driver's
 * name, flags and mount function.
 * Filesystem drivers take the 
 * 
 * \section binfmt Binary Formats
 * See binary.h
 * 
 * \section drivers	Device Drivers
 * All Acess2 device drivers communicate with user-level (and other parts
 * of the greater kernel) via the VFS. They register themselves in a similar
 * way to how filesystem drivers do, however instead of registering with
 * the VFS core, they register with a special filesystem driver called the
 * DevFS (fs_devfs.h). DevFS exports the ::DevFS_AddDevice function that
 * takes a ::tDevFS_Driver structure as an agument that defines the
 * driver's name and the VFS node of it's root. This root is used to
 * provide the user access to the driver's function via IOCtl calls and
 * by Read/Write calls. Drivers are also able to expose a readonly buffer
 * by using ProcDev, usually to provide state information (such as usage
 * statistics and other misc information)
 * 
 * The device driver interfaces are all based on the core specifcation
 * in tpl_drv_common.h (Common Device Driver definitions).
 * The following subsections define the various specific types of driver
 * interfaces. These definitions only define the bare minimum of what the
 * driver must implement, if the driver author so wants to, they can add
 * IOCtl calls and/or files (where allowed by the type specifcation) to
 * their device's VFS layout.
 * 
 * \subsection drv_video Video Devices
 * Video drivers are based on a framebuffer model (unless in 3D mode,
 * which is not yet fully standardised, so should be ignored).
 * The driver will contain only one VFS node, that exposes the video
 * framebuffer (this may not be the true framebuffer, to allow for double-buffering)
 * to the user. See the full documentation in tpl_drv_video.h for the
 * complete specifcation.
 */
