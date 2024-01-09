#ifndef SCANNER_H
#define SCANNER_H

class Scanner {
public:
    char* getCurrentIpAddress();
    char* getCurrentSubnetMask();
    char* getHosts();
    void performScan();
};

#endif 
