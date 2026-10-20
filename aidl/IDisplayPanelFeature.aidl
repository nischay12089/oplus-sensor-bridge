package vendor.oplus.hardware.displaypanelfeature;

@VintfStability
interface IDisplayPanelFeature {
    int getDisplayPanelFeatureValue(int id, out int[] values);
    int setDisplayPanelFeatureValue(int id, in int[] values);
    int getDisplayPanelInfo(int id, out String[] info);
}
