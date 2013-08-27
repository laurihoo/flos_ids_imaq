#pragma once

#include "mwadaptorimaq.h"
#include "uEye.h"

class FlosIDSDeviceInfo : public imaqkit::IMAQInterface {

public:

    FlosIDSDeviceInfo(void);
    ~FlosIDSDeviceInfo(void);

    HIDS m_deviceID;
};

