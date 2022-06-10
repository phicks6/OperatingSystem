#include <rtc.h>

#define RTC_BASE              0x101000

#define RTC_TIME_LOW          0x00
#define RTC_TIME_HI           0x04
#define RTC_ALARM_LOW         0x08
#define RTC_ALARM_HI          0x0c
#define RTC_IRQ               0x10
#define RTC_CLEAR_ALARM       0x14
#define RTC_ALARM_STATUS      0x18
#define RTC_CLEAR_INTERRUPT   0x1c

unsigned long rtc_get_time(void) {
    unsigned int *l = (unsigned int *)(RTC_BASE + RTC_TIME_LOW);
    unsigned int *h = (unsigned int *)(RTC_BASE + RTC_TIME_HI);
    
    unsigned long low = *l;
    unsigned long high = *h;

    // RTC has a 1 nanosecond precision. Divide by 1000000000 to get
    // one second.

    return (high << 32) | low;
}
