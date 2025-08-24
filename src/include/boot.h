#pragma once

#include <stdint.h>

#define E820_TYPE_USABLE           1
#define E820_TYPE_RESERVED         2
#define E820_TYPE_ACPI_RECLAIM     3
#define E820_TYPE_ACPI_NVS         4
#define E820_TYPE_BAD              5

// Video mode information
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t  bpp;           // bits per pixel
    uint8_t  type;          // 0=text, 1=graphics
    uint32_t framebuffer;   // physical address of framebuffer
    uint32_t pitch;         // bytes per scanline
} __attribute__((packed)) VideoInfo;

// Disk information
typedef struct {
    uint8_t drive_number;
    uint8_t drive_type;     // 0=floppy, 1=hard disk
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t total_sectors;
    uint16_t bytes_per_sector;
} __attribute__((packed)) DiskInfo;

// CPU features
typedef struct {
    uint32_t cpuid_max;
    uint32_t features_edx;  // from CPUID EAX=1
    uint32_t features_ecx;
    uint64_t extended_features; // from CPUID EAX=7
    char vendor_id[13];     // CPU vendor string
    char brand_string[49];  // CPU brand string
} __attribute__((packed)) CpuInfo;

// ACPI information
typedef struct {
    uint64_t rsdp_address;  // Root System Description Pointer
    uint8_t  acpi_version;  // 1 for ACPI 1.0, 2+ for ACPI 2.0+
} __attribute__((packed)) AcpiInfo;

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) E820Entry;

typedef struct {
    uint16_t entry_count;
    E820Entry entries[];
} __attribute__((packed)) MemmapInfo;

// File system boot record
typedef struct {
    uint32_t fs_type;       // 0=none, 1=fat12, 2=fat16, 3=fat32, 4=ext2
    uint32_t root_cluster;  // for FAT
    uint32_t fat_start;     // FAT start sector
    uint32_t data_start;    // Data area start sector
    uint16_t cluster_size;  // sectors per cluster
    uint16_t reserved;
} __attribute__((packed)) FsInfo;

// Enhanced BootInfo structure
typedef struct {
    MemmapInfo memmap;
    VideoInfo video;
    DiskInfo disk;
    CpuInfo cpu;
    AcpiInfo acpi;
    FsInfo filesystem;
    uint32_t boot_time;     // boot timestamp
    uint32_t flags;         // boot flags
} __attribute__((packed)) BootInfo;
