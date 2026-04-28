#if defined(MODE_RECORD)
    #include "mode_record.hpp"
#elif defined(MODE_RETRIEVE)
    #include "mode_retrieve.hpp"
#elif defined(MODE_RUN)
    #include "mode_run.hpp"
#else
    #error "No MODE defined"
#endif

void setup() { mode_setup(); }
void loop() { mode_loop(); }
