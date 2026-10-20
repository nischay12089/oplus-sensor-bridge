/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: C:\Users\Nischay_12089\AppData\Local\Android\Sdk\build-tools\35.0.0\aidl.exe --lang=ndk --structured --stability=vintf --version=1 -o C:\Users\Nischay_12089\Desktop\Fingerprint\dpfeature\gen -h C:\Users\Nischay_12089\Desktop\Fingerprint\dpfeature\gen -I C:\Users\Nischay_12089\Desktop\Fingerprint\dpfeature\aidl C:\Users\Nischay_12089\Desktop\Fingerprint\dpfeature\aidl\vendor\oplus\hardware\displaypanelfeature\IDisplayPanelFeature.aidl
 */
#include "aidl/vendor/oplus/hardware/displaypanelfeature/IDisplayPanelFeature.h"

#include <android/binder_parcel_utils.h>
#include <aidl/vendor/oplus/hardware/displaypanelfeature/BnDisplayPanelFeature.h>
#include <aidl/vendor/oplus/hardware/displaypanelfeature/BpDisplayPanelFeature.h>

namespace aidl {
namespace vendor {
namespace oplus {
namespace hardware {
namespace displaypanelfeature {
static binder_status_t _aidl_vendor_oplus_hardware_displaypanelfeature_IDisplayPanelFeature_onTransact(AIBinder* _aidl_binder, transaction_code_t _aidl_code, const AParcel* _aidl_in, AParcel* _aidl_out) {
  (void)_aidl_in;
  (void)_aidl_out;
  binder_status_t _aidl_ret_status = STATUS_UNKNOWN_TRANSACTION;
  std::shared_ptr<BnDisplayPanelFeature> _aidl_impl = std::static_pointer_cast<BnDisplayPanelFeature>(::ndk::ICInterface::asInterface(_aidl_binder));
  switch (_aidl_code) {
    case (FIRST_CALL_TRANSACTION + 0 /*getDisplayPanelFeatureValue*/): {
      int32_t in_id;
      std::vector<int32_t> out_values;
      int32_t _aidl_return;

      _aidl_ret_status = ::ndk::AParcel_readData(_aidl_in, &in_id);
      if (_aidl_ret_status != STATUS_OK) break;

      _aidl_ret_status = ::ndk::AParcel_resizeVector(_aidl_in, &out_values);
      if (_aidl_ret_status != STATUS_OK) break;

      ::ndk::ScopedAStatus _aidl_status = _aidl_impl->getDisplayPanelFeatureValue(in_id, &out_values, &_aidl_return);
      _aidl_ret_status = AParcel_writeStatusHeader(_aidl_out, _aidl_status.get());
      if (_aidl_ret_status != STATUS_OK) break;

      if (!AStatus_isOk(_aidl_status.get())) break;

      _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_out, _aidl_return);
      if (_aidl_ret_status != STATUS_OK) break;

      _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_out, out_values);
      if (_aidl_ret_status != STATUS_OK) break;

      break;
    }
    case (FIRST_CALL_TRANSACTION + 1 /*setDisplayPanelFeatureValue*/): {
      int32_t in_id;
      std::vector<int32_t> in_values;
      int32_t _aidl_return;

      _aidl_ret_status = ::ndk::AParcel_readData(_aidl_in, &in_id);
      if (_aidl_ret_status != STATUS_OK) break;

      _aidl_ret_status = ::ndk::AParcel_readData(_aidl_in, &in_values);
      if (_aidl_ret_status != STATUS_OK) break;

      ::ndk::ScopedAStatus _aidl_status = _aidl_impl->setDisplayPanelFeatureValue(in_id, in_values, &_aidl_return);
      _aidl_ret_status = AParcel_writeStatusHeader(_aidl_out, _aidl_status.get());
      if (_aidl_ret_status != STATUS_OK) break;

      if (!AStatus_isOk(_aidl_status.get())) break;

      _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_out, _aidl_return);
      if (_aidl_ret_status != STATUS_OK) break;

      break;
    }
    case (FIRST_CALL_TRANSACTION + 2 /*getDisplayPanelInfo*/): {
      int32_t in_id;
      std::vector<std::string> out_info;
      int32_t _aidl_return;

      _aidl_ret_status = ::ndk::AParcel_readData(_aidl_in, &in_id);
      if (_aidl_ret_status != STATUS_OK) break;

      _aidl_ret_status = ::ndk::AParcel_resizeVector(_aidl_in, &out_info);
      if (_aidl_ret_status != STATUS_OK) break;

      ::ndk::ScopedAStatus _aidl_status = _aidl_impl->getDisplayPanelInfo(in_id, &out_info, &_aidl_return);
      _aidl_ret_status = AParcel_writeStatusHeader(_aidl_out, _aidl_status.get());
      if (_aidl_ret_status != STATUS_OK) break;

      if (!AStatus_isOk(_aidl_status.get())) break;

      _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_out, _aidl_return);
      if (_aidl_ret_status != STATUS_OK) break;

      _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_out, out_info);
      if (_aidl_ret_status != STATUS_OK) break;

      break;
    }
    case (FIRST_CALL_TRANSACTION + 16777214 /*getInterfaceVersion*/): {
      int32_t _aidl_return;

      ::ndk::ScopedAStatus _aidl_status = _aidl_impl->getInterfaceVersion(&_aidl_return);
      _aidl_ret_status = AParcel_writeStatusHeader(_aidl_out, _aidl_status.get());
      if (_aidl_ret_status != STATUS_OK) break;

      if (!AStatus_isOk(_aidl_status.get())) break;

      _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_out, _aidl_return);
      if (_aidl_ret_status != STATUS_OK) break;

      break;
    }
  }
  return _aidl_ret_status;
}

static AIBinder_Class* _g_aidl_vendor_oplus_hardware_displaypanelfeature_IDisplayPanelFeature_clazz = ::ndk::ICInterface::defineClass(IDisplayPanelFeature::descriptor, _aidl_vendor_oplus_hardware_displaypanelfeature_IDisplayPanelFeature_onTransact);

BpDisplayPanelFeature::BpDisplayPanelFeature(const ::ndk::SpAIBinder& binder) : BpCInterface(binder) {}
BpDisplayPanelFeature::~BpDisplayPanelFeature() {}

::ndk::ScopedAStatus BpDisplayPanelFeature::getDisplayPanelFeatureValue(int32_t in_id, std::vector<int32_t>* out_values, int32_t* _aidl_return) {
  binder_status_t _aidl_ret_status = STATUS_OK;
  ::ndk::ScopedAStatus _aidl_status;
  ::ndk::ScopedAParcel _aidl_in;
  ::ndk::ScopedAParcel _aidl_out;

  _aidl_ret_status = AIBinder_prepareTransaction(asBinder().get(), _aidl_in.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_in.get(), in_id);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = ::ndk::AParcel_writeVectorSize(_aidl_in.get(), *out_values);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AIBinder_transact(
    asBinder().get(),
    (FIRST_CALL_TRANSACTION + 0 /*getDisplayPanelFeatureValue*/),
    _aidl_in.getR(),
    _aidl_out.getR(),
    0
    #ifdef BINDER_STABILITY_SUPPORT
    | FLAG_PRIVATE_LOCAL
    #endif  // BINDER_STABILITY_SUPPORT
    );
  if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IDisplayPanelFeature::getDefaultImpl()) {
    _aidl_status = IDisplayPanelFeature::getDefaultImpl()->getDisplayPanelFeatureValue(in_id, out_values, _aidl_return);
    goto _aidl_status_return;
  }
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AParcel_readStatusHeader(_aidl_out.get(), _aidl_status.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  if (!AStatus_isOk(_aidl_status.get())) goto _aidl_status_return;
  _aidl_ret_status = ::ndk::AParcel_readData(_aidl_out.get(), _aidl_return);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = ::ndk::AParcel_readData(_aidl_out.get(), out_values);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_error:
  _aidl_status.set(AStatus_fromStatus(_aidl_ret_status));
  _aidl_status_return:
  return _aidl_status;
}
::ndk::ScopedAStatus BpDisplayPanelFeature::setDisplayPanelFeatureValue(int32_t in_id, const std::vector<int32_t>& in_values, int32_t* _aidl_return) {
  binder_status_t _aidl_ret_status = STATUS_OK;
  ::ndk::ScopedAStatus _aidl_status;
  ::ndk::ScopedAParcel _aidl_in;
  ::ndk::ScopedAParcel _aidl_out;

  _aidl_ret_status = AIBinder_prepareTransaction(asBinder().get(), _aidl_in.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_in.get(), in_id);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_in.get(), in_values);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AIBinder_transact(
    asBinder().get(),
    (FIRST_CALL_TRANSACTION + 1 /*setDisplayPanelFeatureValue*/),
    _aidl_in.getR(),
    _aidl_out.getR(),
    0
    #ifdef BINDER_STABILITY_SUPPORT
    | FLAG_PRIVATE_LOCAL
    #endif  // BINDER_STABILITY_SUPPORT
    );
  if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IDisplayPanelFeature::getDefaultImpl()) {
    _aidl_status = IDisplayPanelFeature::getDefaultImpl()->setDisplayPanelFeatureValue(in_id, in_values, _aidl_return);
    goto _aidl_status_return;
  }
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AParcel_readStatusHeader(_aidl_out.get(), _aidl_status.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  if (!AStatus_isOk(_aidl_status.get())) goto _aidl_status_return;
  _aidl_ret_status = ::ndk::AParcel_readData(_aidl_out.get(), _aidl_return);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_error:
  _aidl_status.set(AStatus_fromStatus(_aidl_ret_status));
  _aidl_status_return:
  return _aidl_status;
}
::ndk::ScopedAStatus BpDisplayPanelFeature::getDisplayPanelInfo(int32_t in_id, std::vector<std::string>* out_info, int32_t* _aidl_return) {
  binder_status_t _aidl_ret_status = STATUS_OK;
  ::ndk::ScopedAStatus _aidl_status;
  ::ndk::ScopedAParcel _aidl_in;
  ::ndk::ScopedAParcel _aidl_out;

  _aidl_ret_status = AIBinder_prepareTransaction(asBinder().get(), _aidl_in.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_in.get(), in_id);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = ::ndk::AParcel_writeVectorSize(_aidl_in.get(), *out_info);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AIBinder_transact(
    asBinder().get(),
    (FIRST_CALL_TRANSACTION + 2 /*getDisplayPanelInfo*/),
    _aidl_in.getR(),
    _aidl_out.getR(),
    0
    #ifdef BINDER_STABILITY_SUPPORT
    | FLAG_PRIVATE_LOCAL
    #endif  // BINDER_STABILITY_SUPPORT
    );
  if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IDisplayPanelFeature::getDefaultImpl()) {
    _aidl_status = IDisplayPanelFeature::getDefaultImpl()->getDisplayPanelInfo(in_id, out_info, _aidl_return);
    goto _aidl_status_return;
  }
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AParcel_readStatusHeader(_aidl_out.get(), _aidl_status.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  if (!AStatus_isOk(_aidl_status.get())) goto _aidl_status_return;
  _aidl_ret_status = ::ndk::AParcel_readData(_aidl_out.get(), _aidl_return);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = ::ndk::AParcel_readData(_aidl_out.get(), out_info);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_error:
  _aidl_status.set(AStatus_fromStatus(_aidl_ret_status));
  _aidl_status_return:
  return _aidl_status;
}
::ndk::ScopedAStatus BpDisplayPanelFeature::getInterfaceVersion(int32_t* _aidl_return) {
  binder_status_t _aidl_ret_status = STATUS_OK;
  ::ndk::ScopedAStatus _aidl_status;
  if (_aidl_cached_version != -1) {
    *_aidl_return = _aidl_cached_version;
    _aidl_status.set(AStatus_fromStatus(_aidl_ret_status));
    return _aidl_status;
  }
  ::ndk::ScopedAParcel _aidl_in;
  ::ndk::ScopedAParcel _aidl_out;

  _aidl_ret_status = AIBinder_prepareTransaction(asBinder().get(), _aidl_in.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AIBinder_transact(
    asBinder().get(),
    (FIRST_CALL_TRANSACTION + 16777214 /*getInterfaceVersion*/),
    _aidl_in.getR(),
    _aidl_out.getR(),
    0
    #ifdef BINDER_STABILITY_SUPPORT
    | FLAG_PRIVATE_LOCAL
    #endif  // BINDER_STABILITY_SUPPORT
    );
  if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IDisplayPanelFeature::getDefaultImpl()) {
    _aidl_status = IDisplayPanelFeature::getDefaultImpl()->getInterfaceVersion(_aidl_return);
    goto _aidl_status_return;
  }
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AParcel_readStatusHeader(_aidl_out.get(), _aidl_status.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  if (!AStatus_isOk(_aidl_status.get())) goto _aidl_status_return;
  _aidl_ret_status = ::ndk::AParcel_readData(_aidl_out.get(), _aidl_return);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_cached_version = *_aidl_return;
  _aidl_error:
  _aidl_status.set(AStatus_fromStatus(_aidl_ret_status));
  _aidl_status_return:
  return _aidl_status;
}
// Source for BnDisplayPanelFeature
BnDisplayPanelFeature::BnDisplayPanelFeature() {}
BnDisplayPanelFeature::~BnDisplayPanelFeature() {}
::ndk::SpAIBinder BnDisplayPanelFeature::createBinder() {
  AIBinder* binder = AIBinder_new(_g_aidl_vendor_oplus_hardware_displaypanelfeature_IDisplayPanelFeature_clazz, static_cast<void*>(this));
  #ifdef BINDER_STABILITY_SUPPORT
  AIBinder_markVintfStability(binder);
  #endif  // BINDER_STABILITY_SUPPORT
  return ::ndk::SpAIBinder(binder);
}
::ndk::ScopedAStatus BnDisplayPanelFeature::getInterfaceVersion(int32_t* _aidl_return) {
  *_aidl_return = IDisplayPanelFeature::version;
  return ::ndk::ScopedAStatus(AStatus_newOk());
}
// Source for IDisplayPanelFeature
const char* IDisplayPanelFeature::descriptor = "vendor.oplus.hardware.displaypanelfeature.IDisplayPanelFeature";
IDisplayPanelFeature::IDisplayPanelFeature() {}
IDisplayPanelFeature::~IDisplayPanelFeature() {}


std::shared_ptr<IDisplayPanelFeature> IDisplayPanelFeature::fromBinder(const ::ndk::SpAIBinder& binder) {
  if (!AIBinder_associateClass(binder.get(), _g_aidl_vendor_oplus_hardware_displaypanelfeature_IDisplayPanelFeature_clazz)) {
    #if __ANDROID_API__ >= 31
    const AIBinder_Class* originalClass = AIBinder_getClass(binder.get());
    if (originalClass == nullptr) return nullptr;
    if (0 == strcmp(AIBinder_Class_getDescriptor(originalClass), descriptor)) {
      return ::ndk::SharedRefBase::make<BpDisplayPanelFeature>(binder);
    }
    #endif
    return nullptr;
  }
  std::shared_ptr<::ndk::ICInterface> interface = ::ndk::ICInterface::asInterface(binder.get());
  if (interface) {
    return std::static_pointer_cast<IDisplayPanelFeature>(interface);
  }
  return ::ndk::SharedRefBase::make<BpDisplayPanelFeature>(binder);
}

binder_status_t IDisplayPanelFeature::writeToParcel(AParcel* parcel, const std::shared_ptr<IDisplayPanelFeature>& instance) {
  return AParcel_writeStrongBinder(parcel, instance ? instance->asBinder().get() : nullptr);
}
binder_status_t IDisplayPanelFeature::readFromParcel(const AParcel* parcel, std::shared_ptr<IDisplayPanelFeature>* instance) {
  ::ndk::SpAIBinder binder;
  binder_status_t status = AParcel_readStrongBinder(parcel, binder.getR());
  if (status != STATUS_OK) return status;
  *instance = IDisplayPanelFeature::fromBinder(binder);
  return STATUS_OK;
}
bool IDisplayPanelFeature::setDefaultImpl(const std::shared_ptr<IDisplayPanelFeature>& impl) {
  // Only one user of this interface can use this function
  // at a time. This is a heuristic to detect if two different
  // users in the same process use this function.
  assert(!IDisplayPanelFeature::default_impl);
  if (impl) {
    IDisplayPanelFeature::default_impl = impl;
    return true;
  }
  return false;
}
const std::shared_ptr<IDisplayPanelFeature>& IDisplayPanelFeature::getDefaultImpl() {
  return IDisplayPanelFeature::default_impl;
}
std::shared_ptr<IDisplayPanelFeature> IDisplayPanelFeature::default_impl = nullptr;
::ndk::ScopedAStatus IDisplayPanelFeatureDefault::getDisplayPanelFeatureValue(int32_t /*in_id*/, std::vector<int32_t>* /*out_values*/, int32_t* /*_aidl_return*/) {
  ::ndk::ScopedAStatus _aidl_status;
  _aidl_status.set(AStatus_fromStatus(STATUS_UNKNOWN_TRANSACTION));
  return _aidl_status;
}
::ndk::ScopedAStatus IDisplayPanelFeatureDefault::setDisplayPanelFeatureValue(int32_t /*in_id*/, const std::vector<int32_t>& /*in_values*/, int32_t* /*_aidl_return*/) {
  ::ndk::ScopedAStatus _aidl_status;
  _aidl_status.set(AStatus_fromStatus(STATUS_UNKNOWN_TRANSACTION));
  return _aidl_status;
}
::ndk::ScopedAStatus IDisplayPanelFeatureDefault::getDisplayPanelInfo(int32_t /*in_id*/, std::vector<std::string>* /*out_info*/, int32_t* /*_aidl_return*/) {
  ::ndk::ScopedAStatus _aidl_status;
  _aidl_status.set(AStatus_fromStatus(STATUS_UNKNOWN_TRANSACTION));
  return _aidl_status;
}
::ndk::ScopedAStatus IDisplayPanelFeatureDefault::getInterfaceVersion(int32_t* _aidl_return) {
  *_aidl_return = 0;
  return ::ndk::ScopedAStatus(AStatus_newOk());
}
::ndk::SpAIBinder IDisplayPanelFeatureDefault::asBinder() {
  return ::ndk::SpAIBinder();
}
bool IDisplayPanelFeatureDefault::isRemote() {
  return false;
}
}  // namespace displaypanelfeature
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
