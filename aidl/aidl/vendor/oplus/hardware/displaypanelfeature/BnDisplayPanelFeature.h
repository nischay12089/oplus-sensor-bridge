/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: aidl --lang=ndk --structured --stability=vintf --version=1 --hash=39afca22ac77253421ad681eeb2bae0c6ff62c13 -o gen -h gen -I aidl aidl/vendor/oplus/hardware/displaypanelfeature/IDisplayPanelFeature.aidl (build-tools 35.0.0)
 */
#pragma once

#include "aidl/vendor/oplus/hardware/displaypanelfeature/IDisplayPanelFeature.h"

#include <android/binder_ibinder.h>
#include <cassert>

#ifndef __BIONIC__
#ifndef __assert2
#define __assert2(a,b,c,d) ((void)0)
#endif
#endif

namespace aidl {
namespace vendor {
namespace oplus {
namespace hardware {
namespace displaypanelfeature {
class BnDisplayPanelFeature : public ::ndk::BnCInterface<IDisplayPanelFeature> {
public:
  BnDisplayPanelFeature();
  virtual ~BnDisplayPanelFeature();
  ::ndk::ScopedAStatus getInterfaceVersion(int32_t* _aidl_return) final;
  ::ndk::ScopedAStatus getInterfaceHash(std::string* _aidl_return) final;
protected:
  ::ndk::SpAIBinder createBinder() override;
private:
};
class IDisplayPanelFeatureDelegator : public BnDisplayPanelFeature {
public:
  explicit IDisplayPanelFeatureDelegator(const std::shared_ptr<IDisplayPanelFeature> &impl) : _impl(impl) {
     int32_t _impl_ver = 0;
     if (!impl->getInterfaceVersion(&_impl_ver).isOk()) {;
        __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Delegator failed to get version of the implementation.");
     }
     if (_impl_ver != IDisplayPanelFeature::version) {
        __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Mismatched versions of delegator and implementation is not allowed.");
     }
  }

  ::ndk::ScopedAStatus getDisplayPanelFeatureValue(int32_t in_id, std::vector<int32_t>* out_values, int32_t* _aidl_return) override {
    return _impl->getDisplayPanelFeatureValue(in_id, out_values, _aidl_return);
  }
  ::ndk::ScopedAStatus setDisplayPanelFeatureValue(int32_t in_id, const std::vector<int32_t>& in_values, int32_t* _aidl_return) override {
    return _impl->setDisplayPanelFeatureValue(in_id, in_values, _aidl_return);
  }
  ::ndk::ScopedAStatus getDisplayPanelInfo(int32_t in_id, std::vector<std::string>* out_info, int32_t* _aidl_return) override {
    return _impl->getDisplayPanelInfo(in_id, out_info, _aidl_return);
  }
protected:
private:
  std::shared_ptr<IDisplayPanelFeature> _impl;
};

}  // namespace displaypanelfeature
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
