// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint nfat;         // Number of FAT blocks
  uint fatstart;     // Block number of first FAT block
  uint freehead;     // Head of the free block list
  uint freeblks;     // Number of free data blocks
};

#define FSMAGIC 0x10203040
#define FSMAGIC_FATTY 0x46415459    // FATY

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint startblk;       // First block number
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// FAT entry per block
#define FPB           (BSIZE/4)

// Block of FAT containing fat entry for block b
#define FBLOCK(b, sb) ((b)/FPB + sb.fatstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

#define NFAT (FSSIZE/FPB+1)

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

