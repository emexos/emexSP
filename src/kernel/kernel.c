#include "../include/boot.h"
#include "../include/text/text_utils.h"
#include "../drivers/keyboard/keyboard.h"
#include "../include/memory/memory.h"
#include "../shell/shell.h"

void stmain(BootInfo* binfo)
{
    if (!binfo)
        goto halt;

    clear(COLOR_DEFAULT);

    print("emexOS3 loaded successful with XBL2 \n", 0x4D);

    print("Binfo at: ", COLOR_DEFAULT);
    print_hex((uint64_t)binfo, COLOR_DEFAULT);
    print("\n", COLOR_DEFAULT);

    print("Memory map: ", COLOR_DEFAULT);
    print_dec(binfo->memmap.entry_count, COLOR_DEFAULT);
    print(" entries\n", COLOR_DEFAULT);

    for (int i = 0; i < binfo->memmap.entry_count; i++)
    {
        E820Entry* entry = &binfo->memmap.entries[i];
        if (entry->length == 0)
            continue;

        print_hex(entry->base, COLOR_DEFAULT);
        print(" - ", COLOR_DEFAULT);
        print_hex(entry->base + entry->length - 1, COLOR_DEFAULT);
        print(" | ", COLOR_DEFAULT);

        uint8_t type_color;
        const char* type_str;

        switch (entry->type)
        {
            case E820_TYPE_USABLE:
                type_color = 0x0A; // green
                type_str = "Usable";
                break;
            case E820_TYPE_RESERVED:
                type_color = 0x0C; // red
                type_str = "Reserved";
                break;
            case E820_TYPE_ACPI_RECLAIM:
                type_color = 0x0B; // cyan
                type_str = "ACPI Reclaim";
                break;
            case E820_TYPE_ACPI_NVS:
                type_color = 0x0B; // cyan
                type_str = "ACPI NVS";
                break;
            case E820_TYPE_BAD:
                type_color = 0x0C; // red
                type_str = "Bad Memory";
                break;
            default:
                type_color = 0x0F; // white
                type_str = "Unknown";
                break;
        }

        print(type_str, type_color);
        print("\n", COLOR_DEFAULT);
    }

    print("\nInitializing keyboard driver...\n", COLOR_DEFAULT);
    keyboard_init();
    print("Keyboard driver initialized successfully!\n", 0x0A);

    print("\nInitializing memory manager...\n", COLOR_DEFAULT);
    // Memory manager will be initialized in the shell
    print("Memory manager ready!\n", 0x0A);

    print("\nLoading Shell...\n", COLOR_DEFAULT);

    clear(COLOR_DEFAULT);

    shell();

halt:
    while (1)
    {
        asm volatile("hlt");
    }
}
