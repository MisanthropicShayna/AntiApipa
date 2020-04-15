#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <string.h>
#include <memory>
#include <regex>

#include <Windows.h>
#include <iphlpapi.h>

#pragma comment(lib, "IPHLPAPI.lib")

void ClearApipaAddresses() {
    // Gets a linked list of adapter, this being the initial element.
    IP_ADAPTER_INFO* initial_adapter = reinterpret_cast<IP_ADAPTER_INFO*>(malloc(sizeof(IP_ADAPTER_INFO)));

    /* This value is given to GetAdaptersInfo to tell it the size of the IP_ADAPTER_INFO struct,
     * but this must be a separate value since the size of the struct isn't big enough to store
     * all of the data given by GetAdaptersInfo, therefore the required size is written back
     * into adapter_buffer_size should the original size be insufficient. */
    ULONG adapter_buffer_size = sizeof(IP_ADAPTER_INFO);

    /* Here the check is made to see if the size is insufficient, if it is, then initial_adapter is reallocated with the correct size
     * that has now been written back into adapter_buffer_size by GetAdaptersInfo */
    if(GetAdaptersInfo(initial_adapter, &adapter_buffer_size) == ERROR_BUFFER_OVERFLOW) {
        free(initial_adapter);
        initial_adapter = reinterpret_cast<IP_ADAPTER_INFO*>(malloc(adapter_buffer_size));
        GetAdaptersInfo(initial_adapter, &adapter_buffer_size);
    }

    // Iterates through the linked list of adapters.
    for(IP_ADAPTER_INFO* adapter = initial_adapter; adapter != nullptr; adapter = adapter->Next) {
        // Iterates through the linked list of ip addresses stored per adapter.
        for(IP_ADDR_STRING* ip_address = &adapter->IpAddressList; ip_address != nullptr;) {
            /* APIPA Addresses have a fixed range of 169.254.0.1 through to 169.254.255.254,
             * so a regular expression can be used to determine if the current address is
             * one that has been assigned through APIPA */
            if(std::regex_match(ip_address->IpAddress.String, std::regex("169\\.254\\.\\d+\\.\\d+"))) {
                std::cout << "Found APIPA address: " << ip_address->IpAddress.String << std::endl;
                DeleteIPAddress(ip_address->Context);
            }

            /* Unlike the adapter linked list, every element in the address linked list
             * must be free'd to avoid a memory leak, except for the initial element as
             * it's a pointer to a variable within the adapter struct rather than its
             * own memory, and thus it can't be free'd without also freeing the parent
             * struct, so this check has to be made. */
            if(ip_address != &adapter->IpAddressList) {
                IP_ADDR_STRING* next_ip = ip_address->Next;
                free(next_ip);
                ip_address = next_ip;
            } else {
                ip_address = ip_address->Next;
            }
        }
    }

    free(initial_adapter);
}

int main(int argc, char* argv[]) {
    uint32_t refresh_rate = 500;

    // Command line argument parsing for specifying the refresh rate.
    for(int i=0; i<argc; ++i) {
        if((!_stricmp(argv[i], "--rate") || !_stricmp(argv[i], "-r")) && (i+1) < argc) {
            refresh_rate = strtol(argv[i+1], 0, 10);
        }
    }

    std::cout << "Running with refresh rate of " << refresh_rate << "ms" << std::endl;

    for(;;Sleep(refresh_rate)) {
        ClearApipaAddresses();
    }

    return 0;
}
