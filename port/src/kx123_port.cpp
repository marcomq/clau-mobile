/* Replaces hw/kx123.cpp: the KX123 accelerometer reports the host device's accelerometer (in g). Part of glo-mobile, GPL-3.0. */
#include "kx123.h"

extern "C" void glo_port_get_accel(float out[3]);

KX123::KX123(i2c_port_t i2c_port, uint8_t sad, uint8_t wai)
    : setup_mode_on(false), resolution_divider(16384), _sad(sad), _wai(wai)
{
    (void)i2c_port;
}

KX123::~KX123() {}

/* Returns true on error, like the original driver. */
bool KX123::set_defaults() { return false; }

bool KX123::getresults_g(float *buf)
{
    glo_port_check_exit();
    glo_port_get_accel(buf);
    return true;
}
