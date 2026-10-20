/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: C:\Users\Nischay_12089\AppData\Local\Android\Sdk\build-tools\35.0.0\aidl.exe --lang=ndk --structured --stability=vintf --version=1 -o C:\Users\Nischay_12089\Desktop\Fingerprint\dpfeature\gen -h C:\Users\Nischay_12089\Desktop\Fingerprint\dpfeature\gen -I C:\Users\Nischay_12089\Desktop\Fingerprint\dpfeature\aidl C:\Users\Nischay_12089\Desktop\Fingerprint\dpfeature\aidl\vendor\oplus\hardware\displaypanelfeature\IDisplayPanelFeature.aidl
 */
#pragma once

#include "aidl/vendor/oplus/hardware/displaypanelfeature/IDisplayPanelFeature.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace vendor {
namespace oplus {
namespace hardware {
namespace displaypanelfeature {
class BpDisplayPanelFeature : public ::ndk::BpCInterface<IDisplayPanelFeature> {
public:
  explicit BpDisplayPanelFeature(const ::ndk::SpAIBinder& binder);
  virtual ~BpDisplayPanelFeature();

  ::ndk::ScopedAStatus getDisplayPanelFeatureValue(int32_t in_id, std::vector<int32_t>* out_values, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplayPanelFeatureValue(int32_t in_id, const std::vector<int32_t>& in_values, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayPanelInfo(int32_t in_id, std::vector<std::string>* out_info, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getInterfaceVersion(int32_t* _aidl_return) override;
  int32_t _aidl_cached_version = -1;
};
}  // namespace displaypanelfeature
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
