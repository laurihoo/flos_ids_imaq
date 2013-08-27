#include "mwadaptorimaq.h"
#include "flos_ids_adaptor.h"
#include "flos_ids_device_info.h"
#include "uEye.h"

void initializeAdaptor()
{

}

void getAvailHW(imaqkit::IHardwareInfo *hardwareInfo)
{
    int nNumCam;

    if(is_GetNumberOfCameras(&nNumCam) == IS_SUCCESS) {
        if(nNumCam >= 1) {
            UEYE_CAMERA_LIST *pCameraList;

            pCameraList = (UEYE_CAMERA_LIST*) new BYTE [sizeof (DWORD) + nNumCam * sizeof (UEYE_CAMERA_INFO)];
            pCameraList->dwCount = nNumCam;
            
            if(is_GetCameraList(pCameraList) == IS_SUCCESS) {
                for(int i = 0; i < (int)pCameraList->dwCount; i++) {
                    char sCameraName[50];
                    FlosIDSDeviceInfo *adaptorInfo = new FlosIDSDeviceInfo();

                    sprintf(sCameraName, "%d: %s (%s)", pCameraList->uci[i].dwCameraID, 
                        pCameraList->uci[i].Model, pCameraList->uci[i].SerNo);

                    imaqkit::IDeviceInfo* deviceInfo =
                        hardwareInfo->createDeviceInfo(i + 1, sCameraName);
                    imaqkit::IDeviceFormat* deviceFormat = 
                        deviceInfo->createDeviceFormat(1,"FLOS_IDS_1");

                    adaptorInfo->m_deviceID = pCameraList->uci[i].dwCameraID;

                    deviceInfo->setAdaptorData(adaptorInfo);
                    deviceInfo->addDeviceFormat(deviceFormat, true);
                    hardwareInfo->addDevice(deviceInfo); 
                }
            }
            
            delete [] pCameraList;
        }
    }
}

void getDeviceAttributes(
    const imaqkit::IDeviceInfo      *deviceInfo,
    const char                      *formatName,
          imaqkit::IPropFactory     *devicePropFact,
          imaqkit::IVideoSourceInfo *sourceContainer,
          imaqkit::ITriggerInfo     *hwTriggerInfo)
{
    sourceContainer->addAdaptorSource("FLOSDeviceSource", 1);
}

imaqkit::IAdaptor *createInstance(
          imaqkit::IEngine     *engine, 
    const imaqkit::IDeviceInfo *deviceInfo,
    const char                 *formatName)
{
    imaqkit::IAdaptor *adaptor = new FlosIDSAdaptor(engine, deviceInfo, formatName);
    return adaptor;
}

void uninitializeAdaptor() 
{ 

}