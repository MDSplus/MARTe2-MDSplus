/**
 * @file BaslerCamera.h
 * @brief Header file for class BaslerCamera
 * @date 11/04/2022
 * @author Stephen Lane-Walsh
 *
 */

#define DLL_API

/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>

using namespace Pylon;

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "DataSourceI.h"
#include "MessageI.h"
#include "StreamString.h"

/*---------------------------------------------------------------------------*/
/*                           Class declaration                               */
/*---------------------------------------------------------------------------*/

namespace MARTe {

class DLL_API BaslerCamera: public DataSourceI {
public:
    CLASS_REGISTER_DECLARATION()

    /**
     * @brief default constructor
     */
    BaslerCamera();

    /**
     * @brief default destructor.
     */
    virtual ~BaslerCamera();

    /**
     * @brief Copy data from the tree nodes to the dataSourceMemory
     * @details When a node does not have more data to retrieve the dataSourceMemory is filled with 0.
     * @return true if all nodes read return false.
     */
    virtual bool Synchronise();

    /**
     * @brief Reads, checks and initialises the DataSource parameters
     * @details Load from a configuration file the DataSource parameters.
     * If no errors occurs the following operations are performed:
     * <ul>
     * <li>Reads tree name </li>
     * <li>Reads the shot number </li>
     * <li>Opens the tree with the shot number </li>
     * <li>Reads the real-time thread Frequency parameter.</li>
     * </ul>
     * @param[in] data is the configuration file.
     * @return true if all parameters can be read and the values are valid
     */
    virtual bool Initialise(StructuredDataI & data);

    /**
     * @brief Read, checks and initialises the Signals
     * @details If no errors occurs the following operations are performed:
     * <ul>
     * <li>Reads nodes name (could be 1 or several nodes)</li>
     * <li>Opens nodes</li>
     * <li>Gets the node type</li>
     * <li>Verifies the node type</li>
     * <li>Gets number of elements per node (or signal).
     * <li>Gets the the size of the type in bytes</li>
     * <li>Allocates memory
     * </ul>
     * @param[in] data is the configuration file.
     * @return true if all parameters can be read and the values are valid
     */

    virtual bool SetConfiguredDatabase(StructuredDataI & data);

    /**
     * @brief Do nothing
     * @return true
     */
    virtual bool PrepareNextState(const char8 * const currentStateName,
                                  const char8 * const nextStateName);

    /**
     * @brief Do nothing
     * @return true
     */
    virtual bool AllocateMemory();

    /**
     * @return 1u
     */
    virtual uint32 GetNumberOfMemoryBuffers();

    /**
     * @brief Gets the signal memory buffer for the specified signal.
     * @param[in] signalIdx indicates the index of the signal to be obtained.
     * @param[in] bufferIdx indicate the index of the buffer to be obtained. Since only one buffer is allowed this parameter is always 0
     * @param[out] signalAddress is where the address of the desired signal is copied.
     */
    virtual bool GetSignalMemoryBuffer(const uint32 signalIdx,
                                       const uint32 bufferIdx,
                                       void *&signalAddress);

    /**
     * @brief See DataSourceI::GetBrokerName.
     * @details Only InputSignals are supported.
     * @return MemoryMapSynchronisedInputBroker.
     */
    virtual const char8 *GetBrokerName(StructuredDataI &data,
                                       const SignalDirection direction);

    /**
     * @brief See DataSourceI::GetInputBrokers.
     * @details adds a MemoryMapSynchronisedInputBroker instance to the inputBrokers.
     */
    virtual bool GetInputBrokers(ReferenceContainer &inputBrokers,
                                 const char8* const functionName,
                                 void * const gamMemPtr);

    /**
     * @brief See DataSourceI::GetOutputBrokers.
     * @return false.
     */
    virtual bool GetOutputBrokers(ReferenceContainer &outputBrokers,
                                  const char8* const functionName,
                                  void * const gamMemPtr);
private:

    CBaslerUniversalInstantCamera * _camera;

    uint8_t * _outFrameBuffer;

    size_t _frameBufferSize;

    uint64_t _outTimestamp;

    StreamString _ipAddress;

    float _fps;

    int64 _width;

    int64 _height;

    int64 _offsetX;

    int64 _offsetY;

    uint32 _rawGain;

    double _exposureTime;

};

} // namespace MARTe