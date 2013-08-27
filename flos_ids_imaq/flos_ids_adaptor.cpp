#include "flos_ids_adaptor.h"
#include "flos_ids_device_info.h"

FlosIDSAdaptor::FlosIDSAdaptor(
	imaqkit::IEngine* engine, 
	const imaqkit::IDeviceInfo *deviceInfo,
	const char *formatName):imaqkit::IAdaptor(engine)
{
	FlosIDSDeviceInfo *adaptorData = (FlosIDSDeviceInfo *)(deviceInfo->getAdaptorData());
	m_deviceID = adaptorData->m_deviceID;
	
    m_driverGuard = imaqkit::createCriticalSection();
    m_acquisitionActiveGuard = imaqkit::createCriticalSection();
}

FlosIDSAdaptor::~FlosIDSAdaptor()
{ 
	stop();
	close();
}

const char* FlosIDSAdaptor::getDriverDescription() const
{ 
	return "[TODO]";
}

const char* FlosIDSAdaptor::getDriverVersion() const
{ 
	return "[TODO]";
} 

int FlosIDSAdaptor::getMaxWidth() const
{
	return 1280;
}

int FlosIDSAdaptor::getMaxHeight() const 
{ 	
	return 1024;
}

int FlosIDSAdaptor::getNumberOfBands() const
{ 
	return 1;
}

imaqkit::frametypes::FRAMETYPE FlosIDSAdaptor::getFrameType()
              const
{ 
	return imaqkit::frametypes::BGR24_PACKED; 
}

bool FlosIDSAdaptor::isAcquisitionActive(void) const { 
    std::auto_ptr<imaqkit::IAutoCriticalSection> acquisitionActiveSection(imaqkit::createAutoCriticalSection(m_acquisitionActiveGuard, true));
    return m_acquisitionActive; 
}

void FlosIDSAdaptor::setAcquisitionActive(bool state) {
    std::auto_ptr<imaqkit::IAutoCriticalSection> acquisitionActiveSection(imaqkit::createAutoCriticalSection(m_acquisitionActiveGuard, true));
    m_acquisitionActive = state; 
}

DWORD WINAPI FlosIDSAdaptor::acquireThread(void* param)
{
	MSG msg;
	FlosIDSAdaptor* adaptor = reinterpret_cast<FlosIDSAdaptor *>(param);

	while(GetMessage(&msg, NULL, 0, 0) > 0) {
		switch(msg.message) {
		case WM_USER:
			std::auto_ptr<imaqkit::IAutoCriticalSection> driverSection(imaqkit::createAutoCriticalSection(adaptor->m_driverGuard, false));

			while(adaptor->isAcquisitionNotComplete() && adaptor->isAcquisitionActive()) {
			
				driverSection->enter();

				imaqkit::frametypes::FRAMETYPE frameType = adaptor->getFrameType();
                int imWidth  = adaptor->getMaxWidth();
                int imHeight = adaptor->getMaxHeight();

				INT    nMemID   = 0;
				char * pcBuffer = NULL;

				if((is_WaitForNextImage(adaptor->m_deviceID, 25, &pcBuffer, &nMemID)) != IS_SUCCESS) {
				} else  {
					if(adaptor->isSendFrame()) {

						imaqkit::IAdaptorFrame* frame = 
							   adaptor->getEngine()->makeFrame(frameType,
                                                              imWidth, 
                                                            imHeight);

						frame->setImage(pcBuffer,
							             imWidth,
								        imHeight,
									           0,
										       0);

						frame->setTime(imaqkit::getCurrentTime());
						adaptor->getEngine()->receiveFrame(frame);
					}

					is_UnlockSeqBuf(adaptor->m_deviceID, nMemID, pcBuffer);
					adaptor->incrementFrameCount();
				}

				driverSection->leave();
            }

			imaqkit::adaptorWarn("FlosIDSAdaptor:acquireThread","Acquisition is now complete.");

			break;
		}
   }
	
   return 0;
}

bool FlosIDSAdaptor::openDevice()
{
	if(isOpen()) { 
        return true;
	}

	INT nRet = is_InitCamera(&m_deviceID, NULL);

    if(nRet != IS_SUCCESS) {
        if(nRet == IS_STARTER_FW_UPLOAD_NEEDED) {
            return false;
        }
    } else {
		IS_SIZE_2D  imageSize;
		INT		    lMemoryId;
		char       *pcImageMemory;

		is_AOI(m_deviceID, IS_AOI_IMAGE_GET_SIZE, (void *)&imageSize, sizeof(imageSize));
		
		for(unsigned int i = 0; i < 25; i++) {
			is_AllocImageMem(m_deviceID, imageSize.s32Width, imageSize.s32Height, 24, &pcImageMemory, &lMemoryId);
			is_AddToSequence(m_deviceID, pcImageMemory, lMemoryId);
			is_SetImageMem(m_deviceID, pcImageMemory, lMemoryId);
		}

		if(is_InitImageQueue(m_deviceID, 0) != IS_SUCCESS) {
			imaqkit::adaptorWarn("FlosIDSAdaptor:openDevice","Something went wrong while allocating memory.");
			return false;
		}
	}

    m_acquireThread = CreateThread(NULL, 0, acquireThread, this, 0, &m_acquireThreadID);
    
	if(m_acquireThread == NULL) {
		closeDevice();
		return false;
    }
 
    while(PostThreadMessage(m_acquireThreadID, WM_USER + 1, 0, 0) == 0) {
		Sleep(10);
	}

    return true;
}

bool FlosIDSAdaptor::closeDevice()
{
	if(!isOpen()) {
        return true;
	}

    if(m_acquireThread) {
        PostThreadMessage(m_acquireThreadID, WM_QUIT, 0, 0);
        WaitForSingleObject(m_acquireThread, 10000);
        CloseHandle(m_acquireThread);
        m_acquireThread = NULL;
    }

	//is_ExitCamera frees allocated memory
	is_ExitCamera(m_deviceID);

	return true;
}

bool FlosIDSAdaptor::startCapture()
{	
	if(!isOpen()) {
        return false;
	}

	UINT   nMode;
	INT    nRet = IS_SUCCESS;
	double dNominal = 128;
	double dEnable  = 1;
			
	is_SetDisplayMode(m_deviceID, IS_SET_DM_DIB);
	is_SetExternalTrigger(m_deviceID, IS_SET_TRIGGER_SOFTWARE);

	nMode = IO_FLASH_MODE_TRIGGER_HI_ACTIVE;
	is_IO(m_deviceID, IS_IO_CMD_FLASH_SET_MODE, (void *)&nMode, sizeof(nMode));

	is_SetAutoParameter(m_deviceID, IS_SET_ENABLE_AUTO_GAIN, &dEnable, 0);
	is_SetAutoParameter(m_deviceID, IS_SET_AUTO_REFERENCE, &dNominal, 0);

	is_SetColorMode(m_deviceID, IS_CM_BGR8_PACKED);
	is_SetFrameRate(m_deviceID, 20, NULL);

	if((is_CaptureVideo(m_deviceID, IS_DONT_WAIT) != IS_SUCCESS)) {
		imaqkit::adaptorWarn("FlosIDSAdaptor:startCapture", "Could not start capturing.");
		return false;
	}

    setAcquisitionActive(true);
	PostThreadMessage(m_acquireThreadID, WM_USER, 0, 0);

    return true;
}

bool FlosIDSAdaptor::stopCapture()
{
	setAcquisitionActive(false);

    std::auto_ptr<imaqkit::IAutoCriticalSection> driverSection(imaqkit::createAutoCriticalSection(m_driverGuard, true));
	// do something with the IDS sdk, if necessary
    driverSection->leave();

	return true;
}
