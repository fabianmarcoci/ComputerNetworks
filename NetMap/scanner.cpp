#include "scanner.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <net/if.h>
#include <stdio.h>

char *Scanner::getCurrentIpAddress()
{
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *address = NULL;
    int success = 0;

    success = getifaddrs(&interfaces); // Obține lista interfețelor
    if (success == 0)
    {
        for (address = interfaces; address != NULL; address = address->ifa_next)
        {
            if (address->ifa_addr != NULL && address->ifa_addr->sa_family == AF_INET)
            { // Verifică dacă este IPv4 și ifa_addr nu este NULL
                // Verifică dacă interfața nu este loopback și este activă
                if (strcmp(address->ifa_name, "lo") != 0 && (address->ifa_flags & IFF_UP))
                {
                    void *tmpAddrPtr = &((struct sockaddr_in *)address->ifa_addr)->sin_addr;
                    char *addressBuffer = (char *)malloc(INET_ADDRSTRLEN);
                    if (addressBuffer != NULL)
                    { // Asigură-te că malloc nu a eșuat
                        inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                        freeifaddrs(interfaces); // Nu uita să eliberezi lista interfețelor
                        return addressBuffer;    // Acesta trebuie eliberat de către apelant
                    }
                    else
                    {
                        printf("Function malloc failed to allocate memory.");
                    }
                }
            }
        }
    }
    freeifaddrs(interfaces); // Eliberează memoria alocată listei interfețelor
    return NULL;
}

char *Scanner::getCurrentSubnetMask()
{
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *address = NULL;
    int success = 0;

    success = getifaddrs(&interfaces); // Obține lista interfețelor
    if (success == 0)
    {
        for (address = interfaces; address != NULL; address = address->ifa_next)
        {
            if (address->ifa_addr != NULL && address->ifa_addr->sa_family == AF_INET)
            { // Verifică dacă este IPv4
                if (address->ifa_netmask != NULL)
                { // Asigură-te că ifa_netmask nu este NULL
                    // Verifică dacă interfața nu este loopback și este activă
                    if (strcmp(address->ifa_name, "lo") != 0 && (address->ifa_flags & IFF_UP))
                    {
                        void *tmpAddrPtr = &((struct sockaddr_in *)address->ifa_netmask)->sin_addr;
                        char *addressBuffer = (char *)malloc(INET_ADDRSTRLEN);
                        if (addressBuffer != NULL)
                        { // Asigură-te că malloc nu a eșuat
                            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                            freeifaddrs(interfaces); // Nu uita să eliberezi lista interfețelor
                            return addressBuffer;    // Acesta trebuie eliberat de către apelant
                        }
                        else
                        {
                            printf("Function malloc failed to allocate memory.");
                        }
                    }
                }
            }
        }
    }
    freeifaddrs(interfaces); // Eliberează memoria alocată listei interfețelor
    return NULL;
}

char *Scanner::getHosts()
{
    char *ipAddr = Scanner::getCurrentIpAddress();
    char *subnetMask = Scanner::getCurrentSubnetMask();
    if (ipAddr == NULL)
    {
        fprintf(stderr, "Failed to get the current IP address.\n");
        free(subnetMask);
        return NULL;
    }
    if (subnetMask == NULL)
    {
        fprintf(stderr, "Failed to get the current subnet mask.\n");
        free(ipAddr);
        return NULL;
    }
    printf("%s\n", ipAddr);
    printf("%s\n", subnetMask);

    free(ipAddr);
    free(subnetMask);
    return NULL;
}

void Scanner::performScan()
{
    Scanner::getHosts();
}
