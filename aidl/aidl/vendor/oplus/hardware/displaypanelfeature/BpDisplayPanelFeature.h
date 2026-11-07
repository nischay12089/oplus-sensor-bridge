/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: aidl --lang=ndk --structured --stability=vintf --version=1 --hash=39afca22ac77253421ad681eeb2bae0c6ff62c13 -o gen -h gen -I aidl aidl/vendor/oplus/hardware/displaypanelfeature/IDisplayPanelFeature.aidl (build-tools 35.0.0)
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
  ::ndk::ScopedAStatus getInterfaceHash(std::string* _aidl_return) override;
  int32_t _aidl_cached_version = -1;
  std::string _aidl_cached_hash = "-1";
  std::mutex _aidl_cached_hash_mutex;
};
}  // namespace displaypanelfeature
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
