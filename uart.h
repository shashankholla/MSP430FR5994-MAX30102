

#ifndef UCA0_H_
    #define UCA0_H_

    #include <stdbool.h>
    #include <stdint.h>

    #define RX_BUFF                             128

    extern char *uca0Receiver;
    extern bool uca0DataReady;

    void uca0Init(void);
    void uca0WriteByte(const char byte);
    void uca0WriteString(char *str);
    char uca0ReadByte(void);
    char *uca0ReadString(void);


#endif /* UCA0_H_ */
