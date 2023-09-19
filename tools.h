#ifndef _Tools_h
#define _Tools_h
String getFormatedTimeWork(int ti_start, int ti_end){
    uint32_t sec = (ti_end-ti_start)/ 1000ul;
    char str[10];
    sprintf(str, "%02d:%02d:%02d",  (sec / 3600ul), ((sec % 3600ul) / 60ul), ((sec % 3600ul) % 60ul));
    return String(str);
    };
#endif