/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: aidl --lang=ndk --structured --stability=vintf --version=1 --hash=39afca22ac77253421ad681eeb2bae0c6ff62c13 -o gen -h gen -I aidl aidl/vendor/oplus/hardware/displaypanelfeature/IDisplayPanelFeature.aidl (build-tools 35.0.0)
 */
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <android/binder_interface_utils.h>
#ifdef BINDER_STABILITY_SUPPORT
#include <android/binder_stability.h>
#endif  // BINDER_STABILITY_SUPPORT

namespace aidl {
namespace vendor {
namespace oplus {
namespace hardware {
namespace displaypanelfeature {
class IDisplayPanelFeatureDelegator;

class IDisplayPanelFeature : public ::ndk::ICInterface {
public:
  typedef IDisplayPanelFeatureDelegator DefaultDelegator;
  static const char* descriptor;
  IDisplayPanelFeature();
  virtual ~IDisplayPanelFeature();

  static inline const int32_t version = 1;
  static inline const std::string hash = "39afca22ac77253421ad681eeb2bae0c6ff62c13";
  static constexpr uint32_t TRANSACTION_getDisplayPanelFeatureValue = FIRST_CALL_TRANSACTION + 0;
  static constexpr uint32_t TRANSACTION_setDisplayPanelFeatureValue = FIRST_CALL_TRANSACTION + 1;
  static constexpr uint32_t TRANSACTION_getDisplayPanelInfo = FIRST_CALL_TRANSACTION + 2;

  static std::shared_ptr<IDisplayPanelFeature> fromBinder(const ::ndk::SpAIBinder& binder);
  static binder_status_t writeToParcel(AParcel* parcel, const std::shared_ptr<IDisplayPanelFeature>& instance);
  static binder_status_t readFromParcel(const AParcel* parcel, std::shared_ptr<IDisplayPanelFeature>* instance);
  static bool setDefaultImpl(const std::shared_ptr<IDisplayPanelFeature>& impl);
  static const std::shared_ptr<IDisplayPanelFeature>& getDefaultImpl();
  virtual ::ndk::ScopedAStatus getDisplayPanelFeatureValue(int32_t in_id, std::vector<int32_t>* out_values, int32_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus setDisplayPanelFeatureValue(int32_t in_id, const std::vector<int32_t>& in_values, int32_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus getDisplayPanelInfo(int32_t in_id, std::vector<std::string>* out_info, int32_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus getInterfaceVersion(int32_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus getInterfaceHash(std::string* _aidl_return) = 0;
private:
  static std::shared_ptr<IDisplayPanelFeature> default_impl;
};
class IDisplayPanelFeatureDefault : public IDisplayPanelFeature {
public:
  ::ndk::ScopedAStatus getDisplayPanelFeatureValue(int32_t in_id, std::vector<int32_t>* out_values, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplayPanelFeatureValue(int32_t in_id, const std::vector<int32_t>& in_values, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayPanelInfo(int32_t in_id, std::vector<std::string>* out_info, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getInterfaceVersion(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getInterfaceHash(std::string* _aidl_return) override;
  ::ndk::SpAIBinder asBinder() override;
  bool isRemote() override;
};
}  // namespace displaypanelfeature
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
