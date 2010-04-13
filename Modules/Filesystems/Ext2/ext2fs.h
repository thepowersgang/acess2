/**
 * Acess2
 * \file ext2fs.h
 * \brief EXT2 Filesystem Driver
 */
#ifndef _EXT2FS_H_
#define _EXT2FS_H_

/**
 \name Inode Flag Values
 \{
*/
#define EXT2_S_IFMT		0xF000	//!< Format Mask
#define EXT2_S_IFSOCK	0xC000	//!< Socket
#define EXT2_S_IFLNK	0xA000	//!< Symbolic Link
#define EXT2_S_IFREG	0x8000	//!< Regular File
#define EXT2_S_IFBLK	0x6000	//!< Block Device
#define EXT2_S_IFDIR	0x4000	//!< Directory
#define EXT2_S_IFCHR	0x2000	//!< Character Device
#define EXT2_S_IFIFO	0x1000	//!< FIFO
#define EXT2_S_ISUID	0x0800	//!< SUID
#define EXT2_S_ISGID	0x0400	//!< SGID
#define EXT2_S_ISVTX	0x0200	//!< sticky bit
#define EXT2_S_IRWXU	0700	//!< user access rights mask
#define EXT2_S_IRUSR	0400	//!< Owner Read
#define EXT2_S_IWUSR	0200	//!< Owner Write
#define EXT2_S_IXUSR	0100	//!< Owner Execute
#define EXT2_S_IRWXG	0070	//!< Group Access rights mask
#define EXT2_S_IRGRP	0040	//!< Group Read
#define EXT2_S_IWGRP	0020	//!< Group Write
#define EXT2_S_IXGRP	0010	//!< Group Execute
#define EXT2_S_IRWXO	0007	//!< Global Access rights mask
#define EXT2_S_IROTH	0004	//!< Global Read
#define EXT2_S_IWOTH	0002	//!< Global Write
#define EXT2_S_IXOTH	0001	//!< Global Execute
//! \}

#define EXT2_NAME_LEN 255	//!< Maximum Name Length

// === TYPEDEFS ===
typedef struct ext2_inode_s			tExt2_Inode;	//!< Inode Type
typedef struct ext2_super_block_s	tExt2_SuperBlock;	//!< Superblock Type
typedef struct ext2_group_desc_s	tExt2_Group;	//!< Group Descriptor Type
typedef struct ext2_dir_entry_s		tExt2_DirEnt;	//!< Directory Entry Type

// === STRUCTURES ===
/**
 * \brief EXT2 Superblock Structure
 */
struct ext2_super_block_s {
	Uint32	s_inodes_count;		//!< Inodes count
	Uint32	s_blocks_count;		//!< Blocks count
	Uint32	s_r_blocks_count;	//!< Reserved blocks count
	Uint32	s_free_blocks_count;	//!< Free blocks count
	Uint32	s_free_inodes_count;	//!< Free inodes count
	Uint32	s_first_data_block;	//!< First Data Block
	Uint32	s_log_block_size;	//!< Block size
	Sint32	s_log_frag_size;	//!< Fragment size
	Uint32	s_blocks_per_group;	//!< Number Blocks per group
	Uint32	s_frags_per_group;	//!< Number Fragments per group
	Uint32	s_inodes_per_group;	//!< Number Inodes per group
	Uint32	s_mtime;			//!< Mount time
	Uint32	s_wtime;			//!< Write time
	Uint16	s_mnt_count;		//!< Mount count
	Sint16	s_max_mnt_count;	//!< Maximal mount count
	Uint16	s_magic;			//!< Magic signature
	Uint16	s_state;			//!< File system state
	Uint16	s_errors;			//!< Behaviour when detecting errors
	Uint16	s_pad;				//!< Padding
	Uint32	s_lastcheck;		//!< time of last check
	Uint32	s_checkinterval;	//!< max. time between checks
	Uint32	s_creator_os;		//!< Formatting OS
	Uint32	s_rev_level;		//!< Revision level
	Uint16	s_def_resuid;		//!< Default uid for reserved blocks
	Uint16	s_def_resgid;		//!< Default gid for reserved blocks
	Uint32	s_reserved[235];	//!< Padding to the end of the block
};

/**
 * \struct ext2_inode_s
 * \brief EXT2 Inode Definition
 */
struct ext2_inode_s {
	Uint16 i_mode;	//!< File mode
	Uint16 i_uid;	//!< Owner Uid
	Uint32 i_size;	//!< Size in bytes
	Uint32 i_atime;	//!< Access time
	Uint32 i_ctime; //!< Creation time
	Uint32 i_mtime; //!< Modification time
	Uint32 i_dtime; //!< Deletion Time
	Uint16 i_gid;	//!< Group Id
	Uint16 i_links_count;	//!< Links count
	Uint32 i_blocks;	//!< Number of blocks allocated for the file
	Uint32 i_flags;	//!< File flags
	union {
		Uint32 linux_reserved1;	//!< Linux: Reserved
		Uint32 hurd_translator;	//!< HURD: Translator
		Uint32 masix_reserved1;	//!< Masix: Reserved
	} osd1;	//!< OS dependent 1
	Uint32 i_block[15];	//!< Pointers to blocks
	Uint32 i_version; 	//!< File version (for NFS)
	Uint32 i_file_acl;	//!< File ACL
	Uint32 i_dir_acl;	//!< Directory ACL / Extended File Size
	Uint32 i_faddr; 	//!< Fragment address
	union {
		struct {
			Uint8 l_i_frag;	//!< Fragment number
			Uint8 l_i_fsize;	//!< Fragment size
			Uint16 i_pad1;	//!< Padding
			Uint32 l_i_reserved2[2];	//!< Reserved
		} linux2;
		struct {
			Uint8 h_i_frag; //!< Fragment number
			Uint8 h_i_fsize; //!< Fragment size
			Uint16 h_i_mode_high;	//!< Mode High Bits
			Uint16 h_i_uid_high;	//!< UID High Bits
			Uint16 h_i_gid_high;	//!< GID High Bits
			Uint32 h_i_author;	//!< Creator ID
		} hurd2;
		struct {
			Uint8 m_i_frag;	//!< Fragment number
			Uint8 m_i_fsize;	//!< Fragment size
			Uint16 m_pad1;	//!< Padding
			Uint32 m_i_reserved2[2];	//!< reserved
		} masix2;
	} osd2; //!< OS dependent 2
};

/**
 * \struct ext2_group_desc_s
 * \brief EXT2 Group Descriptor
 */
struct ext2_group_desc_s {
	Uint32	bg_block_bitmap;	//!< Blocks bitmap block
	Uint32	bg_inode_bitmap;	//!< Inodes bitmap block
	Uint32	bg_inode_table;	//!< Inodes table block
	Uint16	bg_free_blocks_count;	//!< Free blocks count
	Uint16	bg_free_inodes_count;	//!< Free inodes count
	Uint16	bg_used_dirs_count;	//!< Directories count
	Uint16	bg_pad;	//!< Padding
	Uint32	bg_reserved[3];	//!< Reserved
};

/**
 * \brief EXT2 Directory Entry
 * \note The name may take up less than 255 characters
 */
struct ext2_dir_entry_s {
	Uint32	inode;		//!< Inode number
	Uint16	rec_len; 	//!< Directory entry length
	Uint8	name_len;	//!< Short Name Length
	Uint8	type;		//!< File Type
	char	name[];		//!< File name
};

#endif
