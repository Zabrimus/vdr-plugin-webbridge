#include <vdr/tools.h>

void printBacktrace() {
    cStringList stringList;

    cBackTrace::BackTrace(stringList, 0, false);
    esyslog("[webbridge] Backtrace size: %d", stringList.Size());

    for (int i = 0; i < stringList.Size(); ++i) {
        esyslog("[webbridge] ==> %s", stringList[i]);
    }

    esyslog("[webbridge] ==> Caller: %s", *cBackTrace::GetCaller());
}