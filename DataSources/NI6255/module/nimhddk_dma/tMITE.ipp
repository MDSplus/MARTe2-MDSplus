// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef ___tMITE_ipp___
#define ___tMITE_ipp___

#ifndef ___tMITE_h___
#include "tMITE.h"
#endif

#include "nimdbg/trace.h"
inline void tMITE::tReg32IODirect32::write(
   tBusSpaceReference addrSpace,
   u32 offset,
   u32 value,
   nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;
   addrSpace.write32(offset, value);
}

inline u32 tMITE::tReg32IODirect32::read(
   tBusSpaceReference addrSpace,
   u32 offset,
   nMDBG::tStatus2* s)
{
   u32 value;   if (s && s->isFatal()) return ~0;
   value = addrSpace.read32(offset);
   return value;
}

inline tBusSpaceReference tMITE::getBusSpaceReference(void) const
{
   return _addrSpace;
}

inline void tMITE::setAddressOffset(u32 value, nMDBG::tStatus2* s)
{
   _addressOffset = value;
}
inline u32  tMITE::getAddressOffset(nMDBG::tStatus2* s)
{
   return _addressOffset;
}
inline void tMITE::flushBus(nMDBG::tStatus2* s)
{

}

inline tMITE::tChannelOperation::tChannelOperation()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tChannelOperation::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tChannelOperation::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tChannelOperation& tMITE::tChannelOperation::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tChannelOperation::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tChannelOperation::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0x7fffff20);
}

inline void tMITE::tChannelOperation::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline u32 tMITE::tChannelOperation::readRegister(nMDBG::tStatus2*) 
{
   return _softCopy;
}


inline tMITE::tChannelOperation& tMITE::tChannelOperation::setStart(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffffe) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelOperation::getStart(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffffe) >> 0x0);
}

inline void tMITE::tChannelOperation::writeStart(u32 fieldValue, nMDBG::tStatus2* s)
{
   setStart(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelOperation::readStart(nMDBG::tStatus2* s)
{
   return getStart(s);
}

inline tMITE::tChannelOperation& tMITE::tChannelOperation::setCont(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffffd) | (fieldValue << 0x1));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelOperation::getCont(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffffd) >> 0x1);
}

inline void tMITE::tChannelOperation::writeCont(u32 fieldValue, nMDBG::tStatus2* s)
{
   setCont(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelOperation::readCont(nMDBG::tStatus2* s)
{
   return getCont(s);
}

inline tMITE::tChannelOperation& tMITE::tChannelOperation::setStop(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffffb) | (fieldValue << 0x2));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelOperation::getStop(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffffb) >> 0x2);
}

inline void tMITE::tChannelOperation::writeStop(u32 fieldValue, nMDBG::tStatus2* s)
{
   setStop(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelOperation::readStop(nMDBG::tStatus2* s)
{
   return getStop(s);
}

inline tMITE::tChannelOperation& tMITE::tChannelOperation::setAbort(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffff7) | (fieldValue << 0x3));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelOperation::getAbort(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffff7) >> 0x3);
}

inline void tMITE::tChannelOperation::writeAbort(u32 fieldValue, nMDBG::tStatus2* s)
{
   setAbort(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelOperation::readAbort(nMDBG::tStatus2* s)
{
   return getAbort(s);
}

inline tMITE::tChannelOperation& tMITE::tChannelOperation::setFReset(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xffffffef) | (fieldValue << 0x4));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelOperation::getFReset(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xffffffef) >> 0x4);
}

inline void tMITE::tChannelOperation::writeFReset(u32 fieldValue, nMDBG::tStatus2* s)
{
   setFReset(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelOperation::readFReset(nMDBG::tStatus2* s)
{
   return getFReset(s);
}

inline tMITE::tChannelOperation& tMITE::tChannelOperation::setClrRB(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xffffffbf) | (fieldValue << 0x6));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelOperation::getClrRB(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xffffffbf) >> 0x6);
}

inline void tMITE::tChannelOperation::writeClrRB(u32 fieldValue, nMDBG::tStatus2* s)
{
   setClrRB(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelOperation::readClrRB(nMDBG::tStatus2* s)
{
   return getClrRB(s);
}

inline tMITE::tChannelOperation& tMITE::tChannelOperation::setClrDone(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xffffff7f) | (fieldValue << 0x7));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelOperation::getClrDone(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xffffff7f) >> 0x7);
}

inline void tMITE::tChannelOperation::writeClrDone(u32 fieldValue, nMDBG::tStatus2* s)
{
   setClrDone(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelOperation::readClrDone(nMDBG::tStatus2* s)
{
   return getClrDone(s);
}

inline tMITE::tChannelOperation& tMITE::tChannelOperation::setDmaReset(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0x7fffffff) | (fieldValue << 0x1f));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelOperation::getDmaReset(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0x7fffffff) >> 0x1f);
}

inline void tMITE::tChannelOperation::writeDmaReset(u32 fieldValue, nMDBG::tStatus2* s)
{
   setDmaReset(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelOperation::readDmaReset(nMDBG::tStatus2* s)
{
   return getDmaReset(s);
}

inline void tMITE::tChannelOperation::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tChannelControl::tChannelControl()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tChannelControl::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tChannelControl::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tChannelControl::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tChannelControl::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0x3cfcffff);
}

inline void tMITE::tChannelControl::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline u32 tMITE::tChannelControl::readRegister(nMDBG::tStatus2*) 
{
   return _softCopy;
}


inline tMITE::tChannelControl& tMITE::tChannelControl::setXMode(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffff8) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getXMode(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffff8) >> 0x0);
}

inline void tMITE::tChannelControl::writeXMode(u32 fieldValue, nMDBG::tStatus2* s)
{
   setXMode(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readXMode(nMDBG::tStatus2* s)
{
   return getXMode(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setDir(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffff7) | (fieldValue << 0x3));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getDir(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffff7) >> 0x3);
}

inline void tMITE::tChannelControl::writeDir(u32 fieldValue, nMDBG::tStatus2* s)
{
   setDir(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readDir(nMDBG::tStatus2* s)
{
   return getDir(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setBurstEnable(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xffffbfff) | (fieldValue << 0xe));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getBurstEnable(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xffffbfff) >> 0xe);
}

inline void tMITE::tChannelControl::writeBurstEnable(u32 fieldValue, nMDBG::tStatus2* s)
{
   setBurstEnable(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readBurstEnable(nMDBG::tStatus2* s)
{
   return getBurstEnable(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setClrContinueIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffeffff) | (fieldValue << 0x10));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getClrContinueIE(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffeffff) >> 0x10);
}

inline void tMITE::tChannelControl::writeClrContinueIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   setClrContinueIE(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readClrContinueIE(nMDBG::tStatus2* s)
{
   return getClrContinueIE(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setSetContinueIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffdffff) | (fieldValue << 0x11));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getSetContinueIE(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffdffff) >> 0x11);
}

inline void tMITE::tChannelControl::writeSetContinueIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   setSetContinueIE(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readSetContinueIE(nMDBG::tStatus2* s)
{
   return getSetContinueIE(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setClrDoneIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfeffffff) | (fieldValue << 0x18));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getClrDoneIE(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfeffffff) >> 0x18);
}

inline void tMITE::tChannelControl::writeClrDoneIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   setClrDoneIE(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readClrDoneIE(nMDBG::tStatus2* s)
{
   return getClrDoneIE(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setSetDoneIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfdffffff) | (fieldValue << 0x19));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getSetDoneIE(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfdffffff) >> 0x19);
}

inline void tMITE::tChannelControl::writeSetDoneIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   setSetDoneIE(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readSetDoneIE(nMDBG::tStatus2* s)
{
   return getSetDoneIE(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setClrDmaIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xbfffffff) | (fieldValue << 0x1e));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getClrDmaIE(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xbfffffff) >> 0x1e);
}

inline void tMITE::tChannelControl::writeClrDmaIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   setClrDmaIE(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readClrDmaIE(nMDBG::tStatus2* s)
{
   return getClrDmaIE(s);
}

inline tMITE::tChannelControl& tMITE::tChannelControl::setSetDmaIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0x7fffffff) | (fieldValue << 0x1f));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelControl::getSetDmaIE(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0x7fffffff) >> 0x1f);
}

inline void tMITE::tChannelControl::writeSetDmaIE(u32 fieldValue, nMDBG::tStatus2* s)
{
   setSetDmaIE(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tChannelControl::readSetDmaIE(nMDBG::tStatus2* s)
{
   return getSetDmaIE(s);
}

inline void tMITE::tChannelControl::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tChannelStatus::tChannelStatus()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tChannelStatus::registerMap(void)
{
   return _MITE;
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tChannelStatus::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tChannelStatus::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tChannelStatus::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setDeviceErr(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffffc) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getDeviceErr(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffffc) >> 0x0);
}

inline u32 tMITE::tChannelStatus::readDeviceErr(nMDBG::tStatus2* s)
{
   refresh(s);
   return getDeviceErr(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setMemoryErr(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffff3) | (fieldValue << 0x2));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getMemoryErr(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffff3) >> 0x2);
}

inline u32 tMITE::tChannelStatus::readMemoryErr(nMDBG::tStatus2* s)
{
   refresh(s);
   return getMemoryErr(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setTransferErr(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffdff) | (fieldValue << 0x9));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getTransferErr(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffdff) >> 0x9);
}

inline u32 tMITE::tChannelStatus::readTransferErr(nMDBG::tStatus2* s)
{
   refresh(s);
   return getTransferErr(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setOperationErr(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffff3ff) | (fieldValue << 0xa));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getOperationErr(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffff3ff) >> 0xa);
}

inline u32 tMITE::tChannelStatus::readOperationErr(nMDBG::tStatus2* s)
{
   refresh(s);
   return getOperationErr(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setStoppedStatus(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xffffefff) | (fieldValue << 0xc));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getStoppedStatus(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xffffefff) >> 0xc);
}

inline u32 tMITE::tChannelStatus::readStoppedStatus(nMDBG::tStatus2* s)
{
   refresh(s);
   return getStoppedStatus(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setSoftwareAbort(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xffffbfff) | (fieldValue << 0xe));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getSoftwareAbort(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xffffbfff) >> 0xe);
}

inline u32 tMITE::tChannelStatus::readSoftwareAbort(nMDBG::tStatus2* s)
{
   refresh(s);
   return getSoftwareAbort(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setError(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xffff7fff) | (fieldValue << 0xf));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getError(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xffff7fff) >> 0xf);
}

inline u32 tMITE::tChannelStatus::readError(nMDBG::tStatus2* s)
{
   refresh(s);
   return getError(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setContinueStatus(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffdffff) | (fieldValue << 0x11));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getContinueStatus(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffdffff) >> 0x11);
}

inline u32 tMITE::tChannelStatus::readContinueStatus(nMDBG::tStatus2* s)
{
   refresh(s);
   return getContinueStatus(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setDmaDone(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfdffffff) | (fieldValue << 0x19));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getDmaDone(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfdffffff) >> 0x19);
}

inline u32 tMITE::tChannelStatus::readDmaDone(nMDBG::tStatus2* s)
{
   refresh(s);
   return getDmaDone(s);
}

inline tMITE::tChannelStatus& tMITE::tChannelStatus::setInterrupting(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0x7fffffff) | (fieldValue << 0x1f));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tChannelStatus::getInterrupting(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0x7fffffff) >> 0x1f);
}

inline u32 tMITE::tChannelStatus::readInterrupting(nMDBG::tStatus2* s)
{
   refresh(s);
   return getInterrupting(s);
}

inline void tMITE::tChannelStatus::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tBaseCount::tBaseCount()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tBaseCount::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tBaseCount::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tBaseCount& tMITE::tBaseCount::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tBaseCount::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tBaseCount::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0xffffffff);
}

inline void tMITE::tBaseCount::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline void tMITE::tBaseCount::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tBaseCount::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tBaseCount& tMITE::tBaseCount::setValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0x0) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tBaseCount::getValue(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0x0) >> 0x0);
}

inline void tMITE::tBaseCount::writeValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   setValue(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tBaseCount::readValue(nMDBG::tStatus2* s)
{
   refresh(s);
   return getValue(s);
}

inline void tMITE::tBaseCount::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tTransferCount::tTransferCount()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tTransferCount::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tTransferCount::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tTransferCount& tMITE::tTransferCount::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tTransferCount::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tTransferCount::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0xffffffff);
}

inline void tMITE::tTransferCount::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline void tMITE::tTransferCount::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tTransferCount::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tTransferCount& tMITE::tTransferCount::setValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0x0) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tTransferCount::getValue(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0x0) >> 0x0);
}

inline void tMITE::tTransferCount::writeValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   setValue(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tTransferCount::readValue(nMDBG::tStatus2* s)
{
   refresh(s);
   return getValue(s);
}

inline void tMITE::tTransferCount::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tFifoCount::tFifoCount()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tFifoCount::registerMap(void)
{
   return _MITE;
}

inline tMITE::tFifoCount& tMITE::tFifoCount::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tFifoCount::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tFifoCount::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tFifoCount::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tFifoCount& tMITE::tFifoCount::setFifoCR(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xffffff00) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tFifoCount::getFifoCR(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xffffff00) >> 0x0);
}

inline u32 tMITE::tFifoCount::readFifoCR(nMDBG::tStatus2* s)
{
   refresh(s);
   return getFifoCR(s);
}

inline tMITE::tFifoCount& tMITE::tFifoCount::setEmptyCR(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xff00ffff) | (fieldValue << 0x10));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tFifoCount::getEmptyCR(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xff00ffff) >> 0x10);
}

inline u32 tMITE::tFifoCount::readEmptyCR(nMDBG::tStatus2* s)
{
   refresh(s);
   return getEmptyCR(s);
}

inline void tMITE::tFifoCount::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tMemoryConfig::tMemoryConfig()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tMemoryConfig::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tMemoryConfig::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tMemoryConfig& tMITE::tMemoryConfig::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tMemoryConfig::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tMemoryConfig::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0xffffffff);
}

inline void tMITE::tMemoryConfig::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline void tMITE::tMemoryConfig::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tMemoryConfig::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tMemoryConfig& tMITE::tMemoryConfig::setPortSize(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffcff) | (fieldValue << 0x8));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tMemoryConfig::getPortSize(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffcff) >> 0x8);
}

inline void tMITE::tMemoryConfig::writePortSize(u32 fieldValue, nMDBG::tStatus2* s)
{
   setPortSize(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tMemoryConfig::readPortSize(nMDBG::tStatus2* s)
{
   refresh(s);
   return getPortSize(s);
}

inline void tMITE::tMemoryConfig::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tDeviceConfig::tDeviceConfig()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tDeviceConfig::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tDeviceConfig::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tDeviceConfig& tMITE::tDeviceConfig::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tDeviceConfig::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tDeviceConfig::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0xffffffff);
}

inline void tMITE::tDeviceConfig::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline void tMITE::tDeviceConfig::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tDeviceConfig::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tDeviceConfig& tMITE::tDeviceConfig::setPortSize(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfffffcff) | (fieldValue << 0x8));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tDeviceConfig::getPortSize(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfffffcff) >> 0x8);
}

inline void tMITE::tDeviceConfig::writePortSize(u32 fieldValue, nMDBG::tStatus2* s)
{
   setPortSize(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tDeviceConfig::readPortSize(nMDBG::tStatus2* s)
{
   refresh(s);
   return getPortSize(s);
}

inline tMITE::tDeviceConfig& tMITE::tDeviceConfig::setReqSource(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0xfff8ffff) | (fieldValue << 0x10));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tDeviceConfig::getReqSource(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0xfff8ffff) >> 0x10);
}

inline void tMITE::tDeviceConfig::writeReqSource(u32 fieldValue, nMDBG::tStatus2* s)
{
   setReqSource(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tDeviceConfig::readReqSource(nMDBG::tStatus2* s)
{
   refresh(s);
   return getReqSource(s);
}

inline void tMITE::tDeviceConfig::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tBaseAddress::tBaseAddress()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tBaseAddress::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tBaseAddress::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tBaseAddress& tMITE::tBaseAddress::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tBaseAddress::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tBaseAddress::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0xffffffff);
}

inline void tMITE::tBaseAddress::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline void tMITE::tBaseAddress::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tBaseAddress::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tBaseAddress& tMITE::tBaseAddress::setValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0x0) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tBaseAddress::getValue(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0x0) >> 0x0);
}

inline void tMITE::tBaseAddress::writeValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   setValue(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tBaseAddress::readValue(nMDBG::tStatus2* s)
{
   refresh(s);
   return getValue(s);
}

inline void tMITE::tBaseAddress::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tMemoryAddress::tMemoryAddress()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tMemoryAddress::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tMemoryAddress::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tMemoryAddress& tMITE::tMemoryAddress::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tMemoryAddress::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tMemoryAddress::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0xffffffff);
}

inline void tMITE::tMemoryAddress::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline void tMITE::tMemoryAddress::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tMemoryAddress::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tMemoryAddress& tMITE::tMemoryAddress::setValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0x0) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tMemoryAddress::getValue(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0x0) >> 0x0);
}

inline void tMITE::tMemoryAddress::writeValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   setValue(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tMemoryAddress::readValue(nMDBG::tStatus2* s)
{
   refresh(s);
   return getValue(s);
}

inline void tMITE::tMemoryAddress::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

inline tMITE::tDeviceAddress::tDeviceAddress()
{
   _softCopy = 0;
}

inline tMITE* tMITE::tDeviceAddress::registerMap(void)
{
   return _MITE;
}

inline void tMITE::tDeviceAddress::flushBus(nMDBG::tStatus2* s)
{
   _MITE->flushBus(s);
}

inline tMITE::tDeviceAddress& tMITE::tDeviceAddress::setRegister(u32 fieldValue, nMDBG::tStatus2*)
{
   _softCopy = fieldValue;
   return *this;
}

inline u32 tMITE::tDeviceAddress::getRegister(nMDBG::tStatus2*) const
{
   return _softCopy;
}

inline void tMITE::tDeviceAddress::flush(nMDBG::tStatus2* s)
{
   tIOStrategy::write(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), _softCopy, s);
   _softCopy &= (u32)(0xffffffff);
}

inline void tMITE::tDeviceAddress::writeRegister(u32 fieldValue, nMDBG::tStatus2* s)
{
   _softCopy = fieldValue;
   flush(s);
}

inline void tMITE::tDeviceAddress::refresh(nMDBG::tStatus2* s)
{
   _softCopy = tIOStrategy::read(_MITE->getBusSpaceReference(), kOffset + _MITE->getAddressOffset(s), s);
}

inline u32 tMITE::tDeviceAddress::readRegister(nMDBG::tStatus2* s) 
{
   refresh(s);
   return _softCopy;
}

inline tMITE::tDeviceAddress& tMITE::tDeviceAddress::setValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   u32 newValue;   newValue = (u32) ((_softCopy & 0x0) | (fieldValue << 0x0));
   setRegister(newValue, s);
   return *this;
}

inline u32 tMITE::tDeviceAddress::getValue(nMDBG::tStatus2*) const
{
   return (u32)((_softCopy & ~0x0) >> 0x0);
}

inline void tMITE::tDeviceAddress::writeValue(u32 fieldValue, nMDBG::tStatus2* s)
{
   setValue(fieldValue, s);
   flush(s);
}
inline u32 tMITE::tDeviceAddress::readValue(nMDBG::tStatus2* s)
{
   refresh(s);
   return getValue(s);
}

inline void tMITE::tDeviceAddress::setRegisterMap(tMITE* pMITE)
{
   _MITE = pMITE;
}

#endif


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

