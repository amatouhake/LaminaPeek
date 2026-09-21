// Single entry point for the game-independent test binary: runs the layout
// suite, the durability-overlay suite and the Bundle-preview suite.

#include <cstdio>

int runPreviewLayoutTests();
int runDurabilityBarTests();
int runBundlePreviewTests();

int main() {
    int failures = 0;
    failures += runPreviewLayoutTests();
    failures += runDurabilityBarTests();
    failures += runBundlePreviewTests();
    if (failures == 0) {
        std::printf("LaminaPeekTests: all passed\n");
        return 0;
    }
    std::printf("LaminaPeekTests: %d failure(s)\n", failures);
    return 1;
}
