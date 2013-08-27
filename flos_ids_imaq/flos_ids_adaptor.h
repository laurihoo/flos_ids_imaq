#pragma once

#include "mwadaptorimaq.h"
#include "uEye.h"

class FlosIDSAdaptor : public imaqkit::IAdaptor {

public:

    FlosIDSAdaptor(
              imaqkit::IEngine     *engine,
        const imaqkit::IDeviceInfo *deviceInfo,
        const char                 *formatName);

    virtual ~FlosIDSAdaptor();

    virtual const char* getDriverDescription() const;
    virtual const char* getDriverVersion() const; 
    virtual int getMaxWidth() const;
    virtual int getMaxHeight() const;
    virtual int getNumberOfBands() const; 
    virtual imaqkit::frametypes::FRAMETYPE getFrameType() const;
    
    virtual bool openDevice();
    virtual bool closeDevice(); 
    virtual bool startCapture();
    virtual bool stopCapture();

    bool FlosIDSAdaptor::isAcquisitionActive(void) const;
    void FlosIDSAdaptor::setAcquisitionActive(bool state);
   
    HIDS m_deviceID;

    imaqkit::ICriticalSection *m_driverGuard;
    imaqkit::ICriticalSection *m_acquisitionActiveGuard;
    
    bool m_acquisitionActive;

private:

    static DWORD WINAPI acquireThread(void *param);

    HANDLE m_acquireThread;
    DWORD  m_acquireThreadID;

};

