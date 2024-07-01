
#define DLL_API

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/

#include "BaslerCamera.h"

using namespace Basler_UniversalCameraParams;

#include "AdvancedErrorManagement.h"
#include "MemoryMapSynchronisedInputBroker.h"

/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/

namespace MARTe {

BaslerCamera::BaslerCamera()
    : DataSourceI()
{
    _camera = NULL_PTR(CBaslerUniversalInstantCamera *);
    _outFrameBuffer = NULL_PTR(uint8_t *);
    _frameBufferSize = 0;
    _outTimestamp = 0u;
    _fps = 0.0f;
    _width = 0;
    _height = 0;
    _offsetX = 0;
    _offsetY = 0;
    _rawGain = 0;
    _exposure = 0;

    PylonInitialize();

    printf("PylonInitialize\n");
    fflush(stdout);
}

BaslerCamera::~BaslerCamera()
{
    if (_outFrameBuffer != NULL_PTR(uint8_t *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(_outFrameBuffer));
    }

    if (_camera) {
        _camera->StopGrabbing();
        _camera->Close();
        delete _camera;

        printf("StopGrabbing / Close\n");
        fflush(stdout);
    }

    PylonTerminate();

    printf("PylonTerminate\n");
    fflush(stdout);
}

bool BaslerCamera::Synchronise()
{
    // if (!_triggerSet) {
    //     int64 triggerTimestamp = 0; // TODO: get from WRTD

    //     _camera->GevTimestampControlLatch();
    //     int64 currentTimestamp = _camera->GevTimestampValue();

    //     if (currentTimestamp >= triggerTimestamp) {
    //         REPORT_ERROR(ErrorManagement::FatalError, "Trigger timestamp (%lld) is in the past", triggerTimestamp);
    //         return false;
    //     }

    //     _camera->SyncFreeRunTimerStartTimeLow = (triggerTimestamp & 0x00000000FFFFFFFF);
    //     _camera->SyncFreeRunTimerStartTimeHigh = (triggerTimestamp & 0xFFFFFFFF00000000) >> 32;
    //     _camera->SyncFreeRunTimerUpdate();
    //     _camera->SyncFreeRunTimerEnable = true;

    //     _camera->StartGrabbing();

    //     _triggerSet = true;
    // }

    // printf("Synchronise %llu\n", HighResolutionTimer::Counter());
    // fflush(stdout);

    // TODO: INFINITE or 0?
    CGrabResultPtr grabResult;
    _camera->RetrieveResult(INFINITE, grabResult, Pylon::TimeoutHandling_Return);

    // printf("RetrieveResult\n");
    // fflush(stdout);

    if (grabResult->GrabSucceeded()) {
        
        // printf("GrabSucceeded\n");
        // fflush(stdout);

        if (grabResult->GetBufferSize() != _frameBufferSize) {
            // freak out
        }
        
        _outTimestamp = grabResult->GetTimeStamp();
        memcpy(_outFrameBuffer, grabResult->GetBuffer(), _frameBufferSize);

        grabResult.Release();
    }
    else {
        // printf("GrabFailed\n");
        // fflush(stdout);
    }

    return true;
}

bool BaslerCamera::Initialise(StructuredDataI & data)
{
    if (!DataSourceI::Initialise(data)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "DataSourceI::Initialise(data) returned false");
        return false;
    }

    if (!data.Read("IpAddress", _ipAddress)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "IpAddress shall be specified");
        return false;
    }

    if (!data.Read("FPS", _fps)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "FPS shall be specified");
        return false;
    }

    if (!data.Read("Width", _width)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "Width shall be specified");
        return false;
    }

    if (!data.Read("Height", _height)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "Height shall be specified");
        return false;
    }

    if (!data.Read("OffsetX", _offsetX)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "OffsetX shall be specified");
        return false;
    }

    if (!data.Read("OffsetY", _offsetY)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "OffsetY shall be specified");
        return false;
    }

    if (!data.Read("RawGain", _rawGain)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "RawGain shall be specified");
        return false;
    }

    if (!data.Read("ExposureTime", _exposureTime)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "ExposureTime shall be specified");
        return false;
    }

    return true;
}

bool BaslerCamera::SetConfiguredDatabase(StructuredDataI & data)
{
    if (!DataSourceI::SetConfiguredDatabase(data)) {
        REPORT_ERROR(ErrorManagement::ParametersError, "DataSourceI::SetConfiguredDatabase(data) returned false");
        return false;
    }
    
    uint32 numberOfSignals = GetNumberOfSignals();

    if (numberOfSignals != 2) {
        REPORT_ERROR(ErrorManagement::ParametersError, "The number of signals must be 2, not %d", numberOfSignals);
        return false;
    }

    TypeDescriptor frameType = GetSignalType(0u);
    if (frameType.type != UnsignedInteger || frameType.numberOfBits != 8) {
        REPORT_ERROR(ErrorManagement::ParametersError, "The first signal's type must be a 8-bit UnsignedInteger");
        return false;
    }

    uint8 frameDimensions = 0;
    GetSignalNumberOfDimensions(0u, frameDimensions);
    printf("Frame Dimensions: %u", (unsigned)frameDimensions);

    TypeDescriptor timestampType = GetSignalType(1u);
    if (timestampType.type != UnsignedInteger || timestampType.numberOfBits != 64) {
        REPORT_ERROR(ErrorManagement::ParametersError, "The second signal's type must be a 64-bit UnsignedInteger");
        return false;
    }

    CDeviceInfo info;

    try {
        info.SetIpAddress(_ipAddress.Buffer());

        // TODO: GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc ?
        _camera = new CBaslerUniversalInstantCamera(CTlFactory::GetInstance().CreateFirstDevice(info));
        _camera->Open();
    }
    catch (RuntimeException &e) {
        REPORT_ERROR(ErrorManagement::CommunicationError, "RuntimeException thrown from pylon '%s'", e.what());
    }

    if (!_camera->IsOpen()) {
        REPORT_ERROR(ErrorManagement::CommunicationError, "Unable to connect to BaslerCamera at '%s'", _ipAddress.Buffer());
        return false;
    }

    printf("Camera Open\n");
    fflush(stdout);

    // Basler wants width to be a factor of 16 (minimum dimensions for an image are 16x1)
    if ((_width % 16) != 0) {
        REPORT_ERROR(ErrorManagement::ParametersError, "Width must be a factor of 16");
        return false;
        // _width = ((_width - 16) / 16.0f) * 16;
    }

    // TODO: Minimum dimensions?

    int64 maxWidth = _camera->WidthMax.GetValue();
    int64 maxHeight = _camera->HeightMax.GetValue();

    if ((_width + _offsetX) > maxWidth) {
        REPORT_ERROR(ErrorManagement::OutOfRange,
            "Requested (OffsetX + Width) of (%lld + %lld) is larger than the maximum width of %lld",
            _offsetX, _width, maxWidth
        );
        return false;
    }

    if ((_height + _offsetY) > maxHeight) {
        REPORT_ERROR(ErrorManagement::OutOfRange,
            "Requested (OffsetY + Height) of (%lld + %lld) is larger than the maximum height of %lld",
            _offsetY, _height, maxHeight
        );
        return false;
    }

    if (_rawGain < 136 || _rawGain > 542) {
        REPORT_ERROR(ErrorManagement::OutOfRange,
            "Requested Gain of %u is outside of the range of values (136 - 542)",
            _rawGain
        );
        return false;
    }

    if (_exposureTime < 80 || _exposureTime > 10000000) {
        REPORT_ERROR(ErrorManagement::OutOfRange,
            "Requested Exposure of %f is outside of the range of values (80 - 10000000)",
            _exposureTime
        );
        return false;
    }

    _camera->Width = _width;
    _camera->Height = _height;
    _camera->OffsetX = _offsetX;
    _camera->OffsetY = _offsetY;
    
    _camera->ExposureMode = 'Timed';
    _camera->ExposureAuto = 'Off';
    _camera->ExposureTimeAbs = _exposureTime;
    
    _camera->GainAuto = 'Off';
    _camera->GainRaw = _rawGain;

    _camera->GevIEEE1588 = true;
    _camera->AcquisitionMode = AcquisitionMode_Continuous;
    _camera->AcquisitionFrameRateEnable = true;
    _camera->AcquisitionFrameRateAbs = _fps;

    _camera->StartGrabbing();

    printf("StartGrabbing\n");
    fflush(stdout);

    return true;
}

bool BaslerCamera::PrepareNextState(const char8 * const currentStateName, const char8 * const nextStateName)
{
    return true;
}

bool BaslerCamera::AllocateMemory()
{
    _frameBufferSize = _width * _height * 1; // TODO: Depth?
    _outFrameBuffer = reinterpret_cast<uint8_t *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(_frameBufferSize));
    memset(_outFrameBuffer, 0, _frameBufferSize);
    return true;
}

uint32 BaslerCamera::GetNumberOfMemoryBuffers()
{
    return 1u;
}

bool BaslerCamera::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void *&outSignalAddress)
{
    printf("Accessing Memory Buffer for Signal #%u\n", signalIdx);
    fflush(stdout);

    if (signalIdx == 0u) {
        outSignalAddress = _outFrameBuffer;
        return true;
    }
    else if (signalIdx == 1u) {
        outSignalAddress = &_outTimestamp;
        return true;
    }

    return false;
}

const char8 *BaslerCamera::GetBrokerName(StructuredDataI &data, const SignalDirection direction)
{
    const char8 *brokerName = "MemoryMapSynchronisedInputBroker";
    return brokerName;
}

bool BaslerCamera::GetInputBrokers(ReferenceContainer &inputBrokers, const char8* const functionName, void * const gamMemPtr)
{
    ReferenceT <MemoryMapSynchronisedInputBroker> broker("MemoryMapSynchronisedInputBroker");
    bool ok = broker.IsValid();
    if (ok) {
        ok = broker->Init(InputSignals, *this, functionName, gamMemPtr);
    }
    if (ok) {
        ok = inputBrokers.Insert(broker);
    }
    return ok;
}

bool BaslerCamera::GetOutputBrokers(ReferenceContainer &outputBrokers, const char8* const functionName, void * const gamMemPtr)
{
    return false;
}

CLASS_REGISTER(BaslerCamera, "1.0")

} // namespace MARTe