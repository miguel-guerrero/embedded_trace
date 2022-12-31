//--------------------------------------------------------------------------
// Example of use
//--------------------------------------------------------------------------
#include "emb_log.h"
#include "msgs_auto.h"

void some_event()
{
    EMB_LOG_SOME_EVENT();
}

void long_comp()
{
    unsigned long long i;
    EMB_LOG_LONG_COMP_BODY(1);
    for (i=0; i<1000000ULL; i++) {}
    EMB_LOG_LONG_COMP_BODY(0);
}


void misc()
{
    EMB_LOG_MSG1(1, 111);
    EMB_LOG_MSG2(1, 221);
    EMB_LOG_MSG3(1, 331);
    // ...
    EMB_LOG_MSG3(0, 330);
    EMB_LOG_MSG2(0, 220);
    EMB_LOG_MSG1(0, 110);
}

int main() 
{
    int i;

    emb_log_init();
    emb_log_set_enable(1);

    for (i=100; i; i--)
    {
        EMB_LOG_ITER_START(); // this is just an event to signal start of iteration

        some_event();
        long_comp();
        misc();

        EMB_LOG_ITER_STOP(); // event to indicate end of the iteration
    }
    emb_log_dump(0);

    return 0;
}
